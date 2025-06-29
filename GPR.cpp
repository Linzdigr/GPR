#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <cstdint>
#include <math.h>
#include <fftw3.h>

using namespace std;

#include "GPR.h"
#include "MCP4921/MCP4921.h"
#include "waveforms.h"
#include "recorder.h"

#define SECOND_US               1e6F

#define DAC_CMD_RATE_S     96000
#define ADC_SAMPLING_RATE_S     48000

GPR* GPR::_instance = nullptr;

GPR::GPR(float freq_low, float freq_high, float tsweep_us)
:f_low(freq_low), f_hi(freq_high), relevant_time(false), sweep_length_us(tsweep_us) {
  cout << "Creating 1 GPR" << endl;

  this->bw = this->f_low - this->f_hi;

  this->recorder_ready = false;

  thread thread_dac(&GPR::waveformGenerator, this);
  thread thread_record(&GPR::record, this);
  thread thread_fft(&GPR::processFFT, this);

  thread_dac.join();
  thread_record.join();
  thread_fft.join();
}

GPR* GPR::getInstance(const float freq_start, const float freq_stop, const float tsweep_us) {
  /**
   * This is a safer way to create an instance. instance = new Singleton is
   * dangerous in case two instance threads wants to access at the same time
   */
  if(GPR::_instance == nullptr) {
    GPR::_instance = new GPR(freq_start, freq_stop, tsweep_us);
  }

  return GPR::_instance;
}

float GPR::beat2Dist(float f) {
  return GPR::c * f / (2 * (this->bw / this->sweep_length_us));
}

void GPR::waveformGenerator() {
  cout << "GPR::waveformGenerator()" << endl;

  unsigned int total_steps = 4096;
  float step_hold_us = (this->sweep_length_us / total_steps);

  printf("\nPeriod: %fms\nTotal steps: %u\nStep hold: %fµs\n\n", (this->sweep_length_us / 1000), total_steps, step_hold_us);

  MCP4921 *dac = nullptr;

  try {
    dac = new MCP4921();
  } catch(const string &e) {
    cerr << e << endl;
    exit(-1);
  }

  uint16_t *wf = nullptr;
  
  try {
    wf = new uint16_t [total_steps];
  }  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
  }  

  Waveform::ramp(wf, total_steps, 0, MCP4921::MAX_DAC_VALUE);

  do {
    unsigned int start_i = total_steps * 0.1f;
    unsigned int stop_i  = total_steps * 0.9f;
    for(unsigned int i = 0; i < total_steps; i++) {
      this->relevant_time = (i >= start_i && i <= stop_i);

      dac->setRawValue(wf[i]);
      // usleep(step_hold_us);
      std::this_thread::sleep_for(std::chrono::nanoseconds((int)(step_hold_us*1000)));
    }
  } while(1);
}

void GPR::record() {
  unique_lock<mutex> lksd(this->mtx_sweep_data, std::defer_lock);
  Recorder *rec = nullptr;
  int32_t *bloc_data = nullptr;

  try {
    rec = new Recorder("plughw:2,0", ADC_SAMPLING_RATE_S, SND_PCM_FORMAT_S32_LE, 2048);
    rec->pause();
  } catch(const string &e) {
    cerr << e << endl;
    exit(-1);
  }

  do {
    lksd.lock();
    cout << "record: wait for data lock" << endl;
    this->cv_sweep_data.wait(lksd, [this]{return this->sweep_data.size() == 0;});
    this->recorder_ready = true;
    cout << "record: got data lock!" << endl;

    cout << "record: Waiting for relevant time" << endl;

    bool last_relevant_val;

    while(!this->relevant_time) {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    rec->resume();

    do {
      if(this->relevant_time) { // Fetch data phase
        unsigned int len = rec->captureBloc(bloc_data);
        GPR::windowing(bloc_data, len, HANN_FUNCTION);
        this->sweep_data.insert(this->sweep_data.end(), bloc_data, bloc_data + len);
        delete []bloc_data;
      } else { // Full data set is available
        rec->pause();
        cout << "record: data set is ready to be read. Unlocking the current state. Have lock : " << lksd.owns_lock() << endl;
        /* Dropping unusable frames */

        lksd.unlock();
        this->cv_sweep_data.notify_one();

        rec->saveToWaveFile("mi.wav", sizeof(int32_t) * this->sweep_data.size(), this->sweep_data.data());

        break;
      }
    } while(1);
  } while(1);

  rec->cleanup();
}

int x = 0;

void GPR::processFFT() {
  fftw_plan p;

  while(!this->recorder_ready);

  do {
    unique_lock<mutex> lk(this->mtx_sweep_data);
    cout << "processFFT waiting for lock sd" << endl;
    this->cv_sweep_data.wait(lk, [this]{return this->recorder_ready;});
    cout << "processFFT got lock sd" << endl;
    
    int len = this->sweep_data.size();

    // if(len == 0 || (len & (len - 1)) != 0) {
    //     cerr << "FFT input size must be > 0 and power of 2. Got: " << len << endl;
    //     continue;
    // }

    int output_size = (len/2 + 1);
    float magnitude_plot[(len/2)-2][2];
    double *in = static_cast<double*>(fftw_malloc(len * sizeof(double)));
    fftw_complex *out = static_cast<fftw_complex*>(fftw_malloc(output_size * sizeof(fftw_complex)));
    std::copy(this->sweep_data.data(), this->sweep_data.data() + len, in);

    this->sweep_data.clear();

    lk.unlock();
    cout << "processFFT copied data, released lock." << endl;
    this->cv_sweep_data.notify_one();

    p = fftw_plan_dft_r2c_1d(len, in, out, FFTW_ESTIMATE);

    fftw_execute(p);

    ofstream fd;
    fd.open("pim.csv");

    for(uint32_t i = 1; i < (len/2)-1; i++) { // Compute magnitude data and setting output bins, we don't need extrems
      magnitude_plot[i-1][0] = (i * ADC_SAMPLING_RATE_S / len);
      // magnitude_plot[i-1][0] = (double)((double)i / (double)(len / 2)) * (ADC_SAMPLING_RATE_S / 2);
      magnitude_plot[i-1][1] = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
      fd << to_string((int)magnitude_plot[i-1][0]) << ';' << to_string((int)magnitude_plot[i-1][1]) << endl;
    }

    fftw_free(in);
    fftw_free(out);
  } while (1);

  fftw_destroy_plan(p);
}

void GPR::windowing(int32_t *(&data), unsigned int len, unsigned int method) {
  if(method == HANN_FUNCTION) {
    for(unsigned int i = 0; i < len; i++) {
      double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (len - 1)));
      data[i] *= multiplier;
    }
  }
}

GPR::~GPR() {
  cout << "Destroying 1 GPR" << endl;
}
