#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>

#define PROCFS_NAME "tsulab"

static struct proc_dir_entry *our_proc_file = NULL;

// Функция для вычисления дней до Китайского Нового года
static int days_until_chinese_new_year(void) {
    struct tm cny_date = {
        .tm_year = 2025 - 1900, // Год (tm_year — годы с 1900)
        .tm_mon = 0,            // Январь (месяцы от 0 до 11)
        .tm_mday = 29,          // День месяца
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
    };

    struct timespec64 current_time;
    struct timespec64 cny_time;
    s64 diff;

    // Получение текущего времени
    ktime_get_real_ts64(&current_time);

    // Преобразование даты Китайского Нового года в время
    cny_time.tv_sec = mktime64(cny_date.tm_year + 1900, cny_date.tm_mon + 1, cny_date.tm_mday,
                               cny_date.tm_hour, cny_date.tm_min, cny_date.tm_sec);
    cny_time.tv_nsec = 0; // Устанавливаем наносекунды в 0

    // Разница в секундах между текущим временем и временем Китайского Нового года
    diff = (cny_time.tv_sec - current_time.tv_sec) / 86400; // Разница в днях

    pr_info("Current time (seconds): %lld\n", (long long)current_time.tv_sec);
    pr_info("CNY time (seconds): %lld\n", (long long)cny_time.tv_sec);
    pr_info("Difference in days: %lld\n", (long long)diff);

    return diff > 0 ? diff : 0; // Если разница отрицательная, возвращаем 0
}

// Чтение данных из файла /proc/tsulab
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char message[64];
    int days = days_until_chinese_new_year();
    int len;

    // Формируем сообщение для вывода
    len = snprintf(message, sizeof(message), "Days until Chinese New Year: %d\n", days);

    // Если пользователь уже прочитал данные или запрашивает меньше, чем длина сообщения
    if (*ppos > 0 || count < len)
        return 0;

    // Копируем данные в пользовательский буфер
    if (copy_to_user(buf, message, len))
        return -EFAULT;

    *ppos = len; // Устанавливаем позицию чтения
    return len;
}

// Операции с файлом /proc
static const struct file_operations proc_file_ops = {
    .owner = THIS_MODULE,
    .read = proc_read,
};

// Инициализация модуля
static int __init tsu_init(void) {
    our_proc_file = proc_create(PROCFS_NAME, 0444, NULL, &proc_file_ops);
    if (!our_proc_file) {
        pr_err("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    pr_info("Module tsu loaded: /proc/%s created\n", PROCFS_NAME);
    return 0;
}

// Завершение работы модуля
static void __exit tsu_exit(void) {
    proc_remove(our_proc_file);
    pr_info("Module tsu unloaded: /proc/%s removed\n", PROCFS_NAME);
}

module_init(tsu_init);
module_exit(tsu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Илья");
MODULE_DESCRIPTION("Модуль для вычисления дней до Китайского Нового года.");
