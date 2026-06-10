#include "scheduling.h"
#include "applications.h"
#include "mapping.h"
#include "allocation.h"
#include "../jellyfish/allocation_jellyfish.h"
#include "list.h"
#include "globals.h"
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

scheduling_t scheduling; ///< the scheduling policy we are using in the simulation
char* scheduling_name; ///< the name of the used scheduling policy as a string
scheduling_info *sched_info;
long scheduling_nparam;    ///< Number of parameters passed to the scheduling policy

long scheduling_params[MAX_SCHEDULING_PARAMS];    ///< Parameters passed to the scheduling policy


extern long ports_switches;

void init_scheduling(long nservers, long nswitches){

    long i, j;

    sched_info = malloc(sizeof(scheduling_info));
    sched_info->makespan = 0;
    sched_info->last_makespan = 0;
    sched_info->total_servers = nservers;
    sched_info->busy_cores = 0;
    sched_info->free_cores = 0;
    sched_info->inactive_cores = 0;
    sched_info->stopped = 0;
    sched_info->running_apps = calloc(MAX_CONCURRENT_APPS + 1, sizeof(long));

    sched_info->servers = malloc(sizeof(server_t) * nservers);
    sched_info->san = malloc(sizeof(san_t));
    sched_info->san->san_links = malloc(sizeof(san_link_t) * n_io_servers);
    sched_info->san->bandwidth_read = san_capacity;
    sched_info->san->bandwidth_write = san_capacity;

    for(i = 0; i < nservers;i++){
        sched_info->free_cores += server_cores;
        sched_info->servers[i].free_cores = server_cores;
        sched_info->servers[i].busy_cores = 0;
        sched_info->servers[i].cont = 0;
        sched_info->servers[i].memory = server_memory;
        sched_info->servers[i].cont = 0;
        sched_info->servers[i].cores = malloc(sizeof(long) * server_cores); 
        for(j = 0; j < server_cores; j++){
            sched_info->servers[i].cores[j] = -1;
        }
        sched_info->total_cores += server_cores;
        sched_info->servers[i].storage = 0;
        
        
    }

    for(i = 0; i < n_io_servers;i++){
        sched_info->san->san_links[i].n_flows_read = 0;
        sched_info->san->san_links[i].n_flows_write = 0;
        sched_info->san->san_links[i].current_speed_read = 0.0;
        sched_info->san->san_links[i].current_speed_write = 0.0;
    }


        list_initialize(&running_applications, sizeof(application));
}

unsigned long long schedule_next_application(){

    unsigned long long time_next_app = ULLONG_MAX;

    if(!sched_info->stopped){
    switch(scheduling){
        case FCFS:
            time_next_app = fcfs();
            break;
        default:
            printf("Unkwown scheluding policy.\n");
            exit(-1);
            break;

    }
    }
    return(time_next_app);
}

unsigned long long fcfs(){

    long done = 0;
    long alloc = -1;
    unsigned long long time_next_app = ULLONG_MAX;
    application *next_app = NULL;

    while(!done && list_length(&workload) > 0){
        list_head(&workload, (void *)&next_app);                  
        if(next_app->info.arrive_time <= sched_info->makespan){
            alloc = allocate_application(next_app);
            if(alloc != 0){
                map_application(next_app);
                init_running_application(next_app);
            }
            else{
                sched_info->stopped = 1;
                done = 1;
            }
        }
        else{
            time_next_app = next_app->info.arrive_time;
            done = 1;
        }
    }
    return(time_next_app);
}

void finish_scheduling(long nservers){
    long i;

    free(sched_info->running_apps);
    
    for(i = 0; i < nservers;i++){
        free(sched_info->servers[i].cores);
    }
    free(sched_info->san->san_links);
    free(sched_info->san);
    free(sched_info->servers);
    free(sched_info);
}

/**
 * Given a switch, returns the number of available servers. 
 * A server is available if all cores are idle or if it has idle cores (>0) and the server is marked with cont == 0 (only one application can be allocated). 
 * av_servers contains the available servers.
 */
long available_servers(long switch_id, long *av_servers, long ports_servers){
    
    long i;
    long server_id = switch_id * ports_servers;
    long available = 0;

    for(i = 0; i < ports_servers; i++){
        if(sched_info->servers[server_id + i].busy_cores == 0 || (sched_info->servers[server_id + i].free_cores > 0 && sched_info->servers[server_id + i].cont == 0)){
            av_servers[available++] = server_id + i;
        }
    }
    return(available);
}

/**
 * Given a server, returns the number of idle cores in that servers. 
 * Available cores contains the global identifiers of those cores.
 */
long available_cores(long server_id, long *available_cores, long ports_servers){
    
    long i;
    long available = 0;

    for(i = 0; i < server_cores; i++){
        if(sched_info->servers[server_id].cores[i] == -1){
            available_cores[i] = server_id + i;
            available++;       
        }       
    }
    return(available);
}

/**
 * Given a switch, returns the number of available servers. 
 * A server is available if all cores are idle or if it has idle cores (>0) and the server is marked with cont == 0 (only one application can be allocated).  
 */
long are_available_servers(long switch_id, long ports_servers){
    
    long i;
    long server_id = switch_id * ports_servers;
    long available = 0;

    for(i = 0; i < ports_servers; i++){
        if(sched_info->servers[server_id + i].busy_cores == 0 || (sched_info->servers[server_id + i].free_cores > 0 && sched_info->servers[server_id + i].cont == 0)){
            available++;
        }
    }
    return(available);
}

/**
 * Given a server, returns the number of idle cores in that servers. 
 * Available cores contains the global identifiers of those cores.
 */
long are_available_cores(long server_id){
    
    long available = 0;

    if(sched_info->servers[server_id].cont == 0){
        available = sched_info->servers[server_id].free_cores;
    }
    return(available);
}

