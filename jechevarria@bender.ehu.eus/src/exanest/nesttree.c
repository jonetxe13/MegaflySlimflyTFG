/** @mainpage
exanest hybrid torus+tree topology <nodes_per_dim, uplinks per torus, stages, (ports_down, ports_up)^stages>
*/

#include <stdlib.h>
#include <stdio.h>

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "../inrflow/globals.h"
#include "nesttree.h"

extern char filename_params[100];

static long nodes_per_dim;	///< nodes in each dimension of a subtorus
static long nodes_torus;   ///< nodes per subtorus
static long uplinks_per_torus; ///< how many nodes per torus are connected to the tree. Currently only either 1,2 or a multiple of 8 allowed to spread connections nicely
static long uplinks_per_dim;   ///< how many nodes per dimension are connected to the uplink network

static long inter_src; ///< the closest node to the source (internal to its torus) that has an uplink
static long s_hops[3];   ///< hops in x, y and z directions to get to inter_src
static long inter_dst; ///< the closest node to the destination that is uplinked
static long d_hops[3];   ///< hops in x, y and z directions to get to inter_dst

static long param_k;	///< parameter k of the topology, number of stages
static long *param_down;	///< Number of downwards ports in each level
static long *param_up; ///< number of upwards ports in each level

static long *down_pow;	///< an array with the downward-link size for different levels of the topology, useful for doing some calculations.
static long *up_pow;	///< an array with the upward-link count, useful for doing some calculations.
static long *sw_per_stage;  ///< an array with the number of switches per stage, used often for many purposes

static long servers; 	///< The total number of servers
static long switches;	///< The total number of switches
static long ports;		///< The total number of links

static long *cur_route;    ///< Array to store the current route
static long cur_hop;   ///< Current hop in this route
static long max_paths;	///< Maximum number of paths to generate when using multipath
static long *path_index;	///< per-node index, used for round robin routing

static char* network_token="nesttree";
static char routing_token[20];
static char* topo_version="v0.1";
static char* topo_param_tokens[11]= {"stages","down0","up0","down1","up1","down2","up2","down3","up3","down4","up4"};

static char *routing_param_tokens[1]= {"max_paths"};

/**
* Checks whether the number is a power of 8 and returns the number of nodes per dimension that will be connected to an uplink
* @param n is the number we want to check
* @return -1 if not a power of 8 or the number of uplinked nodes per dimension
*/
static long check_pow8(long n){
    long c=2;

    if (n<8)
        return -1;
    else
        while (n>8){
            c=c*2;
            n=n/8;
        }
    if (n==8)
        return c;
    else
        return -1;
}

/**
* Initializes the topology and sets the parameters k & n.
*/
long init_topo_nesttree(long np, long* par)
{
	long i,j, c;
	long buffer_length;

	if (np<1) {
		printf("parameters needed\n");
		exit(-1);
	}

	nodes_per_dim=par[0];
	nodes_torus=nodes_per_dim*nodes_per_dim*nodes_per_dim;

	uplinks_per_torus=par[1];
	if (uplinks_per_torus!=1 && uplinks_per_torus!=2 && 2*uplinks_per_torus!=nodes_torus && 4*uplinks_per_torus!=nodes_torus && (uplinks_per_dim=check_pow8(uplinks_per_torus))==-1){
        printf("uplinks per subtorus is currently %ld, but needs to be 1, 2, a power of 8, or either 1/4 or 1/2 of the nodes per torus!!!\n",uplinks_per_torus);
		exit(-1);
	}
	param_k=par[2];

	if(param_k<1){
		printf("positive number of stages needed\n");
		exit(-1);
	}
	if(param_k>5){
		printf("number of stages limited to 5\n");
		exit(-1);
	}

	param_down=malloc(param_k*sizeof(long));
	param_up=malloc(param_k*sizeof(long));
	c=3;
	for (i=0;i<param_k;i++){
		param_down[i]=par[c++];
		param_up[i]=par[c++];
	}

	buffer_length=sprintf(filename_params,"k%ld",param_k);
	for (i=0;i<param_k;i++){
		buffer_length+=sprintf(filename_params+buffer_length,"d%ldu%ld",param_down[i],param_up[i]);
	}

	up_pow=malloc((param_k+1)*sizeof(long));
	down_pow=malloc((param_k+1)*sizeof(long));
	cur_route=malloc(2*param_k*sizeof(long));   // UP*/DOWN routes cannot be longer than 2*k
	sw_per_stage=malloc(param_k*sizeof(long));
	down_pow[0]=1;
    up_pow[0]=1;

	for (i=0; i<param_k; i++) {
		down_pow[i+1]=down_pow[i]*param_down[i];	// product of param_down[i] for 0<=i<n
		up_pow[i+1]=up_pow[i]*param_up[i];			// product of param_up[i]   for 0<=i<n
	} // numbers of up and down ports will be useful throughout,so let's compute them just once.

    switches=0;
	for (i=0; i<param_k; i++) {
		sw_per_stage[i]=up_pow[i];
		for (j=i+1;j<param_k;j++){
			sw_per_stage[i]*=param_down[j];
		}
		switches+=sw_per_stage[i];
		ports+=sw_per_stage[i]*param_down[i];
	}// number of switches per stage will be useful throughout, so let's compute them just once.
	servers=(nodes_torus/uplinks_per_torus)*down_pow[param_k];

	if (servers<nodes_torus){
		printf("Not enough ports in the tree for a single subtorus");
		exit(-1);
	}

	if (down_pow[param_k]%uplinks_per_torus != 0){
		printf("The network cannot be divided into an integer number of subtorus");
		exit(-1);
	}

//	switch(routing){
//	case TREE_STATIC_ROUTING:
		snprintf(routing_token,20,"tree-static");
//		break;
//    case TREE_RND_ROUTING:
//		snprintf(routing_token,20,"tree-random");
//		break;
//    case TREE_RR_ROUTING:
//		snprintf(routing_token,20,"tree-roundrobin");
//		path_index=malloc(servers/(nodes_torus/uplinks_per_torus)*sizeof(long));
//		for (i=0;i<servers/(nodes_torus/uplinks_per_torus); i++)
//			path_index[i]=i;
//		break;
//	default:
//		printf("Not a tree-compatible routing %d", routing);
//		exit(-1);
//    }

	if(routing_nparam>0){
		max_paths=routing_params[0];
	} else {
		max_paths=1;
	}

	printf("%ld servers, %ld switches\n",servers,switches);

	return 1; //Return status, not used here.
}

/**
* Release the resources used by the topology.
**/
void finish_topo_nesttree()
{
	free(cur_route);
	free(up_pow);
	free(down_pow);
	free(sw_per_stage);
	free(param_down);
	free(param_up);
	if (path_index!=NULL)
		free(path_index);
}

/**
* Get the number of servers of the network
*/
long get_servers_nesttree()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_nesttree()
{
	return switches;
}

/**
* Get the number of ports of a given node (either a server or a switch, see above)
*/
long get_radix_nesttree(long n)
{
	int i=0;
	if (n<servers)
		return 7;	// If this is a server it has 6 ports for the subtorus + 1 uplink port
	else {
		n-=servers;
		while(n>=sw_per_stage[i]){
			n-=sw_per_stage[i];
			i++;
		}
	}
	return param_down[i]+param_up[i]; // If this is a switch the number of ports depends on the stage
}

/**
* Calculates connections
*/
tuple_t connection_nesttree(long node, long port)
{
	tuple_t res;
	long  t; // local subtorus
	long  p; // destination port
	long  x,  y,  z; //coordinates of this node in its subtorus, t
	long lvl;
	long pos;
	long nl_first; // id of the first switch in the NEIGHBOURING level

	t=node/nodes_torus;
    pos=node%nodes_torus;
    x=pos%nodes_per_dim;
    y=(pos/nodes_per_dim)%nodes_per_dim;
    z=(pos/nodes_per_dim)/nodes_per_dim;

	if (node<servers) {
        if (port>6) {
			res.node=-1; // disconnected, should never arrive here!
			res.port=-1;
		} else if (port==6){ // uplink
		    if ( uplinks_per_torus==1 ) {
				if ((node%nodes_torus)==0) { // only node (0, 0, 0) is uplinked
					pos=node/nodes_torus;
					res.node=servers+(pos/param_down[0]);
					res.port=(pos%param_down[0]);
				} else{
					res.node=-1; // this node is not connected to the ToR network
					res.port=-1;
				}
			} else if ( uplinks_per_torus==2) {
				 if((node%nodes_torus)==0) { // node (0, 0, 0) is uplinked
					pos=2*(node/nodes_torus);
					res.node=servers+(pos/param_down[0]);
					res.port=(pos%param_down[0]);
				} else if (x==nodes_per_dim/2 && y==nodes_per_dim/2 && z==nodes_per_dim/2) {// connect also node (x/2, y/2, z/2)
					pos=2*(node/nodes_torus)+1;
					res.node=servers+(pos/param_down[0]);
					res.port=(pos%param_down[0]);
				} else{
					res.node=-1; // this node is not connected to the ToR network
					res.port=-1;
				}
			} else if  (2*uplinks_per_torus==nodes_torus){	// Half of the nodes are connected to the upper layer
            	if ((node%2) == 0) { // every other node is uplinked (even nodes)
					pos=(node/2);
					res.node=servers+(pos/param_down[0]);
					res.port=(pos%param_down[0]);
				} else {
					res.node=-1; // this node is not connected to the ToR network
					res.port=-1;
				}
            } else if  (4*uplinks_per_torus==nodes_torus){  // One quarter of the nodes are connected to the upper layer
            	if ( (x%2) == (y%2) && (x%2) == (z%2) ) { // 1/4 of the nodes are uplinked (nodes in the <0,0,0> and <1,1,1> corners of a 2x2x2 subtorus)
					pos=(t*uplinks_per_torus) + ((x%2)*(uplinks_per_torus/2)) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
					res.node=servers+(pos/param_down[0]);
					res.port=(pos%param_down[0]);
				} else {
					res.node=-1; // this node is not connected to the ToR network
					res.port=-1;
				}
            } else if (uplinks_per_torus> 2 && (x%(nodes_per_dim/uplinks_per_dim)==0 && y%(nodes_per_dim/uplinks_per_dim)==0 && z%(nodes_per_dim/uplinks_per_dim)==0 )) { // for more uplinks, connect only if required by the connection rule (one link every uplinks_per_dim in each dimension to end up in a checkered pattern
                pos=(t*uplinks_per_torus) + (x/(nodes_per_dim/uplinks_per_dim)) + (y/(nodes_per_dim/uplinks_per_dim))*uplinks_per_dim + (z/(nodes_per_dim/uplinks_per_dim))*uplinks_per_dim*uplinks_per_dim;
                res.node=servers+(pos/param_down[0]);
                res.port=(pos%param_down[0]);
		    } else {
                res.node=-1; // this node is not connected to the ToR network
                res.port=-1;
		    }
		} else {
            switch(port){
                case 0: // X+
                    x=(x+1)%nodes_per_dim;
                    p=1;
                    break;
                case 1: // X-
                    x=(x+nodes_per_dim-1)%nodes_per_dim;
                    p=0;
                    break;
                case 2: // Y+
                    y=(y+1)%nodes_per_dim;
                    p=3;
                    break;
                case 3: // Y-
                    y=(y+nodes_per_dim-1)%nodes_per_dim;
                    p=2;
                    break;
                case 4: // Z+
                    z=(z+1)%nodes_per_dim;
                    p=5;
                    break;
                case 5: // Z-
                    p=4;
                    z=(z+nodes_per_dim-1)%nodes_per_dim;
                    break;
                default: // Should never get here
                    perror("BIG FAT ERROR!!!");
                    exit(-1);
            }
            res.node=(t*nodes_torus)+x+(y*nodes_per_dim)+(z*nodes_per_dim*nodes_per_dim);
            res.port=p;
		}
	} else {
	    pos=node-servers;
	    lvl=0;
        nl_first=servers;
	    while (pos>=sw_per_stage[lvl]){
            pos-=sw_per_stage[lvl];
            nl_first+=sw_per_stage[lvl];
            lvl++;
	    }
		if (lvl==param_k-1 && port>=param_down[lvl]) {   //disconnected links in the last stage of the nesttree (param_up[last] should be 0 any way!!!)
			res.node=-1;
			res.port=-1;
		} else if (lvl==0 && port<param_down[lvl] ) { // connected to server
		    long tmp=port+(pos*down_pow[1]);  // overall position in the tree

            if (uplinks_per_torus==1) {
                res.node=(tmp*nodes_torus);
                res.port=6;
            } else if (uplinks_per_torus==2) {
                if (tmp%2==0){
                    res.node=(tmp*nodes_torus)/2;
                    res.port=6;
                } else {
                    res.node=(nodes_per_dim/2) + (nodes_per_dim*nodes_per_dim/2) + (nodes_per_dim*nodes_per_dim*nodes_per_dim/2) + ((tmp/2)*nodes_torus);
                    res.port=6;
                }
            } else if (2*uplinks_per_torus==nodes_torus){	// Half of the nodes are connected to the upper layer
				res.node=(tmp*2);
				res.port=6;
			} else if  (4*uplinks_per_torus==nodes_torus){  // One quarter of the nodes are connected to the upper layer
				t=tmp/uplinks_per_torus; // number of subtorus
				tmp=tmp%uplinks_per_torus;
				if (tmp<(uplinks_per_torus/2)){
					x=(tmp%(nodes_per_dim/2))*2;
					y=((tmp/(nodes_per_dim/2))%(nodes_per_dim/2))*2;
					z=((tmp/(nodes_per_dim/2))/(nodes_per_dim/2))*2;
				} else {
					tmp=tmp-(uplinks_per_torus/2);
					x=(tmp%(nodes_per_dim/2))*2 + 1;
					y=((tmp/(nodes_per_dim/2))%(nodes_per_dim/2))*2 + 1;
					z=((tmp/(nodes_per_dim/2))/(nodes_per_dim/2))*2 + 1;
				}
				res.node=(t*nodes_torus) + x + (y*nodes_per_dim) + (z*nodes_per_dim*nodes_per_dim);
				res.port=6;
            } else {
                t=tmp/uplinks_per_torus; // number of subtorus
                tmp=tmp%uplinks_per_torus;
                x=(tmp%uplinks_per_dim)*(nodes_per_dim/uplinks_per_dim);
                y=((tmp/uplinks_per_dim)%uplinks_per_dim)*(nodes_per_dim/uplinks_per_dim);
                z=((tmp/uplinks_per_dim)/uplinks_per_dim)*(nodes_per_dim/uplinks_per_dim);

                //printf("tmp:%ld...[%ld], %ld, %ld, %ld\n",tmp,t,x,y,z);

                res.node=(t*nodes_torus) + x + (y*nodes_per_dim) + (z*nodes_per_dim*nodes_per_dim);
                res.port=6;
            }
		} else if (port>=param_down[lvl]) { //upwards port
			long p=port-param_down[lvl];
            nl_first+= sw_per_stage[lvl]; // we are connecting up, so the neighbouring level is above, we need to add the number of switches in this level
			res.node = (p*up_pow[lvl]) + (mod(pos,up_pow[lvl])) + ((pos/(param_down[lvl+1]*up_pow[lvl]))*up_pow[lvl+1]) + nl_first;
			res.port = (mod(pos/up_pow[lvl],param_down[lvl+1]));
		} else { //downwards port
			nl_first-=sw_per_stage[lvl-1]; // we are connecting down, so the neighbouring level is below, we need to substract the number of switches in the previous level
			res.node = (port*up_pow[lvl-1]) + (mod(pos,up_pow[lvl-1])) + ((pos/up_pow[lvl])*up_pow[lvl-1]*param_down[lvl])  + nl_first;
			res.port = param_down[lvl-1] + mod(pos/up_pow[lvl-1], param_up[lvl-1]);
        }
	}
	return res;
}

long is_server_nesttree(long i)
{
	return (i<servers);
}

char * get_network_token_nesttree()
{
	return network_token;
}

char * get_routing_token_nesttree()
{
	return routing_token;
}

char *get_routing_param_tokens_nesttree(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_nesttree()
{
	return topo_version;
}

char * get_topo_param_tokens_nesttree(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_nesttree()
{
	return filename_params;
}

long get_server_i_nesttree(long i)
{
	return i;
}

long get_switch_i_nesttree(long i)
{
	return servers+i;
}

long node_to_server_nesttree(long i)
{
	return i;
}

long node_to_switch_nesttree(long i)
{
	return i-servers;
}

long get_ports_nesttree()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_nesttree(long src, long dst)
{
    return (1);
}

/**
* Simple oblivious UP/DOWN. Others can be implemented.
*/
long init_routing_nesttree_static(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=6; //first hop is away from server, so no option to choose.
	for (i=1; i<mca; i++) { // Choose option based on source server, ensures load balancing.
		cur_route[i]=param_down[i-1]+((src/down_pow[i-1]) % param_up[i-1]);
	}

	for (i=0; i<mca; i++) {
		cur_route[mca+i]=(dst/down_pow[mca-1-i]) % param_down[mca-1-i];
	}

	cur_hop=0;
	return 0;
}

/**
* Simple randomized UP/DOWN. Multipath-capable
*/
long init_routing_nesttree_random(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;
	long p=rand();	// a random number used to select the path to use

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=6; //first hop is away from server, so no option to choose.
	for (i=1; i<mca; i++) { // Choose option based on source server, ensures load balancing.
		cur_route[i]=param_down[i-1]+((p/down_pow[i-1]) % param_up[i-1]);
	}

	for (i=0; i<mca; i++) {
		cur_route[mca+i]=(dst/down_pow[mca-1-i]) % param_down[mca-1-i];
	}

	cur_hop=0;
	return 0;
}

/**
* Simple UP/DOWN with round robin between all possible paths. Multipath-capable
*/
long init_routing_nesttree_roundrobin(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=6; //first hop is away from server, so no option to choose.
	for (i=1; i<mca; i++) { // Choose option based on source server, ensures load balancing.
		cur_route[i]=param_down[i-1]+((path_index[src]/down_pow[i-1]) % param_up[i-1]);
	}

	for (i=0; i<mca; i++) {
		cur_route[mca+i]=(dst/down_pow[mca-1-i]) % param_down[mca-1-i];
	}
	path_index[src]=(path_index[src]+1)%(servers*uplinks_per_torus/nodes_torus);
	cur_hop=0;
	return 0;
}

static void route_subtorus(long src, long dst, long *h){
    long i,s,d;

    for (i=0; i<3; i++) {
		s=src%nodes_per_dim;
		src=src/nodes_per_dim;

		d=dst%nodes_per_dim;
		dst=dst/nodes_per_dim;

		h[i]= d - s;

		if (h[i] > nodes_per_dim/2)
			h[i]-=nodes_per_dim;
		else if (h[i] < -(nodes_per_dim/2))
			h[i]+=nodes_per_dim;
	}
}
/**
* Routing selector. Others can be implemented.
*/
long init_routing_nesttree(long src, long dst)
{
    long i, j, k;
    long t_hops[3];

    for (i=0; i<3; i++){
        d_hops[i]=0;
        s_hops[i]=0;
    }

    if (src/nodes_torus == dst/nodes_torus){
        route_subtorus(src, dst, d_hops);
        return 0;
    }

	if (uplinks_per_torus==1){
        inter_src=src/nodes_torus; // position in the tree
        route_subtorus(src, inter_src*nodes_torus, s_hops);

        inter_dst=dst/nodes_torus;
        route_subtorus(inter_dst*nodes_torus, dst, d_hops);

	} else if (uplinks_per_torus==2){
        /// Routing in the torus from the source to the uplink
        inter_src=(src/nodes_torus);
        route_subtorus(src, inter_src*nodes_torus, s_hops);
        route_subtorus(src, (inter_src*nodes_torus)+ (nodes_per_dim/2) + (nodes_per_dim/2)*nodes_per_dim + (nodes_per_dim/2)*nodes_per_dim*nodes_per_dim, t_hops);

        if (abs(s_hops[0])+abs(s_hops[1])+abs(s_hops[2]) > abs(t_hops[0])+abs(t_hops[1])+abs(t_hops[2])){
            for (i=0; i<3; i++)
                s_hops[i]=t_hops[i];
            inter_src = 1 + (inter_src *2);
        } else {
            inter_src = inter_src * 2;
        }

        /// Routing in the torus from the uplink to the destination
        inter_dst=(dst/nodes_torus);
        route_subtorus(inter_dst*nodes_torus, dst, d_hops);
        route_subtorus((inter_src*nodes_torus)+ (nodes_per_dim/2) + (nodes_per_dim/2)*nodes_per_dim + (nodes_per_dim/2)*nodes_per_dim*nodes_per_dim, dst, t_hops);

        if (abs(d_hops[0])+abs(d_hops[1])+abs(d_hops[2]) > abs(t_hops[0])+abs(t_hops[1])+abs(t_hops[2])){
            for (i=0; i<3; i++)
                d_hops[i]=t_hops[i];
            inter_dst = 1 + (inter_dst *2);
        } else {
            inter_dst = inter_dst * 2;
        }

	} else if  (2*uplinks_per_torus==nodes_torus){
		if ((src%2)!=0)
			s_hops[0]=-1;
		if ((dst%2)!=0)
			d_hops[0]=1;
		inter_src=src/2; // position in the tree
        inter_dst=dst/2;
	} else if  (4*uplinks_per_torus==nodes_torus){
		long  t; // local subtorus
		long  x,  y,  z; //coordinates of the node in its subtorus
		long pos;

		t=src/nodes_torus;
		pos=src%nodes_torus;
		x=pos%nodes_per_dim;
		y=(pos/nodes_per_dim)%nodes_per_dim;
		z=((pos/nodes_per_dim)/nodes_per_dim)%nodes_per_dim;

		if (x%2==0 && y%2==0 && z%2==0) {
			inter_src=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==0 && z%2==0) {
			s_hops[0]=-1;
			inter_src=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==0 && y%2==1 && z%2==0) {
			s_hops[1]=-1;
			inter_src=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==0 && y%2==0 && z%2==1) {
			s_hops[2]=-1;
			inter_src=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==0 && y%2==1 && z%2==1) {
			s_hops[0]=1;
			inter_src=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==0 && z%2==1) {
			s_hops[1]=1;
			inter_src=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==1 && z%2==0) {
			s_hops[2]=1;
			inter_src=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==1 && z%2==1) {
			inter_src=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		}

		t=dst/nodes_torus;
		pos=dst%nodes_torus;
		x=pos%nodes_per_dim;
		y=(pos/nodes_per_dim)%nodes_per_dim;
		z=((pos/nodes_per_dim)/nodes_per_dim)%nodes_per_dim;

		if (x%2==0 && y%2==0 && z%2==0) {
			inter_dst=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==0 && z%2==0) {
			d_hops[0]=1;
			inter_dst=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==0 && y%2==1 && z%2==0) {
			d_hops[1]=1;
			inter_dst=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==0 && y%2==0 && z%2==1) {
			d_hops[2]=1;
			inter_dst=(t*uplinks_per_torus) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==0 && y%2==1 && z%2==1) {
			d_hops[0]=-1;
			inter_dst=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==0 && z%2==1) {
			d_hops[1]=-1;
			inter_dst=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==1 && z%2==0) {
			d_hops[2]=-1;
			inter_dst=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		} else if (x%2==1 && y%2==1 && z%2==1) {
			inter_dst=(t*uplinks_per_torus) + (uplinks_per_torus/2) + (x/2) + (nodes_per_dim/2)*(y/2) + (nodes_per_dim/2)*(nodes_per_dim/2)*(z/2);
		}
	} else {
	    long a=0,n;
	    inter_src=(src/nodes_torus);
        route_subtorus(src, inter_src*nodes_torus, s_hops);
        for (i=0; i< uplinks_per_dim; i++){
            for (j=0; j< uplinks_per_dim; j++){
                for (k=0; k< uplinks_per_dim; k++)
                {
                    route_subtorus(src, (inter_src*nodes_torus) + (i*nodes_per_dim/uplinks_per_dim) + (j*nodes_per_dim/uplinks_per_dim)*nodes_per_dim + (k*nodes_per_dim/uplinks_per_dim)*nodes_per_dim*nodes_per_dim, t_hops);
                    if (abs(s_hops[0])+abs(s_hops[1])+abs(s_hops[2]) > abs(t_hops[0])+abs(t_hops[1])+abs(t_hops[2])){
                        for (n=0; n<3; n++)
                            s_hops[n]=t_hops[n];
                        a = i + (j*uplinks_per_dim) + (k*uplinks_per_dim*uplinks_per_dim);
                    }
                }
            }
        }
        inter_src=(inter_src*uplinks_per_torus) + a;

        a=0;
        inter_dst=(dst/nodes_torus);
        route_subtorus(inter_dst*nodes_torus, dst, d_hops);
        for (i=0; i< uplinks_per_dim; i++){
            for (j=0; j< uplinks_per_dim; j++){
                for (k=0; k< uplinks_per_dim; k++)
                {
                    route_subtorus((inter_dst*nodes_torus) + (i*nodes_per_dim/uplinks_per_dim) + (j*nodes_per_dim/uplinks_per_dim)*nodes_per_dim + (k*nodes_per_dim/uplinks_per_dim)*nodes_per_dim*nodes_per_dim, dst, t_hops);
                    if (abs(d_hops[0])+abs(d_hops[1])+abs(d_hops[2]) > abs(t_hops[0])+abs(t_hops[1])+abs(t_hops[2])){
                        for (n=0; n<3; n++)
                            d_hops[n]=t_hops[n];
                        a = i + (j*uplinks_per_dim) + (k*uplinks_per_dim*uplinks_per_dim);
                    }
                }
            }
        }
        inter_dst=(inter_dst*uplinks_per_torus) + a;
	}

//	switch(routing){
//	case TREE_STATIC_ROUTING:
		return init_routing_nesttree_static(inter_src, inter_dst);
//		break;
//    case TREE_RND_ROUTING:
//		return init_routing_nesttree_random(inter_src, inter_dst);
//		break;
//    case TREE_RR_ROUTING:
//    	return init_routing_nesttree_roundrobin(inter_src, inter_dst);
//		break;
//	default:
//		printf("Not a tree-compatible routing %d", routing);
//		exit(-1);
//	}
}

void finish_route_nesttree()
{

}

/**
* Return current hop. Calculated in init_routing
*/
long route_nesttree(long current, long destination)
{
    if (current/nodes_torus == destination/nodes_torus) {
        if (d_hops[0]>0){
            d_hops[0]--;
            return 0;
        }
        if (d_hops[0]<0){
            d_hops[0]++;
            return 1;
        }
        if (d_hops[1]>0){
            d_hops[1]--;
            return 2;
        }
        if (d_hops[1]<0){
            d_hops[1]++;
            return 3;
        }
        if (d_hops[2]>0){
            d_hops[2]--;
            return 4;
        }
        if (d_hops[2]<0){
            d_hops[2]++;
            return 5;
        }
    } if (current < servers) {
        if (s_hops[0]>0){
            s_hops[0]--;
            return 0;
        }
        if (s_hops[0]<0){
            s_hops[0]++;
            return 1;
        }
        if (s_hops[1]>0){
            s_hops[1]--;
            return 2;
        }
        if (s_hops[1]<0){
            s_hops[1]++;
            return 3;
        }
        if (s_hops[2]>0){
            s_hops[2]--;
            return 4;
        }
        if (s_hops[2]<0){
            s_hops[2]++;
            return 5;
        }
    }
    return cur_route[cur_hop++];
}
