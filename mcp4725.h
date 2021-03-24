/*
    Default I2C bus speed is 100,000 - to up it to 400,000 change the line in config.txt

    dtparam=i2c_arm=on,i2c_arm_baudrate=400000

*/
struct mcp4725 {
    int i2c_id;                 // I2C buss ID
    int i2c_file;               // Internal use
    char error[80];             // The last error returned
    unsigned char buffer[6];    // I/O buffer - set by read command, used internally by write command
    int lastvalue;              // The last value (12 bit int) sent to the DAC
    float fullscale;            // The full scale voltage of the DAC - set before using voltage.
    float voltage;              // The output voltage set by mcp4725_setvolts()
};

enum mcp4725_mode {                 // The mode for the mcp4725 - 
    MODE_NORMAL = 0x00,           // Outputs a voltage
    MODE_1K = 0x10,               // 0V output, 1K resistor to ground
    MODE_100K = 0x20,             // 0V output, 100K resistor to ground
    MODE_500K = 0x30              // 0V output, 500K resistor to ground
};

int mcp4725_init(struct mcp4725 *device);
void mcp4725_close(struct mcp4725 *device);
int mcp4725_write(struct mcp4725 *device, unsigned short int output);
int mcp4725_setvolts(struct mcp4725 *device, float volts);
int mcp4725_setmode(struct mcp4725 *device, enum mcp4725_mode mode);
int writebuffer(struct mcp4725 *device, int count);
