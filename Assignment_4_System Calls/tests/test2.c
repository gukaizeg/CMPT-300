#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <sys/syscall.h>
#include "process_ancestors.h"
#define _PROCESS_ANCESTORS_TEST_ 438 // for a 64 bit system

// long sys_process_ancestors(struct process_info *info_array,
//                                       long size,
//                                       long *num_filled);

int main(int argc, char *argv[])
{

    long i, result, num_filled;
    struct process_info info_array[10];
    
    printf("Hello World!\n");
    // result = syscall(_PROCESS_ANCESTORS_TEST_, info_array, 10, &num_filled);
    result = syscall(_PROCESS_ANCESTORS_TEST_, info_array, 10, &num_filled);
    printf("result = %ld\n", result);
    if (result == 0)
    {
        printf("num_filled = %ld\n", num_filled);
        for (i = 0; i < num_filled; i++)
        {
            printf("======== %ld ========\n", i);
            printf("pid          = %ld\n", info_array[i].pid);
            printf("uid          = %ld\n", info_array[i].uid);
            printf("name         = %s\n", info_array[i].name);
            printf("num_children = %ld\n", info_array[i].num_children);
            printf("num_siblings = %ld\n", info_array[i].num_siblings);
        }

    }
    // else if (result == -EINVAL)
    else if (result == -22)
    {
        printf("-EINVAL\n");
    }
    // else  if (result == -EFAULT)
    else  if (result == -10)
    {
        printf("-EFAULT\n");
    }
    else
    {
        printf("ERROR\n");
    }

    return 0;
  return 0;
}