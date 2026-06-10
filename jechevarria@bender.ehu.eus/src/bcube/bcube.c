/** @mainpage
BCube topology
*/

#include <stdlib.h>
#include <stdio.h>

#include "../inrflow/node.h"

static long param_k;	///< parameter k of the topology, typically number of levels/dimensions
static long param_n;	///< parameter n of the topology, switch radix

static long *n_pow;	///< an array with the n^i, useful for doing some calculations.
static long servers; 	///< The total number of servers : n^k
static long switches;	///< The total number of switches : k*n^(k-1)
static long ports;		///< The total number of links

static long *srcb; ///< The coordinates for the current
static long *dstb; ///< The coordinates for the destination
static char* network_token="bcube";
static char* routing_token="dimensional";
static char* topo_version="v0.1";
static char* topo_param_tokens[2]= {"n","k"};
//AE: make error checking so that we don't overrun this buffer

extern node_t *network;
extern char filename_params[100];

/**
* Initializes the topology and sets the parameters k & n.
*/
long init_topo_bcube(long np, long* par)
{
	long i;

	if (np<2) {
		printf("2 parameters are needed for BCUBE <n, k>\n");
		exit(-1);
	}
	param_n=par[0];
	param_k=par[1];
	sprintf(filename_params,"n%ldk%ld",param_n,param_k);
	n_pow=malloc((param_k+1)*sizeof(long));
	n_pow[0]=1;

	for (i=1; i<=param_k; i++) {
		n_pow[i]=n_pow[i-1]*param_n;
	} // powers of n will be useful throughout,so let's compute them just once.

	servers=n_pow[param_k];
	switches=n_pow[param_k-1]*param_k;
	ports=servers*param_k*2;

	srcb=malloc(param_k*sizeof(long));
	dstb=malloc(param_k*sizeof(long));

	return 1; // return status, not used here
}

/**
* Release the resources used by the topology.
**/
void finish_topo_bcube()
{
	free(n_pow);
	free(srcb);
	free(dstb);
}


/**
* Get the number of servers of the network
*/
long get_servers_bcube()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_bcube()
{
	return switches;
}

// Get the number of ports of a given node (either a server or a switch, see above)
long get_radix_bcube(long n)
{

	if (n<servers)
		return param_k;	// If this is a server it has k ports
	else
		return param_n; // If this is a switch it has n ports
}

/**
* Calculates connections
*/
tuple_t connection_bcube(long node, long port)
{
	tuple_t res;
	long i;
	long sw=0;
	long lvl;
	long pos;

	if (node<servers) {
		for(i=param_k-1; i>=0; i--) {
			srcb[i]=(node/n_pow[i])%param_n; // calculate coordinate in dimension i:[0, k)
			if (i!=port) {
				sw=(sw*param_n)+srcb[i];
			}
		}
		res.node=servers+(port*n_pow[param_k-1])+sw; // switch 'sw' in level 'port'
		res.port=srcb[port];
	} else {
		lvl=(node-servers)/n_pow[param_k-1]; // level of the switch
		pos=(node-servers)%n_pow[param_k-1]; // position of the switch in that level

		res.node=((pos/n_pow[lvl])*n_pow[lvl+1]) + (pos%n_pow[lvl])+(port*n_pow[lvl]);
		res.port=lvl;
	}
	return res;
}

long is_server_bcube(long i)
{
	return (i<servers);
}


char * get_network_token_bcube()
{
	return network_token;
}

char * get_routing_token_bcube()
{
	return routing_token;
}

char * get_topo_version_bcube()
{
	return topo_version;
}


char * get_topo_param_tokens_bcube(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_bcube()
{
	return filename_params;
}

long get_server_i_bcube(long i)
{
	return i;
}

long get_switch_i_bcube(long i)
{
	return servers+i;
}


long node_to_server_bcube(long i)
{
	return i;
}

long node_to_switch_bcube(long i)
{
	return i-servers;
}

long get_ports_bcube()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_bcube(long src, long dst){

    return(1);
}

long init_routing_bcube(long s, long d)
{
	return 0;
}

void finish_route_bcube()
{
}
/**
* Simple oblivious DOR. Others can be implemented.
*/
long route_bcube(long current, long destination)
{
	long port=0;
	long curr_node=current;
	long lvl;

#ifdef DEBUG
	if(current==destination) {
		printf("should not be routing a packet that has arrived to its destination (curr: %d, dstb: %d)!\n", current, destination);
		return -1; // just in case, let's abort the routing
	}
#endif // DEBUG

	if(current<servers) { // this is a server{
		while( (current % param_n) == (destination % param_n) ) {
			port++;
			current = current/param_n;
			destination=destination/param_n;
		}
	} else { // this is switch
		lvl=(current-servers)/n_pow[param_k-1]; // level of the switch
		//pos=(current-servers)%n_pow[param_k-1]; // position of the switch in that level
		port=(destination/n_pow[lvl])%param_n;
	}
	return (network[curr_node].port[port].faulty)?-1:port;
}

