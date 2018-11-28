#include <linux/kernel.h>
#include <asm/io.h>
#include "xparameters.h"
#include "xcsi.h"
#include "zynq_v4l2.h"

int mipicsi_init(void)
{
	XCsi ins;
	XCsi_Config *psConf;
	void __iomem *mem;
	int rc;
	XStatus Status;

	printk(KERN_INFO "%s\n", __FUNCTION__);

	mem = ioremap_nocache(XPAR_MIPI_CSI2_RX_SUBSYST_0_BASEADDR, XPAR_MIPI_CSI2_RX_SUBSYST_0_HIGHADDR - XPAR_MIPI_CSI2_RX_SUBSYST_0_BASEADDR);
	if (!mem) {
		printk(KERN_ERR "ioremap_nocache failed\n");
		return -ENOMEM;
	}

	psConf = XCsi_LookupConfig(XPAR_MIPI_CSI2_RX_SUBSYST_0_DEVICE_ID);
	if (!psConf) {
		printk(KERN_ERR "XCsiSs_LookupConfig failed\n");
		rc = -ENODEV;
		goto out;
	}

	Status = XCsi_CfgInitialize(&ins, psConf, (UINTPTR)mem);
	if (Status != XST_SUCCESS){
		printk(KERN_ERR "XCsiSs_CfgInitialize failed\n");
		rc = -ENODEV;
		goto out;
	}

	Status = XCsi_Reset(&ins);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XCsiSs_Reset failed\n");
		rc = -ENODEV;
		goto out;
	}

	Status = XCsi_Activate(&ins, XCSI_ENABLE);
	if (Status != XST_SUCCESS) {
		printk(KERN_ERR "XCsiSs_Activate failed\n");
		rc = -ENODEV;
		goto out;
	}
	rc = 0;

out:
	iounmap(mem);
	return rc;
}
