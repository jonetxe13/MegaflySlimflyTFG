/*
 * dragonfly.c
 *
 *  Created on: 3 Jul 2017
 *      Author: yzy
 */
#include <stdlib.h>
#include <stdio.h>

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "../inrflow/globals.h"
#include "dragonfly.h"

/*
 * Parameters (a,p,h) for the dragonfly topology;
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
long init_topo_dragonfly(long np, long *par) {
    //Check the parameters
    if(np != 3) {
        printf("3 parameters are needed for the dragonfly topology <p, a, h>\n");
        exit(-1);
    }
	if(par[0] < 1) {
        printf("param_p must be a positive number; %ld has been inserted",par[2]);
        exit(-1);
    }
    if(par[1] < 1) {
        printf("param_a must be a positive number; %ld has been inserted",par[2]);
        exit(-1);
    }
    if(par[2] < 1) {
        printf("param_h must be a positive number; %ld has been inserted",par[2]);
        exit(-1);
    }

    param_p = par[0]; // number of servers per switch
    param_a = par[1]; // number of switches per group
    param_h = par[2]; // number of uplinks per switch

	// Calculate some useful values from the parameters
    intra_ports = param_a - 1;
    grps = (param_a * param_h) + 1;
    switches = param_a * grps;
    ports = param_a * (param_p + param_h + param_a - 1) * grps;
    servers = param_p * switches;

	// Boring stuff for printing and file generation
    switch(topo) {
        case DRAGONFLY_ABSOLUTE:
            sprintf(network_token,"dragonfly-abs");
            break;
        case DRAGONFLY_RELATIVE:
            sprintf(network_token,"dragonfly-rel");
            break;
        case DRAGONFLY_CIRCULANT:
            sprintf(network_token,"dragonfly-cir");
            break;
        case DRAGONFLY_NAUTILUS:
            sprintf(network_token,"dragonfly-nau");
            {
            	long g,s,p;	// group, switch and port
            	long next_grp, next_sw, next_port; // target group, switch and port
            	long cw, ccw;	// distance for next connection: clockwise for + switches (cw) and counter-clockwise for - switches (ccw)

				// to calculate connections between groups
            	intergroup_connections=malloc(grps*sizeof(long**));
            	for (g=0; g<grps;g++)
				{
					intergroup_connections[g]=malloc(param_a*sizeof(long*));
					for (s=0; s<param_a;s++)
					{
						intergroup_connections[g][s]=malloc(param_h*sizeof(long));
						for (p=0; p<param_h; p++)
							intergroup_connections[g][s][p]=-1; //disconnected
					}
				}

				// to store the port to go through to get from a group 'g' to a target group 'next_grp'
            	intergroup_route=malloc(grps*sizeof(long*));
            	for (g=0; g<grps;g++)
				{
					intergroup_route[g]=malloc(grps*sizeof(long));
					for (next_grp=0; next_grp<grps; next_grp++)
						intergroup_route[g][next_grp]=-1; // no route between these two groups.
				}

				// Let's calculate connections
				for (g=0; g<grps;g++)
				{
					cw=1;	// switches + start connecting to the group at distance 1 clockwise
					ccw=1;	// switches - start connecting to the group at distance 1 counter-clockwise
					for (s=0; s<param_a;s++)
					{
						for (p=0; p<param_h; p++)
						{
							if(intergroup_connections[g][s][p]==-1) // still not connected
							{
								if (s%2 == 0) // + switch; connect clockwise
								{
									do	// ensure these groups are not connected already, skip to the next clockwise group
									{
										next_grp=(g+cw)%grps;
										cw++;
									} while (intergroup_route[g][next_grp]!=-1);
								}
								else	// - switch; connect counter-clockwise
								{
									do	// ensure these groups are not connected already, skip to the next group counter-clockwise
									{
										next_grp=(grps+g-ccw)%grps;
										ccw++;
									} while (intergroup_route[g][next_grp]!=-1);
								}
								next_sw=g%param_a;
								next_port=0;
								while(intergroup_connections[next_grp][next_sw][next_port]!=-1)
								{
									next_port++;
									if (next_port>=param_h)
									{
										printf("Number of ports exceeded when creating dragonfly nautilus %ld\n",next_port);
										exit(-1);
									}
								}
								intergroup_connections[g][s][p]=(((next_grp*param_a)+next_sw)*param_h)+next_port;
								intergroup_connections[next_grp][next_sw][next_port]=(((g*param_a)+s)*param_h)+p;

								intergroup_route[g][next_grp]=(s*param_h)+p;
								if (intergroup_route[next_grp][g]!=-1)
								{
									printf("There is already a route between groups %ld and %ld!\n",next_grp,g);
									exit(-1);
								}

								intergroup_route[next_grp][g]=(next_sw*param_h)+next_port;
							}
						}
					}
				}
            }
            break;
		case DRAGONFLY_HELIX:
            sprintf(network_token,"dragonfly-hel");
            {
            	long g,s,p;	// group, switch and port
            	long next_grp, next_sw, next_port; // target group, switch and port

            	// to calculate connections between groups
            	intergroup_connections=malloc(grps*sizeof(long**));
            	for (g=0; g<grps;g++)
				{
					intergroup_connections[g]=malloc(param_a*sizeof(long*));
					for (s=0; s<param_a;s++)
					{
						intergroup_connections[g][s]=malloc(param_h*sizeof(long));
						for (p=0; p<param_h; p++)
							intergroup_connections[g][s][p]=-1; //disconnected
					}

				}

				// to store the port to go through to get from a group 'g' to a target group 'next_grp'
            	intergroup_route=malloc(grps*sizeof(long*));
            	for (g=0; g<grps;g++)
				{
					intergroup_route[g]=malloc(grps*sizeof(long));
					for (next_grp=0; next_grp<grps; next_grp++)
						intergroup_route[g][next_grp]=-1; // no route between these two groups.
				}

				for (g=0; g<grps; g++)  {
					for (s=0; s<param_a; s++)  {
						for(p=0; p<(param_h/2); p++){
							next_grp = (g + s*(param_h/2) + p + 1)%grps;
							next_sw = (s+1)%param_a;

							next_port=(param_h/2)+(param_h%2);
							while(intergroup_connections[next_grp][next_sw][next_port]!=-1)
							{
								next_port++;
								if (next_port>=param_h)
								{
									printf("Number of ports exceeded when creating dragonfly helix %ld\n",next_port);
									exit(-1);
								}
							}
							intergroup_connections[g][s][p]=(((next_grp*param_a)+next_sw)*param_h)+next_port;
							intergroup_connections[next_grp][next_sw][next_port]=(((g*param_a)+s)*param_h)+p;

							intergroup_route[g][next_grp]=(s*param_h)+p;
							if (intergroup_route[next_grp][g]!=-1 && intergroup_route[next_grp][g]!=(next_sw*param_h)+next_port)
							{
								printf("There is already a route between groups %ld and %ld!\n",next_grp,g);
								//exit(-1);
							}

							intergroup_route[next_grp][g]=(next_sw*param_h)+next_port;
						}
						//Adding the extra edges if odd
						if(param_h%2==1){
							next_grp = (g + (param_h/2)*param_a + s + 1)%grps;
							next_sw = (param_a - s - 1)%param_a;
							next_port=(param_h/2);
							if (g<next_grp){
								while(intergroup_connections[next_grp][next_sw][next_port]!=-1)
								{
									next_port++;
									if (next_port>=param_h)
									{
										printf("Number of ports exceeded when creating dragonfly helix %ld\n",next_port);
										exit(-1);
									}
								}
								intergroup_connections[g][s][p]=(((next_grp*param_a)+next_sw)*param_h)+next_port;
								intergroup_connections[next_grp][next_sw][next_port]=(((g*param_a)+s)*param_h)+p;

								intergroup_route[g][next_grp]=(s*param_h)+p;
								if (intergroup_route[next_grp][g]!=-1 && intergroup_route[next_grp][g]!=(next_sw*param_h)+next_port)
								{
									printf("There is already a route between groups %ld and %ld!\n",next_grp,g);
									exit(-1);
								}

								intergroup_route[next_grp][g]=(next_sw*param_h)+next_port;
							}
						}
					}
				}
            }
            break;
		case DRAGONFLY_OTHER:
            sprintf(network_token,"dragonfly-hel");
            {
            	long i;
            	// Helix is simply a remapping of relative connections. These two variables translate from Relative numbering (orig) to Helix numbering (mapped) and vice versa.
				other_orig2map=malloc(param_a*param_h*sizeof(long));
				other_map2orig=malloc(param_a*param_h*sizeof(long));

				for (i=0; i<param_a*param_h; i++)
				{
					if (i%param_h < (param_h/2))
					{
						other_orig2map[i]=(i%param_h)+((i/param_h)*(param_h/2));
						other_map2orig[other_orig2map[i]]=i;
					}
					else if (param_h%2!=0 && (i%param_h==param_h-1))
					{
						other_orig2map[i]=(i/param_h)+((param_a*(param_h-1)/2));
						other_map2orig[other_orig2map[i]]=i;
					}
					else
					{
						other_orig2map[i]=(param_a*(param_h-(param_h/2)))+((i%param_h)-(param_h/2))+((i/param_h)*(param_h/2));
						other_map2orig[other_orig2map[i]]=i;
					}
				}
            }
            break;
        default:
            printf("Not a valid dragonfly");
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
	    case DRAGONFLY_QUICK_VALIANT_PRIVATE:
		sprintf(routing_token,"quick-valiant-private");
		break;
	    case DRAGONFLY_QUICK_VALIANT_QUASIPRIVATE:
		sprintf(routing_token,"quick-valiant-quasiprivate");
		break;
	    case DRAGONFLY_QUICK_VALIANT_LOCAL:
		sprintf(routing_token,"quick-valiant-local");
		break;
	    case DRAGONFLY_QUICK_VALIANT_REMOTE:
		sprintf(routing_token,"quick-valiant-remote");
		break;
	    case DRAGONFLY_QUICK_VALIANT_DUAL:
		sprintf(routing_token,"quick-valiant-dual");
		break;
	    default:
		printf("Not a Dragonfly-compatible routing!");
		exit(-1);
}

    //
    // switch(routing) {
    //     case DRAGONFLY_MINIMUM:
    //         sprintf(routing_token,"min");
    //         break;
    //     case DRAGONFLY_VALIANT:
    //         sprintf(routing_token,"valiant");
    //         break;
    //     default:
    //         printf("Not a Dragonfly-compatible routing!");
    //         exit(-1);
    // }
    return 0;
}

void finish_topo_dragonfly(){

}

long get_servers_dragonfly(){
    return servers;
}

long get_radix_dragonfly(long n){

    if ( n < servers )
        return 1;	// This is a server
    else{
        return param_h + param_p + param_a -1; // This is a switch with h uplinks, p downlinks.
    }
}

tuple_t connection_dragonfly(long node, long port) {
    tuple_t res={-1,-1};
    long gen_switch_id; // switch id in the general switch count
    long sw_id, grp_id, port_id; // switch (within a group), group and port id for calculating connections
    long next_grp, next_port; // group and port id of the target for calculating connections
    if( node < servers ) { // The node is a server
        if( port == 0 ) {
            res.node = servers + (node / param_p) ; // The server's router
            res.port = node % param_p; // The server's port number
        } // servers only have one connection
    }
    else{ // the node is a switch
        gen_switch_id = node - servers; // id of the switch relative to other switches
        grp_id = gen_switch_id/param_a; // id of the group relative to other groups
        if( port < param_p ) {// This is a downlink to a server
            res.node = (gen_switch_id * param_p) + (port % param_p); // The sequence of the server
            res.port = 0 ; // Every processor only has one port.
        }
        else if ( port < ( param_p + intra_ports ) ){ // Intra-group connection
            sw_id = gen_switch_id % param_a;
            port_id = port - param_p;
            if (port_id>=sw_id){
                res.node = servers + (grp_id * param_a) + port_id+1;
                res.port = param_p + sw_id;
            } else {
                res.node = servers + (grp_id * param_a) + port_id;
                res.port = param_p + sw_id-1;
            }
        }
        else if (port < param_h + intra_ports + param_p ) { // uplinks; many connections possible here
            sw_id = gen_switch_id % param_a; // the switch id relative to the switch group
            port_id = port - param_p - intra_ports + (sw_id*param_h); // the port id relative to the switch group

            /// Let's calculate the next group and its link, based on the connection arrangement.
            switch(topo) {
                case DRAGONFLY_ABSOLUTE:
                    if (port_id >= grp_id){
                        next_grp = port_id+1;
                        next_port = grp_id;
                    } else {
                        next_grp = port_id;
                        next_port = grp_id-1;
                    }
                    break;
                case DRAGONFLY_RELATIVE:
                    next_grp = (grp_id+port_id+1) % grps;
                    next_port = (param_a*param_h) - (port_id+1);
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
                	next_grp=intergroup_connections[grp_id][sw_id][port - param_p - intra_ports]/(param_h*param_a);
                	next_port=intergroup_connections[grp_id][sw_id][port - param_p - intra_ports]%(param_h*param_a);
                    break;
                case DRAGONFLY_HELIX:
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
                    printf("Not a valid dragonfly");
                    exit(-1);
                    break;
            }

//            printf("%ld %ld %ld %ld\n",grp_id,sw_id,next_grp,next_port/param_h);
            res.node = servers + (next_grp * param_a) + (next_port/param_h);
            res.port = param_p + intra_ports + (next_port%param_h);
        }
        else {
            // Should never get here
            res.node = -1;
            res.port = -1;
        }
    }
    return res;
}

long is_server_dragonfly(long i){
    return (i < servers);
}

char * get_network_token_dragonfly(){
    return network_token;
}

char * get_routing_token_dragonfly(){
    return routing_token;
}

long get_swithes_dragonfly(){
    return switches;
}

char *get_routing_param_tokens_dragonfly(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_dragonfly(){
    return topo_version;
}

char * get_topo_param_tokens_dragonfly(long i){
    return topo_param_tokens[i];
}

char * get_filename_params_dragonfly(){
    return filename_params;
}

long get_server_i_dragonfly(long i){
    return i;
}

long get_switch_i_dragonfly(long i){
    return servers + i;
}

long node_to_server_dragonfly(long i){
    return i;
}

long node_to_switch_dragonfly(long i){
    return i - servers;
}

long get_ports_dragonfly(){
    return ports;
}

/*
 * Get the number of paths between a source and a destination.
 * @return the number of paths.
 */
long get_n_paths_routing_dragonfly(long src, long dst){
    return(1);
}

long init_routing_dragonfly(long src, long dst) {
    long src_grp = src/(param_p*param_a);
    long dst_grp = dst/(param_p*param_a);

    proxy_grp = dst_grp;
    
    if (src_grp != dst_grp) {
        if (routing == DRAGONFLY_VALIANT) {
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

void finish_route_dragonfly(){

}
// # TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
long route_dragonfly(long current, long destination) {
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
                    case DRAGONFLY_ABSOLUTE:
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
                        printf("Not a valid dragonfly");
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
