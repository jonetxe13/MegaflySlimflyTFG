#include "applications.h"
#include "dynamic_engine.h"
#include "mapping.h"
#include "allocation.h"
#include "gen_trace.h"
#include "list.h"
#include "metrics.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>

list_t running_applications;

applications_t applications_type;

char applications_file[200];

void init_running_application(application *next_app){

    long i;
    long n_events;
    node_list *node;

    next_app->seed = next_app->pattern_params[5];
    next_app->task_events = malloc(sizeof(list_t*) * next_app->size);
    next_app->task_events_occurred = malloc(sizeof(list_t*) * next_app->size);
    next_app->info.links_utilization = calloc(get_servers() + get_switches(), sizeof(int*));
    for(i = 0;i < next_app->size; i++){
        next_app->task_events[i] = malloc(sizeof(list_t));
        next_app->task_events_occurred[i] = malloc(sizeof(list_t));
        list_initialize(next_app->task_events[i], sizeof(event));
        list_initialize(next_app->task_events_occurred[i], sizeof(event));
    }
    gen_trace(next_app);
    next_app->remaining_flows = malloc(sizeof(long) * next_app->size);
    next_app->remaining_cpus = malloc(sizeof(long) * next_app->size);
    for(i = 0;i < next_app->size; i++){
        next_app->remaining_flows[i] = 0;
        next_app->remaining_cpus[i] = 0;
        n_events = insert_new_events(next_app, i);
        if(n_events == 0){
            next_app->tasks_finished++;
        }
    }
    list_head_node(&workload, (void*)&node);
    next_app->node = node;
    next_app->info.start_time = sched_info->makespan;
    next_app->info.size_tasks = next_app->size_tasks;
    metrics.applications.avg_size_tasks += next_app->size_tasks;
    next_app->info.size_storage = next_app->size_storage;
    next_app->finished_storage_flows = next_app->size_storage;
    metrics.applications.avg_size_storage += next_app->size_storage;
    next_app->info.num_servers = next_app->num_servers;
    metrics.applications.avg_num_servers += next_app->num_servers;
    next_app->info.waiting_time =  (next_app->info.start_time -  next_app->info.arrive_time);
    metrics.scheduling.avg_total_time +=  (next_app->info.start_time -  next_app->info.arrive_time);
    metrics.scheduling.avg_waiting_time +=  (next_app->info.start_time -  next_app->info.arrive_time);
    list_rem_head_append(&workload, &running_applications);
    sched_info->running_apps[next_app->info.id] = 1;

	if(verbose >= 1){
		if (next_app->pattern==FILE_PATTERN){ // if this application is a trace print the trace name
			printf("Application %ld (from file %s) starts at makespan %f\n",next_app->info.id, next_app->pattern_file, sched_info->makespan);
		} else { // if it is a kernel print the name of the kernel
			char * app_name;
			literal_name(tpatterns_l, &app_name, next_app->pattern);
			printf("Application %ld (%s) starts at makespan %f\n",next_app->info.id, app_name, sched_info->makespan);
		}
    }
}

void finish_application(application *app){

    long i;
    if(verbose >= 1){
		if (app->pattern==FILE_PATTERN){ // if this application is a trace print the trace name
			printf("Application %ld (from file %s) finishes at makespan %f\n",app->info.id, app->pattern_file, sched_info->makespan);
		} else { // if it is a kernel print the name of the kernel
			char * app_name;
			literal_name(tpatterns_l, &app_name, app->pattern);
			printf("Application %ld (%s) finishes at makespan %f\n",app->info.id, app_name, sched_info->makespan);
		}
    }
    sched_info->running_apps[app->info.id] = 0;
    metrics.applications.n_apps++;
    app->info.end_time = sched_info->makespan;
    app->info.runtime = (app->info.end_time - app->info.start_time);
    metrics.scheduling.avg_total_time += (app->info.end_time - app->info.start_time);
    metrics.execution.avg_runtime += (app->info.end_time - app->info.start_time);
    app->info.avg_flows_distance = ((float)app->info.flows_distance / (float)app->info.n_subflows);
    app->info.avg_flows_distance_comms = ((float)app->info.flows_distance_comms / (float)app->info.n_subflows_comms);
    app->info.avg_flows_distance_storage = ((float)app->info.flows_distance_storage / (float)app->info.n_subflows_storage);
    app->info.avg_flows_latency = ((float)app->info.flows_latency / (float)app->info.n_subflows);
    app->info.avg_flows_latency_comms = ((float)app->info.flows_latency_comms / (float)app->info.n_subflows_comms);
    app->info.avg_flows_latency_storage = ((float)app->info.flows_latency_storage / (float)app->info.n_subflows_storage);
    list_append(&metrics.applications.apps, &app->info);
    for(i = 0; i < app->size; i++){
        list_destroy(app->task_events[i]);
        list_destroy(app->task_events_occurred[i]);
        free(app->task_events[i]);
        free(app->task_events_occurred[i]);
    }
    free(app->remaining_flows);
    free(app->remaining_cpus);
    free(app->task_events);
    free(app->task_events_occurred);
    release_mapping(app);
    release_application(app);
    list_rem_node(&running_applications, app->node);
    sched_info->stopped = 0;
}

long is_running(long app_id){

    return(sched_info->running_apps[app_id]);
}



