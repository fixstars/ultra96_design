#ifndef CAMCFG_H
#define CAMCFG_H

typedef struct _config_t {
	unsigned short addr;
	unsigned char val;
} config_t;

int init_ov5640(e_resolution resolution);
int init_imx219(e_resolution resolution);
int write_reg(int fd, unsigned short addr, unsigned char write_data);

#endif /* CAMCFG_H */
