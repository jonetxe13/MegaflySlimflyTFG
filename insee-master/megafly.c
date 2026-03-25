/**
 * dragonfly.c
 *
 * The dragonfly topology.
 */
#include <stdlib.h>
#include <stdio.h>

#include "spanning_tree.h"
#include "globals.h"

/*
 * Parameters (a,p,h) for the dragonfly topology;
 */

extern long param_p; ///< p: Number of servers connected to each switch
extern long param_a; ///< a: Number of switches in each group
extern long param_h; ///< h: Number of uplinks

extern long grps; ///< Total number of groups
extern long intra_ports; ///<  Total number of ports in one group connecting to other routers in the group

extern long escape_vcs; ///< How many VCs are needed to maintain deadlock-freedom. The remaining VCs ought to be adaptive.

extern long max_paths;

extern long *other_orig2map;
extern long *other_map2orig;

extern long ***intergroup_connections; ///< Stores the port (overall intergroup port = (next_group*param_a*param_h) + next port) connected to a given local group, switch and port; e.g., intergroup_connections[g][s][p] stores to what group is connected port p of switch s in group g. Used for several variants (Helix, Nautilux and Random).
extern long **intergroup_route;		///< Stores the output port within a group that connects to another group; e.g., intergroup_route[g0][g1] stores the group port in g0 that connects to g1. Used for several variants (Helix, Nautilux and Random).

/**
 * Calculates the neighbor for a given node and port
 * @param node. The node which is being connected.
 * @param port. The port number that is being connected.
 * @return a tuple containing the node and port it is being connected to.
 */
tuple_t connection_megafly(long node, long port) {
    tuple_t res={-1,-1}; // node and port to connect to
    long gen_switch_id; // switch id in the general switch count
    long sw_id, grp_id, port_id; // switch (within a group), group and port id for calculating connections
    long next_grp, next_port; // group and port id of the target for calculating connections

    // the node is a switch
    gen_switch_id=node - nprocs; // id of the switch relative to other switches
    int switches = param_a*grps;
    if( node < nprocs) { // The node is a server ESTO ES CORRECTO PARA MEGAFLY TMBN
        if( port == 0 ) {
            res.node = nprocs+ (node / param_p) ; // The server's router
            res.port = node % param_p; // The server's port number
        } // nprocsonly have one connection
    }
    else if(node < (nprocs+ (switches/2))) { // the node is a local switch
        grp_id = (gen_switch_id/(param_a/2)); // id of the group relative to other groups
        if( port < param_p ) {// This is a downlink to a server 
            // res.node = grp_id*param_p*param_p + (gen_switch_id%(param_a/2))*param_p + port; //coger el server
            res.node = gen_switch_id * param_p + port;
            res.port = 0 ; // Every processor only has one port.
        }
        else if ( port < (2*param_p) ){ // Intra-group connection
            sw_id = gen_switch_id % (param_a/2);
            port_id = port - param_p;
            res.node = nprocs+ gen_switch_id + switches/2 - sw_id + port_id;
            res.port = sw_id;
        }
    }
    else if(node >= (nprocs+ (switches/2))){ //the node is a global switch
        grp_id = ((gen_switch_id-(switches/2))/(param_a/2)); // id of the group relative to other groups
        if (port < param_p){ // Intra-group connection
            sw_id = (gen_switch_id % (param_a/2)); //id del switch local dentro del grupo
            // port_id = port;
            res.node = nprocs+ (gen_switch_id - switches/2) - sw_id + port;
            res.port = sw_id+param_p;
            // printf("DEBUG: Spine %ld port %ld -> Leaf %ld port %ld (sw_id=%ld, pp=%d)\n", node, port, res.node, res.port, sw_id, param_p);

        }
	else if(port < 2*param_p ) { // uplinks; many connections possible here
		sw_id = gen_switch_id % (param_a/2); // the switch id relative to the switch group
		port_id = port - param_p + (sw_id*param_h); // the port id relative to the switch group

		/// Let's calculate the next group and its link, based on the connection arrangement.
			if (port_id>=grp_id){
			    next_grp= port_id+1;
			    next_port=grp_id;
			} else {
			    next_grp=port_id;
			    next_port=grp_id-1;
			}
		res.node = nprocs + (switches/2) + (next_grp * param_p) + (next_port/param_h);
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

/**
 * Creates a dragonfly topology.
 *
 * This function defines all the links between the elements in the network.
 */
void create_megafly(){
    long i, j, nr, np;	//neighbor router and port.
    tuple_t res;

    nnics=1;

    // Initializating processors. Only 1 transit port plus injection queues.
    for (i=0; i<nprocs; i++){
        for (j=0; j<ninj; j++)
            inj_init_queue(&network[i].qi[j]);
        init_ports(i);
        nr = nprocs + (i / param_p); // The server's router
        np = i % param_p; // The server's port number
        network[i].nbor[0] = nr;
        network[i].nborp[0] = np;
        network[i].op_i[0] = ESCAPE;
        //		network[nr].nbor[np] = i;
        //		network[nr].nborp[np] = 0;
        //		network[nr].op_i[np] = ESCAPE;
        for (j=1; j<radix; j++){
            network[i].nbor[j] = NULL_PORT;
            network[i].nborp[j] = NULL_PORT;
            network[i].op_i[j] = ESCAPE;
        }
    }

    // Initializing switches. No injection queues needed.
    for (i=nprocs; i< NUMNODES; i++ ){
        init_ports(i);
        for (j=0; j < radix; j++ ){
            res=connection_megafly(i,j);
            network[i].nbor[j] = res.node;			// neighbor router
            network[i].nborp[j] = res.port;			// neighbor's port
            network[i].op_i[j] = ESCAPE;
            //				network[nr].nbor[np]=i;			// neighbor router's neighbor
            //				network[nr].nborp[np]=j;		// neighbor router's neighbor's port
            //				network[nr].op_i[np] = ESCAPE;
        }
    }
}

/**
 * Generates the routing record for a dragonfly.
 *
 * @param source The source node of the packet.
 * @param destination The destination node of the packet.
 * @return The routing record needed to go from source to destination.
 */
routing_r megafly_rr(long source, long destination) {
    routing_r res;
    // long src_grp=source/(param_p*param_a);
    // long dst_grp=destination/(param_p*param_a);
    long src_grp = source/(param_p*(param_a/2));
    long dst_grp = destination/(param_p*(param_a/2));
    long proxy_grp;
    long cur=source;
    long next_port;

    if (source == destination)
        panic("Self-sent packet\n");

    res.rr = alloc(16 * sizeof(long));
    res.rr[15] = 0;	// Are we using a proxy? Used to decide in which virtual channel to inject the paper for the Dally mechanism
    res.size=0;

    proxy_grp=dst_grp;
    while(cur!=destination){
        if (res.size >= 16) {
            panic("¡Bucle infinito detectado en el enrutamiento Megafly!");
        }
        next_port = route_megafly(cur, destination, proxy_grp);
        // printf("DEBUG: Routing de %ld a %ld. Size actual: %ld; next_port: %ld\n", source, destination, res.size, next_port);
        // if (next_port >= 0 && next_port < radix) {
        //     printf("Destino fisico: Nodo %ld\n", network[cur].nbor[next_port]);
        // } else {
        //     printf("¡PUERTO INVALIDO!\n");
        // }
        res.rr[res.size]=next_port;
        cur=network[cur].nbor[res.rr[res.size]];
        res.size++;
    }

    return res;
}

/**
 * Routes a packet in a dragonfly.
 *
 * @param current The node where the packet is at this moment.
 * @param destination The destination node of the packet.
 * @param proxy The proxy to route through. If it is the local or destination group, the port is calculated as DIM, otherwise, the port is calculated using proxy routing.
 * @return The port to take the next hop through.
 */
long route_megafly(long current, long destination, long proxy) {
    long cur_sw, dst_sw;
    long cur_grp, dst_grp;
    long outport_sw, outport_grp = -1;
    long tgt_grp;	// the target group, it can be the destination group, or a proxy if valiant (or qvaliant) are used
    long tmp;

    int switches = param_a*grps;

        // Constantes locales para claridad
    long leafs_per_grp = param_a / 2;
    long spines_per_grp = param_a / 2;

    if (current < nprocs) { // Servidor -> Switch
        return 0;
    } 
    else if((current-nprocs) < (switches/2)){ //current es leaf switch
        cur_sw = current-nprocs;
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
    else if(current-nprocs < switches){//current es spine switch
        cur_sw = current-nprocs-(switches/2);
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

/**
 * Frees the data structures used by the dragonfly.
 */
void finish_megafly(){

    if(routing == SPANNING_TREE_ROUTING){
        destroy_spanning_trees();
    }
	if ( topo==MEGAFLY){
		free(other_map2orig);
		free(other_orig2map);
	}

}
