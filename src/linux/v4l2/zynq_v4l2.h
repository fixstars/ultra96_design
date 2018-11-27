#ifndef ZYNQ_V4L2_H
#define ZYNQ_V4L2_H

#include <linux/cdev.h>

struct zynq_v4l2_local {
	unsigned int  major;
	struct cdev   cdev;
	struct class *class;
	void __iomem *mem_vdma;
};

int init_vdma(struct zynq_v4l2_local *lp);
int init_mipi(void);

#endif /* ZYNQ_V4L2_H */
