#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstdint>
// #include <mutex>
// #include <condition_variable>

using namespace std;

#include "GPR.h"
#include "MCP4921.h"
#include "waveforms.h"

#define DAC_SAMPLING_RATE_S     30000

GPR* GPR::_instance = nullptr;

GPR::GPR(float freq_low, float freq_high, float tsweep)
:f_low(freq_low), f_hi(freq_high), relevant_time(false) {
  cout << "Creating 1 GPR" << endl;

  this->bw = this->f_low - this->f_hi;

  thread thread_dac(&GPR::waveformGenerator, this);
  thread thread_fft(&GPR::processFFT, this);

  thread_dac.join();
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

  uint16_t *wf;
  MCP4921 *dac = new MCP4921();

  unsigned int total_steps = (this->sweep_length * (float)DAC_SAMPLING_RATE_S);
  float step_hold_us = (this->sweep_length / total_steps) * 1e6;

  printf("\nPeriod: %fµs\nTotal steps: %u\nStep hold: %fµs\n\n", this->sweep_length, total_steps, step_hold_us);

  Waveform::ramp(wf, total_steps, 0, MCP4921::MAX_DAC_VALUE);

  do {
    for(unsigned short int i = 0; i < total_steps; i++) {
      this->relevant_time = (total_steps / i >= 0.1 && total_steps / i <= 0.9);
      dac->setRawValue(wf[i]);
      usleep(step_hold_us);
    }
  } while(1);
}

void GPR::processFFT() {
  if(this->relevant_time) {
    // Do FFT stuff
  }
}

GPR::~GPR() {
  cout << "Destroying 1 GPR" << endl;
}
