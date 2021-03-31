
class GPR {
  private:
    static GPR* _instance;
    static constexpr float c = 299792458.0;
    float bw;
    float sweep_length;
    float f_low;
    float f_hi;
    volatile bool relevant_time;   // shared between threads DAC and FFT
  protected:
    GPR(const float freq_start, const float freq_stop, const float tsweep);
  public:
    static GPR* getInstance(const float freq_start = 1200.0F, const float freq_stop = 2900.0F, const float tsweep = 10.0F);
    GPR(GPR &other) = delete;
    float freq2Dist(const float f);
    void waveformGenerator();
    float beat2Dist(float f);
    void processFFT();
    void operator=(const GPR &) = delete;
    ~GPR();
};
