#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ftdi.h>

#define FTDI_VID                0x0403
#define FTDI_FT2232H_PID        0x6010
#define FTDI_FT4232H_PID        0x6011
#define AMONTEC_JTAGKEY_PID     0xCFF8

#define  LOOPBACK_OFF 0x85
#define  LOOPBACK_ON 0x84

#define ACC_X_AXIS	0x29
#define ACC_Y_AXIS	0x2b
#define ACC_Z_AXIS	0x2d

#define getbit(val, bitno)	((val & (1 << bitno)) >> bitno)

#define BV_CLK		0
#define BV_SDI		1
#define BV_SDO		2
#define BV_CS		3

#define CV_READ		0
#define CV_MULTI	1

#define BITMODE_BITBANG_NORMAL  1
#define BITMODE_BITBANG_SPI     2

union
{
  struct
  {
    unsigned char addr:6;
    unsigned char multi:1;
    unsigned char rw:1;
  } bits;
  unsigned char val;
} SPIDat;

int max (int a, int b);
unsigned char reverse (unsigned char data);
static int send_buf (struct ftdi_context *ftdic, const unsigned char *buf, int size);
static int get_buf (struct ftdi_context *ftdic, const unsigned char *buf, int size);
int ft2232_spi_send_command (struct ftdi_context *ftdic, unsigned int writecnt,
			 unsigned int readcnt, const unsigned char *writearr,
			 unsigned char *readarr);

int ft2232_spi_init (struct ftdi_context *ftdic);
void printbin (unsigned char data);
unsigned char ReadAccelerometerRegister (struct ftdi_context *ftdic, char reg);

