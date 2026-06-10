/** @mainpage
Euroexa topology-less network
*/
#include <stdlib.h>
#include <stdio.h>
#include "../inrflow/node.h"
#include "../inrflow/globals.h"
#include "../inrflow/misc.h"
#include "../inrflow/literal.h"

extern routing_t routing;

extern char filename_params[100];

static long dimensions=3;	///< number of dimensions
static long nodes_dim[3];	///< number of nodes per dimension
static long *routing_hops;	///< number of hops per dimension in this route
static long *src_torus;	///< coordinates of the source
static long *dst_torus;	///< coordinates of the destination

static long eth_radix;   ///< switch radix

static long servers; 	///< The total number of servers :
static long blades;         ///< Number of pizza boxes (local switches)
static long switches;	///< The total number of switches :
static long switches_tree;	///< The total number of switches in the each of the tree subnetworks
static long nodes_torus;	///< The total number of nodes in each subtorus
static long ports;		    ///< The total number of links

static long cur_route[4];    ///< Array to store the current route. It cannot be larger than 4 hops (1 in and 1 out of each eth subnet)
static long cur_hop;   ///< Current hop in this route
static long max_paths;	///< Maximum number of paths to generate when using multipath
//static long *path_index;	///< per-node index, used for round robin routing
static long torus_priority; ///< Percent of flows that are sent through the torus, even if the path is shorter through the tree.

static char* network_token="euroexa-tl";
static char routing_token[20];
static char* topo_version="v0.1";
static char* topo_param_tokens[4]= {"nodes_X","nodes_Y","nodes_Z","radix"};
static char *routing_param_tokens[1]= {"torus_prio"};

/**
* Prints a (hopefully) more meaningful description of a node id.
*/
void print_euroexa_tl_node (long node)
{
    if(node<servers)
        printf("DB %3ld Blade %3ld Nd %2ld ",node, node/16, node%16);
    else if (node<servers+(4*blades)) // lower part of the local switch;
        printf("LS %3ld Blade %3ld Sw %ld ",node-servers, ((node-servers)/4), ((node-servers)%4));
    else if (node<servers+(5*blades)) // upper part of the local switch;
        printf("US %3ld Blade %3ld Tt %ld %ld",node-servers-(4*blades), node-servers-(4*blades),(node-servers-(4*blades))%eth_radix,(node-servers-(4*blades))/eth_radix);
    else // ETH switches
        printf("NS %3ld T %1ld sw %2ld ", node-servers-(5*blades), (node-servers-(5*blades))/eth_radix, (node-servers-(5*blades))%eth_radix);
}

/**
* Initializes the topology and sets the dimensions.
*/
long init_topo_euroexa_tl(long np, long* par)
{
    long buffer_length;

    if (np<4) {
		printf("parameters needed: <nodes_X> <nodes_Y> <nodes_Z> <eth_switch_radix>\n");
		exit(-1);
	}
	nodes_dim[0]=par[0];
	nodes_dim[1]=par[1];
	nodes_dim[2]=par[2];
	nodes_torus=nodes_dim[0]*nodes_dim[1]*nodes_dim[2];
    dimensions=3;

	eth_radix=par[3];

	routing_hops=malloc(dimensions*sizeof(long));
	src_torus=malloc(dimensions*sizeof(long));
	dst_torus=malloc(dimensions*sizeof(long));

	buffer_length=sprintf(filename_params,"%ldx%ldx%ld",nodes_dim[0],nodes_dim[1],nodes_dim[2]);
	buffer_length+=sprintf(filename_params+buffer_length,"r%ld",eth_radix);

	blades=eth_radix*eth_radix;
	servers=16*blades;
	switches=2*eth_radix + (5*blades); // we have 2 trees connected to the eth ports plus 5 switches per blade : 4 lower switches connected to each quad and 1 upper switch connected to the outer interconnect.
    ports=2*eth_radix*eth_radix;   // we have eth_radix switches with eth_radix ports each in each Eth subnet.
	ports+=(servers*4)+(blades*((4*8)+12)); // 4 lower switches with 8 ports each + 1 upper switch with 12 ports

    if (blades%nodes_torus != 0)
    {
        printf("The number of blades is not divisible by the number of nodes per subtorus!\n    Blades: %ld, nodes_per_torus= %ld\n", blades, nodes_torus);
        exit (-1);
    }

	printf("\nservers %ld\n",servers);
	printf("blades %ld\n",blades);
	printf("switches %ld\n",switches);

    switch(routing){
	case EUROEXA_TORUS_ROUTING: // Use the Torus if possible
		snprintf(routing_token,20,"euroexa-torus");
		break;
    case EUROEXA_ETH_ROUTING: // Use the Eth network even if the torus is shorter
		snprintf(routing_token,20,"euroexa-eth");
		break;
    case EUROEXA_SHORTEST_ROUTING: // Use the shortest among the torus or the Eth subnet
		snprintf(routing_token,20,"euroexa-shortest");
		break;
    case EUROEXA_RANDOM_ROUTING: // Decide randomly between the torus and the Eth subnet
		snprintf(routing_token,20,"euroexa-random");
        if(routing_nparam>0){
            torus_priority=routing_params[0];
            if (torus_priority<0 || torus_priority>100)
                torus_priority=50;
        } else {
            torus_priority=50;
        }
		break;
	default:
		printf("Not a EuroEXA compatible routing %d, switching to euroexa-torus\n", routing);
		routing=EUROEXA_TORUS_ROUTING;
		snprintf(routing_token,20,"euroexa-torus");
		break;
    }

/// JNP: Do we want multipath for this topology? Not really a good topology for multipath, only 2 shortest paths.
//	if(routing_nparam>0){
//		max_paths=routing_params[0];
//		if (max_paths<0)
//			max_paths=1;
//	} else {
//		max_paths=1;
//	}

    max_paths=1; /// multipath disabled for the time being

	return 1; // return status, not used here
}

/**
* Release the resources used by the topology.
**/
void finish_topo_euroexa_tl()
{
    free(routing_hops);
	free(src_torus);
	free(dst_torus);
}

/**
* Get the number of servers of the network
*/
long get_servers_euroexa_tl()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_euroexa_tl()
{
	return switches;
}

// Get the number of ports of a given node (BOTH a server AND a switch, see above)
long get_radix_euroexa_tl(long node)
{
    if(node<servers)
        return 4;
    else if (node<servers+(4*blades)) // lower part of the local switch;
        return 8;
    else if (node<servers+(5*blades)) // upper part of the local switch;
        return 12;
    else    // rest of the network
        return eth_radix;
}

/**
* Calculates connections
*/
tuple_t connection_euroexa_tl(long node, long port)
{
	tuple_t res;
	long i, dim, dir;
	long local_sw;

	if(node<servers) {
        if (port==0) { // uplink to the local switch
            res.node=servers + (node/4);
            res.port=node%4;
        }
        else { // local 4-node clique (quadrant)
            res.node=((node/4)*4) + (node+port)%4;
            res.port=4-port;
        }
    }
    else if (node<servers+(4*blades)) { // lower part of the local switch;
        local_sw=node-servers;
        if (port < 4) { // connections to CRDBs
            res.node=(local_sw*4)+port;
            res.port=0;
        }
        else if (port < 7) { // connection within lower 4-switch clique
            res.node=servers + ((local_sw/4)*4) + ((local_sw+port+1)%4);
            res.port=10-port;
        }
        else if (port==7) { // connection to upper switch
            res.node=servers+(4*blades)+(local_sw/4);
            res.port=local_sw%4;
        }
    }
    else if (node<servers+(5*blades)){   // higher part of the local switch
        if (port < 4) {// going down
            local_sw=node-servers-(4*blades);
            res.node=servers + (4 * local_sw) + port;
            res.port=7;
        }
        else if (port < 10) {// torus subnetwork
            long subtorus;

            local_sw=node-servers-(4*blades);
            subtorus=local_sw/nodes_torus;
            local_sw=local_sw%nodes_torus;

            for (i=0; i<dimensions; i++)
            {
                src_torus[i]=local_sw%nodes_dim[i];
                local_sw=local_sw/nodes_dim[i];
            }

            dir=port%2;
            dim=(port-4)/2;

            if (dir==1) // connecting in the negative dimension
                src_torus[dim]=(src_torus[dim]-1+nodes_dim[dim])%nodes_dim[dim];
            else	// connecting in the positive direction
                src_torus[dim]=(src_torus[dim]+1)%nodes_dim[dim];

            res.node=0;
            for (i=dimensions-1; i>=0; i--)
            {
                res.node=(res.node*nodes_dim[i])+src_torus[i];
            }
            res.node+=servers+(4*blades)+(subtorus*nodes_torus);
            res.port=(dim*2) +(1-dir) +4;
        }
        else // Connections to the eth subnets
        {
            long local_subnet=port-10; // port 10 goes to subnet 0; port 11 goes to subnet 1 (and so on if more uplinks were added)
            long a, b;

            local_sw=node-servers-(4*blades);
            a=local_sw%eth_radix;
            b=local_sw/eth_radix;
            if (local_subnet==0){
                res.node=servers+(5*blades)+b;
                res.port=a;
            }
            else { // local subnet==1
                res.node=servers+(5*blades)+eth_radix+a;
                res.port=b;
            }
        }
    }
    else { // Tree subnetwork
        long local_subnet; // port 10 goes to subnet 0; port 11 goes to subnet 1 (and so on if more uplinks were added)
        long pos; // position of the local switch within the subnetwork

        local_sw=node-servers-(5*blades);
        local_subnet=local_sw/eth_radix;
        pos=local_sw%eth_radix;

        res.port=10+local_subnet;
        if (local_subnet==0)
            res.node=servers+(4*blades)+(pos*eth_radix)+port;
        else
            res.node=servers+(4*blades)+(port*eth_radix)+pos;
    }

	return res;
}

long is_server_euroexa_tl(long i)
{
	return (i<servers);
}


char * get_network_token_euroexa_tl()
{
	return network_token;
}

char * get_routing_token_euroexa_tl()
{
	return routing_token;
}

char *get_routing_param_tokens_euroexa_tl(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_euroexa_tl()
{
	return topo_version;
}

char * get_topo_param_tokens_euroexa_tl(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_euroexa_tl()
{
	return filename_params;
}

long get_server_i_euroexa_tl(long i)
{
	return i;
}

long get_switch_i_euroexa_tl(long i)
{
	return i+servers;
}

long node_to_server_euroexa_tl(long i)
{
	return i;
}

long node_to_switch_euroexa_tl(long i)
{
	return i-servers;
}

long get_ports_euroexa_tl()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_euroexa_tl(long s, long d){
    /// JNP Multipath is disabled for the time being.
	return (1);
}

/**
* Simple routing. DOR for the torus, UP/DOWN with round robin for the tree. Shortest among the two is used.
*/
long init_routing_euroexa_tl(long src, long dst)
{
	long i;
	long hops_torus=0;
    long s_torus=src/16,s_a=(src/16)%eth_radix,s_b=(src/16)/eth_radix;
    long d_torus=dst/16,d_a=(dst/16)%eth_radix,d_b=(dst/16)/eth_radix;

	if (routing!=EUROEXA_ETH_ROUTING && s_torus/nodes_torus==d_torus/nodes_torus) { // same subtorus
        for (i=0; i<dimensions; i++){
            src_torus[i]=s_torus%nodes_dim[i];
            s_torus=s_torus/nodes_dim[i];
            dst_torus[i]=d_torus%nodes_dim[i];
            d_torus=d_torus/nodes_dim[i];
            routing_hops[i]=dst_torus[i]-src_torus[i];

            if (routing_hops[i] > nodes_dim[i]/2)
                routing_hops[i]-=nodes_dim[i];
            else if (routing_hops[i] < -(nodes_dim[i]/2))
                routing_hops[i]+=nodes_dim[i];
            else if ((nodes_dim[i]%2==0) && (routing_hops[i] == nodes_dim[i]/2) && rand()%2)
                routing_hops[i]-=nodes_dim[i];
            else if ((nodes_dim[i]%2==0) && (routing_hops[i] == -(nodes_dim[i]/2)) && rand()%2)
                routing_hops[i]+=nodes_dim[i];

            hops_torus+=abs(routing_hops[i]);
        }
        if (routing==EUROEXA_TORUS_ROUTING || (routing==EUROEXA_RANDOM_ROUTING && rand()%100<torus_priority))
            return 0; // This prioritizes the use of the subtorus over the Eth subnets.
    }
    else{
        hops_torus=2000; // larger than any tree path;
    }

    i=0;
    if (s_a!=d_a){
        cur_route[i++]=10;
        cur_route[i++]=d_a;
    }
    if (s_b!=d_b){
        cur_route[i++]=11;
        cur_route[i++]=d_b;
    }

    if (hops_torus<=i) {
        return 0; //the shortest path is through the torus
    }
    else {//use the tree. we need to zero the hops in the torus to avoid travelling through both the subnets and the torus
        for (i=0; i<dimensions; i++) {// undo the torus routing part
            routing_hops[i]=0;
        }

        cur_hop=0;
        return 0;
    }
}

void finish_route_euroexa_tl()
{

}

/**
* Main routing function. Does all the routing within the blade and uses the routes generated by init_routing.
*/
long route_euroexa_tl(long current, long destination)
{
	long i,c;

#ifdef DEBUG
	if(current==destination) {
		printf("should not be routing a packet that has arrived to its destination (curr: %ld, dstb: %ld)!\n", current, destination);
		return -1; // just in case, let's abort the routing
	}
#endif // DEBUG

#ifdef FINEGRAINOUTPUT
    if (current < servers)
        printf("\n");
    print_euroexa_tl_node(current);
    printf("  ->  ");
    print_euroexa_tl_node(destination);
    printf("\n");
#endif //FINEGRAINOUTPUT

    if ((current/4)==(destination/4)) // in the same 4-node group
    {
        c=(destination%4)-(current%4);
        if (c<0)
            c+=4;
        return c;
    }
    else if (current<servers)
    {
        return 0; // uplink to lower local switch
    }
    else if (current<servers+(4*blades)) // in a lower switch
    {
        if ((current-servers)==(destination/4))
        {
            return destination%4; // downlink to destination from lower local switch
        }
        else if ((current-servers)/4==(destination/16)) // in the same CRDB but different quad; move within the lower switch-clique.
        {
            long o,d;
            o=(current-servers)%4;
            d=(destination/4)%4;

            if (o<d)
                return d-o+3;
            else // d<o
                return d-o+7;
        }
        else // not the same CRDB
        {
            return 7; // uplink to upper switch
        }
    }
    else  if (current<servers+(5*blades)) // in an upper switch
    {
        if ((current-servers-(4*blades))==(destination/16))// arrived to the destination's upper switch
        {
            return (destination/4)%4;
        }
        else //route in the torus if needed
        {
            for (i=0; i<dimensions; i++){
                if (routing_hops[i]>0){
                    routing_hops[i]--;
                    return 4+(2*i);
                }
                else if (routing_hops[i]<0){
                    routing_hops[i]++;
                    return 4+(2*i)+1;
                }
            }
            // otherwise go through the subnets
            return cur_route[cur_hop++];
        }
    }
    else { //we are in an Eth subnet
        return cur_route[cur_hop++];
    }
	return -1;// should never get here
}
