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
#include <asm/cacheflush.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/videodev2.h>
#include "zynq_v4l2.h"

MODULE_DESCRIPTION("ZYNQ v4l2 device driver");
MODULE_AUTHOR("osawa");
MODULE_LICENSE("Dual BSD/GPL");

int vdma_h_res = 640;
int vdma_v_res = 480;
module_param(vdma_h_res, int, S_IRUGO);
module_param(vdma_v_res, int, S_IRUGO);
MODULE_PARM_DESC(vdma_h_res, "v4l2 buffer width");
MODULE_PARM_DESC(vdma_v_res, "v4l2 buffer height");

int zynq_v4l2_find_oldest_slot(uint32_t slot_bits, int latest)
{
	uint32_t bits = slot_bits;

	bits &= ~(((1 << latest) << 1) - 1);
	if (bits ) {
		return __builtin_ctz(bits);
	} else if (slot_bits) {
		return __builtin_ctz(slot_bits);
	} else {
		return -1;
	}
}

static int zynq_v4l2_open(struct inode *inode, struct file *filp)
{
	struct zynq_v4l2_sys_data *sp;
	struct zynq_v4l2_data *dp;
	int minor;

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	sp = container_of(inode->i_cdev, struct zynq_v4l2_sys_data, cdev);
	if (sp == NULL) {
		printk(KERN_ERR "container_of failed\n");
		return -EFAULT;
	}
	minor = iminor(inode);
	dp = &sp->dev[minor];
	filp->private_data = dp;

	return 0;
}

static int zynq_v4l2_close(struct inode *inode, struct file *filp)
{
	struct zynq_v4l2_data *dp = filp->private_data;

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	zynq_v4l2_vdma_intr_disable(dp);
	spin_lock_irq(&dp->lock);
	dp->queue_bits = 0;
	dp->active_bits = 0;
	spin_unlock_irq(&dp->lock);
	if (dp->user_mem) {
		#ifdef USE_VMALLOC
		vfree(dp->user_mem);
		#else /* !USE_VMALLOC */
		dma_free_coherent(dp->dma_dev, dp->frame_size * dp->user_frame_count, dp->user_mem, dp->user_phys_mem);
		#endif /* !USE_VMALLOC */
		dp->user_mem = NULL;
	}

	return 0;
}

static ssize_t zynq_v4l2_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct zynq_v4l2_data *dp = filp->private_data;
	int wb;

	if (count > vdma_h_res * vdma_v_res * 3) {
		return -EINVAL;
	}

	wb = XAxiVdma_CurrFrameStore(dp->inst_vdma, XAXIVDMA_WRITE);
	wb = (wb + dp->inst_vdma->MaxNumFrames - 1) % dp->inst_vdma->MaxNumFrames;
	printk(KERN_INFO "%s wb = %d\n", __FUNCTION__, wb);
	if (raw_copy_to_user(buf, dp->virt_wb_vdma + dp->frame_size * wb, count) != 0) {
		return -EFAULT;
	}

	return count;
}

static ssize_t zynq_v4l2_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	return 0;
}

static unsigned int zynq_v4l2_poll(struct file *filp, struct poll_table_struct *waitp)
{
	struct zynq_v4l2_data *dp = filp->private_data;
	int slot, rc;

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);	

	spin_lock_irq(&dp->lock);
	slot = zynq_v4l2_find_oldest_slot(dp->active_bits, dp->latest_frame);
	spin_unlock_irq(&dp->lock);

	if (slot == -1) {
		poll_wait(filp, &dp->waitq, waitp);
		spin_lock_irq(&dp->lock);
		if ((slot = zynq_v4l2_find_oldest_slot(dp->active_bits, dp->latest_frame)) == -1) {
			rc = 0;
		} else {
			rc = (POLLIN | POLLRDNORM);
		}
		spin_unlock_irq(&dp->lock);
	} else {
		rc = (POLLIN | POLLRDNORM);
	}
	PRINTK(KERN_INFO "  rc = %d, slot = %d\n", rc, slot);

	return rc;
}

static long zynq_v4l2_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	enum   v4l2_buf_type type;
	struct zynq_v4l2_data *dp = filp->private_data;
	int rc, slot;
	#ifdef USE_VMALLOC
	unsigned long offset;
	#endif /* USE_VMALLOC */

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	switch (cmd) {
	case VIDIOC_G_FMT:
		if (raw_copy_from_user(&fmt, (void __user *)arg, sizeof(fmt))) {
			return -EFAULT;
		}
		switch (fmt.type) {
		case V4L2_BUF_TYPE_VIDEO_CAPTURE:
			fmt.fmt.pix.width = vdma_h_res;
			fmt.fmt.pix.height = vdma_v_res;
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
			fmt.fmt.pix.field = V4L2_FIELD_ANY;
			if (raw_copy_to_user((void __user *)arg, &fmt, sizeof(fmt))) {
				return -EFAULT;
			}
			return 0;
		default:
			return -ENOSYS;
		}
		break;
	case VIDIOC_S_FMT:
		if (raw_copy_from_user(&fmt, (void __user *)arg, sizeof(fmt))) {
			return -EFAULT;
		}
		if (fmt.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			return -ENOSYS;
		}
		if ((fmt.fmt.pix.width != vdma_h_res) ||
			(fmt.fmt.pix.height != vdma_v_res) ||
			(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24)) {
			return -EINVAL;
		}
		return 0;
	case VIDIOC_REQBUFS:
		if (raw_copy_from_user(&req, (void __user *)arg, sizeof(req))) {
			return -EFAULT;
		}
		if (req.memory != V4L2_MEMORY_MMAP) {
			return -EINVAL;
		}
		if (req.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			return -EINVAL;
		}
		if (req.count > 32) {
			return -EINVAL;
		}
		dp->user_frame_count = req.count;
		#ifdef USE_VMALLOC
		dp->user_mem = vmalloc(dp->frame_size * req.count);
		#else /* !USE_VMALLOC */
		dp->user_mem = dma_alloc_coherent(dp->dma_dev, dp->frame_size * req.count, &dp->user_phys_mem, GFP_KERNEL);
		#endif /* !USE_VMALLOC */
		PRINTK(KERN_INFO "dp->user_mem = %p\n", dp->user_mem);
		if (!dp->user_mem) {
			return -ENOMEM;
		}
		#ifdef USE_VMALLOC
		/* assign physical memory */
		offset = 0;
		while (offset < dp->frame_size * req.count) {
			*((uint32_t *)((size_t)dp->user_mem + offset)) = 0;
			offset += PAGE_SIZE;
		}
		#endif /* USE_VMALLOC */
		return 0;
	case VIDIOC_QUERYBUF:
		if (raw_copy_from_user(&buf, (void __user *)arg, sizeof(buf))) {
			return -EFAULT;
		}
		if (buf.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			return -EINVAL;
		}
		if (buf.type != V4L2_MEMORY_MMAP) {
			return -EINVAL;
		}
		if (buf.index >= dp->user_frame_count) {
			return -EINVAL;
		}
		buf.length = dp->frame_size;
		buf.m.offset = buf.index * dp->frame_size;
		if (raw_copy_to_user((void __user *)arg, &buf, sizeof(buf))) {
			return -EFAULT;
		}
		return 0;
	case VIDIOC_QBUF:
		if (raw_copy_from_user(&buf, (void __user *)arg, sizeof(buf))) {
			return -EFAULT;
		}
		if (buf.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			return -EINVAL;
		}
		if (buf.type != V4L2_MEMORY_MMAP) {
			return -EINVAL;
		}
		if (buf.index >= dp->user_frame_count) {
			return -EINVAL;
		}
		spin_lock_irq(&dp->lock);
		dp->queue_bits |= (1 << buf.index);
		PRINTK(KERN_INFO "QBUF: buf.index = %d, dp->queue_bits = %d, dp->active_bits = %d\n", buf.index, dp->queue_bits, dp->active_bits);
		spin_unlock_irq(&dp->lock);
		return 0;
	case VIDIOC_DQBUF:
		if (raw_copy_from_user(&buf, (void __user *)arg, sizeof(buf))) {
			return -EFAULT;
		}
		if (buf.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			return -EINVAL;
		}
		if (buf.type != V4L2_MEMORY_MMAP) {
			return -EINVAL;
		}
		if (buf.index >= dp->user_frame_count) {
			return -EINVAL;
		}
		spin_lock_irq(&dp->lock);
		slot = zynq_v4l2_find_oldest_slot(dp->active_bits, dp->latest_frame);
		if (slot == -1 ) {
			spin_unlock_irq(&dp->lock);
			rc = -EAGAIN;
		} else {
			dp->queue_bits &= ~(1 << slot);
			dp->active_bits &= ~(1 << slot);
			PRINTK(KERN_INFO "DQBUF: slot = %d, dp->queue_bits = %d, dp->active_bits = %d\n", slot, dp->queue_bits, dp->active_bits);			buf.index = slot;
			spin_unlock_irq(&dp->lock);
			if (raw_copy_to_user((void __user *)arg, &buf, sizeof(buf))) {
				return -EFAULT;
			}
			rc = 0;
		}
		return rc;
	case VIDIOC_STREAMON:
		if (raw_copy_from_user(&type, (void __user *)arg, sizeof(type))) {
			return -EFAULT;
		}
		if (type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			return -EINVAL;
		}
		zynq_v4l2_vdma_intr_enable(dp);
		return 0;
	case VIDIOC_STREAMOFF:
		if (raw_copy_from_user(&type, (void __user *)arg, sizeof(type))) {
			return -EFAULT;
		}
		if (type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			return -EINVAL;
		}
		zynq_v4l2_vdma_intr_disable(dp);
		return 0;
	default:
		return -ENOSYS;
	}
}

static int zynq_v4l2_vma_fault(struct vm_fault *vmf)
{
	struct page *page;
	unsigned long offset = vmf->pgoff << PAGE_SHIFT;
	struct zynq_v4l2_data *dp = vmf->vma->vm_private_data;

	if (offset > dp->frame_size * dp->user_frame_count) {
		return VM_FAULT_SIGBUS;
	}

	#ifdef USE_VMALLOC
	page = vmalloc_to_page((void *)((unsigned long)dp->user_mem + offset));
	#else /* !USE_VMALLOC */
	page = phys_to_page(dp->user_phys_mem + offset);
	#endif /* !USE_VMALLOC */
	get_page(page);
	vmf->page = page;

	return 0;
}

static struct vm_operations_struct zynq_v4l2_vm_ops = {
	.fault = zynq_v4l2_vma_fault,
};

static int zynq_v4l2_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct zynq_v4l2_data *dp = filp->private_data;

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	if (vma->vm_pgoff + vma_pages(vma) > ((dp->frame_size * dp->user_frame_count + PAGE_SIZE - 1) >> PAGE_SHIFT)) {
		return -EINVAL;
	}

	vma->vm_private_data = dp;
	vma->vm_ops = &zynq_v4l2_vm_ops;

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

static int zynq_v4l2_create_cdev(struct device *parent, struct zynq_v4l2_sys_data *sp)
{
	int ret;
	dev_t dev;
	int minor;

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
	if (ret != 0) {
		printk(KERN_ERR "alloc_chrdev_region = %d\n", ret);
		return -1;
	}

	sp->class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(sp->class)) {
		printk(KERN_ERR "class_create\n");
		goto error;
	}

	cdev_init(&sp->cdev, &zynq_v4l2_fops);
	sp->cdev.owner = THIS_MODULE;
	sp->major = MAJOR(dev);

	ret = cdev_add(&sp->cdev, dev, MINOR_NUM);
	if (ret != 0) {
		printk(KERN_ERR "cdev_add = %d\n", ret);
		goto error;
	}
	sp->cdev_inited = true;

	for (minor = 0; minor < MINOR_NUM; minor++) {
		struct zynq_v4l2_data *dp = &sp->dev[minor];

		device_create(sp->class, parent, MKDEV(sp->major, minor), dp, "video%d", minor);
		dp->dma_dev = parent;
	}

	return 0;

error:
	for (minor = 0; minor < MINOR_NUM; minor++) {
		if (sp->class) {
			device_destroy(sp->class, MKDEV(sp->major, minor));
		}
	}
	if (sp->cdev_inited) {
		cdev_del(&sp->cdev);
		sp->cdev_inited = false;
	}
	if (sp->class) {
		class_destroy(sp->class);
		sp->class = NULL;
	}
	unregister_chrdev_region(dev, MINOR_NUM);

	return -1;
}

static void zynq_v4l2_delete_cdev(struct zynq_v4l2_sys_data *sp)
{
	int minor;
	dev_t dev = MKDEV(sp->major, MINOR_BASE);

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
		device_destroy(sp->class, MKDEV(sp->major, minor));
	}
	if (sp->cdev_inited) {
		cdev_del(&sp->cdev);
	}
	class_destroy(sp->class);
	unregister_chrdev_region(dev, MINOR_NUM);
}

static int zynq_v4l2_hw_init(struct device *dev, struct zynq_v4l2_sys_data *sp)
{
	int minor, rc;

	PRINTK(KERN_INFO "%s\n", __FUNCTION__);

	if (zynq_v4l2_vdma_init(dev, sp)) {
		return -ENODEV;
	}

	if (zynq_v4l2_demosaic_init()) {
		rc = -ENODEV;
		goto error;
	}

	if (zynq_v4l2_mipicsi_init()) {
		rc = -ENODEV;
		goto error;
	}

	return 0;

error:
	for (minor = 0; minor < MINOR_NUM; minor++) {
		struct zynq_v4l2_data *dp = &sp->dev[minor];

		free_irq(dp->irq_num_vdma, dp);
		dma_free_coherent(dp->dma_dev, dp->alloc_size_wb_vdma, dp->virt_wb_vdma, dp->phys_wb_vdma);
		iounmap(dp->reg_vdma);
	}

	return rc;
}

static void zynq_v4l2_free_data_all(struct zynq_v4l2_sys_data *sp)
{
	int minor;

	for (minor = 0; minor < MINOR_NUM; minor++) {
		struct zynq_v4l2_data *dp = &sp->dev[minor];

		if (dp->irq_num_vdma) {
			free_irq(dp->irq_num_vdma, dp);
		}
		if (dp->virt_wb_vdma) {
			dma_free_coherent(dp->dma_dev, dp->alloc_size_wb_vdma, dp->virt_wb_vdma, dp->phys_wb_vdma);
		}
		if (dp->reg_vdma) {
			iounmap(dp->reg_vdma);
		}
		if (dp->user_mem) {
			#ifdef USE_VMALLOC
			vfree(dp->user_mem);
			#else /* !USE_VMALLOC */
			dma_free_coherent(dp->dma_dev, dp->frame_size * dp->user_frame_count, dp->user_mem, dp->user_phys_mem);
			#endif /* !USE_VMALLOC */
		}
	}
	if (sp->cdev_inited) {
		zynq_v4l2_delete_cdev(sp);
		sp->cdev_inited = false;
	}
	kfree(sp);
}

static int zynq_v4l2_probe(struct platform_device *pdev)
{
	int minor;
	struct device *dev = &pdev->dev;
	struct zynq_v4l2_sys_data *sp;

	dev_info(dev, "Device Tree Probing");
	printk(KERN_INFO "%s\n", __FUNCTION__);

	sp = (struct zynq_v4l2_sys_data *)kzalloc(sizeof(struct zynq_v4l2_sys_data), GFP_KERNEL);
	if (!sp) {
		dev_err(dev, "Could not allocate memory\n");
		return -ENOMEM;
	}

	if (zynq_v4l2_create_cdev(&pdev->dev, sp)) {
		dev_err(dev, "zynq_v4l2_create_cdev failed\n");
		zynq_v4l2_free_data_all(sp);
		dev_set_drvdata(dev, NULL);
		return -EBUSY;
	}

	if (zynq_v4l2_hw_init(&pdev->dev, sp)) {
		dev_err(dev, "hw_init failed\n");
		zynq_v4l2_free_data_all(sp);
		dev_set_drvdata(dev, NULL);
		return -ENODEV;
	}

	for (minor = 0; minor < MINOR_NUM; minor++) {
		struct zynq_v4l2_data *dp = &sp->dev[minor];

		spin_lock_init(&dp->lock);
		init_waitqueue_head(&dp->waitq);
		dp->latest_frame = dp->inst_vdma->MaxNumFrames - 1;
	}

	dev_set_drvdata(dev, sp);

	return 0;
}

static int zynq_v4l2_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct zynq_v4l2_sys_data *sp = dev_get_drvdata(dev);

	printk(KERN_INFO "%s\n", __FUNCTION__);

	zynq_v4l2_free_data_all(sp);
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
