#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "camctrl.h"
#include "camcfg.h"

static int (*init_camera[])(e_resolution resolution) = {
	init_ov5640,
	init_imx219
};

static int set_cam_gpio(int val)
{
	int fd;
	char attr[32];

	DIR *dir = opendir("/sys/class/gpio/gpio375");
	if (!dir) {
		fd = open("/sys/class/gpio/export", O_WRONLY);
		if (fd < 0) {
			perror("open(/sys/class/gpio/export)");
			return -1;
		}
		strcpy(attr, "375");
		write(fd, attr, strlen(attr));
		close(fd);
		dir = opendir("/sys/class/gpio/gpio375");
		if (!dir) {
			perror("opendir(/sys/class/gpio/gpio375)");
			return -1;
		}
	}
	closedir(dir);

	fd = open("/sys/class/gpio/gpio375/direction", O_WRONLY);
	if (fd < 0) {
		perror("open(/sys/class/gpio/gpio375/direction)");
		return -1;
	}
	strcpy(attr, "out");
	write(fd, attr, strlen(attr));
	close(fd);

	fd = open("/sys/class/gpio/gpio375/value", O_WRONLY);
	if (fd < 0) {
		perror("open(/sys/class/gpio/gpio375/value)");
		return -1;
	}
	sprintf(attr, "%d", val);
	write(fd, attr, strlen(attr));
	close(fd);

	return 0;
}

int caminit(e_sensor sensor, e_resolution resolution)
{
	set_cam_gpio(0);
	usleep(300*1000);
	set_cam_gpio(1);
	usleep(300*1000);

	init_camera[sensor](resolution);

    return 0;
}
