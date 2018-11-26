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
#include "xparameters.h"

MODULE_DESCRIPTION("Zynq v4l2 device driver");
MODULE_AUTHOR("osawa");
MODULE_LICENSE("MIT");

#define DRIVER_NAME "Zynqv4l2"
#define MINOR_BASE  0
#define MINOR_NUM   1

static unsigned int zynq_v4l2_major;
static struct cdev zynq_v4l2_cdev;

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

static struct file_operations zynq_v4l2_fops = {
	.open           = zynq_v4l2_open,
	.release        = zynq_v4l2_close,
	.read           = zynq_v4l2_read,
	.write          = zynq_v4l2_write,
	.poll           = zynq_v4l2_poll,
	.unlocked_ioctl = zynq_v4l2_ioctl,
	.compat_ioctl   = zynq_v4l2_ioctl,
};

static int zynq_v4l2_init(void)
{
	int   ret;
	dev_t dev;

	printk(KERN_INFO "%s\n", __FUNCTION__);	

	ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
	if (ret != 0) {
		printk(KERN_ERR "alloc_chrdev_region = %d\n", ret);
		return -1;
	}

	zynq_v4l2_major = MAJOR(dev);
	printk(KERN_INFO " major = %d\n", zynq_v4l2_major);
	dev = MKDEV(zynq_v4l2_major, MINOR_BASE);

	cdev_init(&zynq_v4l2_cdev, &zynq_v4l2_fops);
	zynq_v4l2_cdev.owner = THIS_MODULE;

	ret = cdev_add(&zynq_v4l2_cdev, dev, MINOR_NUM);
	if (ret != 0) {
		printk(KERN_ERR "cdev_add = %d\n", ret);
		unregister_chrdev_region(dev, MINOR_NUM);
		return -1;
	}

	return 0;
}

static void zynq_v4l2_exit(void)
{
	dev_t dev = MKDEV(zynq_v4l2_major, MINOR_BASE);
	printk(KERN_INFO "%s\n", __FUNCTION__);		
	cdev_del(&zynq_v4l2_cdev);
	unregister_chrdev_region(dev, MINOR_NUM);
}

module_init(zynq_v4l2_init);
module_exit(zynq_v4l2_exit);
