#ifndef _storage_apps
#define _storage_apss

#include "../inrflow/applications.h"

typedef struct properties{

    long ntasks_input;
    long ntasks_output;
    long read_input_size;
    long write_output_size;
    long read_size;
    long write_size;
    long nflows;
    long groups;

} properties;

void storageapp(application *app);

void transition_matrix(application *app, long n_states, long **states_matrix);

long next_state(long current_state, long n_states, long **states_matrix, long n_current, long n_min);

void markov_application(application *app);

void computation_block(application *app);

void communications_block(application *app, long size, long nflows, long groups, long *tag_s, long *tag_r);

void io_read_block(application *app, long read_size, long ntasks,long *tag_s, long *tag_r);

void io_write_block(application *app, long read_size, long ntasks,long *tag_s, long *tag_r);

void io_input_block(application *app, long read_size, long ntasks,long *tag_s, long *tag_r);

void io_output_block(application *app, long write_size, long ntasks,long *tag_s, long *tag_r);

double expon(double x);

double rand_val(int seed);

void set_properties(application *app);
#endif
