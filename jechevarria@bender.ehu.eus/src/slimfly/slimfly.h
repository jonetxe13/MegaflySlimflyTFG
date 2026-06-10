/*
 * slimfly.h
 *
 *  Created on: 3 Jul 2017
 *      Author: yzy
 */

#ifndef SLIMFLY_SLIMFLY_H_
#define SLIMFLY_SLIMFLY_H_



#endif /* slimfly*/


long init_topo_slimfly(long nparam, long *params); // Initialise theslimfly 
tuple_t connection_slimfly(long node, long port);
void finish_topo_slimfly();

long init_routing_slimfly(long src, long dst);
long route_slimfly(long current, long destination);
void finish_route_slimfly();

//long selecting_relative_global_port(long sg, long dg);

long get_radix_slimfly(long node); //Given a node ( router or server ), get the radix of the node

long get_servers_slimfly(); // Get the total number of servers
long get_swithes_slimfly();
long get_ports_slimfly();


long is_server_slimfly(long i);
long get_server_i_slimfly(long i);
long get_switch_i_slimfly(long i);
long node_to_server_slimfly(long i);
long node_to_switch_slimfly(long i);

long get_n_paths_routing_slimfly(long source, long destination);



char * get_network_token_slimfly();
char * get_routing_token_slimfly();
char * get_topo_version_slimfly();
char * get_topo_param_tokens_slimfly();
char * get_filename_params_slimfly();
char * get_routing_param_tokens_slimfly();
/*
 * For global view, nodes are separately processors and routers, processors belong to [ 0 , terminals),
 * routers belong to [ terminals, routers + terminals ).
 * For every node, port number is [ 0, param_p + param_h)
 * thus global link can be expressed as relative_global_port = ( routers - terminals  - sub_g * param_a ) * param_h
 */


