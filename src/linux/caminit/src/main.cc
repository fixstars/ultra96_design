#include <stdio.h>
#include "camctrl.h"

int main(int argc, char *argv[])
{
	int ret;

	ret = caminit(SENSOR_IMX219, RESOLUTION_640_480);

	return ret;
}
