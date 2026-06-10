#ifndef _allocation_jellyfish_strategies
#define _allocation_jellyfish_strategies

#include "../inrflow/applications.h"
#include "../inrflow/scheduling.h"

long jellyfish_allocation_spread(application *next_app, long *cores);

void jellyfish_release_spread(application *app);

long jellyfish_allocation_random(application *app, long *cores);

void jellyfish_release_random(application *app);
    
long jellyfish_allocation_locality(application *app, long *cores);

void jellyfish_release_locality(application *app);

long jellyfish_allocation_contiguous(application *next_app, long *cores);

void jellyfish_release_contiguous(application *app);

#endif
