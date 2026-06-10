#include "applications.h"
#include "mapping.h"
#include "workloads.h"
#include "dynamic_engine.h"
#include "photonic_engine.h"
#include "list.h"
#include "metrics.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "float.h"
#include <math.h>
#include <assert.h>

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

int n_channels;
int n_lambdas;
long channel_bandwidth;
long n_inj;
channel_assignment_policy_t channel_assign_pol;
lambda_assignment_policy_t lambda_assign_pol;
list_t *path;

/**
 * Calculates the time in which the first event (CPU, flow sent) will finish or occur.
 */
//long time_next_event(list_t *list_cpus, list_t *list_flows){
double time_next_event_photonic(list_t *list_cpus, list_t *list_flows){

    long i;
    int src_aux;
    long found;
    //long min = LONG_MAX;
    double min = DBL_MAX;
    //long time_next = 0;
    double time_next = 0.0;
    long path_length = 0;
    application *app = NULL;
    event *ev = NULL;
    event **ev_p = NULL;
    dflow_t *flow = NULL;
    long src = -1;
    long dst = -1;

    path = malloc(sizeof(list_t));
    list_reset(&running_applications);
    while(list_next(&running_applications, (void*)&app)){
        for(i = 0; i < app->size; i++){
            list_reset(app->task_events_occurred[i]);
            while(list_next(app->task_events_occurred[i],(void*)&ev)){
                switch(ev->type){
                    case COMPUTATION:
                        if(ev->count == ev->length){
                            list_append(list_cpus, &ev);
                        }
                        break;
                    case SENDING:
                        if(ev->count == ev->length){
                            ev->dflow.start_time = sched_info->makespan;
                            flow = &ev->dflow;
                            src = do_translation(app, ev->pid, ev->type_flow);
                            dst = do_translation(app, ev->pid2, ev->type_flow);
                            if(src == dst && ev->type_flow == 1){
                                ev->dflow.speed = read_capacity;
                            }
                            else if(src == dst && ev->type_flow == 2){
                                ev->dflow.speed = write_capacity;
                            }
                            else if(src == dst){
                                ev->dflow.speed = memory_capacity;
                            }
                            if(ev->type_flow == 5){
                                src_aux = random() % n_io_servers;
                                sched_info->san->san_links[src_aux].n_flows_read++;
                                ev->dflow.san_link = src_aux;
                            }
                            else if(ev->type_flow == 6){
                                src_aux = random() % n_io_servers;
                                sched_info->san->san_links[src_aux].n_flows_write++;
                                ev->dflow.san_link = src_aux;
                            }
                            found = explore_routes(path,src, dst);
                            if(found == 0){
                                printf("## Not Allocated flow (%ld --> %ld)\n", src, dst);
                                continue;
                            }
                            flow->allocated = 1;
                            path_length = mark_route_dynamic_photonic(app, flow, src, dst, ev->type_flow, path);
                            update_flows_distance(&(app->info), path_length, ev->type_flow);
                            list_append(list_flows, &ev);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
    list_reset(list_cpus);
    while(list_next(list_cpus, (void*)&ev_p)){
        if((*ev_p)->count < min){
            min = (*ev_p)->count;
        }
    }

    san_links_bandwidth();

    min_links_bandwidth();

    list_reset(list_flows);
    while(list_next(list_flows, (void*)&ev_p)){
        if((*ev_p)->type_flow == 5){
            src = (*ev_p)->dflow.san_link;
            //src = do_translation(app, (*ev_p)->pid, (*ev_p)->type_flow);
            if(sched_info->san->san_links[src].current_speed_read > read_capacity){
                (*ev_p)->dflow.speed = read_capacity;
            }
            else{
                (*ev_p)->dflow.speed = sched_info->san->san_links[src].current_speed_read;
            }
        }
        else if((*ev_p)->type_flow == 6){
            src = (*ev_p)->dflow.san_link;
            //src = do_translation(app, (*ev_p)->pid, (*ev_p)->type_flow);
            if(sched_info->san->san_links[src].current_speed_write > write_capacity){
                (*ev_p)->dflow.speed = write_capacity;
            }
            else{
                (*ev_p)->dflow.speed = sched_info->san->san_links[src].current_speed_write;
            }
        }
        time_next = round_ns((*ev_p)->count / (*ev_p)->dflow.speed);
        if(time_next < min){
            min = time_next;
        }
    }
    printf("MIN: %f\n",min);
    free(path);
    return(min);
}

/**
 * Insert new events from a given task.
 */
long insert_new_events_photonic(application *app, long ntask){

    long done = 0;
    long num = 0;
    long src, dst, path, n_paths, length_subflow;

    event *ev;
    list_reset(app->task_events[ntask]);
    while(!done && list_next(app->task_events[ntask], (void*)&ev)){
        switch(ev->type){
            case COMPUTATION:
                done = 1;
                if(num == 0 && list_length(app->task_events_occurred[ntask]) == 0){
                    app->remaining_cpus[ev->pid]++;
                    list_append(app->task_events_occurred[ntask], ev);
                    list_rem_elem(app->task_events[ntask]);
                    num++;
                }
                else if( list_length(app->task_events[ntask]) > 0){
                    num = 1;
                }
                break;
            case SENDING:
                if(flow_inj_mode > app->remaining_flows[ev->pid] || flow_inj_mode == 0){
                    if(ev->type_flow == 6){
                        app->finished_storage_flows--;
                        if(app->finished_storage_flows == 0){
                            set_runtime_stg(&(app->info), (sched_info->makespan - app->info.start_time));
                        }
                    }
                    update_flows_number(&(app->info), ev->type_flow);
                    app->remaining_flows[ev->pid]++;
                    injected_flows++;
                    if(flow_inj_mode == app->remaining_flows[ev->pid])
                        done = 1;
                    if(load_balancing && ev->type_flow !=5 && ev->type_flow !=6){
                        src = do_translation(app, ev->pid, ev->type);
                        dst = do_translation(app, ev->pid2, ev->type);
                        n_paths = get_n_paths_routing(src, dst);
                        length_subflow = (long)ceilf(ev->length / n_paths);
                        ev->length =  length_subflow;
                        ev->count = length_subflow;
                    }
                    else{
                        n_paths = 1;
                    }
                    path = 0;
                    ev->total_subflows = n_paths;
                    update_subflows_number(&(app->info), n_paths, ev->type_flow);
                    while(path < n_paths){
                        ev->subflows_aux = path;
                        list_append(app->task_events_occurred[ntask], ev);
                        path++;
                    }
                    list_rem_elem(app->task_events[ntask]);
                }
                num++;
                break;
            case RECEPTION:
                num++;
                done = 1;
                break;
            default:
                printf("Unknown event type\n.");
                exit(-1);
        }
    }
    return(num);
}

long mark_route_dynamic_photonic(application *app, dflow_t *flow, long src, long dst, int type, list_t *path)
{
    long app_id;
    long apps_running = 0;
    long path_n_apps = 0;
    photonic_path_t *path_hop;


    if(type == 1){
        network[src].flows_storage_read_injected++;
        flow->type_id = src;
    }
    else if(type == 2){
        network[dst].flows_storage_write_injected++;
        flow->type_id = dst;
    }
    else{
        flow->type_id = -1;
    }

    printf("Allocated flow (%ld --> %ld) Channels: ", src, dst);
    flow->type = type;
    //while(current != dst) {	// not at destination yet
    list_reset(path);
    while(list_next(path, (void*)&path_hop)){

        network[path_hop->node_id].port[path_hop->next_port].flows++;
        network[path_hop->node_id].opt_port[path_hop->next_port].channels[path_hop->channel_id].flows++;
        printf("%ld ", path_hop->channel_id);
        flow->speed = network[path_hop->node_id].opt_port[path_hop->next_port].channel_bandwidth;
        if(type == 1){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_read++;
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_read_fault++;
        }
        else if(type == 3){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_read++;
        }
        else if(type == 2){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_write++;
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_write_fault++;
        }
        else if(type == 4){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_write++;
        }
        if(app->info.links_utilization[path_hop->node_id] == NULL){
            app->info.links_utilization[path_hop->node_id] = calloc(get_radix(path_hop->node_id), sizeof(int));
        }
        else if(app->info.links_utilization[path_hop->node_id][path_hop->next_port] == 0){
            app->info.num_links_used++;
        }
        app->info.links_utilization[path_hop->node_id][path_hop->next_port]++;

        if(network[path_hop->node_id].port[path_hop->next_port].link_info[app->info.id] == 0){
            apps_running = 0;
            for(app_id = 1; app_id < MAX_CONCURRENT_APPS + 1;app_id++){
                if((network[path_hop->node_id].port[path_hop->next_port].link_info[app_id] > 0) && is_running(app_id)){
                    apps_running++;
                }
            }
            network[path_hop->node_id].port[path_hop->next_port].link_info[0] = apps_running + 1;
        }
        network[path_hop->node_id].port[path_hop->next_port].link_info[app->info.id]++;


        //printf("iINFO_APPS: %ld (%ld) #%ld, %ld#\n", network[current].port[next_port].link_info_apps[app->info.id],  network[current].port[next_port].link_info_apps[0], current, next_port);

        if(network[path_hop->node_id].port[path_hop->next_port].link_info_apps[app->info.id] == 0){
            network[path_hop->node_id].port[path_hop->next_port].link_info_apps[0] = app->info.id;
        }
        network[path_hop->node_id].port[path_hop->next_port].link_info_apps[app->info.id]++;
        if(network[path_hop->node_id].port[path_hop->next_port].link_info_apps[0] > path_n_apps){
            path_n_apps = network[path_hop->node_id].port[path_hop->next_port].link_info_apps[0];
        }

        //list_append(&network[current].port[path_hop->next_port].dflows, &flow);
        //list_tail_node(&network[current].port[path_hop->next_port].dflows, (void*)&node);
        //path.n_l = node;
        //dflows = &network[current].port[path_hop->next_port].dflows;
        //    calc_insert_flow_max_min(dflows, current, next_port);
        //current_aux = current;
        //current = network[current].port[path_hop->next_port].neighbour.node;

        //if(network[current_aux].port[path_hop->next_port].flows > path_n_flows_max && current_aux != src && current != dst){
        //   path_n_flows_max = network[current_aux].port[path_hop->next_port].flows;
        //}
    }
    printf("\n");
    list_initialize(flow->path, sizeof(photonic_path_t));
    list_concat(flow->path,path);
    return(path->length);
}

long explore_routes(list_t* path, long src, long dst){

    long n_paths;
    long found = 0;
    long current_path = 0;

    list_initialize(path, sizeof(photonic_path_t));
    n_paths = get_n_paths_routing(src, dst);

    while (!found &&  current_path < n_paths){
        found = explore_route(path, src, dst);
        current_path++;
    }
    return(found);
}


long explore_route_static_static(list_t* path, long src, long dst){

    long current;
    long next_port, current_channel;
    long found = 0;
    photonic_path_t path_hop;

    current_channel = 0;
    current = src;
    init_routing(current, dst);
    next_port = route(current, dst);

    while(!found && current_channel < network[current].opt_port[next_port].n_channels){
        while(current != dst){	// not at destination yet
            if(network[current].opt_port[next_port].channels[current_channel].flows != 0){
                list_destroy(path);
                break;
            }
            set_path(&path_hop, current, next_port, current_channel, 0, NULL);
            list_append(path, &path_hop);
            current = network[current].port[next_port].neighbour.node;
            next_port = route(current, dst);
            if(current == dst){
                found = 1;
            }
        }
        current_channel++;
        init_routing(src, dst);
        next_port = route(src, dst);
        current = src;
    }
    //assert(!found && path->head == NULL);
    return(found);
}

long explore_route_adaptive_static(list_t* path, long src, long dst){

    int current_channel;
    long current;
    long next_port;
    long found = 1;
    photonic_path_t path_hop;

    current_channel = 0;
    init_routing(src, dst);
    next_port = route(src, dst);
    current = src;

    while(found && current != dst){	// not at destination yet

        while(current_channel < network[current].opt_port[next_port].n_channels){

            if(network[current].opt_port[next_port].channels[current_channel].flows == 0){
                set_path(&path_hop, current, next_port, current_channel, 0, NULL);
                list_append(path, &path_hop);
                break;
            }
            current_channel++;
        }
        if(current_channel == network[current].opt_port[next_port].n_channels){
            found = 0;
            list_destroy(path);
            break;
        }
        current_channel = 0;
        current = network[current].port[next_port].neighbour.node;
        next_port = route(current, dst);
    }
    return(found);
}

void remove_flow_photonic(dflow_t *flow, long id, long src, long dst){

    photonic_path_t *path_hop;
    //list_t *dflows;
    //long node, port;
    long path_n_apps = 0;

    if(flow->type == 1){
        network[src].flows_storage_read_injected--;
    }
    else if(flow->type == 2){
        network[dst].flows_storage_write_injected--;
    }
    else if(flow->type == 5){
        src = flow->san_link;
        sched_info->san->san_links[src].n_flows_read--;
    }
    else if(flow->type == 6){
        src = flow->san_link;
        sched_info->san->san_links[src].n_flows_write--;
    }
    list_reset(flow->path);
    while(list_next(flow->path, (void*)&path_hop)){
        network[path_hop->node_id].port[path_hop->next_port].flows--;
        network[path_hop->node_id].opt_port[path_hop->next_port].channels[path_hop->channel_id].flows--;
        if(flow->type == 1){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_read--;
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_read_fault--;
        }
        else if(flow->type == 3){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_read--;
        }
        else if(flow->type == 2){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_write--;
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_write_fault--;
        }
        else if(flow->type == 4){
            network[path_hop->node_id].port[path_hop->next_port].flows_storage_write--;
        }
        network[path_hop->node_id].port[path_hop->next_port].link_info_apps[id]--;
        //printf("RF: #%ld,%ld#\n",path->node_id, path->next_port);
        if(network[path_hop->node_id].port[path_hop->next_port].link_info_apps[id] == 0){
            network[path_hop->node_id].port[path_hop->next_port].link_info_apps[0] = 0;
        }
        if(network[path_hop->node_id].port[path_hop->next_port].link_info_apps[0] > path_n_apps){
            path_n_apps = network[path_hop->node_id].port[path_hop->next_port].link_info_apps[0];
        }


        //dflows = &network[path->node_id].port[path->next_port].dflows;
        //node = path->node;
        //port = path->next_port;
        //list_rem_node(&network[path->node_id].port[path->next_port].dflows, path->n_l);
        list_rem_elem(flow->path);
        //calc_remove_flow_max_min(dflows, node, port);
    }
    free(flow->path);
}

void min_links_bandwidth_photonic(){

    long i,j,i_aux, j_aux;
    long bandwidth_read = 0;
    long bandwidth_write = 0;
    float total_link_bw = 0.0;
    float link_bw = 0.0;
    float l_bandwidth = 0;
    float bandwidth = 0;
    list_t *dflows;
    dflow_t **flow;
    min_speed = FLT_MAX;
    avg_link_bandwidth = 0.0;
    links = 0;
    total_links = 0;
    /*
       for(i=0; i<servers; i++){
       if(network[i].flows_storage_read_injected > 0){
       bandwidth_read = read_capacity / network[i].flows_storage_read_injected;
       for(j = 0 ;j<network[i].nports; j++){
       dflows = &network[i].port[j].dflows;
       list_reset(dflows);
       while(list_next(dflows, (void*)&flow)){
       if((*flow)->type_id == i && (*flow)->type == 1){
       (*flow)->speed = bandwidth_read;
       }
       }
       }
       }
       if(network[i].flows_storage_write_injected > 0){
       bandwidth_write = write_capacity / network[i].flows_storage_write_injected;
       for(j = 0 ;j<network[i].nports; j++){
       i_aux = network[i].port[j].neighbour.node;
       for(j_aux = 0; j_aux < network[i_aux].nports; j_aux++){
       if(network[i_aux].port[j_aux].neighbour.node == i){
       dflows = &network[i_aux].port[j_aux].dflows;
       list_reset(dflows);
       while(list_next(dflows, (void*)&flow)){
       if((*flow)->type_id == i  && (*flow)->type == 2){
       (*flow)->speed = bandwidth_write;
       }
       }
       break;
       }
       }
       }
       }
       }
       */
    for(i = 0; i < servers + switches; i++){
        for(j = 0; j < network[i].nports; j++){
            total_links++;
            if(network[i].port[j].neighbour.node == -1 || network[i].port[j].neighbour.port == -1 ||  network[i].port[j].flows == 0)
                continue;
            links++;
            link_bw = 0;
            //l_bandwidth = (float)network[i].port[j].bandwidth_capacity;
            //bandwidth = l_bandwidth / network[i].port[j].flows;
            //dflows = &network[i].port[j].dflows;
            //list_reset(dflows);
            //while(list_next(dflows, (void*)&flow)){
            //    if(bandwidth < (*flow)->speed && (*flow)->type != 5 && (*flow)->type != 6){
            //        (*flow)->speed = bandwidth;
            //    }
            //    if((*flow)->type != 5 && (*flow)->type != 6){
            //        link_bw += (*flow)->speed;
            //    }
            //}
            // if(link_bw < min_speed)
            //     min_speed = link_bw;

            //total_link_bw += (link_bw / (float)l_bandwidth);
            // avg_link_bandwidth += link_bw;
        }
    }
    //avg_link_bandwidth /= links;
    //agg_bw += (((total_link_bw / links) - agg_bw) / (float)(++steps));
    //metrics.execution.avg_agg_bw += ((link_bw - metrics.execution.avg_agg_bw) / (float)(++metrics.execution.n_steps));
    /*
       if((dmetrics_time > 0 && (sched_info->makespan % dmetrics_time) == 0)){
       list_append(&metrics.execution.agg_bw, &agg_bw);
       agg_bw = 0.0;
       steps = 0;
       }
       */

}

void set_path(photonic_path_t *path_hop, long node_id, long next_port, int channel_id, int n_lambdas, int *lambdas){

    path_hop->node_id = node_id;
    path_hop->next_port = next_port;
    path_hop->channel_id = channel_id;
    path_hop->n_lambdas= n_lambdas;
    path_hop->lambdas = lambdas;;
}



