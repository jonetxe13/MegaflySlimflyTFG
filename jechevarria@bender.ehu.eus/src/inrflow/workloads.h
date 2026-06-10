#ifndef _workloads
#define _workloads

#include "list.h"

typedef struct workload_t{

    long n_apps;
    arrival_t arrival_time;
    char arrival_mode_s[20];
    tpattern_t pattern;
    char pattern_file[50];
    char pattern_s[20];
    long pattern_params[3];
    long min_size;
    long max_size;
    allocation_t allocation;
    char allocation_s[20];
    long allocation_param[2];
    mapping_t mapping;
    char mapping_s[20];

}workload_t;

void init_workload(list_t *list);

void read_applications_from_file(char *file);

void generate_automatic_application();

long next_arrival_time(long previous);

long generate_application_size(long min, long max);
#endif

