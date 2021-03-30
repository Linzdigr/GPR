#ifndef MCP4921_H_
#define MCP4921_H_
#include <stdint.h>

#define DAC_SELECT_A          0x80
#define DAC_SELECT_B          0x00

#define BUFFERED_OUTPUT       0x40
#define UNBUFFERED_OUTPUT     0x00

#define OUTPUT_GAIN_1X        0x20
#define OUTPUT_GAIN_2X        0x00

class MCP4921 {
  private:
    uint8_t raw_buffer [2];
    uint16_t value;
    uint8_t dac_select;
    uint8_t output_buffered;
    uint8_t selected_gain;
    int spi_fd;
  public:
    static const uint16_t MIN_DAC_VALUE = 0;
    static const uint16_t MAX_DAC_VALUE = 4095;
    static const uint32_t DEFAULT_SPI_SPEED = 20000000; // 20MHz
    static const uint8_t REGISTER_BIT_SIZE = 16;
    MCP4921(uint16_t val = MCP4921::MIN_DAC_VALUE,
            uint32_t spi_speed = MCP4921::DEFAULT_SPI_SPEED,
            bool channel_b = false,
            bool buffered_output =false,
            bool gain2x = false);
    void setRawValue(uint16_t value);
    void spi_tx();
    ~MCP4921();
  };

#endif /* MCP4921_H_ */
