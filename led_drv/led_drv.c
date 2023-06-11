#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <asm/io.h>

#define LED_NUM 2

static int major;
static struct class *led_class;

/* registers */
// IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 地址：0x02290000 + 0x14
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;

// GPIO5_GDIR 地址：0x020AC004
static volatile unsigned int *GPIO5_GDIR;

// GPIO5_DR 地址：0x020AC000
static volatile unsigned int *GPIO5_DR;

static ssize_t led_write(struct file *filp, const char __user *buf,
						 size_t count, loff_t *ppos)
{
	char val;
	int ret;

	/* copy_from_user : get data from app */
	ret = copy_from_user(&val, buf, 1);

	/* to set gpio register: out 1/0 */
	if (val)
	{
		/* set gpio to let led on */
		*GPIO5_DR &= ~(1 << 3);
	}
	else
	{

		/* set gpio to let led off */
		*GPIO5_DR |= (1 << 3);
	}
	return 1;
}

static int led_drv_open(struct inode *inode, struct file *filp)
{
	/* enable gpio5
	 * configure gpio5_io3 as gpio
	 * configure gpio5_io3 as output
	 */
	*IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 &= ~0xf;
	*IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 |= 0x5;

	*GPIO5_GDIR |= (1 << 3);

	return 0;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_drv_open,
	.read = led_drv_read,
	.write = led_drv_write,
	.release = led_drv_close,
};

/* 入口函数 */
static int __init led_init(void)
{
	int err;
	int i;

	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	major = register_chrdev(0, "edgar_led", &led_fops);

	led_class = class_create(THIS_MODULE, "edgar_led_class");
	err = PTR_ERR(led_class);
	if (IS_ERR(led_class))
	{
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "edgar_led");
		return -1;
	}

	for (i = 0; i < LED_NUM; i++)
	{
		device_create(led_class, NULL, MKDEV(major, i), NULL, "edgar_led%d", i);
	}

	/* ioremap */
	// IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 地址：0x02290000 + 0x14
	IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = ioremap(0x02290000 + 0x14, 4);

	// GPIO5_GDIR 地址：0x020AC004
	GPIO5_GDIR = ioremap(0x020AC004, 4);

	// GPIO5_DR 地址：0x020AC000
	GPIO5_DR = ioremap(0x020AC000, 4);

	return 0;
}

static void __exit led_exit(void)
{
	iounmap(IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3);
	iounmap(GPIO5_GDIR);
	iounmap(GPIO5_DR);

	device_destroy(led_class, MKDEV(major, 0));
	class_destroy(led_class);

	unregister_chrdev(major, "edgar_led");
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");