/** @mainpage
Euroexa torus+tree network
*/
#include <stdlib.h>
#include <stdio.h>
#include "../inrflow/node.h"
#include "../inrflow/globals.h"
#include "../inrflow/misc.h"
#include "../inrflow/literal.h"

#define SUBTREES 2

///JNP shows the mapping applied to the tree when creating the topology
//#define SHOW_SUBTREE_MAPPING

//#define FINEGRAINOUTPUT 1

extern routing_t routing;

extern char filename_params[100];

static long dimensions=3;	///< number of dimensions
static long nodes_dim[3];	///< number of nodes per dimension
static long *routing_hops;	///< number of hops per dimension in this route
static long *src_torus;	///< coordinates of the source
static long *dst_torus;	///< coordinates of the destination

static long param_k;	///< parameter k of the topology, number of stages
static long *param_down;	///< Number of downwards ports in each level
static long *param_up;  ///< number of upwards ports in each level

static long *down_pow;	///< an array with the downward-link size for different levels of the topology, useful for doing some calculations.
static long *up_pow;	///< an array with the upward-link count, useful for doing some calculations.
static long *sw_per_stage;  ///< an array with the number of switches per stage, used often for many purposes

static long **bl2sw; ///< a translation table from blade id to switch id to allow different connection rules
static long **sw2bl; ///< a translation table from switch id to blade id to allow different connection rules

static long servers; 	///< The total number of servers :
static long blades;         ///< Number of pizza boxes (local switches)
static long switches;	///< The total number of switches :
static long switches_tree;	///< The total number of switches in the each of the tree subnetworks
static long nodes_torus;	///< The total number of nodes in each subtorus
static long ports;		    ///< The total number of links

static long *cur_route;    ///< Array to store the current route
static long cur_hop;   ///< Current hop in this route
static long max_paths;	///< Maximum number of paths to generate when using multipath
static long *path_index;	///< per-node index, used for round robin routing within the tree
static long torus_priority; ///< Percent of flows that are sent through the torus, even if the path is shorter through the tree.

static char network_token[20];
static char routing_token[20];
static char* topo_version="v0.1";
static char* topo_param_tokens[14]= {"nodes_X","nodes_Y","nodes_Z","stages","down0","up0","down1","up1","down2","up2","down3","up3","down4","up4"}; // Any way of doing this better?
//AE: make error checking so that we don't overrun this buffer
static char *routing_param_tokens[1]= {"torus_prio"};

// greatest common divisor using euclid's algorithm
long gcd(long a, long b) {
    long t;
    while (b != 0) {
        t = b;
        b = a % t;
        a = t;
    }
    return a;
}

/**
* Prints a (hopefully) more meaningful description of a node id.
*/
void print_euroexa_node (long node)
{
    if(node<servers)
        printf("DB %3ld Blade %3ld Nd %2ld ",node, node/16, node%16);
    else if (node<servers+(4*blades)) // lower part of the local switch;
        printf("LS %3ld Blade %3ld Sw %ld ",node-servers, ((node-servers)/4), ((node-servers)%4));
    else if (node<servers+(5*blades)) // upper part of the local switch;
        printf("US %3ld Blade %3ld Tt %ld %ld",node-servers-(4*blades), node-servers-(4*blades),bl2sw[0][node-servers-(4*blades)],bl2sw[1][node-servers-(4*blades)]);
    else // ETH switches
    {
        long st=0, sw=(node-servers-(5*blades))%switches_tree;
        while (sw>=sw_per_stage[st])
        {
            sw-=sw_per_stage[st++];
        }
        printf("NS %3ld T %1ld st %1ld sw %2ld ", node-servers-(5*blades), (node-servers-(5*blades))/switches_tree, st, sw);
    }
}


/// creates the translation tables for euroexa-base
void translation_tables_base(){
    long i;

    for (i=0; i<blades; i++) /// identity connection rule.
    {
        bl2sw[0][i]=i;
        sw2bl[0][i]=i;
        bl2sw[1][i]=i;
        sw2bl[1][i]=i;
    }
}


/// creates the translation tables for euroexa-rnd
void translation_tables_rnd(){
    long i;
    long tmp, r;

    for (i=0; i<blades; i++) /// let's start with the identity connection rule.
    {
        bl2sw[0][i]=i;
        bl2sw[1][i]=i;
    }
    for (i=0; i<blades; i++) /// now shuffle randomly.
    {
        r=rand()%blades;
        tmp=bl2sw[0][i];
        bl2sw[0][i]=bl2sw[0][r];
        bl2sw[0][r]=tmp;

        r=rand()%blades;
        tmp=bl2sw[1][i];
        bl2sw[1][i]=bl2sw[1][r];
        bl2sw[1][r]=tmp;
    }

    for (i=0; i<blades; i++) /// create sw2blade.
    {
        sw2bl[0][bl2sw[0][i]]=i;
        sw2bl[1][bl2sw[1][i]]=i;
#ifdef SHOW_SUBTREE_MAPPING
        printf("blade %ld: %ld %ld\n", i, bl2sw[0][i],bl2sw[1][i]);
#endif
    }

}

/// creates the translation tables for euroexa-multi
void translation_tables_multi(){
    long i;
    long c1=0, c2=0, step2;

    step2=nodes_torus*param_down[0];
    if (step2>blades)
        step2=param_down[0]; /// This gives more variety to the second tree in small topologies

    for (i=0; i<blades; i++) /// let's start with the identity connection rule.
    {
        bl2sw[0][c1]=i;
        c1+=nodes_torus;
        if (c1>=blades)
            c1=(c1%blades)+1;

        bl2sw[1][c2]=i;
        c2+=step2;
        if (c2>=blades)
            c2=(c2%blades)+1;
    }
    for (i=0; i<blades; i++) /// create sw2blade.
    {
        sw2bl[0][bl2sw[0][i]]=i;
        sw2bl[1][bl2sw[1][i]]=i;

#ifdef SHOW_SUBTREE_MAPPING
        printf("blade %ld: %ld %ld\n", i, bl2sw[0][i],bl2sw[1][i]);
#endif
    }
}


/// creates the translation tables for euroexa-rs
void translation_tables_rs(){
    long i;
    long c1=0, c2=0, step1, step2;

    while ( gcd(blades,(step1=rand()%blades))!=1);
    while ( gcd(blades,(step2=rand()%blades))!=1 || step2==step1); // ensure different strides are used

    printf ("s1: %ld, s2: %ld\n",step1,step2);

    for (i=0; i<blades; i++) /// let's start with the identity connection rule.
    {
        bl2sw[0][c1]=i;
        c1+=step1;
        if (c1>=blades)
            c1=(c1%blades);

        bl2sw[1][c2]=i;
        c2+=step2;
        if (c2>=blades)
            c2=(c2%blades);
    }
    for (i=0; i<blades; i++) /// create sw2blade.
    {
        sw2bl[0][bl2sw[0][i]]=i;
        sw2bl[1][bl2sw[1][i]]=i;

#ifdef SHOW_SUBTREE_MAPPING
        printf("blade %ld: %ld %ld\n", i, bl2sw[0][i],bl2sw[1][i]);
#endif
    }
}

/**
* Initializes the topology and sets the dimensions.
*/
long init_topo_euroexa(long np, long* par)
{
	long i,j,c=0;
	long buffer_length;

	blades=1;

    if (np<5) {
		printf("parameters needed: <nodes_X> <nodes_Y> <nodes_Z> <stages> <down_ports0> <up_ports0> <down_ports1> <up_ports1> ...\n");
		exit(-1);
	}
	nodes_dim[0]=par[0];
	nodes_dim[1]=par[1];
	nodes_dim[2]=par[2];
	nodes_torus=nodes_dim[0]*nodes_dim[1]*nodes_dim[2];

	param_k=par[3];
	if(param_k<1){
		printf("positive number of stages needed\n");
		exit(-1);
	}
	if(param_k>5){
		printf("number of stages limited to 5\n");
		exit(-1);
	}
	if (np<2*param_k+3){
		printf("Not enough parameters for a %ld-stage tree\n",param_k);
		exit(-1);
	}
	if (np>2*param_k+4){
		printf("There are more parameters than necessary for a %ld-stage tree. Ignoring trailing parameters.\n",param_k);
	}

	dimensions=3;

	routing_hops=malloc(dimensions*sizeof(long));
	src_torus=malloc(dimensions*sizeof(long));
	dst_torus=malloc(dimensions*sizeof(long));
    param_down=malloc(param_k*sizeof(long));
	param_up=malloc(param_k*sizeof(long));
	up_pow=malloc((param_k+1)*sizeof(long));
	down_pow=malloc((param_k+1)*sizeof(long));
	cur_route=malloc(2*param_k*sizeof(long));   // UP*/DOWN routes cannot be longer than 2*k
	sw_per_stage=malloc(param_k*sizeof(long));


	c=4;
	for (i=0;i<param_k;i++){
		param_down[i]=par[c++];
		param_up[i]=par[c++];
	}

    param_up[param_k-1]=0;  // up ports in the last stage should be disconnected, so let's assume they are not there, for simplicity.

	buffer_length=sprintf(filename_params,"%ldx%ldx%ld",nodes_dim[0],nodes_dim[1],nodes_dim[2]);
	buffer_length+=sprintf(filename_params+buffer_length,"k%ld",param_k);
	for (i=0;i<param_k;i++){
		buffer_length+=sprintf(filename_params+buffer_length,"d%ldu%ld",param_down[i],param_up[i]);
	}

    down_pow[0]=1;
    up_pow[0]=1;

	for (i=0; i<param_k; i++) {
		down_pow[i+1]=down_pow[i]*param_down[i];	// product of param_down[i] for 0<=i<n
		up_pow[i+1]=up_pow[i]*param_up[i];			// product of param_up[i]   for 0<=i<n
	} // numbers of up and down ports will be useful throughout,so let's compute them just once.


    switches_tree=0;
	for (i=0; i<param_k; i++) {
		sw_per_stage[i]=up_pow[i];
		for (j=i+1;j<param_k;j++){
			sw_per_stage[i]*=param_down[j];
		}
		printf("st%ld %ld, ",i,sw_per_stage[i]);
		switches_tree+=sw_per_stage[i];
		ports+=sw_per_stage[i]*param_down[i];
	}// number of switches per stage and per tree will be useful throughout, so let's compute them just once.

	blades=down_pow[param_k];
	servers=16*blades;
	switches=2*switches_tree + (5*blades); // we have 2 trees connected to the eth ports plus 5 switches per blade : 4 lower switches connected to each quad and 1 upper switch connected to the outer interconnect.
    ports*=2;   // we have 2 trees connected to the Eth ports.
	ports+=(servers*4)+(blades*((4*8)+12)); // 4 lower switches with 8 ports each + 1 upper switch with 12 ports

    if (blades%nodes_torus != 0)
    {
        printf("The number of blades is not divisible by the number of nodes per subtorus!\n    Blades: %ld, nodes_per_torus= %ld\n", blades, nodes_torus);
        exit (-1);
    }

	printf("\nservers %ld\n",servers);
	printf("blades %ld\n",blades);
	printf("switches %ld\n",switches);

	/// prepare the infrastructure for connection rules
	bl2sw=malloc(SUBTREES*sizeof(long*));
	for (i=0; i<SUBTREES; i++)
    {
        bl2sw[i]=malloc(blades*sizeof(long));
    }

	sw2bl=malloc(SUBTREES*sizeof(long*));
	for (i=0; i<SUBTREES; i++)
    {
        sw2bl[i]=malloc(blades*sizeof(long));
    }

    /// Calculate blade to switch translation tables based on the topology. Others might be possible
    switch(topo){
    case EUROEXA_BASE:
        translation_tables_base();
        strcpy(network_token,"euroexa-seq");
        break;
    case EUROEXA_RND:
        translation_tables_rnd();
        strcpy(network_token,"euroexa-rnd");
        break;
    case EUROEXA_MULTI:
        translation_tables_multi();
        strcpy(network_token,"euroexa-multi");
        break;
    case EUROEXA_RS:
        translation_tables_rs();
        strcpy(network_token,"euroexa-rs");
        break;
    case EUROEXA_SINGLE:    // For simplicity implemented as Euroexa_base but only routing through the first tree
        translation_tables_base();
        strcpy(network_token,"euroexa-single");
        break;
    default:
		printf("Not a EuroEXA compatible topology/mapping\n");
		exit(-1);
		break;
    }

	switch(routing){
	case EUROEXA_TORUS_ROUTING: // Use the Torus if possible
		strcpy(routing_token,"euroexa-torus");
		break;
    case EUROEXA_ETH_ROUTING: // Use the Eth network even if the torus is shorter
		strcpy(routing_token,"euroexa-tree");
		break;
    case EUROEXA_SHORTEST_ROUTING: // Use the shortest among the torus or the tree
		strcpy(routing_token,"euroexa-shortest");
		break;
    case EUROEXA_RANDOM_ROUTING: // Decide randomly between the torus and the tree
        if(routing_nparam>0){
            torus_priority=routing_params[0];
            if (torus_priority<0 || torus_priority>100){
                printf("Illegal value for euroexa-random priority (%ld), defaulting to 50\%\n", torus_priority);
                torus_priority=50;
            }
        } else {
            torus_priority=50;
        }
		snprintf(routing_token,20,"euroexa-random%ld",torus_priority);
		break;
	default:
		printf("Not a EuroEXA compatible routing %d, switching to euroexa-torus\n", routing);
		routing=EUROEXA_TORUS_ROUTING;
		strcpy(routing_token,"euroexa-torus");
		break;
    }

    path_index=malloc(servers*sizeof(long));
	for (i=0;i<servers; i++)
        path_index[i]=i;

/// JNP: Do we want multipath for this topology?
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
void finish_topo_euroexa()
{
    long i;

    free(routing_hops);
	free(src_torus);
	free(dst_torus);
    free(param_down);
	free(param_up);
	free(up_pow);
	free(down_pow);
	free(cur_route);
	free(sw_per_stage);

	for (i=0; i<SUBTREES; i++)
    {
        free(bl2sw[i]);
        free(sw2bl[i]);
    }
    free(bl2sw);
    free(sw2bl);

}

/**
* Get the number of servers of the network
*/
long get_servers_euroexa()
{
	return servers;
}

/**
* Get the number of switches of the network
*/
long get_switches_euroexa()
{
	return switches;
}

// Get the number of ports of a given node (BOTH a server AND a switch, see above)
long get_radix_euroexa(long node)
{
    if(node<servers)
        return 4;
    else if (node<servers+(4*blades)) // lower part of the local switch;
        return 8;
    else if (node<servers+(5*blades)) // upper part of the local switch;
        return 12;
    else    // rest of the network
    {
        long i=0;
        long k=(node-servers-(5*blades))%switches_tree;

        while (k>=sw_per_stage[i])
        {
            k-=sw_per_stage[i++];
        }
        return param_down[i]+param_up[i];
    }
}

/**
* Calculates connections
*/
tuple_t connection_euroexa(long node, long port)
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
        else // Connections to the tree topology
        {
            long local_tree=port-10; // port 10 goes to tree 0; port 11 goes to tree 1 (and so on if more uplinks were added)

            local_sw=bl2sw[local_tree][node-servers-(4*blades)];
            res.node=servers+(5*blades)+(local_tree*switches_tree)+(local_sw/param_down[0]);
            res.port=local_sw%param_down[0];
        }
    }
    else { // Tree subnetwork
        long local_tree, nl_first;
        long lvl, pos;

        local_sw=node-servers-(5*blades);
        local_tree=local_sw/switches_tree;
        pos=local_sw%switches_tree;

        lvl=0;
        nl_first=servers + (5*blades) + (local_tree*switches_tree);

        while (pos>=sw_per_stage[lvl]){
            pos-=sw_per_stage[lvl];
            nl_first+=sw_per_stage[lvl];
            lvl++;
        }
        if (lvl==param_k-1 && port>=param_down[lvl]) {   //disconnected links in the last stage of the gtree (param_up[last] should be 0 any way!!!)
            res.node=-1;
            res.port=-1;
        } else if (lvl==0 && port<param_down[lvl] ) { // connected to server
            res.node=servers+ (4*blades) + sw2bl[local_tree][port+(pos*down_pow[1])];
            res.port=10+local_tree;
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

long is_server_euroexa(long i)
{
	return (i<servers);
}


char * get_network_token_euroexa()
{
	return network_token;
}

char * get_routing_token_euroexa()
{
	return routing_token;
}

char *get_routing_param_tokens_euroexa(long i){

    return routing_param_tokens[i];
}

char * get_topo_version_euroexa()
{
	return topo_version;
}

char * get_topo_param_tokens_euroexa(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_euroexa()
{
	return filename_params;
}

long get_server_i_euroexa(long i)
{
	return i;
}

long get_switch_i_euroexa(long i)
{
	return i+servers;
}

long node_to_server_euroexa(long i)
{
	return i;
}

long node_to_switch_euroexa(long i)
{
	return i-servers;
}

long get_ports_euroexa()
{
	return ports;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_euroexa(long s, long d){
    /// JNP Multipath is disabled for the time being.
	return (1);
}

/**
* Simple routing. DOR for the torus, UP/DOWN with round robin for the tree. Shortest among the two is used.
*/
long init_routing_euroexa(long src, long dst)
{
	long i, mca[2]={0,0};
	long hops_torus=0;
    long s_torus=src/16,s_tree[2]={bl2sw[0][src/16],bl2sw[1][src/16]};
    long d_torus=dst/16,d_tree[2]={bl2sw[0][dst/16],bl2sw[1][dst/16]};
    long local_tree;

	if (routing!=EUROEXA_ETH_ROUTING && s_torus/nodes_torus==d_torus/nodes_torus) // same subtorus
    {
        for (i=0; i<dimensions; i++)
        {
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
            return 0; // This prioritizes the use of the subtorus over the tree.
    }
    else
    {
        hops_torus=2000; // larger than any tree path;
    }

    for (i=0;i<2;i++){
        while (s_tree[i]/down_pow[mca[i]]!=d_tree[i]/down_pow[mca[i]]) {
            mca[i]++;
        }
    }
    if (hops_torus<=2*mca[0] && hops_torus<=2*mca[1]) // 2*mcs is the number of hops within each tree.
    {
        return 0; //if the shortest path is through the torus, don't bother calculating the tree path
    }
    else //use the tree. we need to zero the hops in the torus to avoid travelling through both the tree and the torus
    {
        for (i=0; i<dimensions; i++) // undo the torus routing part
        {
            routing_hops[i]=0;
        }



        if (topo==EUROEXA_SINGLE)
            local_tree=0; // only the first tree is connected
        else{ // choose between the two subtrees
            if (mca[0]<mca[1])
                local_tree=0;
            else if (mca[0]>mca[1])
                local_tree=1;
            else
                local_tree = (rand()%2); //both trees have the same distance: choose one at random
        }

        cur_route[0]=10 + local_tree;

        for (i=1; i<mca[local_tree]; i++) { // Choose option based on source server, ensures load balancing.
            cur_route[i]=param_down[i-1]+((path_index[s_tree[local_tree]/down_pow[i-1]]) % param_up[i-1]);
        }

        for (i=0; i<mca[local_tree]; i++) {
            cur_route[mca[local_tree]+i]=(d_tree[local_tree]/down_pow[mca[local_tree]-1-i]) % param_down[mca[local_tree]-1-i];
        }
        path_index[s_tree[local_tree]]=(path_index[s_tree[local_tree]]+1)%servers;
        cur_hop=0;
        return 0;
    }
}

void finish_route_euroexa()
{

}

/**
* Main routing function. Does all the routing within the blade and uses the routes generated by init_routing.
*/
long route_euroexa(long current, long destination)
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
    print_euroexa_node(current);
    printf("  ->  ");
    print_euroexa_node(destination);
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
            // otherwise go up the tree
            return cur_route[cur_hop++];
        }
    }
    else { //we are in one of the trees
        return cur_route[cur_hop++];
    }
	return -1;// should never get here
}

