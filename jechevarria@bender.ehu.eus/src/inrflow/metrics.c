#include "metrics.h"
#include "list.h"
#include "globals.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <libgen.h>

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

char output_dir[200];

void init_metrics(struct metrics_t *metrics){

    //Scheduling metrics
    metrics->scheduling.makespan = 0.0;
    metrics->scheduling.avg_waiting_time = 0.0;
    metrics->scheduling.avg_total_time = 0.0;
    metrics->scheduling.utilization = 0.0;
    metrics->scheduling.utilization_if = 0.0;

    //Applications metrics
    metrics->applications.n_apps = 0;
    metrics->applications.avg_size_tasks = 0.0;
    metrics->applications.avg_size_storage = 0.0;
    metrics->applications.avg_num_servers = 0.0;
    list_initialize(&metrics->applications.apps, sizeof(struct app_metrics));

    //Execution metrics
    metrics->execution.avg_runtime = 0.0;
    metrics->execution.avg_agg_bw = 0.0;
    metrics->execution.n_steps = 0;
    metrics->execution.links_info = NULL;
    list_initialize(&metrics->execution.agg_bw, sizeof(float));
}

void init_metrics_application(struct app_metrics *info){

    info->arrive_time = 0;
    info->start_time = 0;
    info->end_time = 0;
    info->runtime = 0;
    info->runtime_stg = 0;
    info->n_flows = 0;
    info->n_flows_comms = 0;
    info->n_flows_storage = 0;
    info->n_subflows = 0;
    info->n_subflows_comms = 0;
    info->n_subflows_storage = 0;
    info->avg_flows_distance = 0;
    info->avg_flows_distance_comms = 0;
    info->avg_flows_distance_storage = 0;
    info->max_flows_distance = 0;
    info->max_flows_distance_comms = 0;
    info->max_flows_distance_storage = 0;
    info->min_flows_distance = LONG_MAX;
    info->min_flows_distance_comms = LONG_MAX;
    info->min_flows_distance_storage = LONG_MAX;
    info->flows_distance = 0;
    info->flows_distance_comms = 0;
    info->flows_distance_storage = 0;
    info->flows_latency = 0.0;
    info->flows_latency_comms = 0.0;
    info->flows_latency_storage = 0.0;
    info->num_links_used = 0;
    info->id = 1;
}

void set_makespan(struct metrics_t *metrics, double makespan){
    metrics->scheduling.makespan = makespan;
}

double get_makespan(struct metrics_t *metrics){
    return( metrics->scheduling.makespan);
}

void report_metrics(struct metrics_t *metrics){

    long i, j;
    FILE *fd_sched, *fd_applications, *fd_execution, *fd_list_applications, *fd_topology;
    char sched_filename[300];
    char applications_filename[300];
    char list_applications_filename[300];
    char execution_filename[300];
    char topology_filename[300];
    app_metrics *a_m = NULL;
    float *agg_bw = NULL;
    char *workload_filename;

    workload_filename = basename(applications_file);
    snprintf(sched_filename,300,"%s/%s_%s_%s_fr%.2f_injmode%d_wl-%s_seed%ld_lb%d_mode%d.scheduling", output_dir, get_network_token(), get_filename_params(), get_routing_token(), failure_rate, flow_inj_mode, workload_filename, r_seed, load_balancing, mode);
    snprintf(applications_filename,300,"%s/%s_%s_%s_fr%.2f_injmode%d_wl-%s_seed%ld_lb%d_mode%d.applications", output_dir, get_network_token(), get_filename_params(), get_routing_token(), failure_rate, flow_inj_mode, workload_filename, r_seed, load_balancing, mode);
    snprintf(list_applications_filename,300,"%s/%s_%s_%s_fr%.2f_injmode%d_wl-%s_seed%ld_lb%d_mode%d.list_applications", output_dir, get_network_token(), get_filename_params(), get_routing_token(), failure_rate, flow_inj_mode, workload_filename, r_seed, load_balancing, mode);
    snprintf(execution_filename,300,"%s/%s_%s_%s_fr%.2f_injmode%d_wl-%s_seed%ld_lb%d_mode%d.execution", output_dir, get_network_token(), get_filename_params(), get_routing_token(), failure_rate, flow_inj_mode, workload_filename, r_seed, load_balancing, mode);
    snprintf(topology_filename,300,"%s/%s_%s_%s_fr%.2f_injmode%d_wl-%s_seed%ld_lb%d_mode%d.topology", output_dir,get_network_token(), get_filename_params(), get_routing_token(), failure_rate, flow_inj_mode, workload_filename, r_seed, load_balancing, mode);

    //free(workload_filename);
    if((fd_sched = fopen(sched_filename, "w")) == NULL){
        printf("Error opening the scheduling file %s.\n", sched_filename);
        exit(-1);
    }
    fprintf(fd_sched,"Makespan:             %f\n", metrics->scheduling.makespan);
    fprintf(fd_sched,"Average waiting time: %.5f\n", metrics->scheduling.avg_waiting_time / (double)metrics->applications.n_apps);
    fprintf(fd_sched,"Average total time:   %.5f\n", metrics->scheduling.avg_total_time / (double)metrics->applications.n_apps);
    fprintf(fd_sched,"Utilization:          %.5f\n", metrics->scheduling.utilization / metrics->scheduling.makespan);
    fprintf(fd_sched,"Utilization-if:       %.5f\n", metrics->scheduling.utilization_if / metrics->scheduling.makespan);
    fclose(fd_sched);

    if((fd_applications = fopen(applications_filename, "w")) == NULL){
        printf("Error opening the applications file.\n");
        exit(-1);
    }
    fprintf(fd_applications,"Number of applications:    %ld\n", metrics->applications.n_apps);
    fprintf(fd_applications,"Average tasks size:              %.5f\n", (metrics->applications.avg_size_tasks / (float)metrics->applications.n_apps));
    fprintf(fd_applications,"Average storage size:              %.5f\n", (metrics->applications.avg_size_storage / (float)metrics->applications.n_apps));
    fprintf(fd_applications,"Average number of servers: %.5f\n", (metrics->applications.avg_num_servers / (float)metrics->applications.n_apps));
    fclose(fd_applications);

    if((fd_list_applications = fopen(list_applications_filename, "w")) == NULL){
        printf("Error opening the list_applications file.\n");
        exit(-1);
    }
    for(i =0; i < metrics->applications.n_apps;i++){
        list_head(&metrics->applications.apps, (void*)&a_m);

        if(a_m->n_flows_storage == 0){
            a_m->flows_distance_storage = 0;
            a_m->max_flows_distance_storage = 0.0;
            a_m->avg_flows_distance_storage = 0.0;
            a_m->min_flows_distance_storage = 0.0;
            a_m->flows_latency_storage = 0;
            a_m->avg_flows_latency_storage = 0.0;
        }
        fprintf(fd_list_applications,"Id:                                             %ld\n", a_m->id);
        fprintf(fd_list_applications,"Number of cores:                                %ld\n", a_m->size_tasks);
        fprintf(fd_list_applications,"Number of servers:                              %ld\n", a_m->num_servers);
        fprintf(fd_list_applications,"Number of storage servers:                      %ld\n", a_m->size_storage);
        fprintf(fd_list_applications,"Arrive time:                                    %llu\n", a_m->arrive_time);
        fprintf(fd_list_applications,"Start time:                                     %llu\n", a_m->start_time);
        fprintf(fd_list_applications,"Finish time:                                    %llu\n", a_m->end_time);
        fprintf(fd_list_applications,"Runtime:                                        %llu\n", a_m->runtime);
        fprintf(fd_list_applications,"Runtime_stg:                                    %llu\n", a_m->runtime_stg);
        fprintf(fd_list_applications,"Waiting time:                                   %llu\n", a_m->waiting_time);
        fprintf(fd_list_applications,"Total time:                                     %llu\n", a_m->waiting_time + a_m->runtime);
        fprintf(fd_list_applications,"Number of flows:                                %ld\n", a_m->n_flows);
        fprintf(fd_list_applications,"Number of comms flows:                          %ld\n", a_m->n_flows_comms);
        fprintf(fd_list_applications,"Number of storage flows:                        %ld\n", a_m->n_flows_storage);
        fprintf(fd_list_applications,"Number of subflows:                             %ld\n", a_m->n_subflows);
        fprintf(fd_list_applications,"Number of comms subflows:                       %ld\n", a_m->n_subflows_comms);
        fprintf(fd_list_applications,"Number of storage subflows:                     %ld\n", a_m->n_subflows_storage);
        fprintf(fd_list_applications,"Total distance of flows:                        %ld\n", a_m->flows_distance);
        fprintf(fd_list_applications,"Total distance of comms flows:                  %ld\n", a_m->flows_distance_comms);
        fprintf(fd_list_applications,"Total distance of storage flows:                %ld\n", a_m->flows_distance_storage);
        fprintf(fd_list_applications,"Max distance of flows:                          %.5f\n", a_m->max_flows_distance);
        fprintf(fd_list_applications,"Max distance of comms flows:                    %.5f\n", a_m->max_flows_distance_comms);
        fprintf(fd_list_applications,"Max distance of storage flows:                  %.5f\n", a_m->max_flows_distance_storage);
        fprintf(fd_list_applications,"Average distance of flows:                      %.5f\n", a_m->avg_flows_distance);
        fprintf(fd_list_applications,"Average distance of comms flows:                %.5f\n", a_m->avg_flows_distance_comms);
        fprintf(fd_list_applications,"Average distance of storage flows:              %.5f\n", a_m->avg_flows_distance_storage);
        fprintf(fd_list_applications,"Flows min distance:                             %.5f\n", a_m->min_flows_distance);
        fprintf(fd_list_applications,"Comms flows min distance:                       %.5f\n", a_m->min_flows_distance_comms);
        fprintf(fd_list_applications,"Storage flows min distance:                     %.5f\n", a_m->min_flows_distance_storage);
        fprintf(fd_list_applications,"Total flows latency:                            %ld\n", a_m->flows_latency);
        fprintf(fd_list_applications,"Total comms flows latency:                      %ld\n", a_m->flows_latency_comms);
        fprintf(fd_list_applications,"Total storage flows latency:                    %ld\n", a_m->flows_latency_storage);
        fprintf(fd_list_applications,"Average flows latency:                          %.5f\n", a_m->avg_flows_latency);
        fprintf(fd_list_applications,"Average comms flows latency:                    %.5f\n", a_m->avg_flows_latency_comms);
        fprintf(fd_list_applications,"Average storage flows latency:                  %.5f\n", a_m->avg_flows_latency_storage);
        fprintf(fd_list_applications,"Num links used:                                 %ld\n", a_m->num_links_used);
        fprintf(fd_list_applications,"\n-----------------------------------------\n");
        for(j = 0; j < get_servers() + get_switches();j++){
            free(a_m->links_utilization[j]);
        }
        free(a_m->links_utilization);
        list_rem_head(&metrics->applications.apps);
    }
    fclose(fd_list_applications);

    if((fd_execution = fopen(execution_filename, "w")) == NULL){
        printf("Error opening the execution file.\n");
        exit(-1);
    }
    fprintf(fd_execution,"Average runtime:              %.5f\n", metrics->execution.avg_runtime / (double)metrics->applications.n_apps);
    fprintf(fd_execution,"Average latency:              %.5f\n", metrics->execution.avg_latency);
    fprintf(fd_execution,"Aggregated bandwidth:         %.5f\n", metrics->execution.avg_agg_bw);
    fprintf(fd_execution,"Number of steps:              %ld\n", metrics->execution.n_steps);
    fprintf(fd_execution,"Number of links shared:       ");
    for(i = 0; i < metrics->applications.n_apps + 1; i++){
        fprintf(fd_execution, "%ld ", metrics->execution.links_info[i]);
    }
    fprintf(fd_execution,"\n");
    free(metrics->execution.links_info);
    fprintf(fd_execution,"Dynamics aggregated bandwith: ");
    while(list_length(&metrics->execution.agg_bw) > 0){
        list_head(&metrics->execution.agg_bw, (void*)&agg_bw);
        fprintf(fd_execution, "%.5f ", *agg_bw);
        list_rem_head(&metrics->execution.agg_bw);
    }
    fclose(fd_execution);

    if((fd_topology = fopen(topology_filename, "w")) == NULL){
        printf("Error opening the topology file.\n");
        exit(-1);
    }
    fprintf(fd_topology,"Topology:                      %s\n", get_network_token());

    fclose(fd_topology);

}

void update_metrics(struct metrics_t *metrics){

    long i,j;

    metrics->execution.links_info = malloc((metrics->applications.n_apps + 1) * sizeof(long));
    for(i = 0; i < metrics->applications.n_apps + 1; i++){
        metrics->execution.links_info[i] = 0;
    }
    for(i=0; i<servers+switches; i++) {
        network[i].nports=get_radix(i);
        for(j=0; j<network[i].nports; j++) {
            metrics->execution.links_info[network[i].port[j].link_info[0]]++;

        }
    }
}


void update_flows_distance(struct app_metrics *info, long path_length, int type){

    info->flows_distance += path_length;
    if(path_length > info->max_flows_distance)
        info->max_flows_distance = path_length;
    if(path_length < info->min_flows_distance)
        info->min_flows_distance = path_length;

    if(type == 0){
        info->flows_distance_comms += path_length;
        if(path_length > info->max_flows_distance_comms)
            info->max_flows_distance_comms = path_length;
        if(path_length < info->min_flows_distance_comms)
            info->min_flows_distance_comms = path_length;

    }
    else{
        info->flows_distance_storage += path_length;
        if(path_length > info->max_flows_distance_storage)
            info->max_flows_distance_storage = path_length;
        if(path_length < info->min_flows_distance_storage)
            info->min_flows_distance_storage = path_length;
    }
}

void update_flows_number(struct app_metrics *info, int type){

    info->n_flows++;

    if(type == 0){
        info->n_flows_comms++;
    }
    else{
        info->n_flows_storage++;
    }
}

void update_subflows_number(struct app_metrics *info, long number, int type){

    info->n_subflows += number;

    if(type == 0){
        info->n_subflows_comms += number;
    }
    else{
        info->n_subflows_storage += number;
    }
}

void update_flows_latency(struct app_metrics *info, float latency, int type){

    info->flows_latency += latency;

    if(type == 0){
        info->flows_latency_comms += latency;
    }
    else{
        info->flows_latency_storage += latency;
    }
}


void set_runtime_stg(struct app_metrics *info, unsigned long long runtime_stg){

    info->runtime_stg = runtime_stg;

}
