#include "applications.h"
#include "dynamic_engine.h"
#include "mapping.h"
#include "allocation.h"
#include "gen_trace.h"
#include "list.h"
#include "metrics.h"
#include "globals.h"
#include "workloads.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


metrics_t metrics;
workload_t auto_wl; ///< parameters of the workload

void init_workload(list_t *workload){

    list_initialize(workload, sizeof(application));

    switch(applications_type){
        case NONE_APP:
            break;
        case FILE_APP:
            read_applications_from_file(applications_file);
            break;
        case AUTO_APP:
            generate_automatic_application(applications_file);
            break;
        default:
            printf("There is no policy to generate applications.\n");
            exit(-1);
    }
}

void read_applications_from_file(char *file){

    FILE *fd;
    long i;
    char pattern[50];
    char mem_storage_access[50];
    char stg_nodes_access[50];
    char *pattern_aux;
    char *mem_storage_access_aux;
    char *stg_nodes_access_aux;
    long servers = get_servers();
    application app;

    init_metrics_application(&(app.info));
    app.packets = 1024000; // 100 MB
    app.comp_time = 10;
    app.phases = 1;
    app.running = 0;
    app.num_servers = 0;
    app.tasks_finished = 0;
    app.info.id = 1;

    if((fd = fopen(file, "r")) == NULL){
        printf("Error opening the workload file.\n");  
        exit(-1);
    }

    while(!feof(fd)){  
        if(fscanf(fd,"%llu %s %ld %ld %s %s %s %s %s %s\n",&app.info.arrive_time, pattern, &app.size_tasks, &app.size_storage, app.allocation_type, app.mapping_type, app.storage_type, mem_storage_access, stg_nodes_access,app.data_access_mode_type) < 9){
            printf("Format of workload file is not correct.\n");
            exit(-1);
        } 
        if(app.size_tasks > servers || app.size_storage > servers){
            printf("WARNING: Application %ld with size %ld and storage %ld is larger than the network: %ld.\n",app.info.id,app.size_tasks, app.size_storage,  servers);
            exit(-1);
        } 
        app.size = app.size_tasks + app.size_storage;
        pattern_aux = strtok(pattern, "_");
        if(!literal_value(tpatterns_l, pattern_aux, (int *) &app.pattern)) {
            printf("Error: Unknown traffic pattern - %s\n", pattern_aux);
            exit(-1);
        }
        app.pattern_nparam=0;
        if(app.pattern == FILE_PATTERN){
            pattern_aux = strtok(NULL, "_");
            sscanf(pattern_aux, "%s", app.pattern_file);
        }

        while ( (pattern_aux = strtok(NULL, "_")) && app.pattern_nparam < MAX_TRAFFIC_PARAMS){
            app.pattern_params[app.pattern_nparam++] = atol(pattern_aux);
        }

        for (i = app.pattern_nparam; i < MAX_TRAFFIC_PARAMS; i++)
            app.pattern_params[i] = -1; // undefine values, should not be checking these anyway.

        if(!literal_value(allocation_l, app.allocation_type, (int *) &app.allocation)) {
            printf("Error: Unknown allocation strategy - %s\n", app.allocation_type);
            exit(-1);
        }
        if(!literal_value(mapping_l, app.mapping_type, (int *) &app.mapping)) {
            printf("Error: Unknown mapping strategy - %s\n", app.mapping_type);
            exit(-1);
        }
        if(!literal_value(storage_l, app.storage_type, (int *) &app.storage)) {
            printf("Error: Unknown storage strategy - %s\n", app.storage_type);
            exit(-1);
        }
        mem_storage_access_aux = strtok(mem_storage_access, "_");
        if(!literal_value(memory_storage_access_l, mem_storage_access_aux, (int *) &app.mem_stg_access)) {
            printf("Error: Unknown memory storage access strategy - %s\n", mem_storage_access_aux);
            exit(-1);
        }
        strncpy(app.mem_storage_access_type, mem_storage_access_aux, 50 * sizeof(char));
        mem_storage_access_aux = strtok(NULL, "_");
        app.mem_storage_access_read = atol(mem_storage_access_aux);
        mem_storage_access_aux = strtok(NULL, "_");
        app.mem_storage_access_write = atol(mem_storage_access_aux);

        stg_nodes_access_aux = strtok(stg_nodes_access, "_");
        if(!literal_value(stg_nodes_access_l, stg_nodes_access_aux, (int *) &app.stg_nodes_access_mode)) {
            printf("Error: Unknown storage nodes access strategy - %s\n", stg_nodes_access_aux);
            exit(-1);
        }
        strncpy(app.stg_nodes_access_mode_type, stg_nodes_access_aux, 50 * sizeof(char));
        stg_nodes_access_aux = strtok(NULL, "_");
        app.stg_nodes_access = atol(stg_nodes_access_aux);
     
        if(!literal_value(data_access_mode_l, app.data_access_mode_type, (int *) &app.data_access_mode)) {
                printf("Error: Unknown data access mode - %s.\n", app.data_access_mode_type);
                exit(-1);
            }
 
        app.phases = app.pattern_params[0];
        list_append(&workload, &app);
        app.info.id++;
    }  
    fclose(fd);
}

void generate_automatic_application(){

    long n_apps = 0;
    long servers = get_servers();
    application app;

    init_metrics_application(&(app.info));
    app.packets = 8192000;
    app.comp_time = 10;
    app.running = 0;
    app.num_servers = 0;
    app.tasks_finished = 0;

    if(auto_wl.max_size > (servers * server_cores)){
        printf("WARNING: Max application size %ld is larger than the network: %ld.\n",auto_wl.max_size, servers);
        exit(-1);
    } 

    while(n_apps < auto_wl.n_apps){  
        app.info.arrive_time = next_arrival_time(app.info.arrive_time); 
        app.pattern = auto_wl.pattern;
        app.pattern_nparam = 0;
        if(app.pattern == FILE_PATTERN){
            sscanf(auto_wl.pattern_file, "%s", app.pattern_file);
            app.pattern_nparam = 1;
        }
        else{
            while ( app.pattern_nparam < 3){
                app.pattern_params[app.pattern_nparam] = auto_wl.pattern_params[app.pattern_nparam];
                app.pattern_nparam++;
            }
            app.phases = app.pattern_params[0];
        }
        app.size = generate_application_size(auto_wl.min_size, auto_wl.max_size);
        app.allocation = auto_wl.allocation;
        app.mapping = auto_wl.mapping;
        app.phases = app.pattern_params[0];
        list_append(&workload, &app);
        app.info.id++;
        n_apps++;
    }  
}

long next_arrival_time(long previous){

    long next;

    switch(auto_wl.arrival_time){

        case INSTANTANEOUS_ARRIVAL:
            next = previous + 1;    
            break;
        case POISSON_ARRIVAL:
            //TBD
            printf("Poisson arrival TBD.\n");
            exit(0);
            break;
        default:
            printf("Unknown arrival time generation mode.\n");
            exit(0);
    }
    return(next);
}

long generate_application_size(long min, long max){

    long size;
    if( (min == 0) || (max < min)){
        printf("Min application size must greater than 0 and max greater than min.\n");
        exit(0);
    }
    if(max - min == 0){
        size = min;
    }
    else{
        size = min + rand() % (max - min);
    }
    return(size);
}



