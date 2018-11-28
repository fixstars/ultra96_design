#include <linux/kernel.h>
#include <asm/io.h>
#include "xparameters.h"
#ifdef __linux__
#undef __linux__
#endif
#include "xdemosaic_root.h"
#include "zynq_v4l2.h"

int demosaic_init(void)
{
	XDemosaic_root ins;
	void __iomem *mem;
	int rc;
	XStatus Status;

	printk(KERN_INFO "%s\n", __FUNCTION__);

	mem = ioremap_nocache(XPAR_DEMOSAIC_ROOT_0_S_AXI_BUS_AXI4LS_BASEADDR, XPAR_DEMOSAIC_ROOT_0_S_AXI_BUS_AXI4LS_HIGHADDR - XPAR_DEMOSAIC_ROOT_0_S_AXI_BUS_AXI4LS_BASEADDR);
	if (!mem) {
		printk(KERN_ERR "ioremap_nocache failed\n");
		return -ENOMEM;
	}

	Status = XDemosaic_root_Initialize(&ins, XPAR_DEMOSAIC_ROOT_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XDemosaic_root_Initialize failed\n");
		rc = -ENODEV;
		goto out;
	}

	ins.Bus_axi4ls_BaseAddress = (UINTPTR)mem;
	XDemosaic_root_EnableAutoRestart(&ins);
	XDemosaic_root_Start(&ins);

	rc = 0;

out:
	iounmap(mem);
	return rc;
}
