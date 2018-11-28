#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "xparameters.h"
#include "xaxivdma.h"
#include "zynq_v4l2.h"

int frame_cnt_intr = 0;
extern int vdma_h_res, vdma_v_res;

/* too large to be placed on stack */
static XAxiVdma inst;

static irqreturn_t vdma_isr(int irq, void *dev)
{
	XAxiVdma_Channel *Channel;
	u32 PendingIntr;

	Channel = XAxiVdma_GetChannel(&inst, XAXIVDMA_WRITE);
	PendingIntr = XAxiVdma_ChannelGetPendingIntr(Channel);
	PendingIntr &= XAxiVdma_ChannelGetEnabledIntr(Channel);

	XAxiVdma_ChannelIntrClear(Channel, PendingIntr);

	if (PendingIntr & XAXIVDMA_IXR_COMPLETION_MASK) {
		frame_cnt_intr++;
	}

    return IRQ_HANDLED;
}

int vdma_init(struct zynq_v4l2_data *dp)
{
	XAxiVdma_Config *psConf;
	XStatus          Status;
	int rc;
	const int RESET_POLL = 1000;
	int Polls = RESET_POLL;
	XAxiVdma_DmaSetup WriteCfg;
	size_t addr;
	int iFrm;

	printk(KERN_INFO "%s\n", __FUNCTION__);

	dp->reg_vdma = ioremap_nocache(XPAR_AXIVDMA_0_BASEADDR, XPAR_AXIVDMA_0_HIGHADDR - XPAR_AXIVDMA_0_BASEADDR);
	if (!dp->reg_vdma) {
		printk(KERN_ERR "ioremap_nocache failed\n");
		return -ENOMEM;
	}

	psConf = XAxiVdma_LookupConfig(XPAR_AXIVDMA_0_DEVICE_ID);
	if (!psConf) {
		printk(KERN_ERR "XAxiVdma_LookupConfig failed\n");
		rc = -ENODEV;
		goto fail2;
	}

	Status = XAxiVdma_CfgInitialize(&inst, psConf, (UINTPTR)dp->reg_vdma);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XAxiVdma_CfgInitialize failed\n");
		rc = -ENODEV;
		goto fail2;
	}

	if (dp->dma_dev[0]->dma_mask == NULL) {
		dp->dma_dev[0]->dma_mask = &dp->dma_dev[0]->coherent_dma_mask;
	}
	dma_set_mask(dp->dma_dev[0], DMA_BIT_MASK(32));
	dma_set_coherent_mask(dp->dma_dev[0], DMA_BIT_MASK(32));

	dp->alloc_size_vdma = vdma_h_res * inst.WriteChannel.StreamWidth * vdma_v_res * inst.MaxNumFrames;
	//printk(KERN_INFO "alloc_size = %ld\n", dp->alloc_size_vdma);
	dp->virt_wb_vdma = dma_alloc_coherent(dp->dma_dev[0], dp->alloc_size_vdma, &dp->phys_wb_vdma, GFP_KERNEL);
	if (IS_ERR_OR_NULL(dp->virt_wb_vdma)) {
		int retval = PTR_ERR(dp->virt_wb_vdma);
		printk(KERN_ERR "dma_alloc_coherent failed, ret = %d\n", retval);
		rc = -ENODEV;
		goto fail2;
	}
	//printk(KERN_INFO "virt_wb_vdma = %p, phys_wb_vdma = 0x%llx\n", dp->virt_wb_vdma, dp->phys_wb_vdma);

	/* invalidate cache */
	dma_sync_single_for_device(dp->dma_dev[0], dp->phys_wb_vdma, dp->alloc_size_vdma, DMA_FROM_DEVICE);

	XAxiVdma_ChannelReset(&inst.WriteChannel);

	while (Polls && XAxiVdma_ChannelResetNotDone(&inst.WriteChannel)) {
		--Polls;
	}
	if (!Polls) {
		printk(KERN_ERR "XAxiVdma_ChannelResetNotDone failed\n");
		rc = -ENODEV;
		goto fail1;
	}

	XAxiVdma_ClearDmaChannelErrors(&inst, XAXIVDMA_WRITE, XAXIVDMA_SR_ERR_ALL_MASK);

	WriteCfg.HoriSizeInput = vdma_h_res * inst.WriteChannel.StreamWidth;
	WriteCfg.VertSizeInput = vdma_v_res;
	WriteCfg.Stride = WriteCfg.HoriSizeInput;
	WriteCfg.FrameDelay = 0;
	WriteCfg.EnableCircularBuf = 1;
	WriteCfg.EnableSync = 0; //Gen-Lock
	WriteCfg.PointNum = 0;
	WriteCfg.EnableFrameCounter = 0;
	WriteCfg.FixedFrameStoreAddr = 0; //ignored, since we circle through buffers
	Status = XAxiVdma_DmaConfig(&inst, XAXIVDMA_WRITE, &WriteCfg);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XAxiVdma_DmaConfig failed\n");
		rc = -ENODEV;
		goto fail1;
	}

	addr = dp->phys_wb_vdma;
	for (iFrm = 0; iFrm < inst.MaxNumFrames; iFrm++) {
		WriteCfg.FrameStoreStartAddr[iFrm] = addr;
		addr += WriteCfg.HoriSizeInput * WriteCfg.VertSizeInput;
	}
	Status = XAxiVdma_DmaSetBufferAddr(&inst, XAXIVDMA_WRITE, WriteCfg.FrameStoreStartAddr);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XAxiVdma_DmaSetBufferAddr failed\n");
		rc = -ENODEV;
		goto fail1;
	}

    dp->irq_num_vdma = XPAR_FABRIC_AXI_VDMA_0_S2MM_INTROUT_INTR;
    dp->irq_dev_id_vdma = &dp->irq_num_vdma;
    rc = request_irq(dp->irq_num_vdma, vdma_isr, IRQF_SHARED | IRQF_NO_SUSPEND, "vdma", dp->irq_dev_id_vdma);
    if (rc) {
        printk(KERN_ERR "request_irq = %d\n", rc);
        goto fail1;
    }

	//Clear errors in SR
	XAxiVdma_ClearChannelErrors(&inst.WriteChannel, XAXIVDMA_SR_ERR_ALL_MASK);
	//Unmask error interrupts
	XAxiVdma_MaskS2MMErrIntr(&inst, ~XAXIVDMA_S2MM_IRQ_ERR_ALL_MASK, XAXIVDMA_WRITE);
	//Enable write channel error and frame count interrupts
	XAxiVdma_IntrEnable(&inst, XAXIVDMA_IXR_ERROR_MASK | XAXIVDMA_IXR_COMPLETION_MASK, XAXIVDMA_WRITE);

	Status = XAxiVdma_DmaStart(&inst, XAXIVDMA_WRITE);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XAxiVdma_DmaStart failed\n");
        free_irq(dp->irq_num_vdma, dp->irq_dev_id_vdma);
		rc = -ENODEV;
		goto fail1;
	}

	return 0;

fail1:
	dma_free_coherent(dp->dma_dev[0], dp->alloc_size_vdma, dp->virt_wb_vdma, dp->phys_wb_vdma);
fail2:
	iounmap(dp->reg_vdma);
	return rc;
}