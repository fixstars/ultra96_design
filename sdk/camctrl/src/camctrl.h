#ifndef CAMINIT_H
#define CAMINIT_H

typedef enum {
	SENSOR_OV5640,
	SENSOR_IMX219,
} e_sensor;

typedef enum {
	RESOLUTION_1920_1080,
	RESOLUTION_1280_720,
	RESOLUTION_960_540,
	RESOLUTION_640_480,
	RESOLUTION_640_360,
	RESOLUTION_320_240,
	RESOLUTION_320_200
} e_resolution;

int caminit(e_sensor sensor, e_resolution resolution, int fnum, size_t fsize);
unsigned long long get_frame_buffer(void);

#endif /* CAMINIT_H */
