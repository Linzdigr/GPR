#include <string.h>
#include <stdio.h>
#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
#include "mcp4725.h"

int mcp4725_init(struct mcp4725 *device)
{
    device->error[0] = '\0';
    memset(&device->buffer, 0, 6*sizeof(unsigned char));

	char *filename = (char*)"/dev/i2c-1";
	if ((device->i2c_file = open(filename, O_RDWR)) < 0) {
		strcpy(device->error, "Failed to open the i2c bus");
		return 1;
	}
	
	if (ioctl(device->i2c_file, I2C_SLAVE, device->i2c_id) < 0) {
		strcpy(device->error, "Failed to acquire bus access and/or talk to slave");
		return 1;
	}
	return 0;
}

/* Write a short int to the DAC 

    0 - 4095 : 0 to FS.

    Also turns on the output - MODE_NORMAL
*/
int mcp4725_write(struct mcp4725 *device, unsigned short int output)
{
    device->buffer[1] = output & 0x00ff;
    device->buffer[0] = (output & 0x0f00)>>8;
    device->error[0] = '\0';

    return writebuffer(device, 2);
}

/* Write raw buffer to the DAC */
 int writebuffer(struct mcp4725 *device, int count)
 {
//    printf ("buf[1] = %02x, buf[0] = %02x\n", device->buffer[1], device->buffer[0]);

 	if (write(device->i2c_file, device->buffer, count) != count) {
		strcpy(device->error, "Failed to write to the i2c bus");
        return 1;
	}
    return 0;
}

void mcp4725_close(struct mcp4725 *device)
{
    close(device->i2c_file);
    device->i2c_file = -1;
}

/* Write a voltage to the DAC - mcp4725.fullscale has to be set for this to work */
int mcp4725_setvolts(struct mcp4725 *device, float volts)
{
    unsigned short int adcvalue;

    if (!device->fullscale) {
        strcpy (device->error, "Full scale value not set");
        return 1;
    }

    if (volts > device->fullscale || volts < 0) {
        strcpy(device->error, "Voltage out of range.");
        return 1;
    }

    adcvalue = (unsigned short int)((volts / device->fullscale) * 4095);
    return (mcp4725_write(device, adcvalue));
}

/* Sets the output mode 

    MODE_NORMAL : Outputs a voltage
    MODE_1K     : 0V output, 1K resistor to ground
    MODE_100K   : 0V output, 100K resistor to ground
    MODE_500K   : 0V output, 500K resistor to ground
*/
int mcp4725_setmode(struct mcp4725 *device, enum mcp4725_mode mode)
{
    device->buffer[0] = (device->buffer[0] & 0xcf) | mode;
    return writebuffer(device, 2);
}
