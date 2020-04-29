#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
///reference: b07902062
asmlinkage long long sys_get_kernel_time(void)
{
    struct timespec t;
    getnstimeofday(&t);
    return (long long)t.tv_sec * 1000000000 + (long long)t.tv_nsec;
}
