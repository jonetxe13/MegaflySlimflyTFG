/**
	 @file
	 @brief Topology virtual function declarations and centralised documentation for
	 required topology functions.

	 Each virtual function pointer, e.g. init_topo, must be set in main.c to the name of the
	 corresponding unique function name in your topology.c, e.g., init_topo=init_topo_bcube.
	 You must then implement init_topo_bcube() so that it satisfies the requirements documented
	 here.
 */


#ifndef _topologies
#define _topologies

#include "globals.h"

void init_topology();
/**
 * @brief Initializes the topology and sets the parameters.
 *
 * @param nparam Number of topology parameters
 * @param params Array of integer (long) topo params
 * @return long
 */
long (*init_topo)(long nparam, long* params);

void (*finish_topo)(); ///< free topology memory here

// Get the number of components of the network
long (*get_servers)();///< number of servers
long (*get_switches)();///< number of switches
long (*get_ports)();///< total number of ports (on servers+switches)

// Functions to deal with server/switch differences
long (*is_server)(long i);///< 1 if server, 0 if switch
long (*get_server_i)(long i);///< index of ith server
long (*get_switch_i)(long i);///< index of ith switch
long (*node_to_server)(long i);///< inverse of get_server_i
long (*node_to_switch)(long i);///< inverse of get_switch_i
long (*get_radix)(long n);///< number of ports of node 'n' (server or switch)

/**
 * @brief Calculates where to connect a port node_port <--> tuple.node_tuple.port
 *
 * Note that links are bidirectional, so when connecting a port the destination port will be
 * connected to the original port as well.  This may lead to undefined behaviour if the
 * connections are not done properly (e.g if n0_p0 <-> n1_p1, and then n1_p1 <-> n2_p2: n0_p0
 * -> n1_p1 and n1_p1 <-> n2_p2, which is probably erroneous @see check_consistency().
 *
 * disconnected nodes can be represented with tuple <-1, -1>
 *
 * @param node connect from this node
 * @param port connect from this port
 * @return tuple_t
 */
struct tuple_t (*connection)(long node, long port);

// Routing functions ***********************************************

//For each src, dst pair, main() calls init_routing() once, followed by route() for each link
// in the path until either the destination is reached or a fault is encounted, and finally it
// calls finish_routing()

/**
 * @brief Initializes the routing for a given path. Can be used for source routing.  In
 * local-routing this might do nothing returns -1 if there is no route to the destination.
 *
 * @param src source (server) node
 * @param dst destination (server) node
 * @return long
 */
long (*init_routing)();

void (*finish_route)();///< Clean up after routing, i.e., free memory / debug.

void (*update_route)();///< Clean up after routing, i.e., free memory / debug.

long (*get_n_paths_routing)(long src, long dst);

/**
 * @brief Select the next port towards a destinationi or return -1
 * Select the output port for a packet in node 'current' addressed to 'destination'.
 *
 * @param current Current node (either server or switch).
 * @param destination Destination server of this flow.
 */
long (*route)(long current, long destination);

// Stat reporting functions *********************************************

char* (*get_network_token)();///< single-token name for network
char* (*get_routing_token)();  ///< single-token name for routing algorithm
char* (*get_topo_version)(); ///< version of the topology (independent of inrflow)
char* (*get_topo_param_tokens)(long i);///< single-token names for topo params, e.g., k and n in knkstar_k_n
char* (*get_filename_params)();///< formatted string to use in data filenames
// [OPTIONAL] Use only if there are routing parameters
char* (*get_routing_param_tokens)(long i);///< single-token names for routing algorithm params

// [OPTIONAL] Functions to report custom status from the topology. See, e.g.,  gdcficonn
long (*get_topo_nstats)();///< number of stats to be reported.
struct key_value (*get_topo_key_value)(long i);///< ith key value to be used in reporting.c
long (*get_topo_nhists)();///< number of histograms to be reported
char (*get_topo_hist_prefix)(long i);///< ith histogram prefix, must avoid 'h' 'p' 'f'
const char* (*get_topo_hist_doc)(long i);///< ith histogram documentation, latex friendly and not contain '\n'
long (*get_topo_hist_max)(long i);///< ith histogram length
void (*get_topo_hist)(long *topo_hist, long i);///< ith histogram (memcpy to topo_hist)

#endif //_topo
