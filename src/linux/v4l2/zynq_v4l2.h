#ifndef ZYNQ_V4L2_H
#define ZYNQ_V4L2_H

#include <linux/cdev.h>
#include "xaxivdma.h"

//#define DEBUG
#ifdef DEBUG
#define PRINTK(fmt, ...) printk(fmt, __VA_ARGS__)
#else
#define PRINTK(fmt, ...)
#endif

#define USE_VMALLOC

#define DRIVER_NAME "v4l2"
#define MINOR_BASE  0
#define MINOR_NUM   1

struct zynq_v4l2_data {
	struct device     *dma_dev;
	size_t             frame_size;
	int                user_frame_count;
	uint32_t           queue_bits;
	uint32_t           active_bits;
	int                latest_frame;
	wait_queue_head_t  waitq;
	spinlock_t         lock;
	void              *user_mem;
	#ifndef USE_VMALLOC
	dma_addr_t         user_phys_mem;
	#endif /* USE_VMALLOC */
	XAxiVdma          *inst_vdma;
	void __iomem      *reg_vdma;
	dma_addr_t         phys_wb_vdma;
	void              *virt_wb_vdma;
	size_t             alloc_size_wb_vdma;
	int                irq_num_vdma;
};

struct zynq_v4l2_sys_data {
	unsigned int          major;
	struct class         *class;
	struct cdev           cdev;
	bool                  cdev_inited;
	struct zynq_v4l2_data dev[MINOR_NUM];
};

int zynq_v4l2_vdma_init(struct device *dev, struct zynq_v4l2_sys_data *sp);
void zynq_v4l2_vdma_intr_enable(struct zynq_v4l2_data *dp);
void zynq_v4l2_vdma_intr_disable(struct zynq_v4l2_data *dp);
int zynq_v4l2_demosaic_init(void);
int zynq_v4l2_mipicsi_init(void);
int zynq_v4l2_find_oldest_slot(uint32_t active_bits, int latest);

#endif /* ZYNQ_V4L2_H */
