#include <stdint.h>

class Waveform {
private:
  /* data */
public:
  Waveform(/* args */);
  static void ramp(uint16_t *sink, unsigned int points, double min, double max, float symmetry = 0.5F);
  ~Waveform();
};
