#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstdint>
// #include <mutex>
// #include <condition_variable>

using namespace std;

#include "GPR.h"
#include "waveform.h"
#include "mcp4725.h"

#define DAC_SAMPLING_RATE_S     30000

GPR::GPR(float freq_low, float freq_high, float tsweep)
:f_low(freq_low), f_hi(freq_high), relevant_time(false) {
  cout << "Creating 1 GPR" << endl;

  this->bw = this->f_low - this->f_hi;

  thread thread_dac(waveformGenerator);
  thread thread_fft(processFFT);

  thread_dac.join();
  thread_fft.join();
}

GPR* GPR::getInstance(const float freq_start = 1200.0F, const float freq_stop = 2900.0F, const float tsweep = 10.0F) {
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangerous in case two instance threads wants to access at the same time
     */
    if(GPR::_singleton == nullptr){
        GPR::_singleton = new GPR(freq_start, freq_stop, tsweep);
    }
    return GPR::_singleton;
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

  Waveform::ramp(wf, total_steps, 0, MCP4921::MAX_VAL);

  do {
    for(unsigned short int i = 0; i < total_steps; i++) {
      this->relevant_time = (total_steps / i >= 0.1 && total_steps / i <= 0.9);
      dac->setRawValue(wf[i]);
      usleep(step_hold_us);
    }
  } while(1);

  mcp4725_close(&lodrive);
}

void processFFT() {
  if(is_POI) {

  }
}

GPR::~GPR() {
  cout << "Destroying 1 GPR" << endl;
}
