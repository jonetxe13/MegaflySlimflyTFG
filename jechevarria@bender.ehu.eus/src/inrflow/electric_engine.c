#include "applications.h"
#include "mapping.h"
#include "workloads.h"
#include "dynamic_engine.h"
#include "electric_engine.h"
#include "list.h"
#include "metrics.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "float.h"
#include <math.h>

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

/**
 * Calculates the time in which the first event (CPU, flow sent) will finish or occur.
 */
double time_next_event_electric(list_t *list_cpus, list_t *list_flows){

    long i;
    int src_aux;
    double min = DBL_MAX;
    double time_next = 0.0;
    long path_length = 0;
    application *app = NULL;
    event *ev = NULL;
    event **ev_p = NULL;
    dflow_t *flow = NULL;
    long src = -1;
    long dst = -1;
#ifdef DEBUG
    long active_events;
#endif // DEBUG

    list_reset(&running_applications);
    while(list_next(&running_applications, (void*)&app)){
#ifdef DEBUG
    active_events=0;
#endif // DEBUG
        for(i = 0; i < app->size; i++){
            list_reset(app->task_events_occurred[i]);
            while(list_next(app->task_events_occurred[i],(void*)&ev)){
                switch(ev->type){
                    case COMPUTATION:
#ifdef DEBUG
                        active_events++;
#endif // DEBUG
                        if(ev->count == ev->length){
                            list_append(list_cpus, &ev);
                        }
                        break;
                    case SENDING:
#ifdef DEBUG
                        active_events++;
#endif // DEBUG
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
                            path_length = mark_route_dynamic_electric(app, flow, src, dst, ev->type_flow);
                            if(path_length == -1)
                                continue;
                            update_flows_distance(&(app->info), path_length, ev->type_flow);
                            list_append(list_flows, &ev);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
#ifdef DEBUG
        if (active_events==0){
            printf("Application deadlocked!!!\n Simulation Aborted\n");
            exit(-1);
        }
#endif // DEBUG

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
            if(sched_info->san->san_links[src].current_speed_read > read_capacity){
                (*ev_p)->dflow.speed = read_capacity;
            }
            else{
                (*ev_p)->dflow.speed = sched_info->san->san_links[src].current_speed_read;
            }
        }
        else if((*ev_p)->type_flow == 6){
            src = (*ev_p)->dflow.san_link;
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
    return(min);
}

/**
 * Insert new events from a given task.
 */
long insert_new_events_electric(application *app, long ntask){

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

/**
 * Follows the route between servers i & j and update flow counts.
 * Side effects: If a path exists, update link_hops and server_hops
 *
 * @return 1 if there is a path between them, 0 otherwise.
 */
long mark_route_dynamic_electric(application *app, dflow_t *flow, long src, long dst, int type)
{
    long current = src;
    long current_aux = 0;
    long path_length = 0;
    node_list *node;
    long next_port;
    long app_id;
    long apps_running = 0;
    route_t path;
    list_t *dflows;
    long path_number = 0;
    long path_n_flows_max = 0;
    long path_n_apps = 0;

    path_number = init_routing(src, dst, app->info.id);
    if(path_number  == -1){
        return -1; // no injection at this time
    }
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

    flow->type = type;
    list_initialize(flow->path, sizeof(route_t));
    while(current != dst) {	// not at destination yet
        next_port = route(current, dst);

        path_length++;

        path.node = current;
        path.port = next_port;
        network[current].port[next_port].flows++;
        if(type == 1){
            network[current].port[next_port].flows_storage_read++;
            network[current].port[next_port].flows_storage_read_fault++;
        }
        else if(type == 3){
            network[current].port[next_port].flows_storage_read++;
        }
        else if(type == 2){
            network[current].port[next_port].flows_storage_write++;
            network[current].port[next_port].flows_storage_write_fault++;
        }
        else if(type == 4){
            network[current].port[next_port].flows_storage_write++;
        }
        if(app->info.links_utilization[current] == NULL){
            app->info.links_utilization[current] = calloc(get_ports(current), sizeof(int));
        }
        else if(app->info.links_utilization[current][next_port] == 0){
            app->info.num_links_used++;
        }
        app->info.links_utilization[current][next_port]++;

        if(network[current].port[next_port].link_info[app->info.id] == 0){
            apps_running = 0;
            for(app_id = 1; app_id < MAX_CONCURRENT_APPS + 1;app_id++){
                if((network[current].port[next_port].link_info[app_id] > 0) && is_running(app_id)){
                    apps_running++;
                }
            }
            network[current].port[next_port].link_info[0] = apps_running + 1;
        }
        network[current].port[next_port].link_info[app->info.id]++;

        if(network[current].port[next_port].link_info_apps[app->info.id] == 0){
            network[current].port[next_port].link_info_apps[0] = app->info.id;
        }
        network[current].port[next_port].link_info_apps[app->info.id]++;
        if(network[current].port[next_port].link_info_apps[0] > path_n_apps){
            path_n_apps = network[current].port[next_port].link_info_apps[0];
        }

        list_append(&network[current].port[next_port].dflows, &flow);
        list_tail_node(&network[current].port[next_port].dflows, (void*)&node);
        path.n_l = node;
        list_append(flow->path, &path);
        dflows = &network[current].port[next_port].dflows;
        calc_insert_flow_max_min_electric(dflows, current, next_port);
        current_aux = current;
        current = network[current].port[next_port].neighbour.node;

        if(network[current_aux].port[next_port].flows > path_n_flows_max && current_aux != src && current != dst){
            path_n_flows_max = network[current_aux].port[next_port].flows;
        }
    }
    finish_route(path_n_flows_max, path_n_apps, app->info.id);
    return(path_length);
}

void remove_flow_electric(dflow_t *flow, long id, long src, long dst){

    route_t *path;
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
    while(list_next(flow->path, (void*)&path)){

        network[path->node].port[path->port].flows--;
        if(flow->type == 1){
            network[path->node].port[path->port].flows_storage_read--;
            network[path->node].port[path->port].flows_storage_read_fault--;
        }
        else if(flow->type == 3){
            network[path->node].port[path->port].flows_storage_read--;
        }
        else if(flow->type == 2){
            network[path->node].port[path->port].flows_storage_write--;
            network[path->node].port[path->port].flows_storage_write_fault--;
        }
        else if(flow->type == 4){
            network[path->node].port[path->port].flows_storage_write--;
        }
        network[path->node].port[path->port].link_info_apps[id]--;
        if(network[path->node].port[path->port].link_info_apps[id] == 0){
            network[path->node].port[path->port].link_info_apps[0] = 0;
        }
        if(network[path->node].port[path->port].link_info_apps[0] > path_n_apps){
            path_n_apps = network[path->node].port[path->port].link_info_apps[0];
        }

        list_rem_node(&network[path->node].port[path->port].dflows, path->n_l);
        list_rem_elem(flow->path);
    }
    free(flow->path);
}

void min_links_bandwidth_fast_electric(){

    long i,j,i_aux, j_aux;
    long bandwidth_read = 0;
    long bandwidth_write = 0;
    float total_link_bw = 0.0;
    float link_bw = 0.0;
    float l_bandwidth = 0;
    float l_bandwidth_comms = 0;
    float l_bandwidth_stg = 0;
    float bandwidth_comms = 0;
    float bandwidth_stg = 0;
    list_t *dflows;
    dflow_t **flow;
    min_speed = FLT_MAX;
    avg_link_bandwidth = 0.0;
    links = 0;
    total_links = 0;

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

    for(i=0; i<servers+switches; i++){
        for(j=0; j<network[i].nports; j++){
            total_links++;
            if(network[i].port[j].neighbour.node == -1 || network[i].port[j].neighbour.port == -1 ||  network[i].port[j].flows == 0)
                continue;
            links++;
            link_bw = 0;

            if(traffic_priority == TTP_TRAFFIC_PRIORITY){
                l_bandwidth = (float)network[i].port[j].bandwidth_capacity;
                l_bandwidth_comms = (float)network[i].port[j].bandwidth_capacity * ((float)traffic_priority_params[0]/100);
                l_bandwidth_stg = (float)network[i].port[j].bandwidth_capacity * ((float)traffic_priority_params[1]/100);

                if((network[i].port[j].flows - (network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write)) == 0){
                    bandwidth_stg =  l_bandwidth / network[i].port[j].flows;
                }
                else if((network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write) == 0){
                    bandwidth_comms =  l_bandwidth / network[i].port[j].flows;
                }
                else{
                bandwidth_comms = l_bandwidth_comms / (network[i].port[j].flows - (network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write));
                bandwidth_stg = l_bandwidth_stg / (network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write);
                }

                dflows = &network[i].port[j].dflows;
                list_reset(dflows);
                while(list_next(dflows, (void*)&flow)){

                    if(bandwidth_comms < (*flow)->speed && (*flow)->type == 0 && (*flow)->type != 5 && (*flow)->type != 6){
                        (*flow)->speed = bandwidth_comms;
                    }

                    if(bandwidth_stg < (*flow)->speed && (*flow)->type != 0 && (*flow)->type != 5 && (*flow)->type != 6){
                        (*flow)->speed = bandwidth_stg;
                    }

                    if((*flow)->type != 5 && (*flow)->type != 6){
                        link_bw += (*flow)->speed;
                    }
                }
            }
            else if(traffic_priority == STP_TRAFFIC_PRIORITY){

                l_bandwidth = (float)network[i].port[j].bandwidth_capacity;

                if((network[i].port[j].flows - (network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write)) > 0){
                    bandwidth_comms = l_bandwidth / (network[i].port[j].flows - (network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write));
                    bandwidth_stg = 0;
                }
                else if(network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write > 0){
                    bandwidth_stg = l_bandwidth / (network[i].port[j].flows_storage_read + network[i].port[j].flows_storage_write);
                    bandwidth_comms = 0;
                }

                dflows = &network[i].port[j].dflows;
                list_reset(dflows);
                while(list_next(dflows, (void*)&flow)){

                    if(bandwidth_comms < (*flow)->speed && bandwidth_comms > 0 && (*flow)->type == 0 && (*flow)->type != 5 && (*flow)->type != 6){
                        (*flow)->speed = bandwidth_comms;
                    }

                    if(bandwidth_stg < (*flow)->speed && bandwidth_stg > 0 && (*flow)->type != 0 && (*flow)->type != 5 && (*flow)->type != 6){
                        (*flow)->speed = bandwidth_stg;
                    }

                    if((*flow)->type != 5 && (*flow)->type != 6){
                        link_bw += (*flow)->speed;
                    }
                }
            }
            else{
                l_bandwidth = (float)network[i].port[j].bandwidth_capacity;
                bandwidth_comms = l_bandwidth / network[i].port[j].flows;
                dflows = &network[i].port[j].dflows;
                list_reset(dflows);
                while(list_next(dflows, (void*)&flow)){
                    if(bandwidth_comms < (*flow)->speed && (*flow)->type != 5 && (*flow)->type != 6){
                        (*flow)->speed = bandwidth_comms;
                    }
                    if((*flow)->type != 5 && (*flow)->type != 6){
                        link_bw += (*flow)->speed;
                    }
                }
            }

            if(link_bw < min_speed)
                min_speed = link_bw;

            total_link_bw += (link_bw / (float)l_bandwidth);
            avg_link_bandwidth += link_bw;
        }
    }
    avg_link_bandwidth /= links;
    agg_bw += (((total_link_bw / links) - agg_bw) / (float)(++steps));
    metrics.execution.avg_agg_bw += ((link_bw - metrics.execution.avg_agg_bw) / (float)(++metrics.execution.n_steps));
}

void min_links_bandwidth_accurate_electric(){

    long i,j;
    float link_bw = 0.0;
    float l_bandwidth = 0.0;
    float bandwidth = 0.0;
    float flow_min_bandwidth = 0.0;
    float flow_max_bandwidth = 0.0;
    float bw_aux = 0.0;
    float remaining_bandwidth = 0.0;
    float remaining_bandwidth_aux = 0.0;
    long remaining_flows = 0;
    list_t *dflows;
    dflow_t **flow;
    min_speed = FLT_MAX;
    avg_link_bandwidth = 0.0;
    links = 0;
    total_links = 0;

    for(i=0; i<servers+switches; i++){
        for(j=0; j<network[i].nports; j++){
            total_links++;
            if(network[i].port[j].neighbour.node == -1 || network[i].port[j].neighbour.port == -1 ||  network[i].port[j].flows == 0)
                continue;
            links++;
            link_bw = 0;
            l_bandwidth = (float)network[i].port[j].bandwidth_capacity;
            bandwidth = l_bandwidth / network[i].port[j].flows;
            dflows = &network[i].port[j].dflows;

            remaining_bandwidth = 0;
            remaining_flows = 0;

            list_reset(dflows);
            while(list_next(dflows, (void*)&flow)){
                // Consider all links with the same bandwidth
                flow_min_bandwidth = l_bandwidth / (*flow)->max_flows;
                flow_max_bandwidth = l_bandwidth / (*flow)->min_flows;

                if(bandwidth <= (*flow)->speed && bandwidth > flow_min_bandwidth){
                    (*flow)->speed = flow_min_bandwidth;
                    link_bw += flow_min_bandwidth;
                    remaining_bandwidth += (bandwidth - flow_min_bandwidth);
                }
                else if(bandwidth <= (*flow)->speed){
                    (*flow)->speed = bandwidth;
                    link_bw += bandwidth;
                    remaining_flows++;
                }
                else{
                    link_bw += (*flow)->speed;
                    remaining_bandwidth += ((*flow)->speed - bandwidth);
                }
                //link_bw += (*flow)->speed;
            }
            while(remaining_flows > 0 && (remaining_bandwidth_aux = floorf(remaining_bandwidth/remaining_flows)) > 0){
                list_reset(dflows);
                while(list_next(dflows, (void*)&flow)){
                    flow_min_bandwidth = l_bandwidth / (*flow)->max_flows;
                    flow_max_bandwidth = l_bandwidth / (*flow)->min_flows;
                    if(bandwidth <= flow_min_bandwidth && ((*flow)->speed + remaining_bandwidth_aux <= flow_max_bandwidth)){
                        (*flow)->speed += remaining_bandwidth_aux;
                        link_bw += remaining_bandwidth_aux;
                        remaining_bandwidth -= remaining_bandwidth_aux;
                    }
                    else  if((bandwidth <= flow_min_bandwidth) && ((*flow)->speed + remaining_bandwidth_aux > flow_max_bandwidth) && ((*flow)->speed != flow_max_bandwidth)){
                        bw_aux = (flow_max_bandwidth - (*flow)->speed);
                        remaining_bandwidth -= bw_aux;
                        link_bw += bw_aux;
                        (*flow)->speed += bw_aux;
                        remaining_flows--;
                    }
                    else if((bandwidth <= flow_min_bandwidth) && ((*flow)->speed + remaining_bandwidth_aux > flow_max_bandwidth) && ((*flow)->speed == flow_max_bandwidth)){
                        remaining_flows--;
                    }
                }
            }
            if(link_bw < min_speed)
                min_speed = link_bw;
            avg_link_bandwidth += link_bw;

            total_link_bw += (link_bw / (float)l_bandwidth);
        }
    }
    avg_link_bandwidth /= links;
    agg_bw += (((total_link_bw / links) - agg_bw) / (float)(++steps));
    metrics.execution.avg_agg_bw += ((link_bw - metrics.execution.avg_agg_bw) / (float)(++metrics.execution.n_steps));
}

void calc_insert_flow_max_min_electric(list_t *dflows, long current, long next_port){

    dflow_t **flow_aux;
    route_t *path_aux;
    switch(mode){
        case DYNAMIC_ELECTRIC_ACCURATE:
            list_reset(dflows);
            while(list_next(dflows, (void*)&flow_aux)){
                if(network[current].port[next_port].flows > (*flow_aux)->max_flows){
                    (*flow_aux)->max_flows = network[current].port[next_port].flows;
                    (*flow_aux)->n_max_flows = 1;
                }
                else if(network[current].port[next_port].flows == (*flow_aux)->max_flows){
                    (*flow_aux)->n_max_flows++;
                }
                if((network[current].port[next_port].flows < (*flow_aux)->min_flows)){
                    (*flow_aux)->min_flows = network[current].port[next_port].flows;
                    (*flow_aux)->n_min_flows = 1;
                }
                else if(network[current].port[next_port].flows - 1 == (*flow_aux)->min_flows && (*flow_aux)->n_min_flows == 1){
                    (*flow_aux)->min_flows = INT_MAX;
                    (*flow_aux)->n_min_flows = 0;
                    list_reset((*flow_aux)->path);
                    while(list_next((*flow_aux)->path, (void*)&path_aux)){
                        if(network[path_aux->node].port[path_aux->port].flows < (*flow_aux)->min_flows){
                            (*flow_aux)->min_flows = network[path_aux->node].port[path_aux->port].flows;
                            (*flow_aux)->n_min_flows = 1;
                        }
                        else if(network[path_aux->node].port[path_aux->port].flows == (*flow_aux)->min_flows){
                            (*flow_aux)->n_min_flows++;
                        }
                    }
                }
                else if(network[current].port[next_port].flows - 1 == (*flow_aux)->min_flows){
                    (*flow_aux)->n_min_flows--;
                }
                else if(network[current].port[next_port].flows == (*flow_aux)->min_flows){
                    (*flow_aux)->n_min_flows++;
                }
            }
            break;
    }
}


void calc_remove_flow_max_min_electric(list_t *dflows, long node, long port){

    dflow_t **flow_aux;
    route_t *path_aux;

    switch(mode){
        case DYNAMIC_ELECTRIC_ACCURATE:
            list_reset(dflows);
            while(list_next(dflows, (void*)&flow_aux)){
                if(((*flow_aux)->max_flows == network[node].port[port].flows + 1) && (*flow_aux)->n_max_flows == 1){
                    (*flow_aux)->max_flows = 0;
                    (*flow_aux)->n_max_flows = 0;
                    list_reset((*flow_aux)->path);
                    while(list_next((*flow_aux)->path, (void*)&path_aux)){
                        if(network[path_aux->node].port[path_aux->port].flows > (*flow_aux)->max_flows){
                            (*flow_aux)->max_flows = network[path_aux->node].port[path_aux->port].flows;
                            (*flow_aux)->n_max_flows = 1;
                        }
                        else if(network[path_aux->node].port[path_aux->port].flows == (*flow_aux)->max_flows){
                            (*flow_aux)->n_max_flows++;
                        }
                    }
                }
                else if((*flow_aux)->max_flows == network[node].port[port].flows + 1){
                    (*flow_aux)->n_max_flows--;
                }
                if(((*flow_aux)->min_flows == network[node].port[port].flows + 1) && (*flow_aux)->n_min_flows == 1){
                    (*flow_aux)->min_flows = INT_MAX;
                    (*flow_aux)->n_min_flows = 0;
                    list_reset((*flow_aux)->path);
                    while(list_next((*flow_aux)->path, (void*)&path_aux)){
                        if(network[path_aux->node].port[path_aux->port].flows < (*flow_aux)->min_flows){
                            (*flow_aux)->min_flows = network[path_aux->node].port[path_aux->port].flows;
                            (*flow_aux)->n_min_flows = 1;
                        }
                        else if(network[path_aux->node].port[path_aux->port].flows == (*flow_aux)->min_flows){
                            (*flow_aux)->n_min_flows++;
                        }
                    }
                }
                else if((*flow_aux)->min_flows == (network[node].port[port].flows + 1)){
                    (*flow_aux)->min_flows = network[node].port[port].flows;
                    (*flow_aux)->n_min_flows--;
                }
            }
            break;
    }
}
