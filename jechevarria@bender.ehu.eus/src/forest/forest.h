#ifndef _forest
#define _forest

struct tree_t{
    
    long *param_down;   ///< Number of downwards ports in each level
    long *param_up;  ///< number of upwards ports in each level

    long *down_pow; ///< an array with the downward-link size for different levels of the topology, useful for doing some calculations.
    long *up_pow;   ///< an array with the upward-link count, useful for doing some calculations.
    long *sw_per_stage;  ///< an array with the number of switches per stage, used often for many purposes
    long *cur_route;    ///< Array to store the current route
    long cur_hop;   ///< Current hop in this route
    long switches;
    long ports;
    long max_paths; ///< Maximum number of paths to generate when using multipath
    long *path_index;   ///< per-node index, used for round robin routing

};
typedef struct forest_t{

    long param_trees;
    long param_k;   ///< parameter k of the topology, number of stages for all the trees
    long servers;
    struct tree_t *tree;


} forest_t;
// Initializes the topology and sets the parameters.
// #params + array_of_params (long).
long init_topo_forest(long nparam, long* params);

// Release the resources.
void finish_topo_forest();


// Get the number of components of the network
// nodes in [0, #servers) should be the servers
// nodes in [#servers, #servers+#switches) should be the switches
long get_servers_forest();
long get_switches_forest();
long get_ports_forest();

long is_server_forest(long i);
long get_server_i_forest(long i);
long get_switch_i_forest(long i);
long node_to_server_forest(long i);
long node_to_switch_forest(long i);

char * get_network_token_forest();
char * get_routing_token_forest();
char * get_topo_version_forest();
char * get_topo_param_tokens_forest();
char* get_filename_params_forest();
char* get_routing_param_tokens_forest();

// Get the number of ports of a given node (either a server or a switch, see above)
long get_radix_forest(long n);

// Calculates where to connect a port
// node_port <--> tuple.node_tuple.port
//
// Note that links are bidirectional, so when connecting a port the destination port
// will be connected to the original port as well.
// This may lead to undefined behaviour if the connections are not done properly
// (e.g if n0_p0 <-> n1_p1, and then n1_p1 <-> n2_p2: n0_p0 -> n1_p1 and n1_p1 <-> n2_p2, which is probably erroneous.
//
// disconnected nodes can be represented with tuple <-1, -1>
tuple_t connection_forest(long node, long port);

long get_n_paths_routing_forest(long src, long dst);

// Initializes the routing for a given path. Can be used for source routing.
// In local-routing this might do nothing
// returns -1 if there is no route to the destination.
long init_routing_forest(long src, long dst);

// Finishes a route, checks route is complete and frees stuff if needed.
void finish_route_forest();

// Select the next port towards a destination
// Select the output port for a packet in node 'current' addressed to 'destination'
long route_forest(long current, long destination);

#endif //_topo
