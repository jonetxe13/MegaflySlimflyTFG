#ifndef _scheduling
#define _scheduling

#include "storage.h"

/**
 * Structure that defines a physical node.
 */
typedef struct server_t{
    
    long n_cores;
    long busy_cores;
    long free_cores;
    long *cores;
    int cont;
    long memory;
    long mem_used;
    long storage;
} server_t;

typedef struct scheduling_info{

    //unsigned long long makespan;
    //unsigned long long last_makespan;
    double makespan;
    double last_makespan;
    
    long *running_apps;
    long total_servers;
    long cores_server;
    long total_cores;
    long busy_cores;
    long free_cores;
    long inactive_cores;
    long stopped;
    server_t *servers;
    struct san_t *san;
} scheduling_info;

void init_scheduling(long nservers, long nswitches);

unsigned long long schedule_next_application();

void finish_scheduling(long nservers);

unsigned long long fcfs();

long available_servers(long switch_id, long *av_servers, long ports_servers);

long available_cores(long server_id, long *available_cores, long ports_servers);

long are_available_servers(long switch_id, long ports_servers);

long are_available_cores(long server_id);

long is_inactive_switch(long switch_id);

long is_free_switch(long switch_id);
#endif
