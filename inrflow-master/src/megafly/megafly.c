/*
 * megafly.c
 *
 *  Created on: 22 Jan 2026
 *      Author: yzy
 */
#include <stdlib.h>
#include <stdio.h>

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "../inrflow/globals.h"
#include "megafly.h"

/*
 * Parameters (a,p,h) for the megaflytopology;
 */

static long param_p; ///< p: Number of servers connected to each switch
static long param_a; ///< a: Number of switches in each group
static long param_h; ///< h: Number of uplinks

static long grps; ///< Total number of groups
static long switches;///< Total number of switches
static long servers;///< Total number of servers
static long ports;///< Total number of links
static long intra_ports; ///<  Total number of ports in one group connecting to other routers in the group

static long proxy_grp; ///< The switch group to use as a proxy.

static long max_paths;

static long *other_orig2map;
static long *other_map2orig;

static long ***intergroup_connections;
static long **intergroup_route;

//long * hop ;
static char network_token[14];

static char* topo_version="v0.1";
static char* topo_param_tokens[3]= {"p","a","h"};

extern char filename_params[100];
static char *routing_param_tokens[1]= {"max_paths"};
static char routing_token[30];

/**
 * declare the number of global connections between groups;
 */
long init_topo_megafly(long np, long *par) {
    //Check the parameters
    if(np != 1) {
        printf("1 parameter is needed for the megafly topology <radix>\n");
        exit(-1);
    }
    if(par[0] < 1) {
        printf("radix must be a positive number; %ld has been inserted",par[2]);
        exit(-1);
    }

    param_p = par[0]/2; // number of servers per switch
    param_a = par[0]; // number of switches per group
    param_h = par[0]/2; // number of uplinks per switch

	// Calculate some useful values from the parameters
    intra_ports = param_a/2;
    grps = ((param_a/2) * param_h) + 1;
    switches = param_a * grps;
    servers = param_p * (switches/2);
    ports = param_a*param_p*2 * grps + servers;

	// Boring stuff for printing and file generation
    switch(topo) {
        case MEGAFLY:
            sprintf(network_token,"megafly");
            break;
        default:
            printf("Not a valid megafly");
            exit(-1);
            break;
    }

    sprintf(filename_params,"p%ld_a%ld_h%ld",param_p,param_a,param_h);

    switch(routing) {
	    case DRAGONFLY_MINIMUM:
		sprintf(routing_token,"min");
		break;
	    case DRAGONFLY_VALIANT:
		sprintf(routing_token,"valiant");
		break;
	    default:
		printf("Not a Megafly-compatible routing!");
		exit(-1);
}

    return 0;
}

void finish_topo_megafly(){

}

long get_servers_megafly(){
    return servers;
}

long get_radix_megafly(long n){

    if ( n < servers )
        return 1;	// This is a server
    else{
        return param_p*2; // This is a switch with h uplinks, p downlinks.
    }
}

tuple_t connection_megafly(long node, long port) {
    tuple_t res={-1,-1};
    long gen_switch_id; // switch id in the general switch count
    long sw_id, grp_id, port_id; // switch (within a group), group and port id for calculating connections
    long next_grp, next_port; // group and port id of the target for calculating connections
    if( node < servers ) { // The node is a server ESTO ES CORRECTO PARA MEGAFLY TMBN
        if( port == 0 ) {
            res.node = servers + (node / param_p) ; // The server's router
            res.port = node % param_p; // The server's port number
        } // servers only have one connection
    }
    else if(node < (servers + (switches/2))) { // the node is a local switch
        gen_switch_id = node - servers; // id of the switch relative to other switches
        grp_id = (gen_switch_id/(param_a/2)); // id of the group relative to other groups
        if( port < param_p ) {// This is a downlink to a server 
            // res.node = grp_id*param_p*param_p + (gen_switch_id%(param_a/2))*param_p + port; //coger el server
            res.node = gen_switch_id * param_p + port;
            res.port = 0 ; // Every processor only has one port.
        }
        else if ( port < (2*param_p) ){ // Intra-group connection
            sw_id = gen_switch_id % (param_a/2);
            port_id = port - param_p;
            res.node = servers + gen_switch_id + switches/2 - sw_id + port_id;
            res.port = sw_id;
        }
    }
    else if(node >= (servers + (switches/2))){ //the node is a global switch
        gen_switch_id = node - servers; // id of the switch relative to other switches
        grp_id = ((gen_switch_id-(switches/2))/(param_a/2)); // id of the group relative to other groups
        if (port < param_p){ // Intra-group connection
            sw_id = (gen_switch_id % (param_a/2)); //id del switch local dentro del grupo
            // port_id = port;
            res.node = servers + (gen_switch_id - switches/2) - sw_id + port;
            res.port = sw_id+param_p;
            // printf("DEBUG: Spine %ld port %ld -> Leaf %ld port %ld (sw_id=%ld, pp=%d)\n", node, port, res.node, res.port, sw_id, param_p);

        }
        else if (port < 2*param_p ) { // uplinks;
            sw_id = gen_switch_id % (param_a/2); // the switch id relative to the switch group
            port_id = port - param_p + (sw_id*param_h); // the port id relative to the switch group

            if (port_id >= grp_id){
                next_grp = port_id+1;
                next_port = grp_id;
            } else {
                next_grp = port_id;
                next_port = grp_id-1;
            }

//            printf("%ld %ld %ld %ld\n",grp_id,sw_id,next_grp,next_port/param_h);
            res.node = servers + (switches/2) + (next_grp * param_p) + (next_port/param_h);
            res.port = param_p + (next_port%param_h);
        }

    }
    else {
        // Should never get here
        res.node = -1;
        res.port = -1;
    }
    return res;
}

long is_server_megafly(long i){
    return (i < servers);
}

char * get_network_token_megafly(){
    return network_token;
}

char * get_routing_token_megafly(){
    return routing_token;
}

long get_swithes_megafly(){
    return switches;
}

char *get_routing_param_tokens_megafly(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_megafly(){
    return topo_version;
}

char * get_topo_param_tokens_megafly(long i){
    return topo_param_tokens[i];
}

char * get_filename_params_megafly(){
    return filename_params;
}

long get_server_i_megafly(long i){
    return i;
}

long get_switch_i_megafly(long i){
    return servers + i;
}

long node_to_server_megafly(long i){
    return i;
}

long node_to_switch_megafly(long i){
    return i - servers;
}

long get_ports_megafly(){
    return ports;
}

/*
 * Get the number of paths between a source and a destination.
 * @return the number of paths.
 */
long get_n_paths_routing_megafly(long src, long dst){
    return(1);
}

long init_routing_megafly(long src, long dst) {
    long src_grp = src/(param_p*(param_a/2));
    long dst_grp = dst/(param_p*(param_a/2));

    proxy_grp = dst_grp;
    
    return 1;
}

void finish_route_megafly(){

}

long route_megafly(long current, long destination) {
    long cur_sw, dst_sw;
    long cur_grp, dst_grp;
    long outport_grp;
    long spine_idx_needed;  // Qué spine (0..1) tiene el enlace global
    long my_spine_idx;      // Si soy spine, cuál soy (0..1)

    // Constantes locales para claridad
    long leafs_per_grp = param_a / 2;
    long spines_per_grp = param_a / 2;

    if (current < servers) { // Servidor -> Switch
        return 0;
    } 
    else if((current-servers) < (switches/2)){ //current es leaf switch
        cur_sw = current-servers;
        cur_grp = cur_sw/(param_a/2);
        dst_sw=destination/(param_p);
        dst_grp=dst_sw/(param_a/2);

        if(cur_sw==dst_sw){//downlink a server
            outport_grp = destination%param_p;
        }
        else if(cur_grp==dst_grp){//mismVo grupo
            outport_grp = param_p + (dst_sw%(param_a/2));
        }
        else if(cur_grp!=dst_grp){//distinto grupo (hacia uplink)
            if(cur_grp<dst_grp){
                 outport_grp = param_p+((dst_grp-1)/param_h);
            }
            else{
                 outport_grp = param_p+(dst_grp/param_h);
            }
        }
    }
    else if(current-servers < switches){//current es spine switch
        cur_sw = current-servers-(switches/2);
        cur_grp = cur_sw/(param_a/2);
        dst_sw=destination/(param_p);
        dst_grp=dst_sw/(param_a/2);

        if(cur_grp==dst_grp){//mismVo grupo
            outport_grp = dst_sw%(param_a/2);
        }
        else if(cur_grp!=dst_grp){//distinto grupo (hacia uplink)
            if(cur_grp<dst_grp){
                 outport_grp = (param_a/2)+(dst_grp-1)%param_h;
            }
            else{
                 outport_grp = (param_a/2)+(dst_grp%param_h);
            }
        }
    }

    return outport_grp;
}

