#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "mcp4725.h"
#include <time.h>

#define MAX_LO_DRIVE      4095
#define DAC_SAMPLES_HZ      30000

void triangleMode(struct mcp4725 dac, unsigned int freq) {
  printf ("Triangle wave mode selected\n");
  float val = 0.0;
  float period = (1.0 / (float)freq);
  unsigned int total_steps = (period * (float)DAC_SAMPLES_HZ);
  float step_hold_us = (period / total_steps) * 1e6;
  float step_val = ((float)MAX_LO_DRIVE / (float)((float)total_steps / 2.0));

  unsigned int waveform [total_steps];

  printf ("\nPeriod: %fµs\nTotal steps: %f\nStep hold: %fµs\nStep value: %fV\n\n", period, total_steps, step_hold_us, step_val);

  for(unsigned short int i = 0; i < total_steps; i++) {
    if((waveform[i] + step_val) > MAX_LO_DRIVE || (waveform[i] + step_val) < 0.0) {
      step_val *= -1.0;
    }

    waveform[i] += step_val;
  }

  do {
    mcp4725_setvolts(&dac, val);
    usleep(step_hold_us);
  } while (1);
}

int main(int argc, char **argv) {
  printf ("STARTING...\n");
  struct mcp4725 lodrive = {
    .i2c_id = 0x60,
    .fullscale = MAX_LO_DRIVE
  };

  if(getuid()) {
    printf ("This program needs to be run as root.\n");
    exit (1);
  }

  mcp4725_init(&lodrive);
  printf ("DAC INIT OK.\n");
  triangleMode(lodrive, 100);
      
  mcp4725_close(&lodrive);
}
