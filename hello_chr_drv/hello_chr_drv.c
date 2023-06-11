#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>

static int major = 0;
static struct cdev hello_cdev;

static char kernel_buf[1024];
static struct class *hello_class;

#define MIN(a, b) (a < b ? a : b)

// read() -> hello_drv_read()
static ssize_t hello_drv_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err;
	printk(KERN_INFO "%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	err = copy_to_user(buf, kernel_buf, MIN(1024, size));
	return MIN(1024, size);
}

// write() -> hello_drv_write()
static ssize_t hello_drv_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	printk(KERN_INFO "%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	err = copy_from_user(kernel_buf, buf, MIN(1024, size));
	return MIN(1024, size);
}

// open() -> hello_drv_open()
static int hello_drv_open(struct inode *node, struct file *file)
{
	printk(KERN_INFO "%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

// close() -> hello_drv_close()
static int hello_drv_close(struct inode *node, struct file *file)
{
	printk(KERN_INFO "%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

/* 定义自己的file_operations结构体                                              */
static struct file_operations hello_drv = {
	.owner = THIS_MODULE,		// 用于module引用计数机制中，防止模块在进行操作时被卸载。
	.open = hello_drv_open,		// open()
	.read = hello_drv_read,		// read()
	.write = hello_drv_write,	// write()
	.release = hello_drv_close, // close()
};

/* 加载驱动 入口函数 */
static int __init hello_init(void)
{
	int err;
	int rc;
	dev_t devid;

	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	/**
	 * 分配一块字符设备区域，次设备号从0开始，设备数量为1，名字为hello。
	 * 此函数会分配一个主设备号，主设备号的值保存在devid中。
	 */
	rc = alloc_chrdev_region(&devid, 0, 1, "hello");

	// 获取主设备号
	major = MAJOR(devid);

	/**
	 * 初始化hello_cdev结构体，这个结构体表示了在内核中的字符设备。
	 * hello_drv是自己定义的file_operations结构体，对此设备的操作方法。
	 */
	cdev_init(&hello_cdev, &hello_drv);

	// 把hello_cdev加入到内核，此时内核才认为这个字符设备是存在的。
	cdev_add(&hello_cdev, devid, 1);

	/**
	 * 创建一个名为“hello_class”的类,创建 /sys/class/hello_class目录
	 * 这是执行下一步 device_create 的前提。
	 */
	hello_class = class_create(THIS_MODULE, "hello_class");
	err = PTR_ERR(hello_class);
	if (IS_ERR(hello_class))
	{
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "hello");
		return -1;
	}

	/**
	 * 这是创建一个设备并向 sysfs 注册它，sysfs 是 Linux 提供的一个虚拟文件系统VFS(virtual file system)，
	 * 它使有关设备和驱动程序的信息在用户空间中可用。 此函数调用还会使设备出现在 /dev 目录中。
	 * 在这里，MKDEV(major, 0) 将主设备号和次设备号组合成一个设备号。
	 */
	device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello"); /* /dev/hello */

	return 0;
}

/* 卸载驱动：出口函数，注销驱动 */
static void __exit hello_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	// 与加载时的顺序相反
	device_destroy(hello_class, MKDEV(major, 0));
	class_destroy(hello_class);

	cdev_del(&hello_cdev);
	unregister_chrdev_region(MKDEV(major, 0), 1);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
