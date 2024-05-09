#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stats.h"

struct record_t {
    int factory_number;
    int made;
    int eaten;
    double max_delay;
    double min_delay;
    double total_delay;
    int count_delay;
};

static struct record_t *records;
static int num_producers;

void stats_init(int _num_producers) {
    int i;
    num_producers = _num_producers;
    records = (struct record_t *) malloc(_num_producers * sizeof(struct record_t));
    assert(records);
    memset(records, 0, num_producers * sizeof(struct record_t));
    for (i = 0; i < _num_producers; i++) {
        records[i].factory_number = i;
    }
}
void stats_cleanup(void) {
    free(records);
}
void stats_record_produced(int factory_number) {
    records[factory_number].made++;
}
void stats_record_consumed(int factory_number, double delay_in_ms) {

    if (records[factory_number].count_delay == 0 
        || records[factory_number].min_delay > delay_in_ms) {
        records[factory_number].min_delay = delay_in_ms;
    }
    if (records[factory_number].count_delay == 0
        || records[factory_number].max_delay < delay_in_ms) {
        records[factory_number].max_delay = delay_in_ms;
    }

    records[factory_number].total_delay += delay_in_ms;
    records[factory_number].count_delay++;

    records[factory_number].eaten++;
}

void stats_display(void) {
    int i;
    printf("Statistics:\n");
    // #Made  #Eaten  Min Delay[ms]  Avg Delay[ms]  Max Delay[ms]
    printf("%8s%10s%10s%20s%20s%20s\n", "Factory#", "#Made", "#Eaten", "Min Delay[ms]",
        "Avg Delay[ms]", "Max Delay[ms]");
    for (i = 0; i < num_producers; i++) {
        double avg_delay = 0;
        if (records[i].count_delay > 0) {
            avg_delay = records[i].total_delay / records[i].count_delay;
        }
        printf("%8d%10d%10d%20.5f%20.5f%20.5f\n", records[i].factory_number, records[i].made,
            records[i].eaten, records[i].min_delay, avg_delay, records[i].max_delay);
    }
}
