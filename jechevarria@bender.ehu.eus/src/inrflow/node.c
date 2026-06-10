#include "node.h"
#include "globals.h"

void set_bandwidth_capacity(port_t *port, long bandwidth_capacity){

}

long get_bandwidth_capacity(long node, long port){

    return(network[node].port[port].bandwidth_capacity);

}

