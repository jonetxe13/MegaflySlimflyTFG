#include "traffic.h"
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"

tpattern_t pattern; ///< the traffic pattern we are using in the simulation
long servers;   ///< Number of servers in this traffic pattern.
long src;       ///< current source
long dst;       ///< current destination
long curr_flows;	///< The current number of sent flows
long max_flows;		///< The total number of flows in this pattern
long group_size;	///< The number of servers per group in many_*
long curr_group;	///< The current group in many_*
long pn, pn1;		///< 2 ^ n & 2 ^ (n+1) - used for logarithmic patterns.
long *shuffle;  ///< Used to rearrange nodes for different purposes (e.g. bisection)
long stride;        ///< The stride for shift traffic pattern.
char* traffic_name; ///< the name of the used traffic pattern as a string
long traffic_nparam;    ///< Number of parameters passed to the traffic generator
long traffic_params[MAX_TRAFFIC_PARAMS];    ///< Parameters passed to the traffic generator


void init_patterns(){

    switch (pattern) {
        case ALL2ALL:
            init_pattern=init_pattern_all2all;
            next_flow=next_flow_all2all;
            break;
        case MANYALL2ALL:
            init_pattern=init_pattern_many_all2all;
            next_flow=next_flow_many_all2all;
            break;
        case MANYALL2ALLRND:
            init_pattern=init_pattern_many_all2all_rnd;
            next_flow=next_flow_many_all2all_rnd;
            break;
        case BUTTERFLY:
            init_pattern=init_pattern_butterfly;
            next_flow=next_flow_butterfly;
            break;
        case ALL2ONE:
            init_pattern=init_pattern_all2one;
            next_flow=next_flow_all2one;
            break;
        case ONE2ALL:
            init_pattern=init_pattern_one2all;
            next_flow=next_flow_one2all;
            break;
        case RANDOM:
            init_pattern=init_pattern_rnd;
            next_flow=next_flow_rnd;
            break;
        case SHIFT:
            init_pattern=init_pattern_shift;
            next_flow=next_flow_shift;
            break;
        case BISECTION:
            init_pattern=init_pattern_bisection;
            next_flow=next_flow_bisection;
            break;
        case HOTREGION:
            init_pattern=init_pattern_hotregion;
            next_flow=next_flow_hotregion;
            break;
        case HOTSPOT:
            init_pattern=init_pattern_hotspot;
            next_flow=next_flow_hotspot;
            break;
        default:
            printf("Error: Unknown traffic pattern.\n");
            exit(-1);
    }
}

void finish_traffic(long nparam){
	long i;
for (i = 0; i < nparam; i++) {
	free(traffic_param_tokens[i]);
 }
 free(traffic_param_tokens);
}

/**
* Initializes the all2all pattern. all2all requires no parameter in the list.
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_all2all(long nservers, long nparam, long *params)
{
    if(nparam!=0) {
		printf("No parameter is needed for all2all!\n");
		exit(-1);
	}

	servers=nservers;
	src=0;
	dst=0;
}

/**
* Get next flow
* @see init_pattern_all2all
*/
flow_t next_flow_all2all()
{
	flow_t tmp;

	tmp.src=src;
	tmp.dst=dst;
	src++;
	if(src==servers) {
		src=0;
		dst++;
		if(dst==servers) {
			src=-1;
			dst=-1;
		}
	}
	return tmp;
}

/**
* Initializes the shift pattern. shift requires a single parameter, the stride. Each node will communicate with the node whose address is src+stride.
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_shift(long nservers, long nparam, long *params)
{
    long i;

	servers=nservers;
	src=0;
	dst=0;
	if(nparam!=1) {
		printf("1 parameter is needed for Shift <#stride>\n");
		exit(-1);
	}
	traffic_param_tokens=malloc(nparam*sizeof(char*));
	for (i = 0; i < nparam; i++) {
        traffic_param_tokens[i]=malloc(15*sizeof(char));
		snprintf(traffic_param_tokens[i],15,"stride");
	}
	stride=params[0];
}

/**
* Get next flow
* @see next_flow_shift
*/
flow_t next_flow_shift()
{
    flow_t tmp;
    tmp.src=src;
	tmp.dst=(src+stride)%servers;
	if (tmp.dst<0)
        tmp.dst+=servers;

	if(src++==servers) {
        tmp.src=-1;
        tmp.dst=-1;
	}
	return tmp;
}


/**
* Initializes the bisection pattern which requires no parameter. The network is split into two randomly and each node will communicate with a node in the other half at random.
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_bisection(long nservers, long nparam, long *params)
{
    long tmp, r, i;

    if(nparam!=0) {
		printf("No parameter is needed for bisection!\n");
		exit(-1);
	}

	servers=nservers;
	max_flows=nservers;//integer division;
	curr_flows=0;

	shuffle=malloc(sizeof(long)*nservers);
	for (i=0; i<nservers; i++){
        shuffle[i]=i;
	}
	for (i=0; i<nservers; i++){
        r=ztm(nservers-i);
        tmp=shuffle[i];
        shuffle[i]=shuffle[i+r];
        shuffle[i+r]=tmp;
	}
}

/**
* Get next flow
* @see init_pattern_bisection
*/
flow_t next_flow_bisection()
{
    flow_t tmp;

	if(curr_flows==max_flows) {
        tmp.src=-1;
        tmp.dst=-1;
        free(shuffle);
	} else{
        tmp.src=shuffle[curr_flows];
        tmp.dst=shuffle[max_flows-curr_flows-1];
    }
    curr_flows++;
	return tmp;
}

/**
* Initializes the butterfly pattern. butterfly requires no parameter
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_butterfly(long nservers, long nparam, long *params)
{
    if(nparam!=0) {
		printf("No parameter is needed for butterfly!\n");
		exit(-1);
	}
	servers=nservers;
	src=0;
	dst=1;
	pn=1;
	pn1=2;
}

/**
* Get next flow
* @see init_pattern_butterfly
*/
flow_t next_flow_butterfly()
{
	flow_t tmp;


	tmp.src=src;
	tmp.dst=dst;
	do {
		src++;
		if (src%pn1<pn)
			dst=src+pn;
		else
			dst=src-pn;

		if(src==servers) {
			src=0;
			pn=pn1;
			pn1=pn1*2;
			if(pn>servers) {
				src=-1;
				dst=-1;
			} else {
				if (src%pn1<pn)
					dst=src+pn;
				else
					dst=src-pn;
			}
		}
	} while(src>=servers || dst>=servers);
	return tmp;
}

/**
* Initializes the many_all2all pattern. It requires 1 parameter: the group size. all2all groups are sequential.
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_many_all2all(long nservers, long nparam, long *params)
{
	long i;
	servers=nservers;
	src=0;
	dst=0;
	if(nparam!=1) {
		printf("1 parameter is needed for MANY_ALL2ALL <#group_size>\n");
		exit(-1);
	}
	traffic_param_tokens=malloc(nparam*sizeof(char*));
	for (i = 0; i < nparam; i++) {
        traffic_param_tokens[i]=malloc(10*sizeof(char));//needs to be freed
		snprintf(traffic_param_tokens[i],10,"group");
	}
	curr_flows=0;
	group_size=params[0];
	curr_group=0;
}

/**
* Get next flow
* @see init_pattern_many_all2all
*/
flow_t next_flow_many_all2all()
{
	flow_t tmp;

	tmp.src=src;
	tmp.dst=dst;

	src++;
	if(src%group_size==0 || src==servers ) {
		src=curr_group*group_size;
		dst++;
		if(dst==servers) {
			src=-1;
			dst=-1;
		} else if(dst%group_size==0) {
			curr_group++;
			src=curr_group*group_size;
		}
	}
	return tmp;
}

/**
* Initializes the many_all2all_rnd pattern. It requires 1 parameter: the group size. all2all groups are chosen randomly.
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_many_all2all_rnd(long nservers, long nparam, long *params)
{
	long i,r,tmp;

	servers=nservers;
	src=0;
	dst=0;

	shuffle=malloc(nservers*sizeof(long));
	for (i=0; i<nservers; i++) {
		shuffle[i]=i;
	}
	for (i=0; i<nservers; i++){
        r=ztm(nservers-i);
        tmp=shuffle[i];
        shuffle[i]=shuffle[i+r];
        shuffle[i+r]=tmp;
	}

	if(nparam!=1) {
		printf("1 parameter is needed for MANY_ALL2ALL_RND <#group_size>\n");
		exit(-1);
	}
	traffic_param_tokens=malloc(nparam*sizeof(char*));
	for (i = 0; i < nparam; i++) {
        traffic_param_tokens[i]=malloc(10*sizeof(char));//needs to be freed
		snprintf(traffic_param_tokens[i],10,"group");
	}
	curr_flows=0;
	group_size=params[0];
	curr_group=0;
}

/**
* Get next flow
* @see init_pattern_many_all2all_rnd
*/
flow_t next_flow_many_all2all_rnd()
{
	flow_t tmp;

	if (src==-1 || dst==-1) {
		tmp.src=-1;
		tmp.dst=-1;
	} else {
		tmp.src=shuffle[src];
		tmp.dst=shuffle[dst];
	}

	src++;
	if(src%group_size==0 || src==servers ) {
		src=curr_group*group_size;
		dst++;
		if(dst==servers) {
			src=-1;
			dst=-1;
            free(shuffle);
		} else if(dst%group_size==0) {
			curr_group++;
			src=curr_group*group_size;
		}
	}
	return tmp;
}

/**
* Initializes the all2one pattern.  The destination is chosen at random. It requires no
* parameter.

* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_all2one(long nservers, long nparam, long *params)
{
	if(nparam!=0) {
		printf("No parameter is needed for all2one!\n");
		exit(-1);
	}
	servers=nservers;
	src=0;
	dst=ztm(nservers);
}

/**
* Get next flow
* @see init_pattern_all2one
*/
flow_t next_flow_all2one()
{
	flow_t tmp;

	tmp.src=src;
	tmp.dst=dst;
	src++;
	if(src==servers) {
		src=-1;
		dst=-1;
	}
	return tmp;
}

/**
* Initializes the one2all pattern.  The source is chosen at random. It requires no parameter.

* @param nservers The number of communicating servers @param nparam The number of parameters
* in the list of parameters @param params A list with the parameters for the traffic pattern
*/
void init_pattern_one2all(long nservers, long nparam, long *params)
{
    if(nparam!=0) {
		printf("No parameter is needed for one2all!\n");
		exit(-1);
	}
	servers=nservers;
	src=ztm(nservers);
	dst=0;
}

/**
* Get next flow
* @see init_pattern_one2all
*/
flow_t next_flow_one2all()
{
	flow_t tmp;

	tmp.src=src;
	tmp.dst=dst;
	dst++;
	if(dst==servers) {
		src=-1;
		dst=-1;
	}
	return tmp;
}

/**
* Initializes the random pattern. It requires 1 parameter: the total number of flows. Flows
* are generated independently: source and destination are selected randomly using a uniform
* distribution.
*
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_rnd(long nservers, long nparam, long *params)
{
	long i;
	servers=nservers;
	if(nparam!=1) {
		printf("1 parameter is needed for RANDOM <#flows>\n");
		exit(-1);
	}
	traffic_param_tokens=malloc(nparam*sizeof(char*));
	for (i = 0; i < nparam; i++) {
    traffic_param_tokens[i]=malloc(20*sizeof(char));//needs to be freed
		snprintf(traffic_param_tokens[i],20,"nrndflows");
	}
	curr_flows=0;
	max_flows=params[0];
}

/**
* Get next flow
* @see init_pattern_rnd
*/
flow_t next_flow_rnd()
{
	flow_t tmp;
	if (curr_flows++ == max_flows) {
		tmp.src = -1;
		tmp.dst = -1;
	} else {
		tmp.src = rand()%servers;
		tmp.dst = rand()%servers;
	}
	return tmp;
}

/**
* Initializes the hotregion pattern. It requires 1 parameter: the total number
* of flows. Flows are generated independently: source is selected randomly using
* a uniform distribution whereas destination follows a hot region distribution
* (the first 12.5% of the servers have 25% chances of being selected, the rest
* is uniform).
*
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_hotregion(long nservers, long nparam, long *params)
{
	long i;
	servers=nservers;
	if(nparam!=1) {
		printf("1 parameter is needed for HOTREGION <#flows>\n");
		exit(-1);
	}
	traffic_param_tokens=malloc(nparam*sizeof(char*));
	for (i = 0; i < nparam; i++) {
        traffic_param_tokens[i]=malloc(20*sizeof(char));//needs to be freed
		snprintf(traffic_param_tokens[i],20,"nhotrflows");
	}
	curr_flows=0;
	max_flows=params[0];
}

/**
* Get next flow
* @see next_flow_hotregion
*/
flow_t next_flow_hotregion()
{
	flow_t tmp;
	if (curr_flows++ == max_flows) {
		tmp.src = -1;
		tmp.dst = -1;
	} else {
		tmp.src=rand()%servers;
		if (rand()%100 < 25)
			tmp.dst=rand()%(servers/8);
		else
			tmp.dst=rand()%servers;
	}
	return tmp;
}

/**
* Initializes the hotspot pattern. It requires 1 parameter: the total number of
* flows. Flows are generated independently: source is selected randomly using a
* uniform distribution whereas destination follows a hot region distribution.
* Flows in the hot-spot traffic pattern are generated uniformly at random with
* the exception that server-node 0 is $100\times$ more likely to be the
* destination of each flow than any other given server-node.
*
* @param nservers The number of communicating servers
* @param nparam The number of parameters in the list of parameters
* @param params A list with the parameters for the traffic pattern
*/
void init_pattern_hotspot(long nservers, long nparam, long *params)
{
	long i;
	servers=nservers;
	if(nparam!=1) {
		printf("1 parameter is needed for HOTSPOT <#flows>\n");
		exit(-1);
	}
	traffic_param_tokens=malloc(nparam*sizeof(char*));
	for (i = 0; i < nparam; i++) {
        traffic_param_tokens[i]=malloc(20*sizeof(char));//needs to be freed
		snprintf(traffic_param_tokens[i],20,"nhsflows");
	}
	curr_flows=0;
	max_flows=params[0];
}

/**
* Get next flow
* @see next_flow_hotspot
*/
flow_t next_flow_hotspot()
{
	flow_t tmp;
	if (curr_flows++ == max_flows) {
		tmp.src = -1;
		tmp.dst = -1;
	} else {
		tmp.src=rand()%servers;
		tmp.dst=rand()%(servers+99);
		if (tmp.dst < 100) // The first 100 tickets [0..99] go to the hot spot
			tmp.dst=0;
		else
			tmp.dst=tmp.dst-99; // The rest [100..servers+99] are uniform to the rest of the servers [1..servers]
	}
	return tmp;
}
