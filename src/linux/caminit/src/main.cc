#include <stdio.h>
#include "caminit.h"

int main(int argc, char *argv[])
{
	int ret;

	ret = caminit(SENSOR_OV5640, RESOLUTION_640_480);

	return ret;
}
