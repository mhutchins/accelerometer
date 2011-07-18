#include "filter.h"

int filtlen=100;

void FiltAddData(char val, signed char *filtdat)
{
	int i;
	signed char *ptr;
	extern int filtlen;

	ptr = (filtdat + filtlen);

	while(ptr > filtdat)
	{
		*ptr = *(ptr - 1);
		ptr = ptr - 1;
	}
	*filtdat=val;
}

signed char FiltGetVal(signed char *filtdat)
{
	int i;
	long val;
	extern int filtlen;

	val = 0;
	for(i=0;i<filtlen;i++)
		val = val + *(filtdat+i);


	return(val/filtlen);
}
