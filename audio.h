#include <alsa/asoundlib.h>
#include <cstdint>

  using namespace std;

class Audio {
  private:
    snd_pcm_t *device;
    snd_pcm_hw_params_t *hw_params;
    unsigned int rate;
    snd_pcm_format_t format;
    uint8_t *buffer;
    uint16_t buffer_frames;
  public:
    Audio(const char *device_name = (const char *)"hw:0",
          unsigned int rate = 96200,
          snd_pcm_format_t format = SND_PCM_FORMAT_S32_LE,
          uint16_t _buffer_frames = 128);
    void capture();
    ~Audio();
};
