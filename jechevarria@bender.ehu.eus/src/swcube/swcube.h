#ifndef _swcube
#define _swcube

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "../inrflow/globals.h"

// Initializes the topology and sets the parameters.
// #params + array_of_params (long).
long init_topo_swcube(long nparam, long* params);

// Release the resources.
void finish_topo_swcube();

// Get the number of components of the network
long get_servers_swcube();
long get_switches_swcube();
long get_ports_swcube();

long is_server_swcube(long i);
long get_server_i_swcube(long i);
long get_switch_i_swcube(long i);
long node_to_server_swcube(long i);
long node_to_switch_swcube(long i);

char * get_network_token_swcube(); //"%s " //"network " //single-token name
char * get_routing_token_swcube();  //"%s " //"routing_algorithm " //single-token name
char * get_topo_version_swcube(); //"%s " //"topo_version " //version of the topology
char * get_topo_param_tokens_swcube(long i);
char* get_filename_params_swcube();

// Get the number of ports of a given node (either a server or a switch, see above)
long get_radix_swcube(long n);

// Given a node and port, computes an ordered pair tuple_t
// res=<res.node, res.port>, where res.node is connected to the node
// "node" on "node"'s port "port", and "node" is connected to res.node
// on res.node's port res.port. ("" used to emphasise variable names
// where confusion might otherwise occur).
//
// Calculates where to connect a port
// node_port <--> tuple.node_tuple.port
//
// Note that links are bidirectional, so when connecting a port the destination port
// will be connected to the original port as well.
// This may lead to undefined behaviour if the connections are not done properly
// (e.g if n0_p0 <-> n1_p1, and then n1_p1 <-> n2_p2: n0_p0 -> n1_p1 and n1_p1 <-> n2_p2, which is probably erroneous.
//
// disconnected nodes can be represented with tuple <-1, -1>
tuple_t connection_swcube(long node, long port);

long get_n_paths_routing_swcube(long src, long dst);

// Initializes the routing for a given path. Can be used for source routing.
// In local-routing this might do nothing
// returns -1 if there is no route to the destination.
long init_routing_swcube(long src, long dst);

// Finishes a route, checks route is complete and frees stuff if needed.
void finish_route_swcube();

// Select the next port towards a destination
// Select the output port for a packet in node 'current' addressed to 'destination'
long route_swcube(long current, long destination);
#endif //_topo
