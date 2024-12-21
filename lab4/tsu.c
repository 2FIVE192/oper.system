#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/version.h>

#define PROCFS_NAME "tsulab"

static struct proc_dir_entry *our_proc_file = NULL;

// Функция для вычисления дней до Китайского Нового года
static int days_until_chinese_new_year(void) {
    struct tm cny_date = {
        .tm_year = 2025 - 1900, // Год Китайского Нового года
        .tm_mon = 1 - 1,        // Январь (.месяцы от 0 до 11)
        .tm_mday = 29,          // День месяца
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
    };

    struct timespec64 current_time;
    struct timespec64 cny_time;
    time64_t cny_seconds;
    s64 diff;

    
    ktime_get_real_ts64(&current_time);

    cny_seconds = mktime64(cny_date.tm_year, cny_date.tm_mon + 1, cny_date.tm_mday,
                           cny_date.tm_hour, cny_date.tm_min, cny_date.tm_sec);
    cny_time.tv_sec = cny_seconds;
    cny_time.tv_nsec = 0;

    
    diff = (cny_time.tv_sec - current_time.tv_sec) / 86400;
    return diff > 0 ? diff : 0; // Если разница отрицательная, возвращаем 0
}


static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char message[64];
    int days = days_until_chinese_new_year();
    int len;

  
    len = snprintf(message, sizeof(message), "Days until Chinese New Year: %d\n", days);

    
    if (*ppos > 0 || count < len)
        return 0;

   
    if (copy_to_user(buf, message, len))
        return -EFAULT;

    *ppos = len; 
    return len;
}


static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};


static int __init tsu_init(void) {
    
    our_proc_file = proc_create(PROCFS_NAME, 0444, NULL, &proc_file_ops);
    if (!our_proc_file) {
        pr_err("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    return 0;
}


static void __exit tsu_exit(void) {
    proc_remove(our_proc_file); // Удаляем файл из /proc
}

module_init(tsu_init);
module_exit(tsu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Илья");
MODULE_DESCRIPTION("Модуль для вывода дней до Китайского Нового года.");
