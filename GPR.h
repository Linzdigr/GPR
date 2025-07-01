#include <vector>
#include <mutex>
#include <condition_variable>

#define HANN_FUNCTION       0

class GPR {
  private:
    static GPR* _instance;
    static constexpr float c = 299792458.0;
    float bw;
    float sweep_length_us;
    float f_low;
    float f_hi;
    bool relevant_time;   // shared between threads WaveformGenerator / FFT / Recorder
    bool recorder_ready;
    vector<int32_t> sweep_data;   // shared between threads Recorder and FFT
    vector<int32_t> sweep_data_filtered;   // shared between threads Recorder and FFT
    mutex mtx_sweep_data;
    condition_variable cv_sweep_data;
    std::vector<uint8_t> spectrogram_buffer;  // grayscale, column-major
    uint32_t spectrogram_height = 0;
    uint32_t spectrogram_width = 0;
    uint32_t window_size = 1024;
    uint32_t sample_rate = 48000;

  protected:
    GPR(const float freq_start, const float freq_stop, const float tsweep_us);

  public:
    static GPR* getInstance(const float freq_start = 1.2e9F, const float freq_stop = 2.7e9F, const float tsweep_us = 20000);
    GPR(GPR &other) = delete;
    float freq2Dist(const float f);
    void waveformGenerator();
    float beat2Dist(float f);
    void record();
    void processFFT();
    static void windowing(int32_t *data, unsigned int len, unsigned int method = HANN_FUNCTION);
    static void generateSpectrogramImageInterlaced(const char* output_file, int32_t* data, uint32_t num_samples, uint32_t sample_rate, uint32_t window_size, uint32_t hop_size);
    void updateSpectrogramImage(const char* output_file, int32_t* data, uint32_t num_samples);
    void operator=(const GPR &) = delete;
    ~GPR();
};
