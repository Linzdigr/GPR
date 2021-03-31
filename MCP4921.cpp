#include <stdint.h>
#include <stdio.h>
#include <unistd.h> 
#include <iostream>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <fcntl.h>

  using namespace std;

#include "MCP4921.h"

MCP4921::MCP4921(uint16_t val,
    uint32_t spi_speed,
    bool channel_b,
    bool buffered_output,
    bool gain2x) {
  if(val > MAX_DAC_VALUE) {
    cout << "Requested DAC value is beyong max limit of " << MCP4921::MAX_DAC_VALUE << "! Setting to the max admissible value." << endl;
    val = MCP4921::MAX_DAC_VALUE;
  }

  this->value = val;

  this->dac_select = channel_b ? DAC_SELECT_B : DAC_SELECT_B;
  this->output_buffered = buffered_output ? BUFFERED_OUTPUT : UNBUFFERED_OUTPUT;
  this->selected_gain = gain2x ? OUTPUT_GAIN_2X : OUTPUT_GAIN_1X;

  char *filename = (char*)"/dev/spidev0.0";

  if((this->spi_fd = open(filename, O_RDWR)) < 0) {
    throw "Failed to open spi device";
  }

  if(ioctl(this->spi_fd, SPI_IOC_WR_MODE, SPI_MODE_0) < 0) {
    throw string("Could not set SPIMode (WR)...ioctl fail");
  }

  if (ioctl(this->spi_fd, SPI_IOC_RD_MODE, SPI_MODE_0) < 0) {
    throw string("Could not set SPIMode (RD)...ioctl fail");
  }

  if(ioctl(this->spi_fd, SPI_IOC_WR_BITS_PER_WORD, 8) < 0) {
    throw string("Could not set SPI bitsPerWord (WR)...ioctl fail");
  }

  if(ioctl(this->spi_fd, SPI_IOC_RD_BITS_PER_WORD, 8) < 0) {
    throw string("Could not set SPI bitsPerWord(RD)...ioctl fail");
  }

  if(ioctl(this->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
    throw string("Could not set SPI speed (WR)...ioctl fail");
  }

  if(ioctl(this->spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed) < 0) {
    throw string("Could not set SPI speed (RD)...ioctl fail");
  }

  // writing to device
  this->spi_tx();
}

void MCP4921::spi_tx() {
  if(write(this->spi_fd, this->raw_buffer, MCP4921::REGISTER_BIT_SIZE) != MCP4921::REGISTER_BIT_SIZE) {
    throw "Error when sending to i2c bus";
  }
}

void MCP4921::setRawValue(uint16_t value) {
  if(value > MCP4921::MAX_DAC_VALUE) {
    value = MCP4921::MAX_DAC_VALUE;
  }

  this->raw_buffer[0] = (this->dac_select | this->output_buffered | this->selected_gain) | value >> 8;
  this->raw_buffer[1] = (uint8_t)(value);

  this->spi_tx();
}

MCP4921::~MCP4921() {
  cout << "Detroying MCP4921" << endl;
  close(this->spi_fd);
  this->spi_fd = -1;
}
