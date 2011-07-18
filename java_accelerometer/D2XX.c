#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "../ftdi/LIS302DL_D2XX.h"
#include "../ftdi/LIS302DL.h"
#include "D2XX.h"


signed char *x_axis_filt=0;
signed char *y_axis_filt=0;
signed char *z_axis_filt=0;

#define RANGE_8G	// If defined then we measure +8G to -8G
			// otherwise we measure +2G to -2G

char *strbuf=0;

#define BUFLEN 128
JNIEXPORT jint JNICALL Java_D2XX_AccelerometerInit
  (JNIEnv *env, jobject obj, jint len, jint freq)
{
	unsigned char val;
	unsigned char cutoff;
	extern char *strbuf;

	strbuf = realloc(strbuf, 128);

	extern char LIS302DL_debug;
	LIS302DL_debug = 0;

	extern int filtlen;


	filtlen=len;
	cutoff=freq;

	if ((x_axis_filt = realloc(x_axis_filt, filtlen)) == NULL)
	{
		printf("Fatal error allocating %d bytes for filter!\n", filtlen);
		exit(1);
	}
	if ((y_axis_filt = realloc(y_axis_filt, filtlen)) == NULL)
	{
		printf("Fatal error allocating %d bytes for filter!\n", filtlen);
		exit(1);
	}
	if ((z_axis_filt = realloc(z_axis_filt, filtlen)) == NULL)
	{
		printf("Fatal error allocating %d bytes for filter!\n", filtlen);
		exit(1);
	}


	// Make sure we can talk to the accelerometer
	if(LIS302DL( REG_WhoAmI, &val, 'r'))
	{
		printf("Error communicating: (WHO_AM_I)\n");
		exit(1);
	}

	val=0x00;
	val |= ( 0 << BV_DR ) | ( 1 << BV_PD );

#ifdef RANGE_8G
	val |= ( 1 << BV_FS );
#else
	val |= ( 0 << BV_FS );
#endif

	val |= ( 0 << BV_STP ) | ( 0 << BV_STM );
	val |= ( 1 << BV_Zen ) | ( 1 << BV_Xen ) | ( 1 << BV_Yen );
	if (LIS302DL( REG_CtrlReg1, &val, 'w'))
			exit(1);
	val=0x00;
	val |= ( 0 << BV_SIM ) | ( 0 << BV_BOOT );
	val |= ( 1 << BV_FDS );
	val |= ( 0 << BV_HPFF_WU2 ) | ( 0 << BV_HPFF_WU1 );
	val |= ( (cutoff & 0x02)>>1 << BV_HPcoeff2 ) | ( (cutoff & 0x01)>>0 << BV_HPcoeff1 );
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

	return;
}


JNIEXPORT jstring JNICALL Java_D2XX_AccelerometerRead
  (JNIEnv *env, jobject obj)
{
	int ret;
	char x_axis, y_axis, z_axis;
	unsigned char status;
	unsigned char overrun, available;
	float ix_g, iy_g, iz_g;
	float fx_g, fy_g, fz_g;
	extern char *strbuf;

	if (strbuf == 0)
	{
		printf("Fatal: accelerometer_read called ahead of accelerometer_init()!!\n");
		exit(1);
	}

	if (LIS302DL( REG_StatusReg, &status, 'r'))
		exit(1);

	available = (status & ( 1 << BV_ZYXDA )) > 0;
	overrun = (status & ( 1 << BV_ZYXOR )) > 0;
	if (available != 0)
	{
		if (LIS302DL( REG_OutX, (unsigned char *)&x_axis, 'r'))
			exit(1);
		if (LIS302DL( REG_OutY, (unsigned char *)&y_axis, 'r'))
			exit(1);
		if (LIS302DL( REG_OutZ, (unsigned char *)&z_axis, 'r'))
			exit(1);
		FiltAddData(x_axis, x_axis_filt);
		FiltAddData(y_axis, y_axis_filt);
		FiltAddData(z_axis, z_axis_filt);
	}

#ifdef RANGE_8G
	// +8G -> -8G swing
	// So we know that -127 = -8G and +127 = +8G
	// If we multiply the value by 0.062992 (8/127), we will have a value in 'g's
	#define G_MULT	0.062992
#else
	// +2G -> -2G swing
	// So we know that -127 = -2G and +127 = +2G
	// If we multiply the value by 0.015748 (127/8), we will have a value in 'g's
	#define G_MULT	0.015748
#endif
	ix_g = x_axis * G_MULT;
	iy_g = y_axis * G_MULT;
	iz_g = z_axis * G_MULT;
	fx_g = FiltGetVal(x_axis_filt) * G_MULT;
	fy_g = FiltGetVal(y_axis_filt) * G_MULT;
	fz_g = FiltGetVal(z_axis_filt) * G_MULT;

	if ((ret = snprintf(strbuf, BUFLEN, "IX:% 01.6f IY:% 01.6f IZ:% 01.6f FX:% 01.6f FY:% 01.6f FZ:% 01.6f O:%01d A:%01d\n", ix_g, iy_g, iz_g, fx_g, fy_g, fz_g, overrun, available)) >= BUFLEN)
	{
		printf("Error: resulting string too long for storage size (%d >= %d)\n", ret, BUFLEN);
		exit(1);
	}

	return (*env)->NewStringUTF(env, strbuf);
}
