#define ALSA_PCM_NEW_HW_PARAMS_API  // Use the newer ALSA API

#include <alsa/asoundlib.h>
#include <cstdint>

  using namespace std;

struct {
  char RIFF_marker[4];
  uint32_t file_size;
  char filetype_header[4];
  char format_marker[4];
  uint32_t data_header_length;
  uint16_t format_type;
  uint16_t number_of_channels;
  uint32_t sample_rate;
  uint32_t bytes_per_second;
  uint16_t bytes_per_frame;
  uint16_t bits_per_sample;
} typedef WaveHeader;

class Recorder {
  private:
    snd_pcm_t *device;
    snd_pcm_hw_params_t *hw_params;
    unsigned int rate;
    snd_pcm_format_t format;
    uint8_t *buffer;
    long buffer_frames;
  public:
    Recorder(const char *device_name = (const char *)"hw:0",
          unsigned int rate = 96000,
          snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE,
          long _buffer_frames = 128);
    unsigned int captureBloc(uint16_t *&sink);
    // void recordToWaveFile(const char *filename = "test.wav");
    WaveHeader* genericWAVHeader(uint32_t sample_rate, uint16_t bit_depth, uint16_t channels);
    int writeWAVHeader(int fd, WaveHeader *hdr);
    ~Recorder();
};
