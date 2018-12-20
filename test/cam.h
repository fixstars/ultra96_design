#ifndef CAM_H
#define CAM_H

#include <sys/types.h>

int v4l2init(int w, int h, __u32 pixelformat);
int v4l2end(void);
int v4l2grab(unsigned char **frame);
int v4l2release(int buf_idx);

#endif /* CAM_H */
