#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "stats.h"
#include "bbuff.h"

typedef struct {
    int factory_number;
    double creation_ts_ms;
} candy_t;

static int factory_working;

double current_time_in_ms(void) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
}

void *factories_thread(void *varg) {

    int factory_number = *((int *) varg);
    while (factory_working) {
        // Pick a number of seconds which it will (later) wait
        int num_second = rand() % 4;
        // Print a message
        printf("Factory %d ships candy & waits %ds\n", factory_number, num_second);

        // Dynamically allocate a new candy item
        candy_t *candy = (candy_t *) malloc(sizeof(candy_t));
        candy->creation_ts_ms = current_time_in_ms();
        candy->factory_number = factory_number;
        bbuff_blocking_insert(candy);

        stats_record_produced(factory_number);

        // Sleep for number of seconds
        sleep(num_second);
    }
    printf("Candy-factory %d done\n", factory_number);
    return NULL;
}

void *kids_thread(void *varg) {

    while (1) {
        void *vcandy = bbuff_blocking_extract();
        candy_t *candy = (candy_t *) vcandy;
        stats_record_consumed(candy->factory_number, current_time_in_ms() - candy->creation_ts_ms);
        free(candy);
        //printf("Eat a candy\n" );
        // Sleep for either 0 or 1 seconds (randomly selected)
        int num_second = rand() % 2;
        sleep(num_second);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int i;
    int num_factories = 5;
    int num_kids = 8;
    int seconds = 4;

    if (argc != 4) {
        printf("usage: ./candykids <#factories> <#kids> <#seconds>\n");
        return -1;
    }

    // 1.  Extract arguments
    num_factories = atoi(argv[1]);
    num_kids = atoi(argv[2]);
    seconds = atoi(argv[3]);

    // 2.  Initialization of modules
    factory_working = 1;
    bbuff_init();
    stats_init(num_factories);

    // 3.  Launch candy-factory threads
    pthread_t factory_pids[100];
    int factory_numbers[100];
    for (i = 0; i < num_factories; i++) {
        factory_numbers[i] = i;
        pthread_create(&factory_pids[i], NULL, factories_thread, factory_numbers + i);
    }

    // 4.  Launch kid threads
    pthread_t kid_pids[100];
    for (i = 0; i < num_kids; i++) {
        pthread_create(&kid_pids[i], NULL, kids_thread, NULL);
    }

    // 5.  Wait for requested time
    for (i = 0; i < seconds; i++) {
        sleep(1);
        printf("Time %ds\n", i + 1);
    }

    // 6.  Stop candy-factory threads
    factory_working = 0;
    for (i = 0; i < num_factories; i++) {
        pthread_join(factory_pids[i], NULL);
    }

    // 7.  Wait until there is no more candies
    while (!bbuff_is_empty()) {
        printf("Waiting for all candies to be consumed\n");
        sleep(1);
    }
    // 8.  Stop kid threads
    for (i = 0; i < num_kids; i++) {
        pthread_cancel(kid_pids[i]);
        pthread_join(kid_pids[i], NULL);
    }
    // 9.  Print statistics
    stats_display();
    // 10. Cleanup any allocated memory
    stats_cleanup();
    return 0;
}
