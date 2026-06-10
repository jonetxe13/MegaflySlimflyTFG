#include "allocation.h"
#include "applications.h"
#include "scheduling.h"
#include "literal.h"
#include "globals.h"
#include "../jellyfish/allocation_jellyfish.h"
#include <stdio.h>
#include <stdlib.h>


long allocate_application(application *app){
    
    long alloc =0;

    alloc = allocate_application_tasks(app);
    if(alloc == 1 && app->size_storage > 0){
        alloc = allocate_application_storage(app);
   }
    return(alloc);
}

long allocate_application_tasks(application *app){

    long i, j, k;
    long allocated =0;
    long *cores = NULL;
    long *cores_aux = NULL;
    long local_core;

    cores = malloc(sizeof(long) * app->size_tasks);
    switch(app->allocation){
        case SEQUENTIAL_ALLOC:
            i=0;
            k = 0;
            if(sched_info->free_cores >=  app->size_tasks){
                while(!allocated && i < sched_info->total_servers){
                    j = 0;
                    while(!allocated && j < server_cores){
                        if(sched_info->servers[i].cores[j] == -1){
                            cores[k++] = (i * server_cores) + j;
                            if(k == app->size_tasks){
                                allocated = 1;
                            }
                        }
                        j++;
                    } 
                    i++;
                } 
            }
            break;
        case RANDOM_ALLOC:
            if(sched_info->free_cores >=  app->size_tasks){
                cores_aux = calloc(sched_info->total_cores, sizeof(long));
                i = 0;
                j = rand() % sched_info->total_servers;
                k = rand() % server_cores;
                while(i < app->size_tasks){
                    local_core = (j * server_cores) + k;
                    if(sched_info->servers[j].cores[k] == -1 && cores_aux[local_core] == 0){
                        cores_aux[local_core] = 1;
                        cores[i++] = local_core;
                    }
                    j = rand() % sched_info->total_servers;
                    k = rand() % server_cores;
                }
                free(cores_aux);
                allocated = 1;
            }
            break;
        case JELLYFISH_SPREAD_ALLOC:
        case JELLYFISH_RANDOM_ALLOC:
        case JELLYFISH_CONTIGUITY_ALLOC:
        case JELLYFISH_CONTIGUITY_IF_ALLOC:
        case JELLYFISH_CONTIGUITY_IF2_ALLOC:
        case JELLYFISH_LOCALITY_ALLOC:
        case JELLYFISH_LOCALITY2_ALLOC:
        case JELLYFISH_QUASICONTIGUITY_ALLOC:
            if(sched_info->free_cores >=  app->size_tasks){
                allocated = allocate_application_jellyfish(app, cores);
            }
            break;
        default:
            printf("Unkwown allocation strategy.\n");
            exit(-1);
    }
    if(allocated == 1){
        assign_active_cores(app, cores);
    }
    else if((allocated == 0) && (sched_info->free_cores ==  sched_info->total_cores)){
        printf("Allocation strategy is not able to find a suitable partition for application %ld.\n",app->info.id);
        exit(-1);
    }
    else{
        free(cores);
    }
    return(allocated);
}

long allocate_application_storage(application *app){ 

    long i, j;
    long alloc =0;
    long server_aux = 0;
    long *servers_storage = NULL;
    long *servers_aux = NULL;

    servers_storage = malloc(sizeof(long) * app->size_storage);
    switch(app->storage){
        case RND_STG:
            i = 0;
            servers_aux = calloc(sched_info->total_servers, sizeof(long));
            while(i < app->size_storage){
                j  = rand() % sched_info->total_servers;
                if(servers_aux[j] == 0){
                    //servers_storage is the nth - 1 core of the server.
                    servers_storage[i++] = ((server_cores * j) + (server_cores - 1));
                    servers_aux[j] = 1;
                }
            }
            alloc = 1;
            free(servers_aux);
            break;
        case LOCAL_STG:
            i = 0;
            servers_aux = calloc(sched_info->total_servers, sizeof(long));
            while(i < app->size_storage){
                j  = rand() % app->size_tasks;
                server_aux = (j / server_cores);
                if(servers_aux[server_aux] == 0){
                    //servers_storage is the nth - 1 core of the server.
                    servers_storage[i++] = ((server_cores * server_aux) + (server_cores - 1));
                    servers_aux[server_aux] = 1;
                }
            }
            alloc = 1;
            free(servers_aux);
            break;
        case CACHE_STG:
            j = 0;
            servers_aux = calloc(sched_info->total_servers, sizeof(long));
            for(i = 0; i < app->size_storage; i++){
                server_aux = (i / server_cores);
                servers_storage[j++] = ((server_cores * server_aux) + (server_cores - 1));
            }
            alloc = 1;
            free(servers_aux);
            break;
        default:
            printf("Unkwown storage strategy.\n"); 
            exit(-1);
    }
    if(alloc == 1){
        assign_servers_storage(app, servers_storage);
    }
    else{
        free(servers_storage);
    }
//    for(i = 0; i < app->size_storage; i++){
//        printf("%ld ", app->servers_storage[i]);
//    }
//    printf("\n");
    return(alloc);
}

void release_application(application *app){

    switch(app->allocation){    
        case JELLYFISH_SPREAD_ALLOC:
        case JELLYFISH_RANDOM_ALLOC:
        case JELLYFISH_CONTIGUITY_ALLOC:
        case JELLYFISH_CONTIGUITY_IF_ALLOC:
        case JELLYFISH_CONTIGUITY_IF2_ALLOC:
        case JELLYFISH_LOCALITY_ALLOC:
        case JELLYFISH_LOCALITY2_ALLOC:
        case JELLYFISH_QUASICONTIGUITY_ALLOC:
            release_application_jellyfish(app);
            break;
        default:
            break;
    }

    release_active_cores(app);
    release_servers_storage(app);
}

void assign_active_cores(application *app, long *cores){

    long i, j;
    long local_server, local_core;

    for(i = 0; i < app->size_tasks; i++){
        local_server = cores[i] / server_cores;
        local_core = cores[i] % server_cores;
        sched_info->servers[local_server].cores[local_core] = app->info.id;
        for(j = 0; j < server_cores; j++){
            if(sched_info->servers[local_server].cores[j] == app->info.id){
                app->num_servers++;
                break;
            }
        }
        sched_info->servers[local_server].free_cores--;
        sched_info->servers[local_server].busy_cores++;
    }
    app->cores_active = cores;
    metrics.scheduling.utilization += ((sched_info->busy_cores * (sched_info->makespan - sched_info->last_makespan)) / sched_info->total_cores);
    sched_info->last_makespan = sched_info->makespan;
    sched_info->busy_cores += app->size_tasks;
    sched_info->free_cores -= app->size_tasks;
}

void assign_servers_storage(application *app, long *servers_storage){

    long i;

    app->servers_storage = servers_storage;
    for(i = 0; i < app->size_storage; i++){
        sched_info->servers[servers_storage[i]].storage++;    
    }
}

void release_servers_storage(application *app){
    
    long i;

    for(i = 0; i < app->size_storage; i++){
        sched_info->servers[app->servers_storage[i]].storage--;    
    }

    if(app->size_storage > 0){
        free(app->servers_storage);
    }
}

void assign_inactive_cores(application *app, long *cores_inactive, long n_cores_inactive){

    long i;
    long local_server, local_core;

    if(n_cores_inactive > 0){
        for(i = 0; i < n_cores_inactive; i++){
            local_server = cores_inactive[i] / server_cores;
            local_core = cores_inactive[i] % server_cores;
            sched_info->servers[local_server].cores[local_core] = app->info.id;
            sched_info->servers[local_server].free_cores--;
            sched_info->servers[local_server].busy_cores++;
        }
        metrics.scheduling.utilization_if += ((sched_info->inactive_cores * (sched_info->makespan - sched_info->last_makespan)) / sched_info->total_cores);

        app->cores_inactive = cores_inactive;
        sched_info->busy_cores += n_cores_inactive;
        sched_info->free_cores -= n_cores_inactive;
        sched_info->inactive_cores += n_cores_inactive;
    }
    else{
        free(cores_inactive);
    }
    app->n_cores_inactive = n_cores_inactive;
}

void release_active_cores(application *app){

    long i;
    long local_server, local_core;

    for(i = 0; i < app->size_tasks; i++){
        local_server = app->cores_active[i] / server_cores;
        local_core = app->cores_active[i] % server_cores;
        sched_info->servers[local_server].cores[local_core] = -1;
        sched_info->servers[local_server].free_cores++;
        sched_info->servers[local_server].busy_cores--;
        if(sched_info->servers[local_server].cont == 1){
            sched_info->servers[local_server].cont = 0;
        }
    }
    metrics.scheduling.utilization += ((sched_info->busy_cores * (sched_info->makespan - sched_info->last_makespan)) / sched_info->total_cores);
    sched_info->last_makespan = sched_info->makespan;
    sched_info->busy_cores -= app->size_tasks;
    sched_info->free_cores += app->size_tasks;
    free(app->cores_active);
}

void release_inactive_cores(application *app){

    long i;
    long local_server, local_core;

    if(app->n_cores_inactive > 0){
        for(i = 0; i < app->n_cores_inactive; i++){
            local_server = app->cores_inactive[i] / server_cores;
            local_core = app->cores_inactive[i] % server_cores;
            sched_info->servers[local_server].cores[local_core] = -1;
            sched_info->servers[local_server].free_cores++;
            sched_info->servers[local_server].busy_cores--;
        }

        metrics.scheduling.utilization_if += ((sched_info->inactive_cores * (sched_info->makespan - sched_info->last_makespan)) / sched_info->total_cores);
        sched_info->busy_cores -= app->n_cores_inactive;
        sched_info->free_cores += app->n_cores_inactive;
        sched_info->inactive_cores -= app->n_cores_inactive;
        free(app->cores_inactive);
    }
}


