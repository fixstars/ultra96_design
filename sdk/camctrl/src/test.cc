#include <stdio.h>
#include "camctrl.h"

int main(int argc, char *argv[])
{
	int ret;
	unsigned char *buf;

	ret = caminit(SENSOR_OV5640, RESOLUTION_640_480, 3, 480 * 270 * 3);
	buf = get_frame_buffer();

	return 0;
}
