#include <iostream>

using namespace std;

#include "waveform.h"

void Waveform::ramp(uint16_t *sink,  unsigned int points, double min, double max, float symmetry = 0.5) {
  float last_val = 0.0;
  sink = new uint16_t [points];
  float step_val = ((float)max / (float)((float)points / 2.0));

  for(unsigned short int i = min; i < (points + min); i++) {
    if((unsigned int)(last_val + step_val) > max || (unsigned int)(last_val + step_val) < min) {
      step_val *= -1.0F;
    }

    sink[i] = (uint16_t)(last_val + step_val);

    last_val = sink[i];
  }
}

Waveform::Waveform(/* args */) { }

Waveform::~Waveform() {
  cout << "Destroying 1 Waveform" << endl;
}
