#ifndef _electric_engine
#define _electric_engine

#include "applications.h"
#include "list.h"

#include "node.h"

void update_events_electric(unsigned long long time_next_app);

double time_next_event_electric(list_t *list_cpus, list_t *list_flows);

long insert_new_events_electric(application *app, long ntask);

long mark_route_dynamic_electric(application *app, dflow_t *flow, long src, long dst, int type);

void remove_flow_electric(dflow_t *flow, long id, long pid, long pid2);

void min_links_bandwidth_fast_electric();

void min_links_bandwidth_accurate_electric();

void calc_insert_flow_max_min_electric(list_t *dflows, long current, long next_port);

void calc_remove_flow_max_min_electric(list_t *dflows, long node, long port);

#endif
