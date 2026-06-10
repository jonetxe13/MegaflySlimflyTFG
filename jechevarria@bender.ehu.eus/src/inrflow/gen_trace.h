#ifndef _gen_trace
#define _gen_trace

#include "applications.h"

void insert_computation(application *app, long t_id, long computation);

void insert_send(application *app, long src, long dst, long tag, long size, int type);

void insert_recv(application *app, long src, long dst, long tag, long size, int type);

void send(application *app, long src, long dst, long tag, long size);

void receive(application *app, long src, long dst, long tag, long size);

void gen_trace(application *app);

#endif
