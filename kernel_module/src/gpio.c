#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/printk.h>
#include "gpio.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sandesh Ghimire");
MODULE_DESCRIPTION("A simple kernel module");
MODULE_VERSION("0.1");

static int __init my_module_init(void) {
    pr_info("Hello, Kernel!\n");
    return 0;
}

static void __exit my_module_exit(void) {
    pr_info("Goodbye, Kernel!\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
