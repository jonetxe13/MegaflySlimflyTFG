/** @mainpage
torus and mesh topologies

Note the mesh is implemented as a routing function over a full torus topology, so connections and radices will be those of a torus.
*/
#include "../inrflow/literal.h"
#include <stdlib.h>
#include <stdio.h>
#include "../inrflow/node.h"

static long dimensions;	///< number of dimensions
static long *nodes_dim;	///< number of nodes per dimension
static long *routing_hops;	///< number of hops per dimension in this route
static long *src_torus;	///< coordinates of the source
static long *dst_torus;	///< coordinates of the destination

static long servers; 	///< The total number of servers
static long ports;		///< The total number of links

static char* network_token="torus";
static char* routing_token="dimensional";
static char* topo_version="v0.1";
static char* topo_param_tokens[11]= {"X","Y","Z","A","B","C","D","E","F","G","H"}; // Any way of doing this better?
//AE: make error checking so that we don't overrun this buffer
static char *routing_param_tokens[1]= {"max_paths"};
extern char filename_params[100];

/**
* Initializes the topology and sets the dimensions.
*/
long init_topo_torus(long np, long* par)
{
	long i,k=0;

	if (np<1) {
		printf("At least 1 parameter is  needed for torus <nodes_per_dim>*\n");
		exit(-1);
	}
	dimensions=np;
	nodes_dim=malloc(dimensions*sizeof(long));
	routing_hops=malloc(dimensions*sizeof(long));

	src_torus=malloc(dimensions*sizeof(long));
	dst_torus=malloc(dimensions*sizeof(long));

	servers=1;

	for (i=0; i<np; i++){
		nodes_dim[i]=par[i];
		servers*=par[i];
		k+=sprintf(filename_params+k,"%s%ld", topo_param_tokens[i], nodes_dim[i]);
	}

	ports=servers*dimensions*2;

	return 1; // return status, not used here
}

/**
* Release the resources used by the topology.
**/
void finish_topo_torus()
{
	free(nodes_dim);
	free(routing_hops);
	free(src_torus);
	free(dst_torus);
}

/**
* Get the number of servers of the network
*/
long get_servers_torus()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_torus()
{
	return 0; //this is a direct topology
}

// Get the number of ports of a given node (BOTH a server AND a switch, see above)
long get_radix_torus(long n)
{
	return dimensions*2;
}

/**
* Calculates connections
*/
tuple_t connection_torus(long node, long port)
{
	tuple_t res;
	long i, dim, dir;

	for (i=0; i<dimensions; i++)
	{
		src_torus[i]=node%nodes_dim[i];
		node=node/nodes_dim[i];
	}

	dir=port%2;
	dim=port/2;

	if (dir==1) // connecting in the negative dimension
		src_torus[dim]=(src_torus[dim]-1+nodes_dim[dim])%nodes_dim[dim];
	else	// connecting in the positive direction
		src_torus[dim]=(src_torus[dim]+1)%nodes_dim[dim];

	res.node=0;
	for (i=dimensions-1; i>=0; i--)
	{
		res.node=(res.node*nodes_dim[i])+src_torus[i];
	}
	res.port=dim*2 +(1-dir);
	return res;
}

long is_server_torus(long i)
{
	return (i<servers);
}

char * get_network_token_torus()
{
	return network_token;
}

char * get_routing_token_torus()
{
	return routing_token;
}

char *get_routing_param_tokens_torus(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_torus()
{
	return topo_version;
}

char * get_topo_param_tokens_torus(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_torus()
{
	return filename_params;
}

long get_server_i_torus(long i)
{
	return i;
}

long get_switch_i_torus(long i)
{
	return i;
}

long node_to_server_torus(long i)
{
	return i;
}

long node_to_switch_torus(long i)
{
	return i;
}

long get_ports_torus()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_torus(long src, long dst){

    return(1);
}

long init_routing_torus(long s, long d)
{
	long i;

	for (i=0; i<dimensions; i++)
	{
		src_torus[i]=s%nodes_dim[i];
		s=s/nodes_dim[i];
		dst_torus[i]=d%nodes_dim[i];
		d=d/nodes_dim[i];
		routing_hops[i]=dst_torus[i]-src_torus[i];
		if (routing_hops[i] > nodes_dim[i]/2)
			routing_hops[i]-=nodes_dim[i];
		else if (routing_hops[i] < -(nodes_dim[i]/2))
			routing_hops[i]+=nodes_dim[i];
		else if ((nodes_dim[i]%2==0) && (routing_hops[i] == nodes_dim[i]/2) && rand()%2)
			routing_hops[i]-=nodes_dim[i];
		else if ((nodes_dim[i]%2==0) && (routing_hops[i] == -(nodes_dim[i]/2)) && rand()%2)
			routing_hops[i]+=nodes_dim[i];
	}

	return 0;
}

long init_routing_mesh(long s, long d)
{
	long i;

	for (i=0; i<dimensions; i++)
	{
		src_torus[i]=s%nodes_dim[i];
		s=s/nodes_dim[i];
		dst_torus[i]=d%nodes_dim[i];
		d=d/nodes_dim[i];
		routing_hops[i]=dst_torus[i]-src_torus[i];
	}

	return 0;
}

void finish_route_torus()
{

}

/**
* Simple oblivious DOR. Others can be implemented.
*/
long route_torus(long current, long destination)
{
	long i;

#ifdef DEBUG
	if(current==destination) {
		printf("should not be routing a packet that has arrived to its destination (curr: %d, dstb: %d)!\n", current, destination);
		return -1; // just in case, let's abort the routing
	}
#endif // DEBUG
	for (i=0; i<dimensions; i++){
		if (routing_hops[i]>0){
			routing_hops[i]--;
			return 2*i;
		}
		else if (routing_hops[i]<0){
			routing_hops[i]++;
			return (2*i)+1;
		}
	}

	return -1;// should never get here
}

