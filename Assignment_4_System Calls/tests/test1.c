#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "array_stats.h"
#define _ARRAY_STATS_TEST_ 437 // for a 64 bit system

int main(int argc, char *argv[])
{

  long array[6] = {9,2,3,7,3,4};
  struct array_stats stat;
  long result;
  printf("\nStart Test array_stats\n");

  result = syscall(_ARRAY_STATS_TEST_, &stat, array, 6);

  printf("result = %ld\n", result);
  if(result == 0) {
  	printf("max = %ld\n", stat.max);
  	printf("min = %ld\n", stat.min);
  	printf("sum = %ld\n", stat.sum);
  }
  return 0;
}