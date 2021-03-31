#ifndef MCP4921_H_
#define MCP4921_H_
#include <stdint.h>
#include <linux/spi/spidev.h>

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
    uint8_t spi_mode;
    uint32_t spi_speed;
    uint8_t spi_bits_per_word;
    uint16_t spi_delay;
    int spi_fd;
  public:
    static const uint16_t MIN_DAC_VALUE = 0;
    static const uint16_t MAX_DAC_VALUE = 4095;
    static const uint32_t MAX_SPI_SPEED = 2e7; // 20MHz
    static const uint32_t REGISTER_BYTE_SIZE = 2;
    MCP4921(uint16_t val = MCP4921::MIN_DAC_VALUE,
            bool channel_b = false,
            bool buffered_output =false,
            bool gain2x = false,
            uint8_t spi_mode = SPI_MODE_0,
            uint32_t spi_hz = MCP4921::MAX_SPI_SPEED,
            uint32_t spi_bits_per_word = 8,
            uint16_t spi_delay = 0
          );
    void setRawValue(uint16_t value);
    int spi_tx();
    ~MCP4921();
  };

#endif /* MCP4921_H_ */
