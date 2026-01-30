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
    ports = param_a * (param_p + param_h + param_a - 1) * grps;
    servers = param_p * (switches/2);

	// Boring stuff for printing and file generation
    switch(topo) {
        case MEGAFLY_ABSOLUTE:
            sprintf(network_token,"megafly-abs");
            break;
        default:
            printf("Not a valid megafly");
            exit(-1);
            break;
    }

    sprintf(filename_params,"p%ld_a%ld_h%ld",param_p,param_a,param_h);

    switch(routing) {
	    case MEGAFLY_MINIMUM:
		sprintf(routing_token,"min");
		break;
	    case MEGAFLY_VALIANT:
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
        return param_h + param_p + param_a -1; // This is a switch with h uplinks, p downlinks.
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
    else if(node <= (servers + (switches/2))) { // the node is a local switch
        gen_switch_id = node - servers; // id of the switch relative to other switches
        grp_id = (gen_switch_id/(param_a/2)); // id of the group relative to other groups
        if( port < param_p ) {// This is a downlink to a server 
            res.node = grp_id*param_p*param_p + (gen_switch_id%param_a/2) + port; //coger el server
            res.port = 0 ; // Every processor only has one port.
        }
        else if ( port < (2*param_p) ){ // Intra-group connection
            sw_id = gen_switch_id % (param_a/2);
            port_id = port - param_p;
            if (port_id>=sw_id){
                res.node = servers + gen_switch_id + switches/2 + port_id+1;
                res.port = sw_id;
            } else {
                res.node = servers + gen_switch_id + switches/2 + port_id;
                res.port = sw_id-1;
            }
        }
    }
    else if(node > (servers + (switches/2))){ //the node is a global switch
        gen_switch_id = node - servers; // id of the switch relative to other switches
        grp_id = ((gen_switch_id-(switches/2))/(param_a/2)); // id of the group relative to other groups
        // if( port < param_p ) {// This is a downlink to a server
        //     res.node = servers + (switches/2) + port; // The sequence of the server
        //     res.port = 0 ; // Every processor only has one port.
        // }
        if (port < param_p){ // Intra-group connection
            sw_id = (gen_switch_id % (param_a/2)); //id del switch local dentro del grupo
            port_id = port - param_p;
            if (port_id>=sw_id){
                res.node = servers + (gen_switch_id - switches/2) + port_id+1;
                res.port = sw_id;
            } else {
                res.node = servers + (gen_switch_id - switches/2) + port_id;
                res.port = sw_id-1;
            }
        }
        else if (port < 2*param_p ) { // uplinks; many connections possible here
            sw_id = gen_switch_id % (param_a/2); // the switch id relative to the switch group
            port_id = port - param_p + (sw_id*param_h); // the port id relative to the switch group

            /// Let's calculate the next group and its link, based on the connection arrangement.
            switch(topo) {
                case MEGAFLY_ABSOLUTE:
                    if (port_id >= grp_id){
                        next_grp = port_id+1;
                        next_port = grp_id;
                    } else {
                        next_grp = port_id;
                        next_port = grp_id-1;
                    }
                    break;
                default:
                    printf("Not a valid megafly");
                    exit(-1);
                    break;
            }

//            printf("%ld %ld %ld %ld\n",grp_id,sw_id,next_grp,next_port/param_h);
            res.node = servers + gen_switch_id + (next_grp * (param_a/2)) + (next_port/param_h);
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
    long src_grp = src/(param_p*param_a);
    long dst_grp = dst/(param_p*param_a);

    proxy_grp = dst_grp;
    
    if (src_grp != dst_grp) {
        if (routing == MEGAFLY_VALIANT) {
            proxy_grp = rand() % grps;
        } 
        else if (routing == DRAGONFLY_QUICK_VALIANT_PRIVATE) {// the proxy is private for each source. If param_p == param_h each endpoint has a dedicated uplink/proxy
            long r0 = (src * param_h) / param_p;  // first possble proxy 
            long r1 = ((src + 1) * param_h) / param_p;  // last possble proxy
            long rp; //choose a proxy (at random if more than 1 suitable)
            
            if (r1 - r0 < 2) // only 1 possible proxy; choose at random
                rp = r0 % param_h;
            else // more than 1 possible proxy; choose at random
                rp = (r0 + (rand() % (r1 - r0))) % param_h;
            
            proxy_grp = ((network[servers + (src/param_p)].port[param_p + param_a - 1 + rp].neighbour.node) - servers) / param_a;
        }
        else if (routing == DRAGONFLY_QUICK_VALIANT_QUASIPRIVATE) {// the proxy is quasi private; the first half of uplinks is shared with the previous processor, the second half is shared with the next.
            long r0 = (src * param_h) / param_p;// first possible proxy
            long r2 = ((src + 2) * param_h) / param_p; // last possible proxy
            long rp;//choose a proxy (at random if more than 1 suitable)
            
            if (r2 - r0 < 2)// only 1 possible proxy; choose r0
                rp = r0 % param_h;
            else// more than 1 possible proxy; choose at random
                rp = (r0 + (rand() % (r2 - r0))) % param_h;
            
            proxy_grp = ((network[servers + (src/param_p)].port[param_p + param_a - 1 + rp].neighbour.node) - servers) / param_a;
        }
        else if (routing == DRAGONFLY_QUICK_VALIANT_LOCAL) {
            proxy_grp = ((network[servers + (src/param_p)].port[param_p + param_a - 1 + (rand() % param_h)].neighbour.node) - servers) / param_a;// pick a neighbor to the local group at random
        }
        else if (routing == DRAGONFLY_QUICK_VALIANT_REMOTE) {
            // Elegir un vecino del grupo remoto al azar
            proxy_grp = ((network[servers + (dst/param_p)].port[param_p + param_a - 1 + (rand() % param_h)].neighbour.node) - servers) / param_a;// pick a neighbor to the remote group at random
        }
        else if (routing == DRAGONFLY_QUICK_VALIANT_DUAL) {
            if (rand() % 2) // pick a neighbor to either the local or the remote group at random
                proxy_grp = ((network[servers + (src/param_p)].port[param_p + param_a - 1 + (rand() % param_h)].neighbour.node) - servers) / param_a;// pick a local neighbor
            else
                proxy_grp = ((network[servers + (dst/param_p)].port[param_p + param_a - 1 + (rand() % param_h)].neighbour.node) - servers) / param_a;// pick a remote neighbor
        }
    }
    
    return 1;
}

void finish_route_megafly(){

}
// # TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
long route_megafly(long current, long destination) {
    long cur_sw, dst_sw;
    long cur_grp, dst_grp;
    long outport_sw, outport_grp;
    long tmp;


    if(current<servers) // Still in the source server, only port 0 is available.
        return 0;
    else{
        cur_sw=current-servers;
        dst_sw=destination/param_p;
        if (cur_sw==dst_sw) // Already in the destination switch, just go down the appropriate port.
            return destination%param_p;
        else{
            cur_grp=cur_sw/param_a;
            dst_grp=dst_sw/param_a;
            if (cur_grp==dst_grp) {// in the same group as the destination; pick the port to the adequate switch
                if (cur_sw>dst_sw)
                    return param_p+(dst_sw%param_a);
                else
                    return param_p+(dst_sw%param_a)-1;
            }
            else { // need to swap to a different group
                if (cur_grp==proxy_grp)
                    proxy_grp=dst_grp;

                switch(topo){
                    case MEGAFLY_ABSOLUTE:
                        if (cur_grp>proxy_grp)
                            outport_grp=proxy_grp;
                        else
                            outport_grp=proxy_grp-1;
                        break;
                    case DRAGONFLY_RELATIVE:
                        outport_grp=(grps+(proxy_grp-cur_grp)-1)%grps;
                        break;
                    case DRAGONFLY_CIRCULANT:
                        tmp=proxy_grp-cur_grp;
                        if (abs(tmp)>(grps/2)){
                            if (tmp>0)
                                tmp-=grps;
                            else
                                tmp+=grps;
                        }
                        outport_grp=(abs(tmp)-1)*2;
                        if(tmp<0)
                            outport_grp+=1;
			    if(outport_grp==grps-1){ // It can happen with uneven param_a and param_h that one of the chords
			    outport_grp--;
                        }
                        break;
                    case DRAGONFLY_NAUTILUS:
                    	outport_grp=intergroup_route[cur_grp][proxy_grp];
                        break;
                    case DRAGONFLY_HELIX:
                    	outport_grp=intergroup_route[cur_grp][proxy_grp];
                        break;
                    case DRAGONFLY_OTHER:
                        outport_grp=other_map2orig[(grps+(proxy_grp-cur_grp)-1)%grps];
                        break;
                    default:
                        printf("Not a valid megafly");
                        exit(-1);
                        break;

                }
                // outport_grp has the port within the group that is connected to the destination group. Now we need to check whether this port is in the local switch or we need to go to a different switch in our group.
                outport_sw=outport_grp/param_h;
                if (outport_sw==(cur_sw%param_a)) // Great!!! it's in the current switch
                    return (outport_grp%param_h)+param_p+intra_ports;
                else{	// Aw! Another extra hop to get there
                    if ((cur_sw%param_a)>outport_sw)
                        return param_p+(outport_sw);
                    else
                        return param_p+(outport_sw)-1;
                }
            }
        }
    }
}
