#include <vector>
#include <mutex>
#include <condition_variable>

class GPR {
  private:
    static GPR* _instance;
    static constexpr float c = 299792458.0;
    float bw;
    float sweep_length;
    float f_low;
    float f_hi;
    bool relevant_time;   // shared between threads WaveformGenerator / FFT / Recorder
    bool recorder_ready;
    vector<uint16_t> sweep_data;   // shared between threads Recorder and FFT
    mutex mtx_sweep_data;
    condition_variable cv_sweep_data;
  protected:
    GPR(const float freq_start, const float freq_stop, const float tsweep);
  public:
    static GPR* getInstance(const float freq_start = 1.2e9F, const float freq_stop = 2.7e9F, const float tsweep = 1e-3F);
    GPR(GPR &other) = delete;
    float freq2Dist(const float f);
    void waveformGenerator();
    float beat2Dist(float f);
    void record();
    void processFFT();
    void operator=(const GPR &) = delete;
    ~GPR();
};
