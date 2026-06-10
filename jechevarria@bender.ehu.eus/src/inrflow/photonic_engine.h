#ifndef _photonic_engine
#define _photonic_engine

#include "applications.h"
#include "list.h"

#include "node.h"

typedef struct photonic_path_t{
    
    long node_id;
    long next_port;
    int channel_id;
    int n_lambdas;
    int *lambdas;
    
} photonic_path_t;

void update_events_photonic(unsigned long long time_next_app);

double time_next_event_photonic(list_t *list_cpus, list_t *list_flows);

long insert_new_events_photonic(application *app, long ntask);

long mark_route_dynamic_photonic(application *app, dflow_t *flow, long src, long dst, int type, list_t *path);

long explore_routes(list_t* path, long src, long dst);

long (*explore_route)(list_t* path, long src, long dst);

long explore_route_static_static(list_t *path, long src, long dst);

long explore_route_adaptive_static(list_t *path, long src, long dst);

void remove_flow_photonic(dflow_t *flow, long id, long pid, long pid2);

void min_links_bandwidth_photonic();

void set_path(photonic_path_t *path, long node_id, long next_port, int channel_id, int n_lambdas, int *lambdas);
#endif
