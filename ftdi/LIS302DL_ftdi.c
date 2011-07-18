#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "LIS302DL_ftdi.h"

int max (int a, int b)
{
  return (a > b) ? a : b;
}

unsigned char reverse (unsigned char data)
{
  int i;
  unsigned char ret = 0;

  for (i = 0; i < 8; i++)
    {
      ret = ret << 1;
      if ((data & (1 << i)) > 0)
	ret = ret | 0x01;
    }
  return ret;
}

static int send_buf (struct ftdi_context *ftdic, const unsigned char *buf, int size)
{
  int r;
  r = ftdi_write_data (ftdic, (unsigned char *) buf, size);
  if (r < 0)
    {
      fprintf (stderr, "ftdi_write_data: %d, %s\n", r,
	       ftdi_get_error_string (ftdic));
      return 1;
    }
  return 0;
}

static int get_buf (struct ftdi_context *ftdic, const unsigned char *buf, int size)
{
  int r;
  r = ftdi_read_data (ftdic, (unsigned char *) buf, size);
  if (r < 0)
    {
      fprintf (stderr, "ftdi_read_data: %d, %s\n", r,
	       ftdi_get_error_string (ftdic));
      return 1;
    }
  return 0;
}

int ft2232_spi_send_command (struct ftdi_context *ftdic, unsigned int writecnt,
			 unsigned int readcnt, const unsigned char *writearr,
			 unsigned char *readarr)
{
  static unsigned char *buf = NULL;
  /* failed is special. We use bitwise ops, but it is essentially bool. */
  int i = 0, ret = 0, failed = 0;
  int bufsize;
  static int oldbufsize = 0;

  if (writecnt > 65536 || readcnt > 65536)
    return -1;

  /* buf is not used for the response from the chip. */
  bufsize = max (writecnt + 9, 260 + 9);
  /* Never shrink. realloc() calls are expensive. */
  if (bufsize > oldbufsize)
    {
      buf = realloc (buf, bufsize);
      if (!buf)
	{
	  fprintf (stderr, "Out of memory!\n");
	  exit (1);
	}
      oldbufsize = bufsize;
    }

  /*
   * Minimize USB transfers by packing as many commands as possible
   * together. If we're not expecting to read, we can assert CS#, write,
   * and deassert CS# all in one shot. If reading, we do three separate
   * operations.
   */
  //fprintf(stdout, "Assert CS#\n");
  buf[i++] = SET_BITS_LOW;
  buf[i++] = 0 & ~0x08;		/* assertive */
  buf[i++] = 0x0b;

  if (writecnt)
    {
      buf[i++] = 0x11;
      buf[i++] = (writecnt - 1) & 0xff;
      buf[i++] = ((writecnt - 1) >> 8) & 0xff;
      memcpy (buf + i, writearr, writecnt);
      i += writecnt;
    }

  /*
   * Optionally terminate this batch of commands with a
   * read command, then do the fetch of the results.
   */
  //usleep(10);
  if (readcnt)
    {
      buf[i++] = 0x20;
      buf[i++] = (readcnt - 1) & 0xff;
      buf[i++] = ((readcnt - 1) >> 8) & 0xff;
      ret = send_buf (ftdic, buf, i);
      failed = ret;
      /* We can't abort here, we still have to deassert CS#. */
      if (ret)
	fprintf (stderr, "send_buf failed before read: %i\n", ret);
      i = 0;
      if (ret == 0)
	{
	  /*
	   * FIXME: This is unreliable. There's no guarantee that
	   * we read the response directly after sending the read
	   * command. We may be scheduled out etc.
	   */
	  ret = get_buf (ftdic, readarr, readcnt);
	  failed |= ret;
	  /* We can't abort here either. */
	  if (ret)
	    fprintf (stderr, "get_buf failed: %i\n", ret);
	}
    }

  //fprintf(stdout, "De-assert CS#\n");
  buf[i++] = SET_BITS_LOW;
  buf[i++] = 0x08;
  buf[i++] = 0x0b;
  ret = send_buf (ftdic, buf, i);
  failed |= ret;
  if (ret)
    fprintf (stderr, "send_buf failed at end: %i\n", ret);

  return failed ? -1 : 0;
}

int ft2232_spi_init (struct ftdi_context *ftdic)
{
  unsigned char buf[128];
  unsigned char ret;
  int f;

  if (ftdi_set_interface (ftdic, INTERFACE_A) < 0)
    {
      fprintf (stderr, "Unable to select interface: %s\n", ftdic->error_str);
    }

  if (ftdi_usb_reset (ftdic) < 0)
    {
      fprintf (stderr, "Unable to reset FTDI device\n");
    }

  if (ftdi_set_latency_timer (ftdic, 1) < 0)
    fprintf (stderr, "Unable to set latency timer\n");

  if (ftdi_write_data_set_chunksize (ftdic, 255))
    {
      fprintf (stderr, "Unable to set chunk size\n");
    }

  if (ftdi_set_bitmode (ftdic, 0x00, BITMODE_BITBANG_SPI) < 0)
    {
      fprintf (stderr, "Unable to set bitmode to SPI\n");
    }

  if (ftdi_setflowctrl (ftdic, SIO_DISABLE_FLOW_CTRL) < 0)
    {
      fprintf (stderr, "Unable to set flow control\n");
    }

  fprintf (stdout, "Disable divide-by-5 front stage\n");
  buf[0] = 0x8a;		/* Disable divide-by-5. */
  if (send_buf (ftdic, buf, 1))
    return -1;

  fprintf (stdout, "Set clock divisor\n");
  buf[0] = 0x86;		/* command "set divisor" */
  /* valueL/valueH are (desired_divisor - 1) */
  buf[1] = (3 - 1) & 0xff;
  buf[2] = ((3 - 1) >> 8) & 0xff;
  if (send_buf (ftdic, buf, 3))
    return -1;

  fprintf (stdout, "SPI clock is %fMHz\n",
	   (double) (60 / (((3 - 1) + 1) * 2)));

  /* Disconnect TDI/DO to TDO/DI for loopback. */
  fprintf (stdout, "No loopback of TDI/DO TDO/DI\n");
  buf[0] = LOOPBACK_OFF;
  if (send_buf (ftdic, buf, 1))
    return -1;

  return 0;
}

void printbin (unsigned char data)
{
  int i;

  for (i = 7; i >= 0; i--)
    {
      if ((data & (1 << i)) > 0)
	printf ("1");
      else
	printf ("0");
    }
  printf ("\n");
}

unsigned char ReadAccelerometerRegister (struct ftdi_context *ftdic, char reg)
{
  unsigned char buf[128];
  unsigned char rbuf[128];

  buf[0] = 0xAB;
  buf[1] = 0x87;
  if (send_buf (ftdic, buf, 2))
    return -1;
  // Flush any output
	//printf("Before flush1: %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
  if (get_buf (ftdic, buf, 4))
    return -1;
	//printf("After flush1: %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

  SPIDat.bits.rw = 1;
  SPIDat.bits.multi = 0;
  SPIDat.bits.addr = reg;
  buf[0] = SPIDat.val;
  ft2232_spi_send_command (ftdic, 1, 1, buf, rbuf);

  if (get_buf (ftdic, buf, 2))
    return -1;
  //printf("Got1: 0x%02x 0x%02x\n", buf[0], buf[1]);
  buf[0] = 0x20;
  buf[1] = 0x67;
  ft2232_spi_send_command (ftdic, 2, 0, buf, rbuf);
  if (get_buf (ftdic, buf, 2))
    return -1;

  return (buf[0]);
  //printf("Got2: 0x%02x 0x%02x\n", buf[0], buf[1]);
}

