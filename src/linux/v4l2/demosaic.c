#include <linux/kernel.h>
#include <asm/io.h>
#include "xparameters.h"
#ifdef __linux__
#undef __linux__
#endif
#include "xdemosaic_root.h"
#include "zynq_v4l2.h"

#define DEMOSAIC_BASEADDR(num)  XPAR_DEMOSAIC_ROOT_ ## num ## _S_AXI_BUS_AXI4LS_BASEADDR
#define DEMOSAIC_HIGHADDR(num)  XPAR_DEMOSAIC_ROOT_ ## num ## _S_AXI_BUS_AXI4LS_HIGHADDR
#define DEMOSAIC_DEVICE_ID(num) XPAR_DEMOSAIC_ROOT_ ## num ## _DEVICE_ID

int zynq_v4l2_demosaic_init(void)
{
	int minor;
	XDemosaic_root ins;
	void __iomem *mem;
	XStatus Status;
	uint32_t baseaddr[MINOR_NUM] = {DEMOSAIC_BASEADDR(0)};
	uint32_t highaddr[MINOR_NUM] = {DEMOSAIC_HIGHADDR(0)};
	uint32_t device_id[MINOR_NUM] = {DEMOSAIC_DEVICE_ID(0)};

	printk(KERN_INFO "%s\n", __FUNCTION__);

	for (minor = 0; minor < MINOR_NUM; minor++) {
		mem = ioremap_nocache(baseaddr[minor], highaddr[minor] - baseaddr[minor]);
		if (!mem) {
			printk(KERN_ERR "ioremap_nocache failed %d\n", minor);
			return -ENOMEM;
		}

		Status = XDemosaic_root_Initialize(&ins, device_id[minor]);
		if (Status != XST_SUCCESS) {
			printk(KERN_ERR "XDemosaic_root_Initialize failed %d\n", minor);
			iounmap(mem);
			return -ENODEV;
		}

		ins.Bus_axi4ls_BaseAddress = (UINTPTR)mem;
		XDemosaic_root_EnableAutoRestart(&ins);
		XDemosaic_root_Start(&ins);
		iounmap(mem);
	}

	return 0;
}
