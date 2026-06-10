#ifndef _metrics
#define _metrics

#include "list.h"

typedef struct scheduling_metrics{
    double makespan;
    double avg_waiting_time;
    double avg_total_time;
    float utilization;
    float utilization_if;


} scheduling_metric;

typedef struct app_metrics{
    long id;
    long size_tasks;
    long size_storage;
    long num_servers;
    unsigned long long arrive_time;
    unsigned long long start_time;
    unsigned long long end_time;
    unsigned long long runtime;
    unsigned long long runtime_stg;
    unsigned long long waiting_time;
    long n_flows;
    long n_flows_comms;
    long n_flows_storage;
    long n_subflows;
    long n_subflows_comms;
    long n_subflows_storage;
    long flows_distance;
    long flows_distance_comms;
    long flows_distance_storage;
    long flows_latency;
    long flows_latency_comms;
    long flows_latency_storage;
    float avg_flows_distance;
    float avg_flows_distance_comms;
    float avg_flows_distance_storage;
    float max_flows_distance;
    float max_flows_distance_comms;
    float max_flows_distance_storage;
    float min_flows_distance;
    float min_flows_distance_comms;
    float min_flows_distance_storage;
    float avg_flows_latency;
    float avg_flows_latency_comms;
    float avg_flows_latency_storage;
    long num_links_used;
    long **links_utilization;
} app_metrics;

typedef struct applications_metrics{
    long n_apps;
    float avg_size_tasks;
    float avg_size_storage;
    float avg_num_servers;
    list_t apps;
} applications_metrics;

typedef struct execution_metrics{
    double avg_runtime;
    double avg_latency;
    float avg_agg_bw;
    long n_steps;
    list_t agg_bw;
    long *links_info;
    double avg_active_flows;
    double avg_injected_flows;
    double avg_consumed_flows;
    double avg_bandwidth;
    double max_bandwidth;
    double min_bandwidth;

} execution_metrics;

typedef struct metrics_t{
    struct scheduling_metrics scheduling;
    struct applications_metrics applications;
    struct execution_metrics execution;
} metrics_t;

void init_metrics(struct metrics_t *metrics);

void init_metrics_application(struct app_metrics *info);

double get_makespan(struct metrics_t *metrics);

void set_makespan(struct metrics_t *metrics, double makespan);

void update_metrics(struct metrics_t *metrics);

void update_flows_distance(struct app_metrics *info, long path_length, int type);

void update_flows_number(struct app_metrics *info, int type);

void update_subflows_number(struct app_metrics *info, long number, int type);

void update_flows_latency(struct app_metrics *info, float latency, int type);

void set_runtime_stg(struct app_metrics *info, unsigned long long runtime_stg);

void report_metrics(struct metrics_t *metrics);
#endif
