#ifndef ZYNQ_V4L2_H
#define ZYNQ_V4L2_H

#include <linux/cdev.h>

#define DRIVER_NAME "v4l2"
#define MINOR_BASE  0
#define MINOR_NUM   1

struct zynq_v4l2_data {
	unsigned int   major;
	struct cdev    cdev;
    struct device *dma_dev[MINOR_NUM];
	struct class  *class;
	void __iomem  *reg_vdma;
    dma_addr_t     phys_wb_vdma;
    void          *virt_wb_vdma;
    size_t         alloc_size_vdma;
    int            irq_num_vdma;
    void          *irq_dev_id_vdma;
};

int vdma_init(struct zynq_v4l2_data *dp);
int demosaic_init(void);
int mipicsi_init(void);

#endif /* ZYNQ_V4L2_H */
