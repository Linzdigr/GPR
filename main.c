#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include "mcp4725.h"

#define MAX_LO_DRIVE      4095
#define DAC_SAMPLES_HZ      30000

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

void triangleMode(mcp4725_t *dac, unsigned int freq) {
  printf ("Triangle wave mode selected\n");

  float period = (1.0 / (float)freq);
  unsigned int total_steps = (period * (float)DAC_SAMPLES_HZ);
  float step_hold_us = (period / total_steps) * 1e6;
  float step_val = ((float)MAX_LO_DRIVE / (float)((float)total_steps / 2.0));

  uint16_t waveform [total_steps];

  printf ("\nPeriod: %fµs\nTotal steps: %u\nStep hold: %fµs\nStep value: %fV\n\n", period, total_steps, step_hold_us, step_val);

  float last_val = 0.0;

  for(unsigned short int i = 0; i < total_steps; i++) {
    if((unsigned int)(last_val + step_val) > MAX_LO_DRIVE || (unsigned int)(last_val + step_val) < 0.0) {
      step_val *= -1.0;
    }

    waveform[i] = (uint16_t)(last_val + step_val);

    last_val = waveform[i];
  }

  do {
    for(unsigned short int i = 0; i < total_steps; i++) {
      mcp4725_write_DAC(dac, waveform[i]);
      usleep(step_hold_us);
    }
  } while (1);
}

int main(int argc, char **argv) {
  printf ("STARTING...\n");
  mcp4725_t lodrive;

  if(getuid()) {
    printf ("This program needs to be run as root.\n");
    exit (1);
  }

  mcp4725_init(&lodrive, mcp4725_addr_0x60, mcp4725_pwrd_md_NORMAL, 4095, 4095);

  printf("DAC INIT OK.\n");
  triangleMode(&lodrive, 100);
      
  mcp4725_close(&lodrive);
}
