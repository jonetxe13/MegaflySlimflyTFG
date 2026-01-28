/*
 * dragonfly.h
 *
 *  Created on: 3 Jul 2017
 *      Author: yzy
 */

#ifndef DRAGONFLY_DRAGONFLY_H_
#define DRAGONFLY_DRAGONFLY_H_



#endif /* DRAGONFLY_DRAGONFLY_H_ */


long init_topo_dragonfly(long nparam, long *params); // Initialise the dragonfly topology
tuple_t connection_dragonfly(long node, long port);
void finish_topo_dragonfly();

long init_routing_dragonfly(long src, long dst);
long route_dragonfly(long current, long destination);
void finish_route_dragonfly();

//long selecting_relative_global_port(long sg, long dg);

long get_radix_dragonfly(long node); //Given a node ( router or server ), get the radix of the node

long get_servers_dragonfly(); // Get the total number of servers
long get_swithes_dragonfly();
long get_ports_dragonfly();


long is_server_dragonfly(long i);
long get_server_i_dragonfly(long i);
long get_switch_i_dragonfly(long i);
long node_to_server_dragonfly(long i);
long node_to_switch_dragonfly(long i);

long get_n_paths_routing_dragonfly(long source, long destination);



char * get_network_token_dragonfly();
char * get_routing_token_dragonfly();
char * get_topo_version_dragonfly();
char * get_topo_param_tokens_dragonfly();
char * get_filename_params_dragonfly();
char * get_routing_param_tokens_dragonfly();
/*
 * For global view, nodes are separately processors and routers, processors belong to [ 0 , terminals),
 * routers belong to [ terminals, routers + terminals ).
 * For every node, port number is [ 0, param_p + param_h)
 * thus global link can be expressed as relative_global_port = ( routers - terminals  - sub_g * param_a ) * param_h
 */


