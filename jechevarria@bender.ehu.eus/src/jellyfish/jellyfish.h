#ifndef _jellyfish
#define _jellyfish

#include "../inrflow/node.h"
// Initializes the topology and sets the parameters.
// #params + array_of_params (long).
long init_topo_jellyfish(long np, long *par);
// Release the resources used by the topology
void finish_topo_jellyfish();

// Get the number of components of the network
// nodes in [0, #servers) should be the servers
// nodes in [#servers, #servers+#switches) should be the switches
long get_servers_jellyfish();
long get_switches_jellyfish();
long get_ports_jellyfish();

long is_server_jellyfish(long i);
long get_server_i_jellyfish(long i);
long get_switch_i_jellyfish(long i);
long node_to_server_jellyfish(long i);
long node_to_switch_jellyfish(long i);

char * get_network_token_jellyfish();
char * get_routing_token_jellyfish();
char * get_topo_version_jellyfish();
char * get_topo_param_tokens_jellyfish();
char *get_routing_param_tokens_jellyfish(long i);
char * get_filename_params_jellyfish();

// Get the number of ports of a given node (either a server or a switch, see above)
long get_radix_jellyfish(long n);

// Calculates where to connect a port
// node_port <--> tuple.node_tuple.port
//
// Note that links are bidirectional, so when connecting a port the destination port
// will be connected to the original port as well.
// This may lead to undefined behaviour if the connections are not done properly
// (e.g if n0_p0 <-> n1_p1, and then n1_p1 <-> n2_p2: n0_p0 -> n1_p1 and n1_p1 <-> n2_p2, which is probably erroneous.
//
// disconnected nodes can be represented with tuple <-1, -1>
tuple_t connection_jellyfish(long node, long port);

long get_n_paths_routing_jellyfish(long src, long dst);

// Initializes the routing for a given path. Can be used for source routing.
// In local-routing this might do nothing
// returns -1 if there is no route to the destination.
long init_routing_jellyfish(long src, long dst, long app_id);

// Finishes a route, checks route is complete and frees stuff if needed.
void finish_route_jellyfish(long path_n_flows_max, long path_n_apps, long app_id);

void update_route_jellyfish(long src, long dst, long path_number, long path_n_apps);

// Select the next port towards a destination
// Select the output port for a packet in node 'current' addressed to 'destination'
long route_jellyfish(long current, long destination);

long nodes_distance_jellyfish(long src, long *dsts, long dist);

long *get_path(long src, long dst, long n_path);

long get_path_length(long src, long dst, long n_path);
#endif //_jellyfish
