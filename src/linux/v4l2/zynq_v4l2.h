#ifndef ZYNQ_V4L2_H
#define ZYNQ_V4L2_H

#include <linux/cdev.h>
#include "xaxivdma.h"

#define DRIVER_NAME "v4l2"
#define MINOR_BASE  0
#define MINOR_NUM   1

struct zynq_v4l2_data {
	unsigned int       major;
	struct cdev        cdev;
	struct device     *dma_dev;
	struct class      *class;
	size_t             frame_size;
	int                user_frame_count;
	uint32_t           queue_bits;
	uint32_t           active_bits;
	int                latest_frame;
	wait_queue_head_t  waitq;
	spinlock_t         lock;
	void              *user_mem;
	XAxiVdma          *inst_vdma;
	void __iomem      *reg_vdma;
	dma_addr_t         phys_wb_vdma;
	void              *virt_wb_vdma;
	size_t             alloc_size_wb_vdma;
	int                irq_num_vdma;
};

int zynq_v4l2_vdma_init(struct device *dev, struct zynq_v4l2_data *dp);
void zynq_v4l2_vdma_intr_enable(struct zynq_v4l2_data *dp);
void zynq_v4l2_vdma_intr_disable(struct zynq_v4l2_data *dp);
int zynq_v4l2_demosaic_init(void);
int zynq_v4l2_mipicsi_init(void);
int zynq_v4l2_find_oldest_slot(uint32_t active_bits, int latest);

#endif /* ZYNQ_V4L2_H */
