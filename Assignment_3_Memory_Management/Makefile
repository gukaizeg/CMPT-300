TARGET = myalloc
OBJS = main.o myalloc.o

CFLAGS = -Wall -g -std=c99 -D_POSIX_C_SOURCE=199309L 
CC = gcc

all: clean $(TARGET)

%.o : %.c
	$(CC) -c $(CFLAGS) $< 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -lpthread

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)
