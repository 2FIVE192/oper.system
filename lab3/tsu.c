#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>

#define procfs_name "tsu"

static int __init tsu_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit tsu_exit(void)
{
    pr_info("Tomsk State University forever!\n");
}

MODULE_LICENSE("GPL");
module_init(tsu_init);
module_exit(tsu_exit);
