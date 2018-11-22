#include <unistd.h>

int write_reg(int fd, unsigned short addr, unsigned char write_data)
{
	unsigned char data[3];

	data[0] = (addr >> 8) & 0xff;
	data[1] = addr & 0xff;
	data[2] = write_data;
	write(fd, data, 3);

	return 0;
}
