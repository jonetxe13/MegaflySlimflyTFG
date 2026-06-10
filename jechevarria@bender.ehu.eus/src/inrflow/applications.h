#ifndef _applications
#define _applications

#include "metrics.h"
#include "misc.h"
#include "list.h"
//#include "../jellyfish/allocation_jellyfish.h"

typedef struct application{

    app_metrics info;

    long size;
    long size_tasks;
    long size_storage;
    long tasks_finished;
    long packets;
    long comp_time;
    long phases;
    long running;
    long seed;
    tpattern_t pattern; ///< the traffic pattern we are using in the simulation
    char pattern_type[50];

    allocation_t allocation;
    mapping_t mapping;
    storage_t storage;
    memory_storage_access_t mem_stg_access;
    data_access_mode_t data_access_mode;
    stg_nodes_access_mode_t stg_nodes_access_mode;

    long pattern_nparam;
    long pattern_params[MAX_TRAFFIC_PARAMS];
    char pattern_file[50];
    char allocation_type[50];
    char mapping_type[50];
    char storage_type[50];
    char mem_storage_access_type[50];
    char data_access_mode_type[50];
    char stg_nodes_access_mode_type[50];
    long stg_nodes_access;
    long mem_storage_access_read;
    long mem_storage_access_write;
    long *cores_active;
    long *servers_storage;
    long *translation;
    long *remaining_flows;
    long *remaining_cpus;
    long n_cores_inactive;
    long *cores_inactive;
    long num_servers;
    long finished_storage_flows;
    node_list *node;
    list_t **task_events;
    list_t **task_events_occurred;

} application;

/** 
 * Types of event.
 * 
 * It should be 'r' for a reception, 's' for a sent or 'c' for a computation event.
 */
typedef enum event_t {
    RECEPTION = 'r',        ///< Reception
    SENDING = 's',          ///< Sent
    COMPUTATION = 'c'       ///< Computation
} event_t;

typedef struct route_t{
    int node;
    short port;
    struct node_list *n_l;
} route_t;

typedef struct dflow_t{
    long start_time;
    long injection_time;
    int path_number;
    float speed;
    int san_link;
    int max_flows;
    int n_max_flows; //remove
    int min_flows;
    int n_min_flows;
    int type;
    int type_id;
    int allocated;
    list_t *path;
    
}dflow_t;

/**
 * Structure that defines an event.
 *
 * It contains all the needed atributes for distinguish it:
 * type (S/R/C), the second node id (destination/source), a task id,
 * the length in packets, and the packets sent/arrived.
 */
typedef struct event {
    event_t type;   ///< Type of the event (Reception / Sent / Computation).
    int id;
    int pid;               ///< The other node (processor id).
    int pid2;               ///< The other node (processor id).
    long length;    ///< Length of the message in packets. Number of cycles in computation.
    long count;             ///< The number of packets sent/arrived. Number of elapsed cycles when running.
    struct dflow_t dflow;
    struct application *app;
    long total_subflows;
    long subflows_aux;
    int type_flow; ///< 0: comms, 1: storage
} event;

void init_running_application(application *next_app);

void finish_application(application *app);

long is_running(long app_id);
#endif

