#ifndef _storage
#define _storage

#include "applications.h"

typedef struct san_link_t{

    long n_flows_read;
    long n_flows_write;
    float current_speed_read;
    float current_speed_write;

} san_link_t;

typedef struct san_t{

    long n_io_servers;
    long *n_io_servers_links;
    long bandwidth_read;
    long bandwidth_write;
    struct san_link_t *san_links;

} san_t;

//Storage bandwidth
long read_capacity;;

long write_capacity;;

long n_io_servers;

long n_io_replicas;

void read_data(application *app, long *tag_s, long *tag_r, long size_response, long sync, long seed, long size_tasks);

void write_data(application *app, long *tag_s, long *tag_r, long size_response, long sync, long seed, long size_tasks);

void input_data(application *app, long from, long *tag_s, long *tag_r, long size_response_read);

void output_data(application *app, long to, long *tag_s, long *tag_r, long size_response_write);

int is_remote_access(application *app);
#endif

