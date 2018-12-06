#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/workqueue.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "xparameters.h"
#include "xaxivdma.h"
#include "zynq_v4l2.h"

#define VDMA_BASEADDR(num)  XPAR_AXIVDMA_ ## num ## _BASEADDR
#define VDMA_HIGHADDR(num)  XPAR_AXIVDMA_ ## num ## _HIGHADDR
#define VDMA_DEVICE_ID(num) XPAR_AXIVDMA_ ## num ## _DEVICE_ID

extern int vdma_h_res, vdma_v_res;

/* too large to be placed on stack */
static XAxiVdma inst[MINOR_NUM];

int zynq_v4l2_get_latest_frame_number(struct zynq_v4l2_data *dp)
{
	int wb;

	wb = XAxiVdma_CurrFrameStore(dp->inst_vdma, XAXIVDMA_WRITE);
	/* we will use previous buffer than one that HW is working on */
	wb = (wb + dp->inst_vdma->MaxNumFrames - 1) % dp->inst_vdma->MaxNumFrames;

	return wb;
}

static void zynq_v4l2_wq_function(struct work_struct *work)
{
	struct zynq_v4l2_work_struct *ws = (struct zynq_v4l2_work_struct *)work;
	struct zynq_v4l2_data *dp = ws->dp;
	int slot, wb;

	wb = zynq_v4l2_get_latest_frame_number(dp);

	spin_lock_irq(&dp->lock);

	/* we will copy to oldest queued buffer */
	slot = zynq_v4l2_find_oldest_slot(dp->queue_bits, dp->latest_frame);

	/* invalidate dcache */
	dma_sync_single_for_cpu(dp->dma_dev, dp->phys_wb_vdma + dp->frame_size * wb, dp->frame_size, DMA_FROM_DEVICE);

	if (dp->user_mmap) {
		memcpy((void *)((unsigned long)dp->user_mmap + dp->frame_size * slot),
			   (void *)((unsigned long)dp->virt_wb_vdma + dp->frame_size * wb),
			   dp->frame_size);
	}

	dp->latest_frame = slot;
	dp->active_bits |= (1 << slot);
	wake_up_interruptible(&dp->waitq);
	PRINTK(KERN_INFO "wb = %d, slot = %d, active_bits = %d, latest_frame = %d\n", wb, slot, dp->active_bits, dp->latest_frame);

	spin_unlock_irq(&dp->lock);

	kfree(ws);
}

static irqreturn_t zynq_v4l2_vdma_isr(int irq, void *dev)
{
	XAxiVdma_Channel *Channel;
	u32 PendingIntr;
	struct zynq_v4l2_data *dp = (struct zynq_v4l2_data *)dev;
	struct zynq_v4l2_work_struct *work;
	int slot;

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	Channel = XAxiVdma_GetChannel(dp->inst_vdma, XAXIVDMA_WRITE);
	PendingIntr = XAxiVdma_ChannelGetPendingIntr(Channel);
	PendingIntr &= XAxiVdma_ChannelGetEnabledIntr(Channel);

	XAxiVdma_ChannelIntrClear(Channel, PendingIntr);

	if (PendingIntr & XAXIVDMA_IXR_COMPLETION_MASK) {
		spin_lock(&dp->lock);
		slot = zynq_v4l2_find_oldest_slot(dp->queue_bits, dp->latest_frame);
		if (slot != -1) {
			work = (struct zynq_v4l2_work_struct *)kmalloc(sizeof(struct zynq_v4l2_work_struct), GFP_ATOMIC);
			if (work) {
				INIT_WORK((struct work_struct *)work, zynq_v4l2_wq_function);
				work->dp = dp;
				queue_work(dp->sp->wq, (struct work_struct *)work);
			}
		}
		spin_unlock(&dp->lock);
	}

    return IRQ_HANDLED;
}

void zynq_v4l2_vdma_intr_enable(struct zynq_v4l2_data *dp)
{
	XAxiVdma_IntrEnable(dp->inst_vdma, XAXIVDMA_IXR_ALL_MASK, XAXIVDMA_WRITE);
}

void zynq_v4l2_vdma_intr_disable(struct zynq_v4l2_data *dp)
{
	XAxiVdma_IntrDisable(dp->inst_vdma, XAXIVDMA_IXR_ALL_MASK, XAXIVDMA_WRITE);
}

int zynq_v4l2_vdma_init(struct device *dev, struct zynq_v4l2_sys_data *sp)
{
	XAxiVdma_Config *psConf;
	XStatus          Status;
	int rc, minor;
	const int RESET_POLL = 1000;
	int Polls = RESET_POLL;
	XAxiVdma_DmaSetup WriteCfg;
	size_t addr;
	int iFrm;
	uint32_t baseaddr[MINOR_NUM] = {VDMA_BASEADDR(0)};
	uint32_t highaddr[MINOR_NUM] = {VDMA_HIGHADDR(0)};
	uint32_t device_id[MINOR_NUM] = {VDMA_DEVICE_ID(0)};

	printk(KERN_INFO "%s\n", __FUNCTION__);

	for (minor = 0; minor < MINOR_NUM; minor++) {
		struct zynq_v4l2_data *dp = &sp->dev[minor];

		dp->inst_vdma = &inst[minor];
		dp->reg_vdma = ioremap_nocache(baseaddr[minor], highaddr[minor] - baseaddr[minor]);
		if (!dp->reg_vdma) {
			printk(KERN_ERR "ioremap_nocache failed %d\n", minor);
			rc = -ENOMEM;
			goto error;
		}

		psConf = XAxiVdma_LookupConfig(device_id[minor]);
		if (!psConf) {
			printk(KERN_ERR "XAxiVdma_LookupConfig failed %d\n", minor);
			rc = -ENODEV;
			goto error;
		}

		Status = XAxiVdma_CfgInitialize(&inst[minor], psConf, (UINTPTR)dp->reg_vdma);
		if (Status != XST_SUCCESS) {
			printk(KERN_ERR "XAxiVdma_CfgInitialize failed %d\n", minor);
			rc = -ENODEV;
			goto error;
		}

		if (dp->dma_dev->dma_mask == NULL) {
			dp->dma_dev->dma_mask = &dp->dma_dev->coherent_dma_mask;
		}
		dma_set_mask(dp->dma_dev, DMA_BIT_MASK(32));
		dma_set_coherent_mask(dp->dma_dev, DMA_BIT_MASK(32));

		dp->frame_size = vdma_h_res * inst[minor].WriteChannel.StreamWidth * vdma_v_res;
		dp->alloc_size_wb_vdma = dp->frame_size * inst[minor].MaxNumFrames;
		PRINTK(KERN_INFO "alloc_size = %ld (%d)\n", dp->alloc_size_wb_vdma, minor);
		dp->virt_wb_vdma = dma_alloc_coherent(dp->dma_dev, dp->alloc_size_wb_vdma, &dp->phys_wb_vdma, GFP_KERNEL);
		if (IS_ERR_OR_NULL(dp->virt_wb_vdma)) {
			int retval = PTR_ERR(dp->virt_wb_vdma);
			printk(KERN_ERR "dma_alloc_coherent failed, ret = %d %d\n", retval, minor);
			rc = -ENODEV;
			goto error;
		}
		PRINTK(KERN_INFO "virt_wb_vdma = %p, phys_wb_vdma = 0x%llx %d\n", dp->virt_wb_vdma, dp->phys_wb_vdma, minor);

		/* no need to flush */
		//dma_sync_single_for_device(dp->dma_dev, dp->phys_wb_vdma, dp->alloc_size_wb_vdma, DMA_TO_DEVICE);

		XAxiVdma_ChannelReset(&inst[minor].WriteChannel);

		while (Polls && XAxiVdma_ChannelResetNotDone(&inst[minor].WriteChannel)) {
			--Polls;
		}
		if (!Polls) {
			printk(KERN_ERR "XAxiVdma_ChannelResetNotDone failed %d\n", minor);
			rc = -ENODEV;
			goto error;
		}

		XAxiVdma_ClearDmaChannelErrors(&inst[minor], XAXIVDMA_WRITE, XAXIVDMA_SR_ERR_ALL_MASK);

		WriteCfg.HoriSizeInput = vdma_h_res * inst[minor].WriteChannel.StreamWidth;
		WriteCfg.VertSizeInput = vdma_v_res;
		WriteCfg.Stride = WriteCfg.HoriSizeInput;
		WriteCfg.FrameDelay = 0;
		WriteCfg.EnableCircularBuf = 1;
		WriteCfg.EnableSync = 0; //Gen-Lock
		WriteCfg.PointNum = 0;
		WriteCfg.EnableFrameCounter = 0;
		WriteCfg.FixedFrameStoreAddr = 0; //ignored, since we circle through buffers
		Status = XAxiVdma_DmaConfig(&inst[minor], XAXIVDMA_WRITE, &WriteCfg);
		if (Status != XST_SUCCESS) {
			printk(KERN_ERR "XAxiVdma_DmaConfig failed %d\n", minor);
			rc = -ENODEV;
			goto error;
		}

		addr = dp->phys_wb_vdma;
		for (iFrm = 0; iFrm < inst[minor].MaxNumFrames; iFrm++) {
			WriteCfg.FrameStoreStartAddr[iFrm] = addr;
			addr += WriteCfg.HoriSizeInput * WriteCfg.VertSizeInput;
		}
		Status = XAxiVdma_DmaSetBufferAddr(&inst[minor], XAXIVDMA_WRITE, WriteCfg.FrameStoreStartAddr);
		if (Status != XST_SUCCESS) {
			printk(KERN_ERR "XAxiVdma_DmaSetBufferAddr failed %d\n", minor);
			rc = -ENODEV;
			goto error;
		}

		dp->irq_num_vdma = irq_of_parse_and_map(dev->of_node, 0) + minor;
		rc = request_irq(dp->irq_num_vdma, zynq_v4l2_vdma_isr, IRQF_SHARED | IRQF_NO_SUSPEND, "vdma", dp);
		if (rc) {
			printk(KERN_ERR "request_irq = %d %d\n", rc, minor);
			goto error;
		}

		//Clear errors in SR
		XAxiVdma_ClearChannelErrors(&inst[minor].WriteChannel, XAXIVDMA_SR_ERR_ALL_MASK);
		//Unmask error interrupts
		XAxiVdma_MaskS2MMErrIntr(&inst[minor], ~XAXIVDMA_S2MM_IRQ_ERR_ALL_MASK, XAXIVDMA_WRITE);

		Status = XAxiVdma_DmaStart(&inst[minor], XAXIVDMA_WRITE);
		if (Status != XST_SUCCESS) {
			printk(KERN_ERR "XAxiVdma_DmaStart failed %d\n", minor);
			rc = -ENODEV;
			goto error;
		}
	}

	return 0;

error:
	for (minor = 0; minor < MINOR_NUM; minor++) {
		struct zynq_v4l2_data *dp = &sp->dev[minor];

		if (dp->virt_wb_vdma) {
			dma_free_coherent(dp->dma_dev, dp->alloc_size_wb_vdma, dp->virt_wb_vdma, dp->phys_wb_vdma);
			dp->virt_wb_vdma = NULL;
		}
		if (dp->reg_vdma) {
			iounmap(dp->reg_vdma);
			dp->reg_vdma = NULL;
		}
		if (dp->irq_num_vdma) {
			free_irq(dp->irq_num_vdma, dp);
			dp->irq_num_vdma = 0;
		}
	}
	return rc;
}
