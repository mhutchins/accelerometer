#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ftdi.h>
#include "LIS302DL_ftdi.h"

int
main (void)
{
  unsigned char buf[128];
  unsigned char rbuf[128];
  int ret;
  int i;
  struct ftdi_context ftdicon;
  struct ftdi_context *ftdic = &ftdicon;
  unsigned char d;

  if (ftdi_init (ftdic) < 0)
    {
      fprintf (stderr, "ftdi_init failed\n");
      return EXIT_FAILURE;
    }

  if ((ret = ftdi_usb_open (ftdic, 0x0403, 0x6010)) < 0)
    {
      fprintf (stderr, "unable to open ftdi device: %d (%s)\n", ret,
	       ftdi_get_error_string (ftdic));
      return EXIT_FAILURE;
    }

  fprintf (stdout, "FTDI Opened. Starting SPI init\n");

  ft2232_spi_init (ftdic);

  // Enable accelerometer
  buf[0] = 0x20;
  buf[1] = 0x67;
  ret = ft2232_spi_send_command (ftdic, 2, 0, buf, rbuf);

  while (1)
    {
      buf[0] = ReadAccelerometerRegister (ftdic, ACC_X_AXIS);
      buf[1] = ReadAccelerometerRegister (ftdic, ACC_Y_AXIS);
      buf[2] = ReadAccelerometerRegister (ftdic, ACC_Z_AXIS);
      printf ("X: 0x%02x\tY: 0x%02x\tZ: 0x%02x\n", buf[0], buf[1], buf[2]);
      usleep (10);
    }

  if ((ret = ftdi_usb_close (ftdic)) < 0)
    {
      fprintf (stderr, "unable to close ftdi device: %d (%s)\n", ret,
	       ftdi_get_error_string (ftdic));
      return EXIT_FAILURE;
    }

  ftdi_deinit (ftdic);

  return EXIT_SUCCESS;
}
