#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include "zynq_v4l2.h"

MODULE_DESCRIPTION("ZYNQ v4l2 device driver");
MODULE_AUTHOR("osawa");
MODULE_LICENSE("Dual BSD/GPL");

int vdma_h_res = 1920;
int vdma_v_res = 1080;
module_param(vdma_h_res, int, S_IRUGO);
module_param(vdma_v_res, int, S_IRUGO);
MODULE_PARM_DESC(vdma_h_res, "v4l2 buffer width");
MODULE_PARM_DESC(vdma_v_res, "v4l2 buffer height");

static int zynq_v4l2_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);
	return 0;
}

static int zynq_v4l2_close(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);	
	return 0;
}

static ssize_t zynq_v4l2_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);	
	return 0;
}

static ssize_t zynq_v4l2_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);	
	return 0;
}

static unsigned int zynq_v4l2_poll(struct file *filp, struct poll_table_struct *tbl)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);	
	return 0;
}

static long zynq_v4l2_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);	
	return 0;
}

static int zynq_v4l2_mmap(struct file *filp, struct vm_area_struct *vma)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);
	return 0;
}

static struct file_operations zynq_v4l2_fops = {
	.open           = zynq_v4l2_open,
	.release        = zynq_v4l2_close,
	.read           = zynq_v4l2_read,
	.write          = zynq_v4l2_write,
	.poll           = zynq_v4l2_poll,
	.unlocked_ioctl = zynq_v4l2_ioctl,
	.compat_ioctl   = zynq_v4l2_ioctl,
	.mmap           = zynq_v4l2_mmap,
};

static int zynq_v4l2_create_cdev(struct device *parent, struct zynq_v4l2_data *dp)
{
	int ret;
	dev_t dev;
	int minor;

	printk(KERN_INFO "%s\n", __FUNCTION__);

	ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
	if (ret != 0) {
		printk(KERN_ERR "alloc_chrdev_region = %d\n", ret);
		return -1;
	}

	dp->major = MAJOR(dev);
	printk(KERN_INFO " major = %d\n", dp->major);
	dev = MKDEV(dp->major, MINOR_BASE);

	cdev_init(&dp->cdev, &zynq_v4l2_fops);
	dp->cdev.owner = THIS_MODULE;

	ret = cdev_add(&dp->cdev, dev, MINOR_NUM);
	if (ret != 0) {
		printk(KERN_ERR "cdev_add = %d\n", ret);
		unregister_chrdev_region(dev, MINOR_NUM);
		return -1;
	}

	dp->class = class_create(THIS_MODULE, "v4l2");
	if (IS_ERR(dp->class)) {
		printk(KERN_ERR "class_create\n");
		cdev_del(&dp->cdev);
		unregister_chrdev_region(dev, MINOR_NUM);
		return -1;
	}

	for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
		device_create(dp->class, parent, MKDEV(dp->major, minor), dp, "video%d", minor);
	}
	dp->dma_dev[0] = parent;

	return 0;
}

static void zynq_v4l2_delete_cdev(struct zynq_v4l2_data *dp)
{
	int minor;
	dev_t dev = MKDEV(dp->major, MINOR_BASE);

	printk(KERN_INFO "%s\n", __FUNCTION__);
	for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
		device_destroy(dp->class, MKDEV(dp->major, minor));
	}
	class_destroy(dp->class);
	cdev_del(&dp->cdev);
	unregister_chrdev_region(dev, MINOR_NUM);
}

static int hw_init(struct zynq_v4l2_data *dp)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);

	if (vdma_init(dp)) {
		return -ENODEV;
	}

	if (demosaic_init()) {
		free_irq(dp->irq_num_vdma, dp->irq_dev_id_vdma);
		dma_free_coherent(dp->dma_dev[0], dp->alloc_size_vdma, dp->virt_wb_vdma, dp->phys_wb_vdma);
		iounmap(dp->reg_vdma);
		return -ENODEV;
	}

	if (mipicsi_init()) {
		free_irq(dp->irq_num_vdma, dp->irq_dev_id_vdma);
		dma_free_coherent(dp->dma_dev[0], dp->alloc_size_vdma, dp->virt_wb_vdma, dp->phys_wb_vdma);
		iounmap(dp->reg_vdma);
		return -ENODEV;
	}
	return 0;
}

static int zynq_v4l2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct zynq_v4l2_data *dp;

	dev_info(dev, "Device Tree Probing");
	printk(KERN_INFO "%s\n", __FUNCTION__);

	dp = (struct zynq_v4l2_data *)kmalloc(sizeof(struct zynq_v4l2_data), GFP_KERNEL);
	if (!dp) {
		dev_err(dev, "Could not allocate memory\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, dp);

	if (zynq_v4l2_create_cdev(&pdev->dev, dp)) {
		dev_err(dev, "zynq_v4l2_create_cdev failed\n");
		kfree(dp);
		dev_set_drvdata(dev, NULL);
		return -EBUSY;
	}

	if (hw_init(dp)) {
		dev_err(dev, "hw_init failed\n");
		zynq_v4l2_delete_cdev(dp);
		kfree(dp);
		dev_set_drvdata(dev, NULL);
		return -ENODEV;
	}

	return 0;
}

static int zynq_v4l2_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct zynq_v4l2_data *dp = dev_get_drvdata(dev);
	extern int frame_cnt_intr;

	printk(KERN_INFO "%s\n", __FUNCTION__);
	printk(KERN_INFO "frame_cnt_intr = %d\n", frame_cnt_intr);
	zynq_v4l2_delete_cdev(dp);
	free_irq(dp->irq_num_vdma, dp->irq_dev_id_vdma);
	dma_free_coherent(dp->dma_dev[0], dp->alloc_size_vdma, dp->virt_wb_vdma, dp->phys_wb_vdma);
	iounmap(dp->reg_vdma);
	kfree(dp);
	dev_set_drvdata(dev, NULL);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id zynq_v4l2_of_match[] = {
	{ .compatible = "fixstars,zynq-v4l2-1.0", },
	{ /* end of table */ },
};
MODULE_DEVICE_TABLE(of, zynq_v4l2_of_match);
#else
#define zynq_v4l2_of_match
#endif

static struct platform_driver zynq_v4l2_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = zynq_v4l2_of_match,
	},
	.probe = zynq_v4l2_probe,
	.remove = zynq_v4l2_remove,
};

static int zynq_v4l2_init(void)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);
    return platform_driver_register(&zynq_v4l2_driver);
}

static void zynq_v4l2_exit(void)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);
	platform_driver_unregister(&zynq_v4l2_driver);
}

module_init(zynq_v4l2_init);
module_exit(zynq_v4l2_exit);
