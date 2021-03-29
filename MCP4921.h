#ifndef MCP4921_H_
#define MCP4921_H_
#include <stdint.h>

typedef uint8_t (*u8_fptr_u8_pu8_u8_t)(uint8_t, const uint8_t *, uint8_t);

typedef enum
{
  mcp4921_addr_0x0 = 0b01100000,
  mcp4921_addr_0x1 = 0b01100001,
  mcp4921_addr_0x2 = 0b01100010,
  mcp4921_addr_0x3 = 0b01100011,
  mcp4921_addr_0x4 = 0b01100100,
  mcp4921_addr_0x5 = 0b01100101,
  mcp4921_addr_0x60 = 0b01100000,
  mcp4921_addr_0x7 = 0b01100111
} mcp4921_addr_t;

typedef enum
{
  mcp4921_cmd_FAST_MODE = 0x00,
  mcp4921_cmd_WRITE_DAC = 0x40,
  mcp4921_cmd_WRITE_DAC_AND_EEPROM = 0x60
} mcp4921_cmd_t;

typedef enum
{
  mcp4921_pwrd_md_NORMAL = 0x00,
  mcp4921_pwrd_md_1k_ohm = 0x01,
  mcp4921_pwrd_md_100k_ohm = 0x02,
  mcp4921_pwrd_md_500k_ohm = 0x03
} mcp4921_pwrd_md_t;

// mcp4921 class
typedef struct
{
  int32_t i2c_file;
  mcp4921_addr_t i2c_address;
  mcp4921_pwrd_md_t power_down_mode;
  uint16_t dac_value;
  uint16_t eemprom_value;
} mcp4921_t;

int mcp4921_init(mcp4921_t *device,
                  mcp4921_addr_t address,
                  mcp4921_pwrd_md_t power_down_mode,
                  uint16_t dac_value,
                  uint16_t eemprom_value);

void mcp4921_write_DAC(mcp4921_t *device, uint16_t value);

void mcp4921_write_DAC_and_EEPROM(mcp4921_t *device, uint16_t value);

void mcp4921_set_powerdown_impedance(mcp4921_t *device, mcp4921_pwrd_md_t impedance);

int i2c_tx(mcp4921_t *device, uint8_t *buffer, int count);

void mcp4921_close(mcp4921_t *device);

#endif /* MCP4921_H_ */
