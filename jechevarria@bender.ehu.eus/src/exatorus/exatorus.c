/** @mainpage
Euroexa torus topologies
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

static long servers; 	///< The total number of servers :
static long pbs;         ///< Number of pizza boxes (local switches)
static long switches;	///< The total number of switches :
static long ports;		    ///< The total number of links

static char* network_token="exatorus";
static char* routing_token="dimensional";
static char* topo_version="v0.1";
static char* topo_param_tokens[11]= {"X","Y","Z","A","B","C","D","E","F","G","H"}; // Any way of doing this better?

static char *routing_param_tokens[1]= {"max_paths"};
extern char filename_params[100];

/**
* Initializes the topology and sets the dimensions.
*/
long init_topo_exatorus(long np, long* par)
{
	long i,k=0;

	pbs=1;

	if (np<1) {
		printf("At least 1 parameter is  needed for torus <nodes_per_dim>*\n");
		exit(-1);
	}

	dimensions=np;
	nodes_dim=malloc(dimensions*sizeof(long));
	routing_hops=malloc(dimensions*sizeof(long));

	src_torus=malloc(dimensions*sizeof(long));
	dst_torus=malloc(dimensions*sizeof(long));

	for (i=0; i<np; i++){
		nodes_dim[i]=par[i];
		pbs*=par[i];
		k+=sprintf(filename_params+k,"%s%ld", topo_param_tokens[i], nodes_dim[i]);
	}

	servers=pbs*16;    // 16 nodes per pizza box
	switches=2*pbs;    // local switches are logically divided into 2 switches.
	ports=(servers*4)+(pbs*17)+(pbs*(2*dimensions+1));

	printf("servers %ld\n",servers);
	printf("pbs %ld\n",pbs);
	printf("switches %ld\n",switches);

	return 1; // return status, not used here
}

/**
* Release the resources used by the topology.
**/
void finish_topo_exatorus()
{
	free(nodes_dim);
	free(routing_hops);
	free(src_torus);
	free(dst_torus);
}

/**
* Get the number of servers of the network
*/
long get_servers_exatorus()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_exatorus()
{
	return switches;
}

// Get the number of ports of a given node (BOTH a server AND a switch, see above)
long get_radix_exatorus(long node)
{
    if(node<servers)
        return 4;
    else if (node<servers+pbs) // lower part of the local switch;
        return 17;
    else    // higher part of the local switch
    	return (dimensions*2)+1;
}

/**
* Calculates connections
*/
tuple_t connection_exatorus(long node, long port)
{
	tuple_t res;
	long i, dim, dir;
	long exanode;
	long local_sw;

	if(node<servers)
    {
        if (port==0)// uplink to the local switch
        {
            res.node=servers + (node/16);
            res.port=node%16;
        }
        else // local 4-node arrangement
        {
            res.node=((node/4)*4) + (node+port)%4;
            res.port=4-port;
        }
    }
    else if (node<servers+pbs) // lower part of the local switch;
    {
        local_sw=node-servers;
        if (port < 16)
        {
            res.node=(local_sw*16)+port;
            res.port=0;
        }
        if (port==16)
        {
            res.node=node+pbs;
            res.port=dimensions*2;
        }
    }
    else    // higher part of the local switch
    {
        if (port==dimensions*2)// going down
        {
            res.node=node-pbs;
            res.port=16;
        }
        else
        {
            exanode=node-pbs-pbs;
            for (i=0; i<dimensions; i++)
            {
                src_torus[i]=exanode%nodes_dim[i];
                exanode=exanode/nodes_dim[i];
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
            res.node+=servers+pbs;
            res.port=dim*2 +(1-dir);
        }
    }

	return res;
}

long is_server_exatorus(long i)
{
	return (i<servers);
}


char * get_network_token_exatorus()
{
	return network_token;
}

char * get_routing_token_exatorus()
{
	return routing_token;
}

char *get_routing_param_tokens_exatorus(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_exatorus()
{
	return topo_version;
}

char * get_topo_param_tokens_exatorus(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_exatorus()
{
	return filename_params;
}

long get_server_i_exatorus(long i)
{
	return i;
}

long get_switch_i_exatorus(long i)
{
	return i+servers;
}

long node_to_server_exatorus(long i)
{
	return i;
}

long node_to_switch_exatorus(long i)
{
	return i-servers;
}

long get_ports_exatorus()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_exatorus(long src, long dst){

    return(1);
}

long init_routing_exatorus(long src, long dst)
{
	long i;
    long s=src/16;
    long d=dst/16;

#ifdef FINEGRAINOUTPUT
	printf("%ld --> %ld (%ld, %ld) ::: [ ",src,dst,s,d);
#endif
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
#ifdef FINEGRAINOUTPUT
		printf("%ld ",routing_hops[i]);
#endif

	}
#ifdef FINEGRAINOUTPUT
	printf("]\n");
#endif

	return 0;
}

void finish_route_exatorus()
{

}

/**
* Simple oblivious DOR.
*/
long route_exatorus(long current, long destination)
{
	long i,c;

#ifdef DEBUG
	if(current==destination) {
		printf("should not be routing a packet that has arrived to its destination (curr: %d, dstb: %d)!\n", current, destination);
		return -1; // just in case, let's abort the routing
	}
#endif // DEBUG

    if ((current/4)==(destination/4)) // in the same 4-node group
    {
        c=(destination%4)-(current%4);
        if (c<0)
            c+=4;
        return c;
    }
    else if (current<servers)
    {
        return 0; // uplink to local switch
    }
    else if ((current-servers)==(destination/16))
    {
        return destination%16; // downlink to destination
    }
    else if (current<servers+pbs)
    {
        return 16; // uplink to upper switch
    }
    else if ((current-servers-pbs)==(destination/16))
    {
        return 2*dimensions; // downlink to local switch
    }
    else //route in the torus
    {
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
    }

	return -1;// should never get here
}
