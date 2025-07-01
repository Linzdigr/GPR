#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <cstdint>
#include <math.h>
#include <algorithm>
#include <fftw3.h>

// Debug
#include <iomanip>
#include <limits>

using namespace std;

#include "GPR.h"
#include "MCP4921/MCP4921.h"
#include "waveforms.h"
#include "recorder.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

template<typename T>
T clamp(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

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

  thread_dac.join();
  thread_record.join();
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

  unsigned int start_i = total_steps * 0.1f;
  unsigned int stop_i  = total_steps * 0.9f;

  do {
    for(unsigned int i = 0; i < total_steps; i++) {
      this->relevant_time = (i >= start_i && i <= stop_i);

      dac->setRawValue(wf[i]);
      // std::this_thread::sleep_for(std::chrono::nanoseconds((int)(step_hold_us*1000)));
    }
  } while(1);
}

void GPR::record() {
  unique_lock<mutex> lksd(this->mtx_sweep_data, std::defer_lock);
  Recorder *rec = nullptr;
  int32_t *bloc_data = nullptr;

  try {
    rec = new Recorder("plughw:2,0", ADC_SAMPLING_RATE_S, SND_PCM_FORMAT_S32_LE, 2048);
  } catch(const string &e) {
    cerr << e << endl;
    exit(-1);
  }

  uint32_t sweeps_done = 0;

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

    rec->start();

    do {
      if(this->relevant_time) { // Sweeping
        unsigned int len = rec->captureBloc(bloc_data);
        this->sweep_data.insert(this->sweep_data.end(), bloc_data, bloc_data + len);
        delete []bloc_data;
      } else { // End of sweep
        cout << "record: data set is ready to be read." << endl;
        rec->stop();
        sweeps_done++;

        GPR::windowing(this->sweep_data.data(), this->sweep_data.size(), HANN_FUNCTION);
        this->sweep_data_filtered = this->sweep_data;
        this->sweep_data.clear();

        // rec->saveToWaveFile("mi.wav", sizeof(int32_t) * this->sweep_data.size(), this->sweep_data.data());
        if(sweeps_done >= 10) {
          cout << "Generating FFT image" << endl;
          // GPR::generateSpectrogramImage("spectrogram.png", this->sweep_data_filtered.data(), this->sweep_data_filtered.size(), ADC_SAMPLING_RATE_S, 2048, 512);
          this->updateSpectrogramImage("spectrogram.png", this->sweep_data.data(), this->sweep_data.size());
          cout << "Done" << endl;
          sweeps_done = 0;
        }
        
        cout << "record: Unlocking." << endl;
        lksd.unlock();
        this->cv_sweep_data.notify_one();

        break;
      }
    } while(1);
  } while(1);

  rec->cleanup();
}

void GPR::generateSpectrogramImageInterlaced(const char* output_file, int32_t* data, uint32_t num_samples, uint32_t sample_rate, uint32_t window_size, uint32_t hop_size) {
  if(num_samples < window_size) {
    std::cerr << "Not enough samples for spectrogram." << std::endl;
    return;
  }

  const uint32_t num_frames = (num_samples - window_size) / hop_size + 1;
  const uint32_t fft_bins = window_size / 2;
  const uint32_t width = num_frames;
  const uint32_t height = fft_bins;

  std::vector<uint8_t> image(width * height);

  double* in = (double*)fftw_malloc(sizeof(double) * window_size);
  fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (fft_bins + 1));
  fftw_plan plan = fftw_plan_dft_r2c_1d(window_size, in, out, FFTW_ESTIMATE);

  std::vector<float> magnitudes(width * height);

  float mag_min = 1e9, mag_max = -1e9;

  for (uint32_t frame = 0; frame < num_frames; ++frame) {
    uint32_t offset = frame * hop_size;

    for (uint32_t i = 0; i < window_size; ++i) {
      in[i] = (double)data[offset + i];
    }

    fftw_execute(plan);

    for (uint32_t k = 0; k < fft_bins; ++k) {
      double re = out[k][0];
      double im = out[k][1];
      float mag_db = 20.0f * log10(std::sqrt(re * re + im * im) + 1e-10);
      magnitudes[frame * fft_bins + k] = mag_db;
      if (mag_db < mag_min) mag_min = mag_db;
      if (mag_db > mag_max) mag_max = mag_db;
    }
  }

  std::cout << "Spectrogram dB range: [" << mag_min << ", " << mag_max << "]" << std::endl;

  float mag_range = mag_max - mag_min + 1e-6f;
  for (uint32_t frame = 0; frame < num_frames; ++frame) {
    for (uint32_t bin = 0; bin < fft_bins; ++bin) {
      float norm = (magnitudes[frame * fft_bins + bin] - mag_min) / mag_range;
      uint8_t pixel = static_cast<uint8_t>(clamp(norm * 255.0f, 0.0f, 255.0f));
      image[(fft_bins - 1 - bin) * width + frame] = pixel;
    }
  }

  stbi_write_png(output_file, width, height, 1, image.data(), width);

  fftw_destroy_plan(plan);
  fftw_free(in);
  fftw_free(out);
}

void GPR::updateSpectrogramImage(const char* output_file, int32_t* data, uint32_t num_samples) {
  if(num_samples != this->window_size) {
    std::cerr << "Expected " << this->window_size << " samples, got " << num_samples << std::endl;
    return;
  }

  // Allocate input buffer
  double* in = static_cast<double*>(fftw_malloc(sizeof(double) * window_size));
  fftw_complex* out = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * (window_size / 2 + 1)));

  // Convert data to double
  for(uint32_t i = 0; i < window_size; ++i)
    in[i] = static_cast<double>(data[i]);

  // Apply window function (e.g. Hann)
  for(uint32_t i = 0; i < window_size; ++i)
    in[i] *= 0.5 * (1 - cos(2 * M_PI * i / (window_size - 1)));

  // FFT
  fftw_plan plan = fftw_plan_dft_r2c_1d(window_size, in, out, FFTW_ESTIMATE);
  fftw_execute(plan);
  fftw_destroy_plan(plan);

  // Compute magnitude in dB
  uint32_t bins = window_size / 2 - 1;
  std::vector<float> magnitudes(bins);
  float max_dB = -1000.0f;

  for(uint32_t i = 1; i <= bins; ++i) {
    double re = out[i][0];
    double im = out[i][1];
    double mag = sqrt(re * re + im * im);
    float dB = 20.0f * log10(mag + 1e-10);
    magnitudes[i - 1] = dB;
    if (dB > max_dB) max_dB = dB;
  }

  fftw_free(in);
  fftw_free(out);

  // Normalize and convert to 8-bit grayscale column
  std::vector<uint8_t> column(bins);
  for (uint32_t i = 0; i < bins; ++i) {
    float norm = (magnitudes[i] - (max_dB - 60.0f)) / 60.0f; // scale to last 60 dB
    norm = std::clamp(norm, 0.0f, 1.0f);
    column[bins - i - 1] = static_cast<uint8_t>(norm * 255); // vertical flip
  }

  // Store column in buffer
  if(this->spectrogram_height == 0)
    this->spectrogram_height = bins;
  if(this->spectrogram_buffer.empty())
    this->spectrogram_buffer.reserve(bins * 1000); // prealloc for 1000 columns

  // Append column to buffer
  for(uint32_t row = 0; row < bins; ++row)
    this->spectrogram_buffer.push_back(column[row]);

  this->spectrogram_width += 1;

  // Save full image
  stbi_write_png(output_file,
                  this->spectrogram_width,
                  this->spectrogram_height,
                  1,  // grayscale
                  this->spectrogram_buffer.data(),
                  this->spectrogram_width);
}

int x = 0;

void GPR::processFFT() {;
  // fftw_plan p;

  // while(!this->recorder_ready);

  // do {
  //   unique_lock<mutex> lk(this->mtx_sweep_data);
  //   cout << "processFFT waiting for lock sd" << endl;
  //   this->cv_sweep_data.wait(lk, [this]{return this->recorder_ready;});
  //   cout << "processFFT got lock sd" << endl;
    
  //   int len = this->sweep_data.size();

  //   // if(len == 0 || (len & (len - 1)) != 0) {
  //   //     cerr << "FFT input size must be > 0 and power of 2. Got: " << len << endl;
  //   //     continue;
  //   // }

  //   int output_size = (len/2 + 1);
  //   float magnitude_plot[(len/2)-2][2];
  //   double *in = static_cast<double*>(fftw_malloc(len * sizeof(double)));
  //   fftw_complex *out = static_cast<fftw_complex*>(fftw_malloc(output_size * sizeof(fftw_complex)));
  //   std::copy(this->sweep_data.data(), this->sweep_data.data() + len, in);

  //   this->sweep_data.clear();

  //   lk.unlock();
  //   cout << "processFFT copied data, released lock." << endl;
  //   this->cv_sweep_data.notify_one();

  //   p = fftw_plan_dft_r2c_1d(len, in, out, FFTW_ESTIMATE);

  //   fftw_execute(p);

  //   ofstream fd;
  //   fd.open("pim.csv");

  //   for(uint32_t i = 1; i < (len/2)-1; i++) { // Compute magnitude data and setting output bins, we don't need extrems
  //     magnitude_plot[i-1][0] = (i * ADC_SAMPLING_RATE_S / len);
  //     // magnitude_plot[i-1][0] = (double)((double)i / (double)(len / 2)) * (ADC_SAMPLING_RATE_S / 2);
  //     magnitude_plot[i-1][1] = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
  //     fd << to_string((int)magnitude_plot[i-1][0]) << ';' << to_string((int)magnitude_plot[i-1][1]) << endl;
  //   }

  //   fftw_free(in);
  //   fftw_free(out);
  // } while (1);

  // fftw_destroy_plan(p);
}

void GPR::windowing(int32_t *data, unsigned int len, unsigned int method) {
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
