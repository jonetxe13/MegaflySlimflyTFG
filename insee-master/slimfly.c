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

long param_p; ///< p: Number of servers connected to each switch
long param_a; ///< a: Number of switches in each group
long param_h; ///< h: Number of uplinks

long grps; ///< Total number of groups
long intra_ports; ///<  Total number of ports in one group connecting to other routers in the group

long escape_vcs; ///< How many VCs are needed to maintain deadlock-freedom. The remaining VCs ought to be adaptive.

long max_paths;

long *other_orig2map;
long *other_map2orig;

long ***intergroup_connections; ///< Stores the port (overall intergroup port = (next_group*param_a*param_h) + next port) connected to a given local group, switch and port; e.g., intergroup_connections[g][s][p] stores to what group is connected port p of switch s in group g. Used for several variants (Helix, Nautilux and Random).
long **intergroup_route;		///< Stores the output port within a group that connects to another group; e.g., intergroup_route[g0][g1] stores the group port in g0 that connects to g1. Used for several variants (Helix, Nautilux and Random).

/**
 * Calculates the neighbor for a given node and port
 * @param node. The node which is being connected.
 * @param port. The port number that is being connected.
 * @return a tuple containing the node and port it is being connected to.
 */
tuple_t connection_dragonfly(long node, long port) {
    tuple_t res={-1,-1}; // node and port to connect to
    long gen_switch_id; // switch id in the general switch count
    long sw_id, grp_id, port_id; // switch (within a group), group and port id for calculating connections
    long next_grp, next_port; // group and port id of the target for calculating connections

    // the node is a switch
    gen_switch_id=node - nprocs; // id of the switch relative to other switches
    grp_id=gen_switch_id/param_a; // id of the group relative to other groups
    if( port < param_p ) {// This is a downlink to a server
        res.node = (gen_switch_id * param_p) + (port % param_p); // The sequence of the server
        res.port = 0 ; // Every processor only has one port.
    }
    else if ( port < ( param_p + intra_ports ) ){ // Intra-group connection
        sw_id = gen_switch_id % param_a;
        port_id = port - param_p;
        if (port_id>=sw_id){
            res.node = nprocs + (grp_id * param_a) + port_id+1;
            res.port = param_p + sw_id;
        } else {
            res.node = nprocs + (grp_id * param_a) + port_id;
            res.port = param_p + sw_id-1;
        }
    }
    else if (port < param_h + intra_ports + param_p ) { // uplinks; many connections possible here
        sw_id = gen_switch_id % param_a; // the switch id relative to the switch group
        port_id = port - param_p - intra_ports + (sw_id*param_h); // the port id relative to the switch group

        /// Let's calculate the next group and its link, based on the connection arrangement.
        switch(topo) {
            case DRAGONFLY_ABSOLUTE:
                if (port_id>=grp_id){
                    next_grp= port_id+1;
                    next_port=grp_id;
                } else {
                    next_grp=port_id;
                    next_port=grp_id-1;
                }
                break;
            case DRAGONFLY_RELATIVE:
                next_grp = (grp_id+port_id+1)%grps;
                next_port = (param_a*param_h)-(port_id+1);
                break;
            case DRAGONFLY_CIRCULANT:
                if (port_id % 2){ // odd ports connect counterclockwise
                    next_grp = (grps+grp_id-(port_id/2)-1)%grps;
                    next_port = port_id-1;
                } else { // even ports connect clockwise
                    next_grp = (grp_id+(port_id/2)+1)%grps;
                    if (port_id==grps-2) // will happen when param_h and param_a uneven, the last port connects with itself
                        next_port = port_id;
                    else
                        next_port = port_id+1;
                }
                break;
			case DRAGONFLY_NAUTILUS:
			case DRAGONFLY_HELIX:
			case DRAGONFLY_RND:
				next_grp=intergroup_connections[grp_id][sw_id][port - param_p - intra_ports]/(param_h*param_a);
				next_port=intergroup_connections[grp_id][sw_id][port - param_p - intra_ports]%(param_h*param_a);
				break;
			case DRAGONFLY_OTHER:
				port_id = other_orig2map[port_id];
				next_grp = (grp_id+port_id+1) % grps;
				next_port = (param_a*param_h) - (port_id+1);
				next_port = other_map2orig[next_port];
				break;
            default:
                panic("Not a valid dragonfly");
                break;
        }
        res.node = nprocs + (next_grp * param_a) + (next_port/param_h);
        res.port = param_p + intra_ports + (next_port%param_h);
    }
    else {
        // Should never get here
        res.node = -1;
        res.port = -1;
    }
    return res;
}

/**
* Some of the dragonfly variants rely on bespoke intergroup connections.
* These are implemented using data structures that hold information about the connections and routes.
* This function allocates memory for these data structures.
*/
void allocate_intergroup_structures(){
	long g,s,p;	// group, switch and port
	long next_grp; // target group

	// to calculate connections between groups
	intergroup_connections=alloc(grps*sizeof(long**));
	for (g=0; g<grps;g++) {
		intergroup_connections[g]=alloc(param_a*sizeof(long*));
		for (s=0; s<param_a;s++) {
			intergroup_connections[g][s]=alloc(param_h*sizeof(long));
			for (p=0; p<param_h; p++)
				intergroup_connections[g][s][p]=-1; //disconnected
		}
	}

	// to store the port to go through to get from a group 'g' to a target group 'next_grp'
	intergroup_route=alloc(grps*sizeof(long*));
	for (g=0; g<grps;g++) {
		intergroup_route[g]=alloc(grps*sizeof(long));
		for (next_grp=0; next_grp<grps; next_grp++)
			intergroup_route[g][next_grp]=-1; // no route between these two groups.
	}
}

/**
* Some of the dragonfly variants rely on bespoke intergroup connections.
* These are implemented using data structures that hold information about the connections and routes.
* This function frees these data structures upon termination.
*/
void free_intergroup_structures(){
	long g,s;	// group and switch

	for (g=0; g<grps;g++) {
		for (s=0; s<param_a;s++) {
			free(intergroup_connections[g][s]);
		}
		free(intergroup_connections[g]);
	}
	free(intergroup_connections);

	for (g=0; g<grps;g++) {
		free(intergroup_route[g]);
	}
	free(intergroup_route);
}

/**
* Some of the dragonfly variants rely on bespoke intergroup connections.
* These are implemented using data structures that hold information about the connections and routes.
* This function fills these data structures for the helix connection rule.
*/
void calculate_intergroup_structures_helix(){
	long g,s,p;	// group, switch and port
	long next_grp, next_sw, next_port; // target group, switch and port

	for (g=0; g<grps; g++)  {
		for (s=0; s<param_a; s++)  {
			for(p=0; p<(param_h/2); p++){
				next_grp = (g + s*(param_h/2) + p + 1)%grps;
				next_sw = (s+1)%param_a;

				next_port=(param_h/2)+(param_h%2);
				while(intergroup_connections[next_grp][next_sw][next_port]!=-1){
					next_port++;
					if (next_port>=param_h){
						char message[100];

						sprintf(message, "Number of ports exceeded when creating dragonfly helix %ld\n",next_port);
						panic(message);
					}
				}
				intergroup_connections[g][s][p]=(((next_grp*param_a)+next_sw)*param_h)+next_port;
				intergroup_connections[next_grp][next_sw][next_port]=(((g*param_a)+s)*param_h)+p;

				intergroup_route[g][next_grp]=(s*param_h)+p;
				if (intergroup_route[next_grp][g]!=-1 && intergroup_route[next_grp][g]!=(next_sw*param_h)+next_port){
					char message[100];

					sprintf(message, "WARNING: There is already a route between groups %ld and %ld!\n",next_grp,g);
					panic(message);
				}

				intergroup_route[next_grp][g]=(next_sw*param_h)+next_port;

			}
			//Adding the extra edges if odd
			if(param_h%2==1){
				next_grp = (g + (param_h/2)*param_a + s + 1)%grps;
				next_sw = (param_a - s - 1)%param_a;
				next_port=(param_h/2);
				if (g<next_grp){
					while(intergroup_connections[next_grp][next_sw][next_port]!=-1){
						next_port++;
						if (next_port>=param_h){
							char message[100];

							sprintf(message, "Number of ports exceeded when creating dragonfly helix %ld\n",next_port);
							panic(message);
						}
					}
					intergroup_connections[g][s][p]=(((next_grp*param_a)+next_sw)*param_h)+next_port;
					intergroup_connections[next_grp][next_sw][next_port]=(((g*param_a)+s)*param_h)+p;

					intergroup_route[g][next_grp]=(s*param_h)+p;
					if (intergroup_route[next_grp][g]!=-1 && intergroup_route[next_grp][g]!=(next_sw*param_h)+next_port){
						char message[100];

						sprintf(message, "There is already a route between groups %ld and %ld!\n",next_grp,g);
						panic(message);
					}

					intergroup_route[next_grp][g]=(next_sw*param_h)+next_port;
				}
			}
		}
	}
}

/**
* Some of the dragonfly variants rely on bespoke intergroup connections.
* These are implemented using data structures that hold information about the connections and routes.
* This function fills these data structures for the nautilus connection rule.
*/
void calculate_intergroup_structures_nautilus(){
	long g,s,p;	// group, switch and port
	long next_grp, next_sw, next_port; // target group, switch and port
	long cw, ccw;	// distance for next connection: clockwise for + switches (cw) and counter-clockwise for - switches (ccw)

	// Let's calculate connections
	for (g=0; g<grps;g++) {
		cw=1;	// switches + start connecting to the group at distance 1 clockwise
		ccw=1;	// switches - start connecting to the group at distance 1 counter-clockwise
		for (s=0; s<param_a;s++) {
			for (p=0; p<param_h; p++) {
				if(intergroup_connections[g][s][p]==-1) { // still not connected
					if (s%2 == 0) { // + switch; connect clockwise
						do {	// ensure these groups are not connected already, skip to the next clockwise group
							next_grp=(g+cw)%grps;
							cw++;
						} while (intergroup_route[g][next_grp]!=-1);
					}
					else {	// - switch; connect counter-clockwise
						do {	// ensure these groups are not connected already, skip to the next group counter-clockwise
							next_grp=(grps+g-ccw)%grps;
							ccw++;
						} while (intergroup_route[g][next_grp]!=-1);
					}
					next_sw=g%param_a;
					next_port=0;
					while(intergroup_connections[next_grp][next_sw][next_port]!=-1) {
						next_port++;
						if (next_port>=param_h) {
							char message[100];

							sprintf(message, "Number of ports exceeded when creating dragonfly nautilus %ld\n",next_port);
							panic(message);
						}
					}
					intergroup_connections[g][s][p]=(((next_grp*param_a)+next_sw)*param_h)+next_port;
					intergroup_connections[next_grp][next_sw][next_port]=(((g*param_a)+s)*param_h)+p;

					intergroup_route[g][next_grp]=(s*param_h)+p;
					if (intergroup_route[next_grp][g]!=-1) {
						char message[100];

						sprintf(message, "There is already a route between groups %ld and %ld!\n",next_grp,g);
						panic(message);
					}

					intergroup_route[next_grp][g]=(next_sw*param_h)+next_port;
				}
			}
		}
	}
}

/**
* Some of the dragonfly variants rely on bespoke intergroup connections.
* These are implemented using data structures that hold information about the connections and routes.
* This function fills these data structures for random dragonfly.
*/
void calculate_intergroup_structures_rnd(){
	long i,j;	// indices for the loops
	long t; 	// temporal variable used to skip group and to shuffle values
	long r; 	// random port for the random shuffle

	long **conn; // structure to contain group-to-group connections. Port j of group i is connected to conn[i][j]

	conn=alloc(grps*sizeof(long*));
	for (i=0; i<grps; i++){
		conn[i]=alloc((grps-1)*sizeof(long));

		// We first create a sequential list of connections.
		t=0;
		for (j=0; j<grps-1; j++){
			if (t==i)
				t++; // Skip connections to the same group
			conn[i][j]=t++;
		}
		// Let's now shuffle connections among groups
		for (j=0; j<grps-1; j++){
			r=rand()%(grps-1);
			t=conn[i][j];
			conn[i][j]=conn[i][r];
			conn[i][r]=t;
		}
	}
	// Finally, let's fill the intergroup structures

	for (i=0; i<grps; i++){
		intergroup_route[i][i]=-1;
		for (j=0; j<grps-1; j++){
			intergroup_route[i][conn[i][j]]=j;
		}
	}

	for (i=0; i<grps; i++){
		intergroup_route[i][i]=-1;
		for (j=0; j<grps-1; j++){
			intergroup_connections[i][j/param_h][j%param_h]=(conn[i][j]*param_h*param_a)+intergroup_route[conn[i][j]][i];
		}
	}

	// free the mallocs!!!!
	for (i=0; i<grps; i++)
		free(conn[i]);
	free(conn);
}

/**
 * Creates a dragonfly topology.
 *
 * This function defines all the links between the elements in the network.
 */
void create_dragonfly(){
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

	if (topo==DRAGONFLY_OTHER){
		long i;

		other_orig2map=alloc(param_a*param_h*sizeof(long));
		other_map2orig=alloc(param_a*param_h*sizeof(long));

		for (i=0; i<param_a*param_h; i++){
			if (i%param_h < (param_h/2)){
				other_orig2map[i]=(i%param_h)+((i/param_h)*(param_h/2));
				other_map2orig[other_orig2map[i]]=i;
			} else if (param_h%2!=0 && (i%param_h==param_h-1)){
				other_orig2map[i]=(i/param_h)+((param_a*(param_h-1)/2));
				other_map2orig[other_orig2map[i]]=i;
			} else{
				other_orig2map[i]=(param_a*(param_h-(param_h/2)))+((i%param_h)-(param_h/2))+((i/param_h)*(param_h/2));
				other_map2orig[other_orig2map[i]]=i;
			}
		}
	} // Helix, nautilus and randomfly use intergroup structures. Let's simply allocate the structures and then fill them for each variant.
	else if (topo==DRAGONFLY_HELIX){
		allocate_intergroup_structures();
		calculate_intergroup_structures_helix();
	}
	else if (topo==DRAGONFLY_NAUTILUS){
		allocate_intergroup_structures();
		calculate_intergroup_structures_nautilus();
	}
	else if (topo==DRAGONFLY_RND){
		allocate_intergroup_structures();
		calculate_intergroup_structures_rnd();
	}

    // Initializing switches. No injection queues needed.
    for (i=nprocs; i< NUMNODES; i++ ){
        init_ports(i);
        for (j=0; j < radix; j++ ){
            res=connection_dragonfly(i,j);
            network[i].nbor[j] = res.node;			// neighbor router
            network[i].nborp[j] = res.port;			// neighbor's port
            network[i].op_i[j] = ESCAPE;
            //				network[nr].nbor[np]=i;			// neighbor router's neighbor
            //				network[nr].nborp[np]=j;		// neighbor router's neighbor's port
            //				network[nr].op_i[np] = ESCAPE;
        }
    }

    if(routing == SPANNING_TREE_ROUTING){
        create_spanning_trees(nchan, param_p);
    }
}

long choose_proxy(long source, long destination)
{
	long proxy_grp=destination/(param_p*param_a);

	if( routing==VALIANT){
		proxy_grp=rand()%grps;
	} else if (routing==QUICK_VALIANT_PRIVATE){ // the proxy is private for each source. If param_p == param_h each endpoint has a dedicated uplink/proxy
		long r0=(source*param_h)/param_p;	// first possible proxy
		long r1=((source+1)*param_h)/param_p;	// last possible proxy
		long rp;	//choose a proxy (at random if more than 1 suitable)
		if (r1-r0<2) // only 1 possible proxy; choose r0
			rp=r0%param_h;
		else	// more than 1 possible proxy; choose at random
			rp=(r0+(rand()%(r1-r0)))%param_h;
		proxy_grp=((network[nprocs+(source/param_p)].nbor[param_p+param_a-1+rp])-nprocs)/param_a ;
	} else if (routing==QUICK_VALIANT_QUASIPRIVATE){ // the proxy is quasi private; the first half of uplinks is shared with the previous processor, the second half is shared with the next.
		long r0=(source*param_h)/param_p;	// first possible proxy
		long r2=((source+2)*param_h)/param_p;	// last possible proxy
		long rp;	//choose a proxy (at random if more than 1 suitable)
		if (r2-r0<2) // only 1 possible proxy; choose r0
			rp=r0%param_h;
		else	// more than 1 possible proxy; choose at random
			rp=(r0+(rand()%(r2-r0)))%param_h;
		proxy_grp=((network[nprocs+(source/param_p)].nbor[param_p+param_a-1+rp])-nprocs)/param_a ;
	} else if (routing==QUICK_VALIANT_LOCAL){
		proxy_grp=((network[nprocs+(source/param_p)].nbor[param_p+param_a-1+(rand()%param_h)])-nprocs)/param_a ;// pick a neighbor to the local group at random
	} else if (routing==QUICK_VALIANT_REMOTE){
		proxy_grp=((network[nprocs+(destination/param_p)].nbor[param_p+param_a-1+(rand()%param_h)])-nprocs)/param_a ;// pick a neighbor to the remote group at random
	} else if (routing==QUICK_VALIANT_DUAL){
		if(rand()%2) // pick a neighbor to either the local or the remote group at random
			proxy_grp=((network[nprocs+(source/param_p)].nbor[param_p+param_a-1+(rand()%param_h)])-nprocs)/param_a;// pick a local neighbor
		else
			proxy_grp=((network[nprocs+(destination/param_p)].nbor[param_p+param_a-1+(rand()%param_h)])-nprocs)/param_a ;// pick a remote neighbor
	}
	return proxy_grp;
}

/**
 * Generates the routing record for a dragonfly.
 *
 * @param source The source node of the packet.
 * @param destination The destination node of the packet.
 * @return The routing record needed to go from source to destination.
 */
routing_r dragonfly_rr (long source, long destination) {
    routing_r res;
    long src_grp=source/(param_p*param_a);
    long dst_grp=destination/(param_p*param_a);
    long proxy_grp;
    long cur=source;

    if (source == destination)
        panic("Self-sent packet\n");

    res.rr = alloc(8 * sizeof(long));
    res.rr[7] = 0;	// Are we using a proxy? Used to decide in which virtual channel to inject the paper for the Dally mechanism
    res.size=0;

    proxy_grp=dst_grp;
    if (src_grp!=dst_grp && grps>2 && routing!=STATIC_ROUTING){  // source and destination are in different groups. Let's check whether we need to use a proxy
		proxy_grp = choose_proxy(source,destination);
    }
    if (proxy_grp!=src_grp && proxy_grp!=dst_grp) // Make sure the proxy is neither the source or the destination group and set to 1.
        res.rr[7] = 1;

    while(cur!=destination){
        res.rr[res.size]=route_dragonfly(cur,destination,proxy_grp);
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
long route_dragonfly(long current, long destination, long proxy) {
    long cur_sw, dst_sw;
    long cur_grp, dst_grp;
    long outport_sw, outport_grp;
    long tgt_grp;	// the target group, it can be the destination group, or a proxy if valiant (or qvaliant) are used
    long tmp;

    if(current<nprocs) // Still in the source server, only port 0 is available.
        return 0;
    else{
        cur_sw=current-nprocs;
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
                if (cur_grp==proxy)
                    tgt_grp=dst_grp;
                else
                    tgt_grp=proxy;

                switch(topo){
                    case DRAGONFLY_ABSOLUTE:
                        if (cur_grp>tgt_grp)
                            outport_grp=tgt_grp;
                        else
                            outport_grp=tgt_grp-1;
                        break;
                    case DRAGONFLY_RELATIVE:
                        outport_grp=(grps+(tgt_grp-cur_grp)-1)%grps;
                        break;
                    case DRAGONFLY_CIRCULANT:
                        tmp=tgt_grp-cur_grp;
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
                    case DRAGONFLY_HELIX:
                    case DRAGONFLY_RND:
                    	// These variants use intergroup structures that are computed separately in create_dragonfly()
						outport_grp=intergroup_route[cur_grp][tgt_grp];
                        break;
                    case DRAGONFLY_OTHER:
                        outport_grp=other_map2orig[(grps+(tgt_grp-cur_grp)-1)%grps];
                        break;
                    default:
                        panic("Not a valid dragonfly");
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

/**
 * Frees the data structures used by the dragonfly.
 */
void finish_dragonfly(){

    if(routing == SPANNING_TREE_ROUTING){
        destroy_spanning_trees();
    }
	if ( topo==DRAGONFLY_NAUTILUS || topo==DRAGONFLY_HELIX  || topo==DRAGONFLY_RND){
		free_intergroup_structures();
	}
	if ( topo==DRAGONFLY_OTHER){
		free(other_map2orig);
		free(other_orig2map);
	}

}
