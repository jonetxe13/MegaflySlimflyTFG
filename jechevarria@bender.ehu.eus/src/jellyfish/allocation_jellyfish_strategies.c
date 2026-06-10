#include "allocation_jellyfish.h"
#include "jellyfish_aux.h"
#include "jellyfish.h"
#include "../inrflow/applications.h"
#include "../inrflow/allocation.h"
#include "../inrflow/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


extern long switches; 
extern long servers; 
extern long ports_switches;
extern long ports_servers;

long jellyfish_allocation_spread(application *app, long *cores){

    long i = 0;
    long current_switch = 0;
    long l = 0;
    long m = 0;

    while(l < app->size){
        while(current_switch < switches){
            if(sched_info->servers[i].cores[m] == -1){
                cores[l] = (i * server_cores) + m;
                l++;
                if(l == app->size){
                    break;
                }
            }
            i = (i + ports_servers) % servers;
            current_switch++;
        }
        current_switch = 0;
        i++;
    }
    return(1);
}

void jellyfish_release_spread(application *app){
    // Not required
}

long jellyfish_allocation_random(application *app, long *cores){

    long i, j, k, l;
    long *sw_aux = NULL;
    long local_server, local_core;

    sw_aux = calloc(switches, sizeof(long));
    i = 0;
    j = rand() % switches;
    while(i < app->size){
        k = 0;
        while(sw_aux[j] == 1){
            j = rand() % switches;
        }
        sw_aux[j] = 1;
        while((i < app->size) && k < ports_servers){
            l = 0;
            while((i < app->size) && l < server_cores){
                local_server = (j * ports_servers) + k;
                local_core = local_server + l;
                if(sched_info->servers[local_server].cores[l] == -1){
                    cores[i++] = local_core;
                }
                l++;
            }
            k++;
        }
    }
    free(sw_aux);
    return(1);
}

void jellyfish_release_random(application *app){
    // Not required
}

long jellyfish_allocation_locality(application *app, long *cores){

    long i, j, l, m;
    long current_switch;
    long alloc = 0;
    long num_switches = 0;
    long num_av_servers = 0;
    long num_av_cores = 0;
    long n_dist_aux = 0;
    long *switches_dist = malloc(switches * sizeof(long));
    long *av_servers = malloc(ports_servers * sizeof(long));
    long *av_cores = malloc(server_cores * sizeof(long));
    long *total_cores = calloc(server_cores * ports_servers * switches, sizeof(long));

    current_switch = -1;
    while(num_av_servers == 0 && current_switch < switches){
        num_av_servers = available_servers(++current_switch, av_servers, ports_servers);
    }
    i = 0;
    while( i < app->size){
        num_switches = nodes_distance_jellyfish(current_switch, switches_dist, n_dist_aux);
        j = 0;
        while(!alloc && j < num_switches){
            num_av_servers = available_servers(switches_dist[j], av_servers, ports_servers);
            l = 0;
            while(!alloc && l < num_av_servers){
                num_av_cores = available_cores(av_servers[l], av_cores, ports_servers);   
                m = 0;
                while(m < num_av_cores && i < app->size){
                    if(total_cores[av_cores[m]] == 0){
                        total_cores[av_cores[m]] = 1;
                        cores[i++] = av_cores[m];
                    }
                    m++;
                }
                if(i == app->size){
                    alloc = 1;
                }
                l++;
            }
            j++;
        }
        n_dist_aux++;
    }
    free(switches_dist);
    free(av_servers);
    free(av_cores);
    free(total_cores);
    return(alloc);
}

void jellyfish_release_locality(application *app){
    //No required
}

long jellyfish_allocation_locality2(application *app, long *cores){

    long i, j, k, l, m;
    long num_free_cores;
    long current_switch;
    long alloc = 0;
    long delta;
    long max_delta;
    long mode;
    long res;
    long num_switches = 0;
    long num_av_servers = 0;
    long num_av_cores = 0;
    long n_dist_aux = 0;
    long *switches_dist = malloc(switches * sizeof(long));
    long *av_servers = malloc(ports_servers * sizeof(long));
    long *av_cores = malloc(server_cores * sizeof(long));
    long *total_cores = calloc(server_cores * ports_servers * switches, sizeof(long));
    long *selected_switches = calloc(switches, sizeof(long));
    long n_selected_switches = 0;


    max_delta = auto_wl.allocation_param[0]; 
    mode = auto_wl.allocation_param[1];
    if(max_delta == -1){
        max_delta = switches - 1;
    }
    if(mode == 0){
        delta = -1;
    }
    else{
        delta = max_delta - 1;
    }

    while(!alloc && ++delta <= max_delta){ 
        current_switch = -1;
        while(current_switch < switches - 1 && !alloc){
            num_av_servers = available_servers(++current_switch, av_servers, ports_servers);
            if(num_av_servers == 0)
                continue;
            i = 0;
            num_free_cores = 0;
            n_selected_switches = 0;
            n_dist_aux = 0;
            while( n_dist_aux < switches && num_free_cores < app->size){
                num_switches = nodes_distance_jellyfish(current_switch, switches_dist, n_dist_aux);
                //print_vector_aux(n_selected_switches,selected_switches);
                j = 0;
                while((num_free_cores < app->size) && (j < num_switches)){
                    res = check_distance_set_switches(n_dist_aux, delta, switches_dist[j], n_selected_switches, selected_switches);
                    if(res){
                        selected_switches[n_selected_switches++] = switches_dist[j];
                        num_av_servers = available_servers(switches_dist[j], av_servers, ports_servers);
                        for(k = 0; k < num_av_servers; k++){
                            num_free_cores += are_available_cores(av_servers[k]);
                        }
                    }
                    j++;
                } 
                if(num_free_cores >= app->size){
                    alloc = 1;
                    break;
                }
                n_dist_aux++;
            }
        }
    }

    if(alloc == 1){
        print_vector_aux(n_selected_switches,selected_switches);
        i = 0;
        j = 0;
        while(j < n_selected_switches){
            num_av_servers = available_servers(selected_switches[j], av_servers, ports_servers);
            l = 0;
            while(l < num_av_servers && i < app->size){
                num_av_cores = available_cores(av_servers[l], av_cores, ports_servers);   
                m = 0;
                while(m < num_av_cores && i < app->size){
                    if(total_cores[av_cores[m]] == 0){
                        total_cores[av_cores[m]] = 1;
                        cores[i++] = av_cores[m];
                    }
                    m++;
                }
                l++;
            }
            j++;
        }
    }
    free(switches_dist);
    free(av_servers);
    free(av_cores);
    free(total_cores);
    return(alloc);
}

void jellyfish_release_locality2(application *app){
    //No required
}

long jellyfish_allocation_contiguous(application *app, long *cores){

    long i = 0;
    long j = 0;
    long k = -1;
    long l = 0;
    long alloc = 0;
    long res = 0;
    long num_switches = 0;
    long n_switches;
    long num_av_servers = 0;
    long n_dist_aux = 0;
    long n_selected = 0;
    long server_id;
    long n_cores_inactive = 0;
    long *cores_inactive = malloc(sizeof(long) * app->size);
    long *switches_dist = malloc(switches * sizeof(long));
    long *av_servers = malloc(ports_servers * sizeof(long));
    long *selected;

    n_switches = (long)ceil((float)app->size / ports_servers);
    selected = malloc(n_switches * sizeof(long));

    while(!alloc && k < switches - 1){
        num_av_servers = available_servers(++k, av_servers, ports_servers);
        if(num_av_servers == 0){
            continue;
        }
        n_dist_aux = 0;
        n_selected = 0;
        while(!alloc && (n_selected < n_switches && n_dist_aux < switches)){
            num_switches = nodes_distance_jellyfish(k, switches_dist, n_dist_aux);
            j = 0;
            while(j < num_switches){
                if(are_available_servers(switches_dist[j], ports_servers)){
                    res = check_selected_set(switches_dist[j], n_selected, selected);
                    if(res){
                        selected[n_selected++] = switches_dist[j];
                        if(n_selected == n_switches){
                            alloc = 1;
                            break;
                        }
                    }
                }
                j++;
            }
            n_dist_aux++;
        }
    }
    if(alloc){
        i = 0;
        j = 0;
        while(i < app->size){
            server_id = selected[j] * ports_servers;
            l = 0;
            while(l < ports_servers){
                sched_info->servers[server_id + l].cont = 1;
                k = 0;
                while(k < server_cores){
                    if(i < app->size){
                        cores[i++] = ((server_id + l)) + k;
                    }
                    else{
                        cores_inactive[n_cores_inactive++] = ((server_id + l)) + k;
                    }
                    k++;
                }
                l++;
            }
            j++;
        }
        assign_inactive_cores(app, cores_inactive, n_cores_inactive);
    }
    else{
        free(cores_inactive);
    }
    free(selected);
    free(switches_dist);
    free(av_servers);
    return(alloc);
}

void jellyfish_release_contiguous(application *app){

    release_inactive_cores(app);

}

