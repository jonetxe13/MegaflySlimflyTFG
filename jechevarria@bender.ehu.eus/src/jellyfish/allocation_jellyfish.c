#include "../inrflow/applications.h"
#include "../inrflow/scheduling.h"
#include "../inrflow/globals.h"
#include "../jellyfish/allocation_jellyfish_strategies.h"
#include <stdio.h>
#include <stdlib.h>

extern long ports_switches;

long allocate_application_jellyfish(application *app, long *cores){

    long alloc =0;

    switch(app->allocation){
        case JELLYFISH_SPREAD_ALLOC:
            alloc = jellyfish_allocation_spread(app, cores); 
            break;
        case JELLYFISH_RANDOM_ALLOC:
            alloc = jellyfish_allocation_random(app, cores); 
            break;
        case JELLYFISH_CONTIGUITY_ALLOC:
            alloc = jellyfish_allocation_contiguous(app, cores); 
            break;
        case JELLYFISH_LOCALITY_ALLOC:
            alloc = jellyfish_allocation_locality(app, cores); 
            break;
        default:
            printf("Unkwown allocation strategy.\n");
            exit(-1);
    }

    return(alloc);
}

void release_application_jellyfish(application *app){

    switch(app->allocation){
        case JELLYFISH_SPREAD_ALLOC:
            jellyfish_release_spread(app); 
            break;
        case JELLYFISH_RANDOM_ALLOC:
            jellyfish_release_random(app); 
            break;
        case JELLYFISH_CONTIGUITY_ALLOC:
            jellyfish_release_contiguous(app); 
            break;
        case JELLYFISH_LOCALITY_ALLOC:
            jellyfish_release_locality(app); 
            break;
        default:
            break;
    }
}



