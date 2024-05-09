

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include "array_stats.h"


SYSCALL_DEFINE3(array_stats, struct array_stats *, stats, long *, data, long, size)
{
	long i;
	long tmp;
	struct array_stats mystat;

	printk("array_stats begin! size = %ld\n", size);
	if(size <= 0 ) {
		return -EINVAL;
	}
	
	// copy_from_user(to, from, n)
	if(copy_from_user(&tmp, &data[0], sizeof(long))) {
		return -EFAULT;
	}

	mystat.sum = 0;
	mystat.min = tmp;
	mystat.max = tmp;
	for(i = 0;i < size; i++) {

		if(copy_from_user(&tmp, &data[i], sizeof(long))) {
			return -EFAULT;
		}

		mystat.sum += tmp;
		if(mystat.min > tmp) {
			mystat.min = tmp;
		}
		if(mystat.max < tmp) {
			mystat.max = tmp;
		}
	}

	// copy_to_user(to, from, n)
	if(copy_to_user(stats, &mystat, sizeof(struct array_stats))){ 
		return -EFAULT;
	}

	printk("array_stats end!\n");
  	return 0;
}





