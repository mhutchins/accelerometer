#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "libftd2xx/ftd2xx.h"
#include "LIS302DL_D2XX.h"
#include "LIS302DL.h"
#include "filter.h"


unsigned char *x_axis_filt;
unsigned char *y_axis_filt;
unsigned char *z_axis_filt;

extern char LIS302DL_debug;

void vumeter(signed char x_axis, signed char y_axis, signed char z_axis);

int main(int argc, char *argv[])
{
	extern int filtlen;
	unsigned char val;
	int i;


	char x_axis, y_axis, z_axis;
	char o_x_axis, o_y_axis, o_z_axis;
	char delta_x, delta_y, delta_z;

	// Clear the screen
	printf("[H[2J");
	printf("\n\n\n\n\n");

	filtlen=100;

	if (argc == 2)
	{
		filtlen=strtol(argv[1], (char **)NULL, 10);
		printf("Using provided filter length of %d\n", filtlen);
		sleep(1);
	}

	x_axis_filt = malloc(filtlen);
	y_axis_filt = malloc(filtlen);
	z_axis_filt = malloc(filtlen);

	LIS302DL_debug = 0;

	if(LIS302DL( REG_WhoAmI, &val, 'r'))
		exit(1);
	printf("Got WHO_AM_I_RESPONSE: 0x%02x\n", val);

	val=0x00;
	val |= ( 0 << BV_DR ) | ( 1 << BV_PD ) | ( 1 << BV_FS );
	val |= ( 0 << BV_STP ) | ( 0 << BV_STM );
	val |= ( 1 << BV_Zen ) | ( 1 << BV_Xen ) | ( 1 << BV_Yen );
	if (LIS302DL( REG_CtrlReg1, &val, 'w'))
			exit(1);

	val=0x00;
	val |= ( 0 << BV_SIM ) | ( 0 << BV_BOOT );
	val |= ( 1 << BV_FDS );
	val |= ( 0 << BV_HPFF_WU2 ) | ( 0 << BV_HPFF_WU1 );
	val |= ( 0 << BV_HPcoeff2 ) | ( 0 << BV_HPcoeff1 );
	if (LIS302DL( REG_CtrlReg2, &val, 'w'))
			exit(1);

	val=0x00;
	val |= ( 0 << BV_IHL ) | ( 0 << BV_PP_OD );
	val |= ( 0 << BV_I2CFG2 ) | ( 0 << BV_I2CFG1 ) | ( 0 << BV_I2CFG0 );
	val |= ( 0 << BV_I1CFG2 ) | ( 0 << BV_I1CFG1 ) | ( 0 << BV_I1CFG0 );
	if (LIS302DL( REG_CtrlReg3, &val, 'w'))
			exit(1);

	// Re-set the filter (only effective if FDS == 1)
	val=0x00;
	if (LIS302DL( REG_HPFilterReset, &val, 'r'))
			exit(1);

	while(1)
	{
		if (LIS302DL( 0x29, &x_axis, 'r'))
			exit(1);
		if (LIS302DL( 0x2B, &y_axis, 'r'))
			exit(1);
		if (LIS302DL( 0x2D, &z_axis, 'r'))
			exit(1);

		FiltAddData(x_axis, x_axis_filt);
		FiltAddData(y_axis, y_axis_filt);
		FiltAddData(z_axis, z_axis_filt);

		printf("[H");

		printf("Filtered Readings	X: %03d\tY: %03d\tZ: %03d\n", (signed char)FiltGetVal(x_axis_filt), (signed char)FiltGetVal(y_axis_filt),(signed char)FiltGetVal(z_axis_filt));


		vumeter(FiltGetVal(x_axis_filt), FiltGetVal(y_axis_filt), FiltGetVal(z_axis_filt));
		printf("Instant Readings	X: %03d\tY: %03d\tZ: %03d\n", (signed char)x_axis, (signed char)y_axis, (signed char)z_axis);

		vumeter(x_axis, y_axis, z_axis);
	}
}
void vumeter(signed char x_axis, signed char y_axis, signed char z_axis)
{
	int i, j;
	signed char data[3];
	signed char label[3];

	label[0]='X';
	label[1]='Y';
	label[2]='Z';

	data[0]=x_axis;
	data[1]=y_axis;
	data[2]=z_axis;

	for(j=0; j<3; j++)
	{
		for (i = -40 ; i< 0; i++)
		{
			if (data[j] < i)
				printf("%c", label[j]);
			else
				printf(" ");
		}
		for (i = 0 ; i<40; i++)
		{
			if (data[j] > i)
				printf("%c", label[j]);
			else
				printf(" ");
		}
		printf("\n");
	}
}
