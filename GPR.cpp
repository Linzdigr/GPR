#include <iostream>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <cstdint>
#include <math.h>
#include <fftw3.h>

using namespace std;

#include "GPR.h"
#include "MCP4921/MCP4921.h"
#include "waveforms.h"
#include "recorder.h"

#define SECOND_US               1e-6

#define DAC_SAMPLING_RATE_S     96000
#define ADC_SAMPLING_RATE_S     48000

GPR* GPR::_instance = nullptr;

GPR::GPR(float freq_low, float freq_high, float tsweep)
:f_low(freq_low), f_hi(freq_high), relevant_time(false), sweep_length(tsweep) {
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

GPR* GPR::getInstance(const float freq_start, const float freq_stop, const float tsweep) {
  /**
   * This is a safer way to create an instance. instance = new Singleton is
   * dangerous in case two instance threads wants to access at the same time
   */
  if(GPR::_instance == nullptr) {
    GPR::_instance = new GPR(freq_start, freq_stop, tsweep);
  }

  return GPR::_instance;
}

float GPR::beat2Dist(float f) {
  return GPR::c * f / (2 * (this->bw / this->sweep_length));
}

void GPR::waveformGenerator() {
  cout << "GPR::waveformGenerator()" << endl;

  unsigned int total_steps = (this->sweep_length * (float)DAC_SAMPLING_RATE_S);
  float step_hold_us = (this->sweep_length / total_steps) / SECOND_US;

  printf("\nPeriod: %fµs\nTotal steps: %u\nStep hold: %fµs\n\n", this->sweep_length, total_steps, step_hold_us);

  MCP4921 *dac = nullptr;

  // try {
  //   dac = new MCP4921();
  // } catch(const string &e) {
  //   cerr << e << endl;
  //   exit(-1);
  // }
  
  uint16_t *wf = new uint16_t [total_steps];

  Waveform::ramp(wf, total_steps, 0, MCP4921::MAX_DAC_VALUE);

  do {
    for(unsigned int i = 0; i < total_steps; i++) {
      float progression = ((float)i / (float)total_steps);
      this->relevant_time = (progression >= 0.1 && progression <= 0.9);

      // dac->setRawValue(wf[i]);
      usleep(step_hold_us);
    }
  } while(1);
}

void GPR::record() {
  unique_lock<mutex> lksd(this->mtx_sweep_data, std::defer_lock);
  Recorder *rec = nullptr;
  int16_t *bloc_data = nullptr;

  try {
    rec = new Recorder("plughw:1,0", ADC_SAMPLING_RATE_S, SND_PCM_FORMAT_S16_LE);
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

    do {
      last_relevant_val = this->relevant_time;
    } while(!(this->relevant_time && !last_relevant_val)); // We only want data from the start of sweep period / phase

    cout << "record: relevant time started!" << endl;

    do {
      if(this->relevant_time) { // Retrieve data phase
        unsigned int len = rec->captureBloc(bloc_data);
        this->sweep_data.insert(this->sweep_data.begin(), bloc_data, bloc_data + len);
        cout << "!!! " << this->sweep_data.size() << endl;
        free(bloc_data);
      } else { // Data set is ready to be read
        cout << "record: data set is ready to be read. Unlocking the current state." << lksd.owns_lock() << endl;
        lksd.unlock();
        this->cv_sweep_data.notify_one();

        int fd = open("mi.wav", O_WRONLY | O_CREAT, 0644);

        write(fd, this->sweep_data.data(), sizeof(int16_t) * this->sweep_data.size());

        close(fd);

        break;
      }
    } while(1);
  } while(1);
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
    int output_size = (len/2 + 1);
    float magnitude_plot[(len/2)-2][2];
    double *in = static_cast<double*>(fftw_malloc(len * sizeof(double)));
    fftw_complex *out = static_cast<fftw_complex*>(fftw_malloc(output_size * sizeof(fftw_complex)));
    std::copy(this->sweep_data.data(), this->sweep_data.data() + len, in);

    this->sweep_data.clear();

    lk.unlock();
    cout << "processFFT copied data, released lock." << endl;
    this->cv_sweep_data.notify_one();

    GPR::windowing(in, len);

    p = fftw_plan_dft_r2c_1d(len, in, out, FFTW_ESTIMATE);

    fftw_execute(p);

    ofstream fd;
    fd.open("pim.csv");

    for(uint32_t i = 1; i < (len/2)-1; i++) { // Compute magnitude data and setting output bins, we don't need extrems
      magnitude_plot[i-1][0] = (i * ADC_SAMPLING_RATE_S / len);
      // magnitude_plot[i-1][0] = (double)((double)i / (double)(len / 2)) * (ADC_SAMPLING_RATE_S / 2);
      magnitude_plot[i-1][1] = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
      fd << to_string((int)magnitude_plot[i-1][0]) << ',' << to_string((int)magnitude_plot[i-1][1]) << endl;
    }

    fftw_free(in);
    fftw_free(out);
  } while (1);

  fftw_destroy_plan(p);
}

void GPR::windowing(double *(&data), int len, int method) {
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
