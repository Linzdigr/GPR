#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstdint>

using namespace std;

#include "GPR.h"
#include "MCP4921/MCP4921.h"
#include "waveforms.h"
#include "recorder.h"

#define SECOND_US               1e-6

#define DAC_SAMPLING_RATE_S     96000

GPR* GPR::_instance = nullptr;

GPR::GPR(float freq_low, float freq_high, float tsweep)
:f_low(freq_low), f_hi(freq_high), relevant_time(false), sweep_length(tsweep) {
  cout << "Creating 1 GPR" << endl;

  this->bw = this->f_low - this->f_hi;

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

  try {
    dac = new MCP4921();
  } catch(const string &e) {
    cerr << e << endl;
    exit(-1);
  }
  
  uint16_t *wf = new uint16_t [total_steps];

  Waveform::ramp(wf, total_steps, 0, MCP4921::MAX_DAC_VALUE);

  do {
    for(unsigned int i = 0; i < total_steps; i++) {
      bool poi = (total_steps / i >= 0.1 && total_steps / i <= 0.9);
      if(this->relevant_time != poi) {
        this->relevant_time = poi;
        this->cv_relevant_time.notify_one();
      }

      dac->setRawValue(wf[i]);
      usleep(step_hold_us);
    }
  } while(1);
}

void GPR::record() {
  Recorder *rec = nullptr;

  uint16_t *bloc_data = nullptr;

  try {
    rec = new Recorder("hw:2,0");
  } catch(const string &e) {
    cerr << e << endl;
    exit(-1);
  }

  unique_lock<mutex> lkrt(this->mtx_relevant_time);
  unique_lock<mutex> lksd(this->mtx_sweep_data);

  do {
    this->cv_relevant_time.wait(lkrt, [this]{return !this->relevant_time;});
    lkrt.unlock();

    do {
      if(this->relevant_time) {
        unsigned int len = rec->captureBloc(bloc_data);
        this->sweep_data.insert(this->sweep_data.end(), bloc_data, bloc_data + len);
      } else {
        lksd.unlock();
        this->cv_sweep_data.notify_one(); // Data is ready to be read
        break;
      }
    } while (1);
    this->cv_sweep_data.wait(lksd);
  } while(1);
}

void GPR::processFFT() {
  do {
    unique_lock<mutex> lk(this->mtx_sweep_data);
    this->cv_sweep_data.wait(lk, [this]{return !this->relevant_time;});

    vector<uint16_t> data = this->sweep_data;
    lk.unlock();
    this->cv_sweep_data.notify_one();
  } while (1);
}

GPR::~GPR() {
  cout << "Destroying 1 GPR" << endl;
}
