#ifndef _dynamic_engine
#define _dynamic_engine

#include "applications.h"
#include "list.h"

#include "node.h"

int flow_inj_mode; ///< Flow injection mode

int dmetrics_time;

int verbose;

int load_balancing;

double (*time_next_event)(list_t *list_cpus, list_t *list_flows);

long (*insert_new_events)(application *app, long ntask);

void (*remove_flow)(dflow_t *flow, long id, long src, long dst);

void (*min_links_bandwidth)();

void run_dynamic();

void update_events(unsigned long long time_next_app);

void update_cpus(list_t *list_cpus, double t_next);

void update_flows(list_t *list_flows, double t_next);

long max_route_dynamic(long src, long dst);

long remove_send_event(list_t *events, long id, long pid, long pid2, long aux);

long remove_reception_event(list_t *events, long id, long pid, long pid2, long length);

void san_links_bandwidth();

double round_ns(double x);
#endif
