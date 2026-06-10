/** @mainpage
* FiConn topology
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// AE:  We need to set this path with a parameter.
#include "../inrflow/node.h"
//#include "../node.h"
#include "routelist.h"

#ifdef DEBUG
#include "../inrflow/globals.h"
#endif

#include "ficonn.h"

extern node_t* network;

static long param_k;	///< parameter k of the topology, number of levels of the ficonn (-1)
static long param_n;	///< parameter n of the topology, switch radix

static long *n_pow;	///< an array with the n^i, useful for doing some calculations.
static long *two_pow;	///< an array with the 2^i, useful for doing some calculations.
static long *fic_lvl;	///< number of ficonn_k-1 to form a ficonn_k
static long *fic_size;	///< number of servers in a ficonn_k
static long *free_ports;///< number of free ports in a ficonn_k

static long switches;	///< The total number of switches : 2(k+2)*((n/4)^(2^k))
static long servers; 	///< The total number of servers : switches/n
static long ports;		///< The total number of links

static long *src; ///< The coordinates for the current
static long *dst; ///< The coordinates for the destination

static route_t *last_route;

static char* network_token="ficonn";
static char* routing_token="tor";
static char* topo_version="v0.1";
static char* topo_param_tokens[2]= {"k","n"};

extern char filename_params[100];

/**
* Initializes the topology and sets the parameters k & n.
*/
long init_topo_ficonn(long np, long* par)
{
	long i;

	if (np<2) {
		printf("2 parameters are needed for FICONN <k, n>\n");
		exit(-1);
	}
	param_k=par[0];
	param_n=par[1];
	sprintf(filename_params,"k%ldn%ld",param_k,param_n);

	n_pow=malloc((param_k+1)*sizeof(long));
	n_pow[0]=1;
	two_pow=malloc((param_k+3)*sizeof(long));
	two_pow[0]=1;
	two_pow[1]=2;
	two_pow[2]=4;
	fic_lvl=malloc((param_k+1)*sizeof(long));
	fic_lvl[0]=1; // k-1,ficonn for k=0 doesn't really makes sense
	fic_size=malloc((param_k+1)*sizeof(long));
	fic_size[0]=param_n;
	free_ports=malloc((param_k+1)*sizeof(long));
	free_ports[0]=param_n;

	for (i=1; i<=param_k; i++) {
		n_pow[i]=n_pow[i-1]*param_n;
		two_pow[i+2]=two_pow[i+1]*2;
		fic_size[i]=((free_ports[i-1]/2)+1)*fic_size[i-1];
		fic_lvl[i]=(free_ports[i-1]/2)+1;
		free_ports[i]=((free_ports[i-1]/2)+1)*(free_ports[i-1]/2);
	}

	servers=fic_size[param_k];
	switches=servers/param_n;
	ports=((2*servers)-free_ports[param_k])+(switches*param_n);

	src=malloc(param_k*sizeof(long));
	dst=malloc(param_k*sizeof(long));

	return 1; // return status, not used here.
}

/**
* Release the resources used by the topology.
**/
void finish_topo_ficonn()
{

}

/**
* Get the number of servers of the network
*/
long get_servers_ficonn()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_ficonn()
{
	return switches;
}

long get_ports_ficonn()
{
	return ports;
}

long is_server_ficonn(long i)
{
	return (i<servers);
}

char * get_network_token_ficonn()
{
	return network_token;
}

char * get_routing_token_ficonn()
{
	return routing_token;
}

char * get_topo_version_ficonn()
{
	return topo_version;
}

char * get_topo_param_tokens_ficonn(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_ficonn()
{
	return filename_params;
}

long get_server_i_ficonn(long i)
{
	return i;
}

long get_switch_i_ficonn(long i)
{
	return servers+i;
}

long node_to_server_ficonn(long i)
{
	return i;
}

long node_to_switch_ficonn(long i)
{
	return i-servers;
}

// Get the number of ports of a given node (either a server or a switch, see above)
long get_radix_ficonn(long n)
{

	if (n<servers)
		return 2;	// If this is a server it has k ports
	else
		return param_n; // If this is a switch it has n ports
}

/**
* Calculates connections
*/
tuple_t connection_ficonn(long node, long port)
{
	tuple_t res;
	long i1,j1,i2,j2;
	long lvl;
	long pos;

	if (network[node].port[port].neighbour.node!=-1) // if this node is connected already
		return network[node].port[port].neighbour;

	else if (node<servers) {
		if (port==0) { //ficonn0 connections
			res.node=servers+(node/param_n);
			res.port=node%param_n;
		} else { // level>0;
			for (lvl=1; lvl<=param_k; lvl++)
				if(((node%fic_size[lvl-1])-two_pow[lvl-1]+1)%two_pow[lvl]==0) { // this is a level lvl connection
					break;
				}

			if (lvl>param_k) { // if the break is not reached then this is a free port
				res.node=-1;
				res.port=-1;
			} else {
				i1=(node/fic_size[lvl-1])%fic_lvl[lvl]; // OK
				j1=node%fic_size[lvl-1]; //OK

				i2=1+((1+j1-two_pow[lvl-1])/two_pow[lvl]);
				j2=i1*two_pow[lvl]+two_pow[lvl-1]-1;

				pos=((node/fic_size[lvl])*fic_size[lvl]) // first node in this ficonn_lvl
				    + i2*fic_size[lvl-1] // first node in the corresponding ficonn_lvl-1
				    + j2;	// node id the in the ficonn_lvl-1
				res.node=pos; // switch 'sw' in level 'port'
				res.port=1;
			}
		}
	} else {
		res.node=((node-servers)*param_n)+port;
		res.port=0; // Switches are connected to level 0 only.
	}

	return res;
}

/**
* Simple oblivious Routing (TOR).
*/
route_t* TOrouting (long src, long dst)
{
	long lvl;
	route_t *aux, *aux2, *res;
	long sc,dc; // cluster (ficonn) the source and destination belong to.
	long pren, postn; // ids of the nodes connecting the two ficonns

#ifdef DEBUG
	if(src==dst) {
		printf("should not be trying to route a packet that is at its destination (curr: %ld, dst: %ld)!\n", current, destination);
		aux=emptylist(); // empty set
		return aux; // just in case, let's abort the routing
	}
#endif// DEBUG

	for (lvl=param_k; lvl>0; lvl--)
		if(src/fic_size[lvl-1]!=dst/fic_size[lvl-1])
			break;

	if(lvl==0)	// Easy, just traverse the switch;
		return concat_nn(0, dst%param_n); // port 0 goes to the switch

	sc=(src%fic_size[lvl])/fic_size[lvl-1];
	dc=(dst%fic_size[lvl])/fic_size[lvl-1];

#ifdef DEBUG
	if(sc==dc) {
		printf("should not be in the same ficonn at this level (curr: %ld, dst: %ld)!\n", sc, dc);
	}
#endif // DEBUG
	if(sc<dc)
		dc--;
	else
		sc--;

	pren  = ((src/fic_size[lvl-1])*fic_size[lvl-1])	// first node in cluster sc
	        + (dc*two_pow[lvl])+two_pow[lvl-1]-1; 	// node in cluster sc connected to cluster dc
	postn = ((dst/fic_size[lvl-1])*fic_size[lvl-1]) // same as above
	        + (sc*two_pow[lvl])+two_pow[lvl-1]-1;

	aux=newlist(1);
	res=aux;

	if (pren!=src) {
		res=concat_ll(TOrouting(src,pren),aux);
		free(aux);
		aux=res;
	}
	if (postn!=dst) {
		aux2=TOrouting(postn,dst);
		res=concat_ll(aux,aux2);
		free(aux2);
	}

	return res;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_ficonn(long src, long dst){

    return(1);
}

/**
* Initializes the routing for a given pair of servers.
* @return -1 if there is no route, any other value if a route is found.
*/
long init_routing_ficonn(long src, long dst)
{
	last_route=TOrouting(src,dst);

	return 0;
}

void finish_route_ficonn()
{
	if (last_route->first!=NULL) {
#ifdef DEBUG
		if(n_failures==0){
		printf("ERROR: there are still some hops in the route.\n");
		exit(-1);
	}
#endif // DEBUG

		while (last_route->first!=NULL){
			get_next(last_route);
		}
	}

    free(last_route);
}

/**
* returns the steps of the route calculated in init_routing
*/
long route_ficonn(long current, long destination)
{
	long p;

#ifdef DEBUG
	if(current==destination) {
		printf("should not be routing a packet that has arrived to its destination (curr: %ld, dst: %ld)!\n", current, destination);
		return -1; // just in case, let's abort the routing
	}
#endif // DEBUG
	if ((p=get_next(last_route))!=-1)
		return p;
	else {
#ifdef DEBUG
		printf("All steps taken and not at destination yet (curr: %ld, dst: %ld)!\n", current, destination);
#endif // DEBUG
		return -1;
	}
}
