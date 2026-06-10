#ifndef _collectives
#define _collectives

#include "../inrflow/list.h"

void all2all(application *app);

void one2all(application *app);

void one2all_rnd(application *app);

void all2one(application *app);

void all2one_rnd(application *app);

void binarytree(application *app);

void inversebinarytree(application *app);

void barrier(application *app);

void bcast(application *app);

void reduce(application *app);

void allreduce(application *app);

void gather(application *app);

void scatter(application *app);

void butterfly(application *app);
#endif
