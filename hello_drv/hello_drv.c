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
#include <linux/delay.h>
#include <linux/kthread.h>

static struct task_struct *print_hello_thread;

static int print_hello(void *unused)
{
	/**
	 * 注意不能写成while(1)，否则无法退出。
	 * 收到kthread_stop()后，kthread_should_stop()会返回true
	 * */
	while (!kthread_should_stop())
	{
		printk(KERN_INFO "hello world\n");
		msleep(4000);
		/**
		 * schedule()主动让出计算资源，kernel会调度其它线程运行。
		 * 不加也可以，加了提高效率。
		 */
		schedule();
	}

	return 0;
}

static int __init hello_init(void)
{

	printk(KERN_INFO "hello_init\n");
	print_hello_thread = kthread_run(print_hello, NULL, "hello");

	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "hello_exit\n");
	if (print_hello_thread)
	{
		printk(KERN_INFO "Stopping thread...\n");
		kthread_stop(print_hello_thread);
		printk(KERN_INFO "Thread stopped\n");
	}
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");