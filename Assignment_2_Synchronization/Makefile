
all:candykids

candykids: *.c *.h
	gcc *.c -o $@ -lpthread -lrt -g

clean:
	rm -rf candykids

# valgrind --tool=helgrind --read-var-info=yes ./nyuenc
# valgrind --tool=drd --read-var-info=yes ./nyuenc

