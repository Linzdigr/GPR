#include "mcp4725.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>        //Needed for I2C port
#include <fcntl.h>         //Needed for I2C port (open)
#include <sys/ioctl.h>     //Needed for I2C port
#include <linux/i2c-dev.h> //Needed for I2C port

int i2c_tx(mcp4725_t *device, uint8_t *buffer, int count) {
  if(write(device->i2c_file, buffer, count) != count) {
    perror("Error when sending to i2c bus.\n");
    return 1;
  }
  return 0;
}

int mcp4725_init(mcp4725_t *device,
                  mcp4725_addr_t address,
                  mcp4725_pwrd_md_t power_down_mode,
                  uint16_t dac_value,
                  uint16_t eemprom_value)
{
  uint8_t temp[4];

  // update device attributes
  device->i2c_address = address;
  device->power_down_mode = power_down_mode;
  device->dac_value = dac_value;
  device->eemprom_value = eemprom_value;

  char *filename = (char*)"/dev/i2c-1";

  if ((device->i2c_file = open(filename, O_RDWR)) < 0) {
    perror("Failed to open i2c bus.\n");
    return 1;
  }

  if (ioctl(device->i2c_file, I2C_SLAVE, device->i2c_address) < 0) {
    perror("Failed to acquire bus access and/or talk to slave.\n");
    return 1;
  }

  // setting up data set to be sent to the device
  temp[0] = (mcp4725_cmd_FAST_MODE | (power_down_mode << 4)) | dac_value >> 8;
  temp[1] = (uint8_t)(dac_value);
  temp[2] = temp[0];
  temp[3] = temp[1];

  // writing to device
  i2c_tx(device, temp, 6);
}

void mcp4725_write_DAC(mcp4725_t *device, uint16_t value)
{
  uint8_t temp[3];

  device->dac_value = value;

  // setting up data set
  temp[0] = (mcp4725_cmd_FAST_MODE | (device->power_down_mode << 4)) | value >> 8;
  temp[1] = (uint8_t)(value);

  i2c_tx(device, temp, 3);
}

void mcp4725_write_DAC_and_EEPROM(mcp4725_t *device, uint16_t value)
{
  uint8_t temp[6];

  device->dac_value = value;
  device->eemprom_value = value;

  // setting up data set to be sent to the device
  temp[0] = mcp4725_cmd_WRITE_DAC_AND_EEPROM | (device->power_down_mode << 1);
  temp[1] = (uint8_t)(value >> 4);
  temp[2] = (uint8_t)(value << 4);
  temp[3] = temp[0];
  temp[4] = temp[1];
  temp[5] = temp[2];

  // writing to the device
  i2c_tx(device, temp, 6);
}

void mcp4725_set_powerdown_impedance(mcp4725_t *device, mcp4725_pwrd_md_t impedance)
{
  device->power_down_mode = impedance;
}

void mcp4725_close(mcp4725_t *device)
{
  close(device->i2c_file);
  device->i2c_file = -1;
}

