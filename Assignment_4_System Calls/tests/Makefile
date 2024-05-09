



all: test1 array_stats_test test2 cmpt300_testapp

cmpt300_testapp: cmpt300_testapp.c
	gcc -std=c99 -D _GNU_SOURCE -static cmpt300_testapp.c -o cmpt300_testapp

test1: array_stats.h test1.c
	gcc -std=c99 -D _GNU_SOURCE -static test1.c -o test1


array_stats_test: array_stats.h array_stats_test.c
	gcc -std=c99 -D _GNU_SOURCE -static array_stats_test.c -o array_stats_test


test2: process_ancestors.h test2.c
	gcc -std=c99 -D _GNU_SOURCE -static test2.c -o test2


clean:
	rm -rf test1 array_stats_test test2 cmpt300_testapp
