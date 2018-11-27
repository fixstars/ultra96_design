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
#include "zynq_v4l2.h"

MODULE_DESCRIPTION("ZYNQ v4l2 device driver");
MODULE_AUTHOR("osawa");
MODULE_LICENSE("Dual BSD/GPL");

#define DRIVER_NAME "v4l2"
#define MINOR_BASE  0
#define MINOR_NUM   1

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

static int zynq_v4l2_create_cdev(struct zynq_v4l2_local *lp)
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

	lp->major = MAJOR(dev);
	printk(KERN_INFO " major = %d\n", lp->major);
	dev = MKDEV(lp->major, MINOR_BASE);

	cdev_init(&lp->cdev, &zynq_v4l2_fops);
	lp->cdev.owner = THIS_MODULE;

	ret = cdev_add(&lp->cdev, dev, MINOR_NUM);
	if (ret != 0) {
		printk(KERN_ERR "cdev_add = %d\n", ret);
		unregister_chrdev_region(dev, MINOR_NUM);
		return -1;
	}

	lp->class = class_create(THIS_MODULE, "v4l2");
	if (IS_ERR(lp->class)) {
		printk(KERN_ERR "class_create\n");
		cdev_del(&lp->cdev);
		unregister_chrdev_region(dev, MINOR_NUM);
		return -1;
	}

	for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
		device_create(lp->class, NULL, MKDEV(lp->major, minor), NULL, "video%d", minor);
	}

	return 0;
}

static void zynq_v4l2_deletel_cdev(struct zynq_v4l2_local *lp)
{
	int minor;
	dev_t dev = MKDEV(lp->major, MINOR_BASE);

	printk(KERN_INFO "%s\n", __FUNCTION__);
	for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
		device_destroy(lp->class, MKDEV(lp->major, minor));
	}
	class_destroy(lp->class);
	cdev_del(&lp->cdev);
	unregister_chrdev_region(dev, MINOR_NUM);
}

static int init_hw(struct zynq_v4l2_local *lp)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);

	if (init_vdma(lp)) {
		return -ENODEV;
	}
	if (init_mipi()) {
		iounmap(lp->mem_vdma);
		return -ENODEV;
	}
	return 0;
}

static int zynq_v4l2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct zynq_v4l2_local *lp;

	dev_info(dev, "Device Tree Probing");
	printk(KERN_INFO "%s\n", __FUNCTION__);

	lp = (struct zynq_v4l2_local *)kmalloc(sizeof(struct zynq_v4l2_local), GFP_KERNEL);
	if (!lp) {
		dev_err(dev, "Could not allocate memory\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, lp);

	if (init_hw(lp)) {
		dev_err(dev, "Could not initialize HW\n");
		return -ENODEV;
	}

	if (!zynq_v4l2_create_cdev(lp)) {
		return 0;
	}

	iounmap(lp->mem_vdma);
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return -EBUSY;
}

static int zynq_v4l2_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct zynq_v4l2_local *lp = dev_get_drvdata(dev);

	printk(KERN_INFO "%s\n", __FUNCTION__);
	zynq_v4l2_deletel_cdev(lp);
	iounmap(lp->mem_vdma);
	kfree(lp);
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
