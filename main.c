#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "mcp4725.h"
#include <time.h>

#define MAX_LO_DRIVE_V      25
#define DAC_SAMPLES_HZ      300000

int main(int argc, char **argv) {
  struct mcp4725 lodrive = {
    .i2c_id = 0x60,
    .fullscale = MAX_LO_DRIVE_V
  };

  if(getuid()) {
    printf ("This program needs to be run as root.\n");
    exit (1);
  }

  mcp4725_init(&lodrive);

  triangleMode(&lodrive, 100);
      
  mcp4725_close(&lodrive);
}

void triangleMode(struct mcp4725 *dac, unsigned int freq) {
  float val = 0;
  float period = (1 / freq);
  float total_steps = (period * DAC_SAMPLES_HZ);
  float step_hold_us = (period / total_steps) * 1e6;
  float step_val = (MAX_LO_DRIVE_V / (total_steps / 2));

  do {
    if(val >= MAX_LO_DRIVE_V) {
      val -= step_val;
    } else {
      val += step_val;
    }
    mcp4725_setvolts(&dac, val);

    usleep(step_hold_us);
  } while (1);
}
