#include "applications.h"
#include "mapping.h"
#include "workloads.h"
#include "dynamic_engine.h"
#include "list.h"
#include "metrics.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "float.h"
#include <math.h>
#include <time.h>

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

list_t workload;
list_t list_cpus;
list_t list_flows;
long dmetrics_step;
float agg_bw;
long steps;
long injected_flows;
long consumed_flows;
float min_speed;
float avg_link_bandwidth;
long links;
long total_links;
float total_link_bw;
traffic_priority_t  traffic_priority;
int traffic_priority_nparams;
int *traffic_priority_params;

/**
 * Performs a dynamic run in which flows are placed following the workload description (message causality and cpu periods).
 * Workloads are described simply by sends, receives and cpu bursts:
 *
 * Send: insert flow(s) into the network.
 * Wait: stall the execution until the required flows are received.
 * CPU: stall the execution for a determined number of time units.
 */
void run_dynamic()
{
    unsigned long long time_next_app;
//	struct timespec s;
//	struct timespec e;
//	long tot;

 //   if(clock_gettime(CLOCK_MODE , &s)!=0)
 //     perror("Error Measuring starting time.");

    agg_bw = 0.0;
    steps = 0;
    dmetrics_step = 1;
    injected_flows = 0;
    consumed_flows = 0;
    min_speed = FLT_MAX;
    init_workload(&workload);
    init_scheduling(servers, switches);
    init_metrics(&metrics);
    list_initialize(&list_cpus, sizeof(event*));
    list_initialize(&list_flows, sizeof(event*));

    while(list_length(&workload) > 0 || list_length(&running_applications) > 0){
        time_next_app = schedule_next_application();
        update_events(time_next_app);
    }
    set_makespan(&metrics, sched_info->makespan);
    finish_scheduling(servers);
    update_metrics(&metrics);
    report_metrics(&metrics);

 //   if(clock_gettime(CLOCK_MODE , &e)!=0)
 //     perror("Error Measuring end time.");

 //   tot=e.tv_sec-s.tv_sec;
 //   printf("Total runtime:  %ld:%02ld:%02ld\n",tot/3600,(tot/60)%60, tot%60);
}

/**
 * Calculates the time in which the first event (CPU, flow sent or appication arrival) will finish or occur.
 * Then updates all events until that time.
 */
void update_events(unsigned long long time_next_app)
{

    //long t_next;
    double t_next;

    t_next = time_next_event(&list_cpus, &list_flows);

    if((time_next_app - sched_info->makespan > 0) && (time_next_app - sched_info->makespan < t_next)){
        t_next = time_next_app - sched_info->makespan;
    }

    if(dmetrics_time > 0){
        if((sched_info->makespan + t_next) / dmetrics_time >= dmetrics_step ){
            t_next = (dmetrics_time * dmetrics_step) - sched_info->makespan;
            dmetrics_step++;
        }
    }

    sched_info->makespan += t_next;

    if(verbose == 2){
        printf("Makespan: %f Active flows: %ld Injected flows: %ld Consumed_flows: %ld Links: %ld/%ld Avg. speed: %f Min speed: %f \n",sched_info->makespan, list_length(&list_flows), injected_flows, consumed_flows, links, total_links, avg_link_bandwidth, min_speed);
    }

    update_cpus(&list_cpus, t_next);
    update_flows(&list_flows, t_next);
}

/**
 * Update the remaining time of the CPU events using the previously calculated time.
 * Checks if a task has finished and, when all tasks have finished, finish the application.
 */
//void update_cpus(list_t *list_cpus, long t_next){
void update_cpus(list_t *list_cpus, double t_next){

    event **ev = NULL;
    long num = 0;
    application *app;
    long pid;

    list_reset(list_cpus);
    while(list_next(list_cpus, (void*)&ev)){
        app = (*ev)->app;
        pid = (*ev)->pid;

        (*ev)->count -= t_next;
        if((*ev)->count <= 0){
            app->remaining_cpus[pid]--;
            list_rem_head((*ev)->app->task_events_occurred[(*ev)->pid]);
            num = insert_new_events(app, pid);
            if(num == 0 && app->remaining_flows[pid] == 0){
                app->tasks_finished++;
                if(app->tasks_finished == app->size){
                    finish_application(app);
                }
            }
            list_rem_elem(list_cpus);
        }
    }
}

/**
 * Update the remaining time of the SEND events using the previously calculated time.
 * Checks if a task has finished and, when all tasks have finished, finish the application.
 */
//void update_flows(list_t *list_flows, long t_next){

void update_flows(list_t *list_flows, double t_next){

    event **ev = NULL;
    application *app;
    int pid, pid2, rem, src, dst;
    long num_ev_send, num_ev_rec;

    list_reset(list_flows);
    while(list_next(list_flows, (void*)&ev)){
        //printf("%f %f %f %ld %d\n", (*ev)->dflow.speed, t_next, ((*ev)->dflow.speed * t_next), (*ev)->count, (*ev)->dflow.allocated);
        if((*ev)->dflow.allocated)
        (*ev)->count -= ((*ev)->dflow.speed * t_next);
        //printf("%f %f %f %ld %d\n", (*ev)->dflow.speed, t_next, ((*ev)->dflow.speed * t_next), (*ev)->count, (*ev)->dflow.allocated);
        list_reset((*ev)->dflow.path);
        //(*ev)->dflow.speed = FLT_MAX;
        if((*ev)->count <= 0){
            app = (*ev)->app;
            pid = (*ev)->pid;
            pid2 = (*ev)->pid2;
            src = do_translation(app, (*ev)->pid, (*ev)->type_flow);
            dst = do_translation(app, (*ev)->pid2, (*ev)->type_flow);
            update_flows_latency(&(app->info), (sched_info->makespan - (*ev)->dflow.start_time), (*ev)->type_flow);
            remove_flow(&(*ev)->dflow, app->info.id, src, dst);
            rem = remove_reception_event((*ev)->app->task_events[pid2], (*ev)->id, pid2, pid, (*ev)->total_subflows);
            remove_send_event(app->task_events_occurred[pid], (*ev)->id, pid, pid2, (*ev)->subflows_aux);
            if(rem){
                consumed_flows++;
                app->remaining_flows[pid]--;
                num_ev_rec = insert_new_events(app, pid2);
                num_ev_send = insert_new_events(app, pid);
                if( num_ev_send == 0 || num_ev_rec == 0 ){
                    if (num_ev_send == 0 && app->remaining_flows[pid] == 0 && app->remaining_cpus[pid] == 0){
                        app->tasks_finished++;
                    }
                    if(num_ev_rec == 0 && app->remaining_flows[pid2] == 0 && (pid != pid2) && app->remaining_cpus[pid2] == 0){
                        app->tasks_finished++;
                    }
                    if(app->tasks_finished == app->size){
                        finish_application(app);
                    }
                }
            }
            list_rem_elem(list_flows);
        }
    }
}

long max_route_dynamic(long src, long dst)
{
    long current = src;
    long next_port;
    long current_aux;
    long path_n_apps = 0;

    while(current != dst) {	// not at destination yet
        next_port = route(current, dst);
        current_aux = current;
        current = network[current].port[next_port].neighbour.node;
        if(network[current].port[next_port].link_info_apps[0] > path_n_apps && current_aux != src && current != dst){
            path_n_apps = network[current].port[next_port].link_info_apps[0];
            //break;

        }
    }
    return(path_n_apps);
}

long remove_send_event(list_t *events, long id, long pid, long pid2, long aux){

    long finish = 0;
    event *ev = NULL;

    list_reset(events);
    while(list_next(events, (void*)&ev)){
        if(ev->id == id && ev->type == SENDING && ev->pid == pid && ev->pid2 == pid2 && ev->subflows_aux == aux){
            finish = 1;
            list_rem_elem(events);
            break;
        }
    }
    return(finish);
}

long remove_reception_event(list_t *events, long id, long pid, long pid2, long total_subflows){

    long finish = 0;
    event *ev = NULL;

    list_reset(events);
    while(list_next(events, (void*)&ev)){
        if(ev->id == id && ev->type == RECEPTION && ev->pid == pid && ev->pid2 == pid2){
            ev->subflows_aux++;
            if(ev->subflows_aux == total_subflows){
                finish = 1;
                list_rem_elem(events);
                break;
            }
            else{
                break;
            }
        }
    }
    return(finish);
}

void san_links_bandwidth(){

    long i;

    for(i = 0; i < n_io_servers; i++){
        if(sched_info->san->san_links[i].n_flows_read > 0){
            sched_info->san->san_links[i].current_speed_read = sched_info->san->bandwidth_read / sched_info->san->san_links[i].n_flows_read;
            if(sched_info->san->san_links[i].current_speed_read < min_speed)
                min_speed = sched_info->san->san_links[i].current_speed_read;
            avg_link_bandwidth += sched_info->san->san_links[i].current_speed_read;
            links++;
            total_links++;
        }
        if(sched_info->san->san_links[i].n_flows_write > 0){
            sched_info->san->san_links[i].current_speed_write = sched_info->san->bandwidth_write / sched_info->san->san_links[i].n_flows_write;
            if(sched_info->san->san_links[i].current_speed_write < min_speed)
                min_speed = sched_info->san->san_links[i].current_speed_write;
            avg_link_bandwidth += sched_info->san->san_links[i].current_speed_write;
            links++;
            total_links++;
        }
    }

}

double round_ns(double x) {

    double ret = x;

    if(x < 0.000000001){
        ret = 0.000000001;
    }

    return(ret);
}
