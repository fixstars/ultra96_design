#include <linux/kernel.h>
#include <asm/io.h>
#include "xparameters.h"
#include "xaxivdma.h"
#include "zynq_v4l2.h"

/* too large to be placed on stack */
static XAxiVdma inst;

int init_vdma(struct zynq_v4l2_local *lp)
{
	XAxiVdma_Config *psConf;
	XStatus          Status;
	int rc;

	printk(KERN_INFO "%s\n", __FUNCTION__);

	lp->mem_vdma = ioremap_nocache(XPAR_AXIVDMA_0_BASEADDR, XPAR_AXIVDMA_0_HIGHADDR - XPAR_AXIVDMA_0_BASEADDR);
	if (!lp->mem_vdma) {
		printk(KERN_ERR "ioremap_nocache failed\n");
		return -ENOMEM;
	}

	psConf = XAxiVdma_LookupConfig(XPAR_AXIVDMA_0_DEVICE_ID);
	if (!psConf) {
		printk(KERN_ERR "XAxiVdma_LookupConfig failed\n");
		rc = -ENODEV;
		goto fail;
	}

	Status = XAxiVdma_CfgInitialize(&inst, psConf, (UINTPTR)lp->mem_vdma);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XAxiVdma_CfgInitialize failed\n");
		rc = -ENODEV;
		goto fail;
	}

	return 0;

fail:
	iounmap(lp->mem_vdma);
	return rc;
}
