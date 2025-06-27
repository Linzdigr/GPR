#include <iostream>

using namespace std;

#include "waveforms.h"

Waveform::Waveform(/* args */) { }

void Waveform::ramp(uint16_t *sink,  unsigned int points, double min, double max) {
  float step_val = ((float)max / (float)(points));

  for(unsigned int i = min; i < max; i++) {
    sink[i] = (uint16_t)(step_val*i);
  }
}

Waveform::~Waveform() {
  cout << "Destroying 1 Waveform" << endl;
}
