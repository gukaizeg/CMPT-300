#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbuff.h"

struct buffer_t {
    void *items[BUFFER_SIZE];
    int first;
    int last;
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
} buffer;

void bbuff_init(void) {
    memset(&buffer, 0, sizeof(struct buffer_t));
    sem_init(&buffer.empty, 0, 0);
    sem_init(&buffer.full, 0, BUFFER_SIZE);
    pthread_mutex_init(&buffer.mutex, NULL);
}

void bbuff_blocking_insert(void *item) {
    //if ((buffer.last + 1) % BUFFER_SIZE == buffer.first) {
    //    printf("Buffer Full!\n");
    //    return;
    //}
    sem_wait(&buffer.full);
    pthread_mutex_lock(&buffer.mutex);
    buffer.items[buffer.last] = item;
    buffer.last = (buffer.last + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&buffer.mutex);
    sem_post(&buffer.empty);
}

void *bbuff_blocking_extract(void) {
    void *item;

    sem_wait(&buffer.empty);
    pthread_mutex_lock(&buffer.mutex);
    item = buffer.items[buffer.first];
    buffer.items[buffer.first] = NULL;
    buffer.first = (buffer.first + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&buffer.mutex);
    sem_post(&buffer.full);

    return item;
}

_Bool bbuff_is_empty(void) {
    int sval;
    sem_getvalue(&buffer.empty, &sval);
    return sval == 0;
}


