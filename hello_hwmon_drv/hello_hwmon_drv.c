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
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

static int major = 0;
static struct cdev hello_cdev;
static struct class *hello_class;

struct hello_data
{
	u8 chip_update_interval;
	u8 temp_input;
	u8 temp_max;
};

static int hello_read(struct device *dev, enum hwmon_sensor_types type,
					  u32 attr, int channel, long *val)
{
	printk(KERN_INFO "[%s %s line %d] channel: %d\n", __FILE__, __FUNCTION__, __LINE__, channel);
	struct hello_data *data = NULL;
	data = dev_get_drvdata(dev);

	switch (type)
	{
	case hwmon_chip:
		switch (attr)
		{
		case hwmon_chip_update_interval:
			*val = data->chip_update_interval;
			break;
		default:
			return -EINVAL;
		}
		break;
	case hwmon_temp:
		switch (attr)
		{
		case hwmon_temp_input:
			++data->temp_input;
			if (data->temp_input > 100)
			{
				data->temp_input = 0;
			}
			*val = data->temp_input;
			break;
		case hwmon_temp_max:
			*val = data->temp_max;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int hello_write(struct device *dev, enum hwmon_sensor_types type,
					   u32 attr, int channel, long val)
{
	printk(KERN_INFO "[%s %s line %d] channel: %d\n", __FILE__, __FUNCTION__, __LINE__, channel);
	struct hello_data *data = dev_get_drvdata(dev);

	switch (type)
	{
	case hwmon_chip:
		switch (attr)
		{
		case hwmon_chip_update_interval:
			data->chip_update_interval = val;
			break;
		default:
			return -EINVAL;
		}
		break;
	case hwmon_temp:
		switch (attr)
		{
		case hwmon_temp_max:
			data->temp_max = val;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

// 是否可见，以及在sysfs中的权限
static umode_t hello_is_visible(const void *data, enum hwmon_sensor_types type,
								u32 attr, int channel)
{
	printk(KERN_INFO "[%s %s line %d] channel: %d\n", __FILE__, __FUNCTION__, __LINE__, channel);
	switch (type)
	{
	case hwmon_chip:
		switch (attr)
		{
		case hwmon_chip_update_interval:
			return 0644;
		}
		break;
	case hwmon_temp:
		switch (attr)
		{
		case hwmon_temp_input:
			return 0444;
		case hwmon_temp_max:
			return 0644;
		}
		break;
	default:
		break;
	}
	return 0;
}

static const struct hwmon_channel_info *hello_info[] = {
	HWMON_CHANNEL_INFO(chip,
					   HWMON_C_UPDATE_INTERVAL),
	HWMON_CHANNEL_INFO(temp,
					   HWMON_T_INPUT | HWMON_T_MAX),
	NULL};

static const struct hwmon_ops hello_hwmon_ops = {
	.is_visible = hello_is_visible,
	.read = hello_read,
	.write = hello_write,
};

static const struct hwmon_chip_info hello_chip_info = {
	.ops = &hello_hwmon_ops,
	.info = hello_info,
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
	struct device *hello_device = device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello"); /* /dev/hello */

	/**
	 * devm_kzalloc() 申请内存，内存释放由内核自动管理。
	 * devm 表示 device managed。申请到的内存与某设备绑定，当设备被卸载时，内存会被自动释放。
	 *
	 * GFP_KERNEL 这就是“分配上下文”。 它是一个Flag，决定如何完成内存分配。
	 * GFP_KERNEL 表示分配是在正常的、非紧急的上下文中完成的。
	 * 如果它需要等待内存被释放，它可以休眠（阻塞），这使得在可以休眠的环境中使用它是安全的。
	 * GFP_KERNEL 是最常用的标志，因为它适用于大多数内核代码。
	 *
	 * 将data绑定到字符设备 hello_device 上。
	 */
	struct hello_data *data = devm_kzalloc(&hello_device, sizeof(struct hello_data), GFP_KERNEL);
	if (!data)
	{
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	/**
	 * devm_hwmon_device_register_with_groups 注册一个hwmon设备。
	 * devm含义同上，将其绑定到字符设备 hello_device 上。hello_device 注销时，hwmon_dev也会被注销。
	 *
	 * data：指向要绑定到hwmon设备的数据。
	 */
	struct device *hwmon_dev;
	hwmon_dev = devm_hwmon_device_register_with_info(&hello_device, "hello_hwmon",
													 data, &hello_chip_info,
													 NULL);
	if (IS_ERR(hwmon_dev))
	{
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		return PTR_ERR(hwmon_dev);
	}

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
