/** @mainpage
thintree topology
*/

#include <stdlib.h>
#include <stdio.h>

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "../inrflow/globals.h"
#include "thintree.h"

static long param_k;	///< parameter k of the topology, typically number of stages
static long param_down;	///< Number of downwards ports
static long param_up;  ///< number of upwards ports

static long *down_pow;	///< an array with the down^i, useful for doing some calculations.
static long *up_pow;	///< an array with the up^i, useful for doing some calculations.
static long servers; 	///< The total number of servers : n^k
static long switches;	///< The total number of switches : k*n^(k-1)
static long ports;		///< The total number of links
static long *sw_per_stage;  ///< Switches per stage
static long last_stage;    ///< id of the first switch in the last stage

static long *cur_route;    ///< Array to store the current route
static long cur_hop;   ///< Current hop in this route
static long max_paths;	///< Maximum number of paths to generate when using multipath
static long *path_index;	///< per-node index, used for round robin routing

static char* network_token="thintree";
static char routing_token[20];
static char* topo_version="v0.1";
static char* topo_param_tokens[3]= {"down","up", "stages"};
//AE: make error checking so that we don't overrun this buffer
extern char filename_params[300];
static char *routing_param_tokens[1]= {"max_paths"};

/**
* Initializes the topology and sets the parameters k & n.
*/
long init_topo_thintree(long np, long* par)
{
	long i;

	if (np<3) {
		printf("3 parameters are needed for thintree <down ports, up ports, stages>\n");
		exit(-1);
	}
	param_down=par[0];
	param_up=par[1];
	param_k=par[2];
	sprintf(filename_params,"down%ldup%ldk%ld",param_down,param_up,param_k);
	up_pow=malloc((param_k+1)*sizeof(long));
	down_pow=malloc((param_k+1)*sizeof(long));
	cur_route=malloc(2*param_k*sizeof(long));   // UP*/DOWN routes cannot be longer than 2*k
	sw_per_stage=malloc(param_k*sizeof(long));
	down_pow[0]=1;
    up_pow[0]=1;

	for (i=1; i<=param_k; i++) {
		down_pow[i]=down_pow[i-1]*param_down;
		up_pow[i]=up_pow[i-1]*param_up;
	} // powers of up and down will be useful throughout,so let's compute them just once.

    switches=0;
	for (i=0; i<param_k; i++) {
		sw_per_stage[i]=down_pow[param_k-i-1]*up_pow[i];
		switches+=sw_per_stage[i];
	}

	servers=down_pow[param_k];
	ports=(switches*(param_down+param_up))-(sw_per_stage[param_k-1]*param_up)+(servers);

    last_stage=servers+switches-sw_per_stage[param_k-1];

	switch(routing){
	case TREE_STATIC_ROUTING:
		snprintf(routing_token,20,"tree-static");
		break;
    case TREE_RND_ROUTING:
		snprintf(routing_token,20,"tree-random");
		break;
    case TREE_RR_ROUTING:
		snprintf(routing_token,20,"tree-roundrobin");
		path_index=malloc(servers*sizeof(long));
		for (i=0;i<servers; i++)
			path_index[i]=i;
		break;
	default:
		printf("Not a tree-compatible routing %d", routing);
		exit(-1);
    }

	if(routing_nparam>0){
		max_paths=routing_params[0];
	} else {
		max_paths=1;
	}

	return 1; //Return status, not used here.
}

/**
* Release the resources used by the topology.
**/
void finish_topo_thintree()
{
	free(cur_route);
	free(up_pow);
	free(down_pow);
	free(sw_per_stage);
	if (path_index!=NULL)
		free(path_index);
}

/**
* Get the number of servers of the network
*/
long get_servers_thintree()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_thintree()
{
	return switches;
}

/**
* Get the number of ports of a given node (either a server or a switch, see above)
*/
long get_radix_thintree(long n)
{
	if (n<servers)
		return 1;	// If this is a server it has 1 port
	else if (n>=last_stage) // this is a switch in the last stage (so upwards ports are disconnected.
		return param_down;
	else
		return param_down+param_up; // If this is a switch it has n ports in each direction
}

/**
* Calculates connections
*/
tuple_t connection_thintree(long node, long port)
{
	tuple_t res;
	long lvl;
	long pos;
	long nl_first; // id of the NEIGHBOURING level first switch

	if (node<servers) {
		if (port!=0) {
			res.node=-1; // switch 'sw' in level 'port'
			res.port=-1;
		} else {
			res.node=servers+(node/param_down);
			res.port=(node%param_down);
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
		if (lvl==param_k-1 && port>=param_down) {   //disconnected links in the last stage of the thintree (for regularity, could be connected to other subtrees)
			res.node=-1;
			res.port=-1;
		} else if (port<param_down && lvl==0) { // connected to server
			res.node=port+(pos*down_pow[1]);
			res.port=0;
		} else if (port>=param_down) { //upward port
			long p=port-param_down;
            nl_first=nl_first+sw_per_stage[lvl]; // we are connecting up, so the neighbouring level is above, we need to add the number of switches in this level

			res.node = (p*up_pow[lvl]) + (mod(pos,up_pow[lvl])) + ((pos/(param_down*up_pow[lvl]))*up_pow[lvl+1]) + nl_first;
			res.port = (mod(pos/up_pow[lvl],param_down));
		} else { //downwards port
			nl_first-=sw_per_stage[lvl-1]; // we are connecting down, so the neighbouring level is below, we need to substract the number of switches in the previous level
			res.node = (port*up_pow[lvl-1]) + (mod(pos,up_pow[lvl-1])) + ((pos/up_pow[lvl])*up_pow[lvl-1]*param_down)  + nl_first;
			res.port = param_down + mod(pos/up_pow[lvl-1], param_up);
        }
	}
	return res;
}

long is_server_thintree(long i)
{
	return (i<servers);
}


char * get_network_token_thintree()
{
	return network_token;
}

char * get_routing_token_thintree()
{
	return routing_token;
}

char *get_routing_param_tokens_thintree(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_thintree()
{
	return topo_version;
}

char * get_topo_param_tokens_thintree(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_thintree()
{
	return filename_params;
}

long get_server_i_thintree(long i)
{
	return i;
}

long get_switch_i_thintree(long i)
{
	return servers+i;
}

long node_to_server_thintree(long i)
{
	return i;
}

long node_to_switch_thintree(long i)
{
	return i-servers;
}

long get_ports_thintree()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_thintree(long src, long dst){
	long mca=0;

	switch(routing){
	case TREE_STATIC_ROUTING:
		return (1);
		break;
    case TREE_RND_ROUTING:
    case TREE_RR_ROUTING:
    	while (src/down_pow[mca]!=dst/down_pow[mca]) {
			mca++;
		}
    	return max(min(up_pow[mca],max_paths),1);
		break;
	default:
		printf("Not a tree-compatible routing %d", routing);
		exit(-1);
	}
}

/**
* Simple oblivious UP/DOWN. Others can be implemented.
*/
long init_routing_thintree_static(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=0; //first hop is away from server, so no option to choose.
	for (i=1; i<mca; i++) { // Choose option based on source server, ensures load balancing.
		cur_route[i]=param_down+((src/down_pow[i-1]) % param_up);
	}

	for (i=0; i<mca; i++) {
		cur_route[mca+i]=(dst/down_pow[mca-1-i]) % param_down;
	}

	cur_hop=0;
	return 0;
}

/**
* Simple randomized UP/DOWN. Multipath-capable
*/
long init_routing_thintree_random(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;
	long p=rand();

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=0; //first hop is away from server, so no option to choose.
	for (i=1; i<mca; i++) { // Choose option based on source server, ensures load balancing.
		cur_route[i]=param_down+((p/down_pow[i-1]) % param_up);
	}

	for (i=0; i<mca; i++) {
		cur_route[mca+i]=(dst/down_pow[mca-1-i]) % param_down;
	}

	cur_hop=0;
	return 0;
}

/**
* Simple UP/DOWN with round robin between all possible paths. Multipath-capable
*/
long init_routing_thintree_roundrobin(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=0; //first hop is away from server, so no option to choose.
	for (i=1; i<mca; i++) { // Choose option based on source server, ensures load balancing.
		cur_route[i]=param_down+((path_index[src]/down_pow[i-1]) % param_up);
	}

	for (i=0; i<mca; i++) {
		cur_route[mca+i]=(dst/down_pow[mca-1-i]) % param_down;
	}

	cur_hop=0;
	path_index[src]=(path_index[src]+1)%servers;
	return 0;
}

/**
* Routing selector. Others can be implemented.
*/
long init_routing_thintree(long src, long dst)
{
	switch(routing){
	case TREE_STATIC_ROUTING:
		return init_routing_thintree_static(src, dst);
		break;
    case TREE_RND_ROUTING:
		return init_routing_thintree_random(src, dst);
		break;
    case TREE_RR_ROUTING:
    	return init_routing_thintree_roundrobin(src, dst);
		break;
	default:
		printf("Not a tree-compatible routing %d", routing);
		exit(-1);
	}
}

void finish_route_thintree()
{

}
/**
* Return current hop. Calculated in init_routing
*/
long route_thintree(long current, long destination)
{
	return cur_route[cur_hop++];
}
