
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include "process_ancestors.h"

long count_node(struct list_head* head)
{
    struct list_head* cur;
    long cnt = 0;
    cur = head;
    do
    {
        cur = cur->next;
        cnt++;
    }
    while (cur != head);
    return cnt;
}

SYSCALL_DEFINE3(process_ancestors, struct process_info *, info_array, long, size, long *, num_filled)
{
    long i;
    struct process_info info;
    struct task_struct* task;
    printk("array_stats begin! size = %ld\n", size);
    if (size <= 0)
    {
        return -EINVAL;
    }
    (*num_filled) = 0;

    task = current;
    i = 0;
    while (i < size)
    {
        info.pid = task->pid;
        strcpy(info.name, task->comm);
        info.state = task->state;
        info.uid = task->loginuid.val;
        // info.uid = 999;
        info.nvcsw = task->nvcsw;
        info.nivcsw = task->nivcsw;

        info.num_children = count_node(&task->children);
        info.num_siblings = count_node(&task->sibling);

        // copy_to_user(to, from, n)
        if (copy_to_user(&info_array[i], &info, sizeof(struct process_info)))
        {
            return -EFAULT;
        }

        i++;
        if (task == task->parent)
        {
            break;
        }
        task = task->parent;
    }

    // copy_to_user(to, from, n)
    if (copy_to_user(num_filled, &i, sizeof(long)))
    {
        return -EFAULT;
    }
    return 0;
}


