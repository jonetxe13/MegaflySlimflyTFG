#ifndef _allocation_jellyfish
#define _allocation_jellyfish

#include "../inrflow/applications.h"

long allocate_application_jellyfish(application *app, long *cores);

void release_application_jellyfish(application *app);

#endif

