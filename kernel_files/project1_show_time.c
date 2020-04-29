#include <linux/linkage.h>
#include <linux/kernel.h>
///reference: b07902062
asmlinkage void sys_show_time(pid_t pid, long long start_time, long long finish_time)
{
    long long start_sec = start_time / 1000000000;
    long long finish_sec = finish_time / 1000000000;
    long long start_nsec = start_time % 1000000000;
    long long finish_nsec = finish_time % 1000000000;
    printk("[Project1] %d %09lld.%09lld %09lld.%09lld\n", pid, start_sec, start_nsec, finish_sec, finish_nsec);
    return;
}
