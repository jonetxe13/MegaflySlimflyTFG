/*
 * megafly.h
 *
 *  Created on: 3 Jul 2017
 *      Author: yzy
 */

#ifndef MEGAFLY_MEGAFLY_H_
#define MEGAFLY_MEGAFLY_H_



#endif /* megafly*/


long init_topo_megafly(long nparam, long *params); // Initialise the megaflytopology
tuple_t connection_megafly(long node, long port);
void finish_topo_megafly();

long init_routing_megafly(long src, long dst);
long route_megafly(long current, long destination);
void finish_route_megafly();

//long selecting_relative_global_port(long sg, long dg);

long get_radix_megafly(long node); //Given a node ( router or server ), get the radix of the node

long get_servers_megafly(); // Get the total number of servers
long get_swithes_megafly();
long get_ports_megafly();


long is_server_megafly(long i);
long get_server_i_megafly(long i);
long get_switch_i_megafly(long i);
long node_to_server_megafly(long i);
long node_to_switch_megafly(long i);

long get_n_paths_routing_megafly(long source, long destination);



char * get_network_token_megafly();
char * get_routing_token_megafly();
char * get_topo_version_megafly();
char * get_topo_param_tokens_megafly();
char * get_filename_params_megafly();
char * get_routing_param_tokens_megafly();
/*
 * For global view, nodes are separately processors and routers, processors belong to [ 0 , terminals),
 * routers belong to [ terminals, routers + terminals ).
 * For every node, port number is [ 0, param_p + param_h)
 * thus global link can be expressed as relative_global_port = ( routers - terminals  - sub_g * param_a ) * param_h
 */


