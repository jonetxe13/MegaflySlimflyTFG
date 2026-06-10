#include "../inrflow/gen_trace.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void read_data(application *app, long *tag_s, long *tag_r, long size_response_read, long sync, long seed, long size_tasks)
{
    long from, to;
    long remote = 0;
    long request_type = 0;
    long response_type = 1;
    long size_request_read = 80;
    char statenew[256];
    char *stateold;


    stateold = initstate(seed, statenew, 256);
    for(from = 0; from < size_tasks;from++){
    if(sync == 0){

    switch(app->mem_stg_access){
        case RND_MEM_STG_ACCESS:
            if( (random() % 100) < app->mem_storage_access_read){
                response_type = 3;
            }
            else{
                response_type = 1;
            }
            break;
        default:
            printf("Unknown memory storage access\n");
            exit(-1);
    }
    }
    switch(app->data_access_mode){
        case RND_DATA_ACCESS_MODE:
            remote = is_remote_access(app);
            if(remote == 1){
                to = app->size_tasks + (random() % app->size_storage);
                //printf("%ld --> %ld\n", from ,to);
            }
            else{
                to = from;
            }
            break;
        case CONSECUTIVE_DATA_ACCESS_MODE:
            remote = is_remote_access(app);
            if(remote == 1){
                to = app->size_tasks + (from % app->size_storage);
            }
            else{
                to = from;
            }
            break;
        case CACHED_DATA_ACCESS_MODE:
            to = from;
            break;
        case SAN_DATA_ACCESS_MODE:
            to = from;
            request_type = 5;
            response_type = 5;
            break;

        default:
            printf("Unknown data acces mode\n");
            exit(-1);
    }

        insert_send(app, from, to, (*tag_s)++, size_request_read, request_type);
    insert_recv(app, from, to, (*tag_r)++, size_request_read, request_type);
    insert_send(app, to, from, (*tag_s)++, size_response_read, response_type);
    insert_recv(app, to, from, (*tag_r)++, size_response_read, response_type);
    }
    setstate(stateold);
}

void write_data(application *app, long *tag_s, long *tag_r, long size_write, long sync, long seed, long size_tasks)
{
    long to, from;
    long remote = 0;
    long request_type = 2;
    long response_type = 0;
    long size_ack_write= 80;
    char statenew[256];
    char *stateold;

    stateold = initstate(seed, statenew, 256);
    for(to = 0; to < size_tasks;to++){
    if(sync == 0){
    switch(app->mem_stg_access){
        case RND_MEM_STG_ACCESS:
            if( (random() % 100) < app->mem_storage_access_write){
                request_type = 4;
            }
            else{
                request_type = 2;
            }
            break;
        default:
            printf("Unknown memory storage access\n");
            exit(-1);
    }
    }
    switch(app->data_access_mode){
        case RND_DATA_ACCESS_MODE:
            remote = is_remote_access(app);
            if(remote == 1){
                from = app->size_tasks + (random() % app->size_storage);
            }
            else{
                from = to;
            }
            break;
        case CONSECUTIVE_DATA_ACCESS_MODE:
            remote = is_remote_access(app);
            if(remote == 1){
                from = app->size_tasks + (to % app->size_storage);
            }
            else{
                from = to;
            }
            break;
        case CACHED_DATA_ACCESS_MODE:
            from = to;
            break;
        case SAN_DATA_ACCESS_MODE:
            from = to;
            request_type = 6;
            response_type = 6;
            break;
        default:
            printf("Unknown data acces mode\n");
            exit(-1);
    }

    insert_send(app, to, from, (*tag_s)++, size_write, request_type);
    insert_recv(app, to, from, (*tag_r)++, size_write, request_type);
    insert_send(app, from, to, (*tag_s)++, size_ack_write, response_type);
    insert_recv(app, from, to, (*tag_r)++, size_ack_write, response_type);
    }
    setstate(stateold);
}

void input_data(application *app, long from, long *tag_s, long *tag_r, long size_response_read){

    long to = from;
    long request_type = 5;
    long response_type = 5;
    long size_request_read = 80; //10KB

    insert_send(app, from, to, (*tag_s)++, size_request_read, request_type);
    insert_recv(app, from, to, (*tag_r)++, size_request_read, request_type);
    insert_send(app, to, from, (*tag_s)++, size_response_read, response_type);
    insert_recv(app, to, from, (*tag_r)++, size_response_read, response_type);
}

void output_data(application *app, long to, long *tag_s, long *tag_r, long size_response_write){

    long from = to;
    long request_type = 6;
    long response_type = 6;
    long size_request_write = 80;

    insert_send(app, to, from, (*tag_s)++, size_request_write, request_type);
    insert_recv(app, to, from, (*tag_r)++, size_request_write, request_type);
    insert_send(app, from, to, (*tag_s)++, size_response_write, response_type);
    insert_recv(app, from, to, (*tag_r)++, size_response_write, response_type);
}

int is_remote_access(application *app){

    long remote = 1;
    float percent_mod;

    percent_mod = ((app->size_storage / app->size_tasks) * app->stg_nodes_access);

    switch(app->stg_nodes_access_mode){
        case RND_STG_NODES_ACCESS_MODE:
            if((random() % 100) < percent_mod){
                remote = 0;
            }
            break;
        default:
            printf("Unknown storage nodes access\n");
            exit(-1);
    }
    return(remote);
}
