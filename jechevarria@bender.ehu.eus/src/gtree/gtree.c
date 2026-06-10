/** @mainpage
generalised tree topology <stages, (ports_down, ports_up)^stages>
*/

#include <stdlib.h>
#include <stdio.h>

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "../inrflow/globals.h"
#include "gtree.h"

static long param_k;	///< parameter k of the topology, number of stages
static long *param_down;	///< Number of downwards ports in each level
static long *param_up;  ///< number of upwards ports in each level

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

static char* network_token="gtree";
static char routing_token[20];
static char* topo_version="v0.1";
static char* topo_param_tokens[11]= {"stages","down0","up0","down1","up1","down2","up2","down3","up3","down4","up4"};
//AE: make error checking so that we don't overrun this buffer
extern char filename_params[100];
static char *routing_param_tokens[1]= {"max_paths"};


/**
* Initializes the topology and sets the parameters k & n.
*/
long init_topo_gtree(long np, long* par)
{
	long i,j, c;
	long buffer_length;

	if (np<1) {
		printf("parameters needed\n");
		exit(-1);
	}
	param_k=par[0];
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
	c=1;
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
	servers=down_pow[param_k];

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
void finish_topo_gtree()
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
long get_servers_gtree()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_gtree()
{
	return switches;
}

/**
* Get the number of ports of a given node (either a server or a switch, see above)
*/
long get_radix_gtree(long n)
{
	int i=0;
	if (n<servers)
		return 1;	// If this is a server it has 1 port
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
tuple_t connection_gtree(long node, long port)
{
	tuple_t res;
	long lvl;
	long pos;
	long nl_first; // id of the first switch in the NEIGHBOURING level

	if (node<servers) {
		if (port!=0) {
			//printf("gtree servers only have 1 port!!!");
			res.node=-1; // switch 'sw' in level 'port'
			res.port=-1;
		} else {
			res.node=servers+(node/param_down[0]);
			res.port=(node%param_down[0]);
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
		if (lvl==param_k-1 && port>=param_down[lvl]) {   //disconnected links in the last stage of the gtree (param_up[last] should be 0 any way!!!)
			res.node=-1;
			res.port=-1;
		} else if (lvl==0 && port<param_down[lvl] ) { // connected to server
			res.node=port+(pos*down_pow[1]);
			res.port=0;
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

long is_server_gtree(long i)
{
	return (i<servers);
}

char * get_network_token_gtree()
{
	return network_token;
}

char * get_routing_token_gtree()
{
	return routing_token;
}

char *get_routing_param_tokens_gtree(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_gtree()
{
	return topo_version;
}

char * get_topo_param_tokens_gtree(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_gtree()
{
	return filename_params;
}

long get_server_i_gtree(long i)
{
	return i;
}

long get_switch_i_gtree(long i)
{
	return servers+i;
}

long node_to_server_gtree(long i)
{
	return i;
}

long node_to_switch_gtree(long i)
{
	return i-servers;
}

long get_ports_gtree()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_gtree(long src, long dst)
{
	long i, mca=0;

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
		printf("Not a tree-compatible routing %d, switching to tree-roundrobin\n", routing);
		routing=TREE_RR_ROUTING;
		snprintf(routing_token,20,"tree-roundrobin");
		path_index=malloc(servers*sizeof(long));
		for (i=0;i<servers; i++)
			path_index[i]=i;
		break;
	}
}

/**
* Simple oblivious UP/DOWN. Others can be implemented.
*/
long init_routing_gtree_static(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=0; //first hop is away from server, so no option to choose.
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
long init_routing_gtree_random(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;
	long p=rand();	// a random number used to select the path to use

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=0; //first hop is away from server, so no option to choose.
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
long init_routing_gtree_roundrobin(long src, long dst)
{
	long mca=0; // minimum common ancestor (levels)
	long i;

	while (src/down_pow[mca]!=dst/down_pow[mca]) {
		mca++;
	}

	cur_route[0]=0; //first hop is away from server, so no option to choose.
	for (i=1; i<mca; i++) { // Choose option based on source server, ensures load balancing.
		cur_route[i]=param_down[i-1]+((path_index[src]/down_pow[i-1]) % param_up[i-1]);
	}

	for (i=0; i<mca; i++) {
		cur_route[mca+i]=(dst/down_pow[mca-1-i]) % param_down[mca-1-i];
	}
	path_index[src]=(path_index[src]+1)%servers;
	cur_hop=0;
	return 0;
}

/**
* Routing selector. Others can be implemented.
*/
long init_routing_gtree(long src, long dst)
{
	switch(routing){
	case TREE_STATIC_ROUTING:
		return init_routing_gtree_static(src, dst);
		break;
    case TREE_RND_ROUTING:
		return init_routing_gtree_random(src, dst);
		break;
    case TREE_RR_ROUTING:
    	return init_routing_gtree_roundrobin(src, dst);
		break;
	default:
		printf("Not a tree-compatible routing %d", routing);
		exit(-1);
	}
}

void finish_route_gtree()
{

}

/**
* Return current hop. Calculated in init_routing
*/
long route_gtree(long current, long destination)
{
	return cur_route[cur_hop++];
}
