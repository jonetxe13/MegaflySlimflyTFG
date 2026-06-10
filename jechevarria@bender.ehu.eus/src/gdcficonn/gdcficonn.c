/**
 * @file
 *
 *@mainpage
 * GDCFiConn topology
 *
 * GDCFiConn stands for Generalized DCell and FiConn.  It currently implements two types of
 * proxy-routing for DCell alpha and beta, as well as FiConn, but is extensible to other
 * connection rules and proxy-routing algorithms.
 *
 * The proxy-routing algorithm is described in our paper at ISPA 2015.  We map the notation and
 * elaborate on Table I from that paper in this documentation.  We borrow python's slice
 * notation x[0:n]=(x[0],x[1],...,x[n-1]) and we let D_m denote a level m GDCFiConn, where m
 * \le k and n is understood, residing in a GDCFiConn_{k,n}, where k='param_k'.  D_{m} contains
 * 'tk[m]' server-nodes, and 'gk[m]' copies of D_{m-1} (we define 'gk[0]=tk[0]'), denoted t_m,
 * g_m below.  The copies of D_{m-1} in D_m are indexed from 0 to g_m-1.  Where the index of
 * D_{m-1} is a, we write D_{m-1}^a.  When introducing expressions used in the source code (or
 * otherwise distinguish them from other notation), we enclose them in ' '.
 *
 * A server-node u in D_m has label 'label[u][:m+1]'=(u_0, u_1, ..., u_m), where
 * 'uid_i[u][m]'=u_0+u_1*t_0+u_2*t_1+ ... + u_m*t{m-1}, and u_i is the index of the containing
 * D_{i-1}, so that 0\le u_i < g_i.  For example u_1*t_0 denotes that u resides in the u_1th
 * copy of D_0, within some D_1, and since each of those copies has t_0 servers, we "advance"
 * the uids by t_0 places for each one.
 *
 * D_m also has a label in D_k that contains it --- label[u][m+1:] --- as well as a uid ---
 * uid_i[u][k]-uid_i[u][m].  We can express u by its uid in some D_{m-1}, and give the index of
 * this D_{m-1} within its containing D_m (this is convenient when selecting proxy D_{m-1}s, of
 * course):  (label[u][m],uid_i[u][m-1]).
 *
 * The recursive nature of the routing algorithm implemented here is highly dependent on a the
 * connection rule for adding level-m links between D_{m-1}s to form D_m, expressed in terms
 * the indices of two D_{m-1}s, 'a' and 'b', and the level m (see 'get_link(a,b,lvl)').  The
 * output of get_link(...) is simply a pair of nodes, one in D_{m-1}^a and the other in
 * D_{m-1}^b, and the calling function puts these in the context of some particular D_m.
 *
 * Fig 1 of our ISPA paper concisely describes our proxying strategy: Where m is the smallest
 * integer such that D_m contains both src and dst, let src \in D_{m-1}^a and dst \in
 * D_{m-1}^b, and locate some distinct D_{m-1}^c to use as a proxy.  Use the level-m links from
 * D_{m-1}^a to D_{m-1}^c and from D_{m-1}^c to D_{m-1}^b to join up three paths computed
 * recursively within the D_{m-1}s.  We test the proxying strategy in a few candidates c for
 * which at least one of the recursive paths is either contained within a D_0 (tight strategy)
 * or within a D_1.  Suppose w\in D_{m-1}^a and x,y\in D_{m-1}^c and z\in D_{m-1}^b so that
 * (w,x) and (y,z) are the level-m bridge links.  We want recursive paths src to w and x to y
 * and z to dst to be short.  For the first pair, (src,w), the condition is
 * uid_i[src][m-1]-uid_i[src][tt] == uid_i[w][m-1]-uid_i[w][tt], where tt is the tightness
 * level (0 or 1).  We will invert get_link(a,c)=[w,x] to obtain a set of of values that
 * satisfy c, where a is known, w is subject to the above, and x doesn't matter at this point.
 * For each c, we use get_link(c,b)=[y,z] to find compute the remainder of the path.  For the
 * connection rules at hand, we must consider the cases c<a and a<c.
 *
 * Let src_d = uid_i[src][m-1]-uid_i[src][tt]
 *
 * DCELL_ALPHA:
 *
 * Case: c<a.  get_link(c,a,m)=[a-1,c] (recall c,a are integers, so a-1 and c are simply
 * nodes within D_{m-1}^c and D_{m-1}^a, respectively.
 *
 * thus, src_d \le c \le min(src_d+t_{tt}-1,a-1), where c \le a-1, since c<a, and with w=c in
 * this range, we satisfy the condition on w
 *
 * Case: a<c.  get_link(a,c,m)=[c-1,a]
 *
 * thus, max(src_d,a+1) + 1 \le c \le src_d + t_{tt} - 1 + 1, where a < c by assumption, and
 * since w=c-1, this satisfies the aforementioned assumption on w
 *
 * the two inequalities can be combined to yield: src_d \le c \le src_d + t_{tt}, c \neq a
 *
 * the same steps must be carried out for dst_d and z (also expressed in terms of c), from
 * which we obtain: dst_d \le c \le dst_d + t_{tt}, c \neq b.  Note that this may or may not be
 * equal to the interval computed prior.
 *
 * Finally, we remark that the proximity of x to y within D_{m-1}^c does not depend on the
 * choice of c, but on the values of a and b, which, indeed, we do not chose.
 *
 * FICONN: The connection rule for FiConn and DCell are similar, and this is reflected below.
 *
 * Case: c<a.  get_link(c,a)=[(a-1)*2^m + 2^{m-1} - 1, c*2^m + 2^{m-1} - 1]
 *
 * Thus, src_d \le c*2^m + 2^{m-1} - 1 \le min(src_d+t_{tt}-1, (a-1)*2^m + 2^{m-1} - 1), and equivalently,
 *
 * ceil((src_d-2^{m-1}+1)/2^m) \le c \le min( floor( (src_d - 2^{m-1}+1+t_{tt}-1)/2^m ), a-1)
 *
 * Notice that we have been slightly careless by not using the fact that 0\le c, as the
 * leftmost expression can be negative (src_d can clearly be 0, and m can be anything!).  Also,
 * for efficiency we will use an integer ceiling operation, such as num/den+(num%dem!=0), and
 * since we only care for num \ge 0, we shall deal with num<0 separately.
 * But first let us combine this with the case a<c.
 *
 * Case: a<c.  get_link(a,c)=[(c-1)*2^m + 2^{m-1} - 1, a*2^m + 2^{m-1} - 1]
 *
 * Thus, max(src_d, a*2^m + 2^{m-1} - 1) \le (c-1)*2^m + 2^{m-1} - 1 \le src_d+t_{tt}-1, and equivalently,
 *
 * max(ceil((src_d-2^{m-1}+1)/2^m),a)+1 \le c \le floor( (src_d - 2^{m-1}+1+t_{tt}-1)/2^m ) + 1
 *
 * Combining the above,
 * low=((src_d-2^{m-1}+1>0)?ceil((src_d-2^{m-1}+1)/2^m):0) and
 *
 * high=((src_d+t_{tt}-1+2^{m-1}+1<t_{m-1})?floor( (src_d - 2^{m-1}+1+t_{tt}-1)/2^m ): g_m-1).  Now,
 *
 * low \le c \le high.
 *
 * We explain the second conditional.  src_d+t{tt}-1<t_{m-1} is a tight bound, so it is
 * possible to exceed this in the numerator of the floor function.  In FiConn,
 * g_m=t_{m-1}/2^m+1, and we must have c<g_m (since c is, by definition, and index in
 * [0,g_m-1]).
 *
 * Versions:
 *
 * v0.2.0 low bottleneck subpaths preferred in proxy_routing
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

//#include "../inrflow/misc.h"
///< We want the histogram functions
#include "../inrflow/reporting.h"
//#include "../ficonn/routelist.h"
//#include "../inrflow/list.h"
#include "../inrflow/globals.h"
#include "gdcficonn.h"

#define DCELL_ALPHA 0
#define DCELL_BETA 1
#define FICONN 2
#define MAX_GDCFICONN_LEVELS 6
#define MAX_INTRS 8
// extern long long ralloc, rfree;

#define MAX_TOKEN_BUFFER 100

extern node_t* network;
extern routing_t routing;///< this is the routing literal

static long param_k;	///< parameter k of the topology, number of levels of the gdcficonn (-1)
static long param_n;	///< parameter n of the topology, switch radix
static long param_t; ///< tightness parameter.  must be 0<=param_t < param_k
static long alpha;///< parameter determines what network this is, including connection rule

static long *n_pow;	///< an array with the n^i, useful for doing some calculations.
static long *two_pow;	///< an array with the 2^i, useful for doing some calculations.
static long *gk;	///< number of gdcficonn_k-1 to form a gdcficonn_k
static long *tk;	///< number of servers in a gdcficonn_k
static long **label;
static long **uid_i;
static long *usedports;
static long link[2];
static bool_t proxy_array[MAX_GDCFICONN_LEVELS];///< proxy_array[i] == TRUE if we will do proxies here.

static long switches;	///< The total number of switches : 2(k+2)*((n/4)^(2^k))
static long servers; 	///< The total number of servers : switches/n
static long ports;		///< The total number of links

///< Throughout this file, route list data is the port number, so just a long, and not a struct
static list_t route_list; ///< this the list that proxy_routing will append to on the top level

static char *network_token="gdcficonn";
static char *routing_token;
static char *topo_version="v0.2.0";

static char *topo_param_tokens[3]= {"k","n","alpha"};
static char **routing_param_tokens;

static long long dr_nconnected;
static long *dr_hist;
static long max_hop_length;
static long *nproxy_hist;///< malloc to max_num_proxies+1
static long max_num_proxies;
static long min_proxy_dimension;
static long *succ_nproxy_hist;///< malloc to max_num_proxies+1
static const char* hist_docs[3] = { "Number of proxies tested per path (proxies tested during a route, number of occurrences)",
                                    "Number of successful proxies found (at any level of the recursion).  Successful iff connected AND (dim route failed || at most as long as dim route)",
                                    "Dimensional route hop-length histogram (server hop path length of dimensional route, number of occurrences)"};

static long break_flag;

static long max_good_proxies = 10;

extern char filename_params[100];

//static long dimensional_routing(list_t *r_list, long src,long dst, long lvl);
static void make_tk();
static void make_gk();
static void construct_labels();
//static long uid_i(long * L,long i);
static tuple_t append_to_route(list_t * r_list, long node, long port, bool_t dry_run);
static long make_connections();
static long rec_connections(long suffix_id, long lvl);
static long get_link(long a, long b, long lvl);
static bool_t proxy_this_lvl(long lvl);
static long compute_proxies(long * intrs,long intrs_length, long src, long dst, long m);
static long intervals_intersect(long * C,long *A,long *B);
static long intervals_union(long * intrs,long intrn);
static tuple_t proxy_routing(list_t *r_list,long src, long dst, long lvl, bool_t noproxies, bool_t dry_run, long *total_nproxies, long *dr_hops, long *succ_nproxies);
static void shuffle_array(long* A, long n);

/// tuple_t is defined in node.h.  We are using it for a slightly different purpose than
/// recording node and port.  See below
//typedef struct tuple_t {
//	long node;	/// for proxy_routing(): the length of a path or -1 if faulty
/// for append_to_route: the next_node or -1 if faulty
//	long port;	/// for proxy_routing(): number of flows in the bottleneck edge or -1 if faulty
/// for append_to_route: the number of flows on the link added.
//} tuple_t;

long init_topo_gdcficonn(long   np, long* par){
	long i;

	if (np<3) {
		printf("3 parameters are needed for GDCFICONN <k, n, dfa>"
					 ", where dfa a choice between different connection"
					 "rules for DCell or FiConn\n");
		exit(-1);
	}
	param_k=par[0];
	if(param_k+1 > MAX_GDCFICONN_LEVELS){
		printf("ERROR: too many gdcficonn levels requested.  Increase MAX_GDCFICONN_LEVELS.\n");
		exit(-1);
	}
	param_n=par[1];
	alpha=par[2];
	sprintf(filename_params,"k%ldn%lda%ld",param_k,param_n,alpha);
	for(i=0;i<param_k+1;i++){
		proxy_array[i]=FALSE;
	}

	if(alpha == FICONN && param_n%2 != 0){
		printf("ERROR: FiConn requires even parameter n, but input was %ld.\n",param_n);
		exit(-1);
	}
	// powers of param_k and powers of 2
	n_pow=malloc((param_k+3)*sizeof(long));
	n_pow[0]=1;
	two_pow=malloc((param_k+3)*sizeof(long));
	two_pow[0]=1;
	for (i=1; i<param_k+3; i++) {
		n_pow[i]=n_pow[i-1]*param_n;
		two_pow[i]=two_pow[i-1]*2;
	}
	list_initialize(&route_list,sizeof(long));
	make_tk();
	make_gk();

	if(routing == GDCFICONN_DIMENSIONAL){
		routing_token = "dim";
	}else if(routing == GDCFICONN_PROXY){
		routing_token = "proxy";
		if(routing_nparam<1){
			printf("ERROR: Expected at least one proxy routing parameter.\n");
			exit(-1);
		}
		routing_param_tokens=malloc(routing_nparam*sizeof(char*));
		routing_param_tokens[0]=malloc(10*sizeof(char));
		//"tightness"
		snprintf(routing_param_tokens[0],10,"tightness");
		for(i=1;i<routing_nparam;i++){
			routing_param_tokens[i]=malloc(MAX_TOKEN_BUFFER*sizeof(char));
			snprintf(routing_param_tokens[i],MAX_TOKEN_BUFFER,"proxydim%ld",i);
		}
		param_t=routing_params[0];
		if (param_t <0 || param_t>=param_k) {
			printf("tightness parameter param_t %ld must be at least 0 and at most param_k-1 (%ld)\n"
						 ,param_t,param_k-1 );
			exit(-1);
		}
		sprintf(filename_params+strlen(filename_params),"_t%ld",param_t);
		/*  if(routing_params[0]==0){
			tight_proxy=TRUE;
			sprintf(filename_params+strlen(filename_params),"_t0");
		}else if(routing_params[0]==1){
			tight_proxy=FALSE;
			sprintf(filename_params+strlen(filename_params),"_t1");
		}else{
			printf("ERROR: First proxy routing parameter should be x=0 or x=1, to indicate a level-x tightness for recursively called proxy paths.\n");
			exit(-1);
		}
		*/

		// min_proxy_dimension and max_num_proxies are for computing an upper bound on the number
		// of proxies that will be used.  In a call at lvl 'i' to proxy_route where we will invoke
		// proxies, there will be at most 2*(tk[tt]-1) proxies tested, where tt is the level of
		// tightness, 0 or 1.  For each of these, at most 2 recursive calls can call proxies at a
		// lower level, since the criteria for proxying is that one of the recursively constructed
		// paths is "short".  In addition, regardless of whether we proxy at level i or not, we
		// shall try one instance of no-proxies at this level, which gives 2 more recursive calls
		// where proxies can occur at a lower level.  Let p_i=2*(tk[tt]-1)+2 if i is a proxy
		// dimension and otherwise let p_i=2.  Let m be the smallest proxy dimension.  The number
		// of proxies used is at most p_{k}*p_{k-1}*...*p_m.  We need to get m=min_proxy_dimension
		// here and do the aforementioned computation below using proxy_array[] below.
		min_proxy_dimension=param_k;
		for(i=1;i<routing_nparam;i++){
			if(routing_params[i]<= param_k && routing_params[i]>1){
				proxy_array[routing_params[i]]=TRUE;
				sprintf(filename_params+strlen(filename_params),"_%ld",routing_params[i]);
				if(routing_params[i]<min_proxy_dimension)
					min_proxy_dimension=routing_params[i];
			}else{
				printf("ERROR: Invalid routing parameter, %ld.\n", routing_params[i]);
				exit(-1);
			}
		}

		max_num_proxies=1;
		// OLD long tktt=tk[(tight_proxy?0:1)];
		long tktt=tk[param_t];
		for(i=min_proxy_dimension;i<=param_k;i++){
			if(proxy_array[i]){
				max_num_proxies = 2*max_num_proxies*(2*tktt-1)+2*(tktt-1);
			}else{
				max_num_proxies*=2;
			}
		}
		max_num_proxies++;
		//max_num_proxies*=(proxy_array[i])?2*(tk[(tight_proxy)?0:1]-1)+2:2;
		// 'nproxy_hist[i]' is the number of times a route tried 'i' proxies.
		//nproxy_hist = malloc((max_num_proxies+1)*sizeof(long));
		init_hist_array(&nproxy_hist,max_num_proxies+1);
		init_hist_array(&succ_nproxy_hist,max_num_proxies+1);
		if(!are_there_failures()){
			max_hop_length=two_pow[param_k+1];
			// allocating for longest possible path of 2^{k+1}-1
			init_hist_array(&dr_hist,max_hop_length);
		}
	}else{
		printf("ERROR: Invalid routing algorithm.\n");
		exit(-1);
	}

	servers=tk[param_k];
	switches=servers/param_n;

#ifdef DEBUG
	printf("There are %ld servers, %ld switches.\n",servers,switches);
#endif

	switch(alpha){
	case DCELL_ALPHA:
	case DCELL_BETA:
		ports=(param_k+1)*servers+param_n*switches;
		break;
	case FICONN:
		ports=2*servers+param_n*switches-servers/(two_pow[param_k]);
		break;
	default:
		printf("Unrecognised connection rule %ld\n",alpha);
		exit(-1);
		break;
	}

	construct_labels();
	return 1; // return status, not used here.
}

void finish_topo_gdcficonn(){
	long i=-1;
	for(i=0;i<servers+switches;i++){
		free(label[i]);
	}
	free(label);
	for(i=0;i<servers;i++){
		free(uid_i[i]);
	}
	finish_hist_array(&nproxy_hist);
	finish_hist_array(&succ_nproxy_hist);
	if(!are_there_failures())
		finish_hist_array(&dr_hist);
	for(i=0;i<routing_nparam;i++)
		free(routing_param_tokens[i]);
	free(routing_param_tokens);
	free(uid_i);
	free(tk);
	free(gk);
        free(n_pow);
        free(two_pow);
	list_destroy(&route_list);
}

long get_servers_gdcficonn(){
	return servers;
}

long get_switches_gdcficonn(){
	return switches;
}

long get_ports_gdcficonn(){
	return ports;
}

long is_server_gdcficonn(long i){
	return (i<servers);
}

long get_server_i_gdcficonn(long i){
	return i;
}

long get_switch_i_gdcficonn(long i){
	return servers+i;
}

long node_to_server_gdcficonn(long i){
	return i;
}

long node_to_switch_gdcficonn(long i){
	return i-servers;
}

long get_radix_gdcficonn(long n){

	if (n<servers)
			switch(alpha){
	case DCELL_ALPHA:
	case DCELL_BETA:
		return param_k+1;
		break;
	case FICONN:
		return 2;
		break;
	default:
		printf("Unrecognised connection rule %ld\n",alpha);
		exit(-1);
		break;
	}
	else
		return param_n; // If this is a switch it has n ports
}

tuple_t connection_gdcficonn(long node, long port){
	if(node==0 && port ==0){
			make_connections();
	}
	return network[node].port[port].neighbour;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_gdcficonn(long src, long dst){

    return(1);
}

long init_routing_gdcficonn(long src, long dst){
	long res,total_nproxies=0, dr_route=0, succ_nproxies=0;
	if(list_length(&route_list)>0){
		printf("ERROR: init_routing with a non-empty route_list, length %ld.\n",route_list.length);
		exit(-1);
	}
	res = proxy_routing(&route_list,src,dst,param_k,(routing==GDCFICONN_DIMENSIONAL),
											FALSE,&total_nproxies, &dr_route, &succ_nproxies).node;
#ifdef DEBUG
	if(routing==GDCFICONN_PROXY){
		if(total_nproxies < 0 || total_nproxies > max_num_proxies){
			printf("ERROR: used more proxies than projected maximum.  max=%ld, nproxies=%ld.\n",
						 max_num_proxies,total_nproxies);
			exit(-1);
		}
		if(!are_there_failures() && (dr_route<0 || dr_route > max_hop_length)){
			printf("ERROR: dr_route longer than max_hop_length.  max=%ld, dr_route=%ld.\n",
						 max_hop_length,dr_route);
			exit(-1);
		}
		//printf("%ld, %ld.\n",dr_route, res);
		if((a_better_than_b(dr_route,res))){
			printf("ERROR: no failures but we chose a longer path than dr_route.\n");
			exit(-1);
		}
	}
#endif
	if(routing==GDCFICONN_PROXY){
		update_hist_array(total_nproxies,&nproxy_hist,max_num_proxies+1);
		update_hist_array(succ_nproxies,&succ_nproxy_hist,max_num_proxies+1);
		if (dr_route>-1){
			dr_nconnected++;
		}
		if(!are_there_failures())
			update_hist_array(dr_route,&dr_hist,max_hop_length);
	}
	return res;
}
void finish_route_gdcficonn(){
#ifdef DEBUG
	if(!are_there_failures() && list_length(&route_list)>0){
		printf("expected path to be empty.\n");
		exit(-1);
	}
#endif
	list_destroy(&route_list);
}

long route_gdcficonn(long current, long destination){
	long *this_port, res;
	if(!list_empty(&route_list)){
		list_head(&route_list,(void*)&this_port);
		res=*this_port;
		list_rem_head(&route_list);
		return res;
	}else{
#ifdef DEBUG
		if(!are_there_failures()){
			printf("ERROR: No network faults but failed to connect path.  curr: %ld dst: %ld.\n", current, destination);
			exit(-1);
		}
#endif
		return -1;
	}
}

char *get_network_token_gdcficonn(){
	return network_token;
}

char *get_routing_token_gdcficonn(){
	return routing_token;
}

char *get_topo_version_gdcficonn(){
	return topo_version;
}

char *get_topo_param_tokens_gdcficonn(long i){
	return topo_param_tokens[i];
}

char *get_routing_param_tokens_gdcficonn(long i){
	return routing_param_tokens[i];
}

char *get_filename_params_gdcficonn(){
	return filename_params;
}

long get_topo_nstats_gdcficonn(){
	if(routing==GDCFICONN_PROXY){
		if(!are_there_failures())
			return 19;
		else
			return 13;
	}else{ return 0;}
}

struct key_value get_topo_key_value_gdcficonn(long i){
	switch(i){
	case 0:
		return make_key_value_node("min.n.proxies","%ld",min_hist_array(nproxy_hist,max_num_proxies+1));
		break;
	case 1:
		return make_key_value_node("max.n.proxies","%ld",max_hist_array(nproxy_hist,max_num_proxies+1));
		break;
	case 2:
		return make_key_value_node("mean.n.proxies","%f",mean_hist_array(nproxy_hist,max_num_proxies+1));
		break;
	case 3:
		return make_key_value_node("median.n.proxies","%f",median_hist_array(nproxy_hist,max_num_proxies+1));
		break;
	case 4:
		return make_key_value_node("var.n.proxies","%f",var_hist_array(nproxy_hist,max_num_proxies+1));
		break;
	case 5:
		return make_key_value_node("std.n.proxies","%f",std_dev_hist_array(nproxy_hist,max_num_proxies+1));
		break;
	case 6:
		return make_key_value_node("min.succ.n.proxies","%ld",min_hist_array(succ_nproxy_hist,max_num_proxies+1));
		break;
	case 7:
		return make_key_value_node("max.succ.n.proxies","%ld",max_hist_array(succ_nproxy_hist,max_num_proxies+1));
		break;
	case 8:
		return make_key_value_node("mean.succ.n.proxies","%f",mean_hist_array(succ_nproxy_hist,max_num_proxies+1));
		break;
	case 9:
		return make_key_value_node("median.succ.n.proxies","%f",median_hist_array(succ_nproxy_hist,max_num_proxies+1));
		break;
	case 10:
		return make_key_value_node("var.succ.n.proxies","%f",var_hist_array(succ_nproxy_hist,max_num_proxies+1));
		break;
	case 11:
		return make_key_value_node("std.succ.n.proxies","%f",std_dev_hist_array(succ_nproxy_hist,max_num_proxies+1));
		break;
	case 12:
		return make_key_value_node("dr.connected","%lld",dr_nconnected);
		break;
	case 13:
		return make_key_value_node("min.dr.hop","%ld",min_hist_array(dr_hist,max_hop_length));
		break;
	case 14:
		return make_key_value_node("max.dr.hop","%ld",max_hist_array(dr_hist,max_hop_length));
		break;
	case 15:
		return make_key_value_node("mean.dr.hop","%f",mean_hist_array(dr_hist,max_hop_length));
		break;
	case 16:
		return make_key_value_node("median.dr.hop","%f",median_hist_array(dr_hist,max_hop_length));
		break;
	case 17:
		return make_key_value_node("var.dr.hop","%f",var_hist_array(dr_hist,max_hop_length));
		break;
	case 18:
		return make_key_value_node("std.dr.hop","%f",std_dev_hist_array(dr_hist,max_hop_length));
		break;
	default:
		printf("ERROR: invalid key_value requested i=%ld.\n",i);
		exit(-1);
	}
}

long get_topo_nhists_gdcficonn(){
	if(routing==GDCFICONN_PROXY){
		if(!are_there_failures())
			return 3;
		else
			return 2;
	}else return 0;
}

char get_topo_hist_prefix_gdcficonn(long i){
	switch(i){
	case 0:
		return 'r';// nproxies
		break;
	case 1:
		return 's';// succ_nproxies
		break;
	case 2:
		return 'd';// dim_route length
		break;
	default:
		printf("ERROR: invalid histogram requested i=%ld.\n",i);
		exit(-1);
	}
}

const char *get_topo_hist_doc_gdcficonn(long i){
	return hist_docs[i];
}

long get_topo_hist_max_gdcficonn(long i){
		switch(i){
	case 0:
	case 1:
		return max_num_proxies+1;
		break;
	case 2:
		return max_hop_length;
		break;
	default:
		printf("ERROR: invalid histogram requested i=%ld.\n",i);
		exit(-1);
	}
}

void get_topo_hist_gdcficonn(long *topo_hist, long i){
		switch(i){
	case 0:
		memcpy(topo_hist,nproxy_hist,(max_num_proxies+1)*sizeof(long));
		break;
	case 1:
		memcpy(topo_hist,succ_nproxy_hist,(max_num_proxies+1)*sizeof(long));
		break;
	case 2:
		memcpy(topo_hist,dr_hist,max_hop_length*sizeof(long));
		break;
	default:
		printf("ERROR: invalid histogram requested i=%ld.\n",i);
		exit(-1);
	}
}

// Begin static functions *******************************************************

/**
 * @brief Return lowest level m of D_{m} containing both servers 'a' and 'b' within a D_{lvl}.
 *
 * Return m, where label[a][m]!=label[b][m], but all larger label[a][i] label[b][i] are
 * equal. We assume that label[a][lvl+1:]=label[b][lvl+1:] See file description for details.
 *
 * @param a server in D_{lvl}
 * @param b  server in D_{lvl}
 * @param lvl  lvl of D_{lvl} containing servers 'a' and 'b'
 * @return long
 */
static long comm_suff(long a, long b,long lvl){
	long i;
#ifdef DEBUG
	for(i=param_k;i>lvl;i--){
		if(label[a][i]!=label[b][i]){
			printf("ERROR: we assume label[a][lvl+1:]==label[b][lvl+1:] in common suffix.\n(a,b,lvl)=(%ld,%ld,%ld)",a,b,lvl);
			exit(-1);
		}
	}
#endif
	for(i=lvl;i>=0;i--){
		if(label[a][i]!=label[b][i])
			break;
	}
	return i;
}

/**
 * @brief If not faulty then return the node that connects to 'node' at 'port'. If 'r_list != NULL'
 * then append 'port' to r_list.  If 'port' at 'node' is faulty, then return -1 and leave
 * 'r_list' untouched.
 *
 * @param r_list a list_t of longs (or NULL)
 * @param node current node
 * @param port from 'node' to next node
 * @param dry_run TRUE iff we do not append to r_list
 */
static tuple_t append_to_route(list_t * r_list, long node, long port, bool_t dry_run){
	tuple_t res;
	if(network[node].port[port].faulty!=0){
		res.node = -1;
		res.port = -1;
		return res;
	}
	if(!dry_run)list_append(r_list,&port);
	res.node = network[node].port[port].neighbour.node;
	res.port = network[node].port[port].flows;
	return res;
}

/**
 * @brief returns port to level-m link that connects two D_{m-1}s together, where
 * 0<m<param_k+1.
 *
 * In all cases port 0 is to switch.
 *
 * For ficonn, if there is a level-m link then port 1 is to the level-m link
 *
 * The way rec_connections is constructed, port 1 is for lvl param_k.  port 2 is for lvl
 * param_k-1, and so on, until port param_k is for level 1

 * @param a is a server
 * @param m is a level, where level-m links join two D_{m-1}s
 * @return long
 */
static long get_port(long a, long m){
	switch(alpha){
	case DCELL_ALPHA:
	case DCELL_BETA:
		return param_k-m+1;
		break;
	case FICONN:
		if(network[a].port[1].neighbour.node==-1){
			printf("Error: requesting port to server from a ficonn node of degree 1.\n");
			exit(-1);
		}
		return 1;
		break;
	default:
		printf("Unrecognised connection rule %ld\n",alpha);
		exit(-1);
		break;
	}
}

/**
 * @brief Recursive routing function with both dimensional and proxy routing, plus stats
 * tracking.
 *
 * If a fault-free path is found, return hop-length of route from src to dst and append the
 * route to 'r_list' if 'r_list!=NULL'.  A proxy route is successful if it is connected
 * (return value != -1) and it is at most as long as the dimensional route.
 *
 * dim-proxy routing is a route that is "dimensional" at this level, but uses a proxy at a
 * lower level.
 *
 * @param r_list struct list_t path that we append to if succesful and !'dry_run'. Or NULL
 * @param src source node in same D_{lvl} as dst
 * @param dst destination node in same D_{lvl} as src
 * @param lvl level of this D_{lvl}
 * @param noproxies TRUE iff we should not proxy here or in any recursive call
 * @param dry_run TRUE iff we should not append to *r_list
 * @param total_nproxies set *total_nproxies to the total number of proxies explored here and in recursive sub-calls. Or NULL.
 * @param dr_hops set *dr_hops to the number of server-hops used in dimensional routing if connected, else return -1. Not NULL.
 * @param succ_nproxies set *succ_nproxies to the total number of succsessful proxies (see detailed description). Or NULL.
 * @return long the length of this proxy path if connected, else return -1
 */
static tuple_t proxy_routing(list_t *r_list,long src, long dst, long lvl,
                            bool_t noproxies, bool_t dry_run, long *total_nproxies,
                            long *dr_hops, long *succ_nproxies){
	long m,a,b,suffix_id,c,d,p;
	long this_route,best_route,dim_route,dim_proxy_route,proxy_route;
	long intrs[MAX_INTRS],intrs_length=MAX_INTRS,intrn;
	tuple_t sub_route1,sub_route2,sub_route3;
	list_t b_list, c_list;// interim struct list_t routes c_list reserved for dim-proxy route
	long best_p;
	bool_t dry_call;
	long succ_nproxies_a=0,succ_nproxies_b=0,succ_nproxies_c=0;
	long nproxies_a=0,nproxies_b=0,nproxies_c=0;
	long i,j,nbests=0;
	long nproxies_list=0;
	long *proxies_list;
	long pli = 0;
	long rnd;
	long proxy_bottleneck,dim_proxy_bottleneck,dim_bottleneck,best_bottleneck,this_bottleneck;
	tuple_t route_src_info,route_dst_info;
	tuple_t next_node_and_flows,next_node_and_flows2;
	tuple_t res;
	m=comm_suff(src,dst,lvl);
	if(m<0){//src == dst
		if(lvl == param_k)
			*dr_hops=0;
		res.node=0;// path length 0
		res.port=0;// bottleneck 0
		return res;// return the length of the sub-path
	}
	if(!dry_run)
		list_initialize(&b_list,sizeof(long));
	if(m==0){//src and dst are connected to the same switch
		if(((next_node_and_flows=append_to_route(&b_list,src,0,dry_run)).node==-1) ||
			 ((res=append_to_route(&b_list,next_node_and_flows.node,dst%param_n,dry_run)).node==-1)){
			if(!dry_run)list_destroy(&b_list);
			dim_route = -1;
		}else{
			if(!dry_run){
				list_concat(r_list,&b_list);
				//list_destroy(&b_list);
			}
			dim_route = 1;
		}
		if(lvl == param_k)
			*dr_hops=dim_route;
		res.node=dim_route;
		if(res.node==-1) res.port=-1;
		else res.port=max(res.port,next_node_and_flows.port);
		return res;// return the hop-length of the sub-path
	}
	suffix_id=uid_i[src][param_k]-uid_i[src][m];
#ifdef DEBUG
	if(suffix_id!=uid_i[dst][param_k]-uid_i[dst][m]){
		printf("ERROR: expected suffix_is of src and dst to match.\n(src,dst,lvl)=(%ld,%ld,%ld)",src,dst,lvl);
		exit(-1);
	}
#endif

	// Begin dimensional routing.  b_list initialized above, and empty. no need for bookmark
	dim_route = best_route = -1;
	dim_bottleneck=best_bottleneck=-1;
	get_link(label[src][m],label[dst][m],m);
	a=suffix_id+label[src][m]*tk[m-1]+link[0];
	b=suffix_id+label[dst][m]*tk[m-1]+link[1];
	if(((route_src_info=proxy_routing(&b_list,src,a,m-1,TRUE,dry_run,NULL,NULL,NULL)).node==-1) ||
		 ((next_node_and_flows=append_to_route(&b_list,a,get_port(a,m),dry_run)).node==-1) ||
		 (route_dst_info=proxy_routing(&b_list,b,dst,m-1,TRUE,dry_run, NULL,NULL,NULL)).node==-1){
		if(!dry_run)list_destroy(&b_list);
		dim_route=best_route=-1;
		dim_bottleneck=best_bottleneck=-1;
		nbests=0;
	}else{
		dim_route=best_route=route_src_info.node+route_dst_info.node+1;
		dim_bottleneck=best_bottleneck=max(route_src_info.port,max(route_dst_info.port,next_node_and_flows.port));
		nbests=1;
	}
	// -1 if not connected
	if(lvl == param_k)
		*dr_hops=dim_route;
	// b_list is empty now

	// if(!dry_run) b_list contains dimensional route if it was successful and this was not a
	// dry_run We need need to concat this to r_list if not dry_run, regardless of noproxies

	dim_proxy_route=-1;
	dim_proxy_bottleneck=-1;
	proxy_route=-1;
	proxy_bottleneck=-1;
	if(!dry_run)
		list_initialize(&c_list,sizeof(long));
	// If requested at this level, perform proxy search with dry runs (specified by dry_call)
	///and then call the "best" proxy again with a wet run (by changing dry_call).
	if(!noproxies){
		*succ_nproxies=0;
		*total_nproxies=0;
		if(m>min_proxy_dimension){// this is dim-proxy route
			// If we will proxy in a dimension lower than this we should try a non-proxy in this
			// dimension (regardless of whether we proxy here).
			if(((route_src_info=proxy_routing(&c_list,src,a,m-1,TRUE,dry_run, &nproxies_a,NULL, &succ_nproxies_a)).node==-1) ||
				 ((next_node_and_flows=append_to_route(&c_list,a,get_port(a,m),dry_run)).node==-1) ||
				 (route_dst_info=proxy_routing(&c_list,b,dst,m-1,TRUE,dry_run, &nproxies_b,NULL, &succ_nproxies_b)).node==-1){
				if(!dry_run)list_destroy(&c_list);
			}else{// have a_con(dim_proxy_route), so if a_better_than_b(dim_proxy_route,dim_route)
						// we must have proxied at lower dimension.
				dim_proxy_route=best_route=route_src_info.node+route_dst_info.node+1;
				dim_proxy_bottleneck=max(route_src_info.port,max(next_node_and_flows.port,route_dst_info.port));
				//AE: need to replace best_bottleneck with dim_proxy_bottleneck if dim_proxy_route <
				// dim_route or if dim_proxy_route == dim_route and
				// dim_proxy_bottleneck<dim_bottleneck.

				//We don't count successful proxies found in unsuccessful recursive calls.
				*succ_nproxies+=succ_nproxies_a+succ_nproxies_b;
				if(a_eq_b_con(dim_proxy_route,dim_route)){
					if(a_eq_b_con(dim_proxy_bottleneck,dim_bottleneck)){
						//if dim_proxy equal to dim_route in length AND bottleneck
						nbests=2;
					}else{
						//bottlenecks different.  take best one
						best_bottleneck=min(dim_proxy_bottleneck,dim_bottleneck);
						nbests=1;
					}
				} else{// if(a_con_le_b(dim_proxy_route , dim_route)){//redundant
#ifdef DEBUG
					if(*total_nproxies==0){
						printf("ERROR: total_nproxies should not be zero here.\n");
						exit(-1);
					}
#endif
					best_bottleneck=dim_proxy_bottleneck;// dim_proxy_route strictly shorter
					nbests=1;// number of best alternatives
				}
				// We count nproxies regardless of return values.
				*total_nproxies+=nproxies_a+nproxies_b;
			}
		}// end dim-proxy
		if(proxy_this_lvl(m)){
			//populate and shuffle proxies_list.
			intrn = compute_proxies(intrs,intrs_length,src,dst,m);
			for (i = 0; i < 2*intrn; i+=2) {
				nproxies_list+=intrs[i+1]-intrs[i]+1;
			}
			proxies_list = malloc(nproxies_list*sizeof(long));
			for (i = 0; i < 2*intrn; i+=2) {
				for (j = intrs[i]; j <= intrs[i+1]; j++) {
					#ifdef DEBUG
					if(pli>=nproxies_list){
						printf("ERROR: buffer overflow in proxies_list.\n");
						exit(-1);
					}
					#endif
					proxies_list[pli++]=j;
				}
			}
			shuffle_array(proxies_list,nproxies_list);

			best_p=-1;
			dry_call=TRUE;
			for (i = 0; i < nproxies_list; i++) {
				p = proxies_list[i];
				if(label[src][m] == proxies_list[i] || label[dst][m] == proxies_list[i] )
					continue;
			DoAndQuit:
				get_link(label[src][m],p,m);
				a=suffix_id+label[src][m]*tk[m-1]+link[0];
				b=suffix_id+p*tk[m-1]+link[1];
				get_link(p,label[dst][m],m);
				c=suffix_id+p*tk[m-1]+link[0];
				d=suffix_id+label[dst][m]*tk[m-1]+link[1];
				this_route=-1;
				this_bottleneck=-1;
				succ_nproxies_a=succ_nproxies_b=succ_nproxies_c=nproxies_a=nproxies_b=nproxies_c=0;// reset
				if(((sub_route1=proxy_routing(&b_list,src,a,m-1,FALSE,dry_call, &nproxies_a,NULL, &succ_nproxies_a)).node==-1) ||
					 ((next_node_and_flows=append_to_route(&b_list,a,get_port(a,m),dry_call)).node==-1)||
					 ((sub_route2=proxy_routing(&b_list,b,c,m-1,FALSE,dry_call, &nproxies_c,NULL, &succ_nproxies_c)).node==-1) ||
					 ((next_node_and_flows2=append_to_route(&b_list,c,get_port(c,m),dry_call)).node==-1)||
					 ((sub_route3=proxy_routing(&b_list,d,dst,m-1,FALSE,dry_call, &nproxies_b,NULL, &succ_nproxies_b)).node==-1)){
					// If the subpath fails we needn't do anything, but this should not happen in a wet-run of the while loop
					if(!dry_call){
						printf("ERROR: we don't expect to find faults when routing through a proxy with b_list!=NULL.\n");
						exit(-1);
					}
				}else{
					this_route=sub_route1.node+sub_route2.node+sub_route3.node+2;
					this_bottleneck=max(max(max(sub_route1.port,next_node_and_flows.port),
																	max(next_node_and_flows2.port,sub_route2.port)),
															sub_route3.port);
					if(dry_call && a_con_le_b(this_route,dim_route)){
					//succ_nproxies counted if this is at least as good as a dim route
						*succ_nproxies+=1+succ_nproxies_a+succ_nproxies_c+succ_nproxies_b;
					}
					if(dry_call && a_con_le_b(this_route,best_route)){
						//if this_route is strictly shorter than best_route or, (if they are the same),
						//this_bottleneck is strictly less than best_bottleneck, the there is one nbest.
						//otherwise, (they are the same length and best_bottleneck is at least as good as
						//this_bottleneck), if this_bottleneck is strictly better than best_bottleneck,
						//then increment nbests.
						if(a_better_than_b(this_route,best_route) ||
							 a_better_than_b(this_bottleneck,best_bottleneck)){
							nbests=1;
							best_p=p;
							best_route=proxy_route=this_route;
							best_bottleneck=proxy_bottleneck=this_bottleneck;
						}else if(a_eq_b_con(this_bottleneck,best_bottleneck)){
							nbests++;
							best_p=p;
							best_route=proxy_route=this_route;
							best_bottleneck=proxy_bottleneck=this_bottleneck;
						}
						//if path lengths equal and this_bottleneck>best_bottleneck, then I don't want to
						//set best_p
					}
					if(!dry_call){
						if(this_route != best_route || this_bottleneck != best_bottleneck){
							printf("ERROR: Expected best_route %ld == this_route %ld or best_bottleneck %ld != this_bottleneck %ld.\n",
										 best_route,this_route,best_bottleneck,this_bottleneck);
							exit(-1);
						}
						list_concat(r_list,&b_list);//b_list now empty but initialised
						//if(best_route<1 || best_route != this_route){
						//	printf("ERROR: expected best_route to be the same as this_route in 'wet' run of proxy_route.\n");
						//	exit(-1);
						//}else
						res.node=best_route;
						res.port=best_bottleneck;
						return res;
					}
				}
				// Count this proxy and recursive proxies during dry_calls, regardless of return
				// values of proxy_routing.  This won't execute in wet_call
				*total_nproxies+=1+nproxies_a+nproxies_c+nproxies_b;
			}// end loop for proxies
			free(proxies_list);

			// There are up to 3 types of routes: dim_route, dim-proxy_route, and proxy_route.  If
			// this is a dry_run, we needn't worry about which route to return, and stats for
			// *total_nproxies, *succ_nproxies, and *dr_route have been collected already.  If not a
			// dry run, we want to select among the shortest computed routes with equal probability,
			// and every combination is possible.  Furthermore, if a proxy-route is chosen, we must
			// go back and recompute it with a !dry_call

			// a_con(proxy_route) iff a_con_le_b(proxy_route,dim_route) and
			// a_con_le_b(proxy_route,proxy_dim_route)
			if(!dry_run && a_con(proxy_route)){// a best route is a proxy route (as regards both
																				 // length and bottleneck)
				rnd=ztm(nbests);
				if(nbests >= 3 && //probably saves computation
					 a_eq_b_con(dim_proxy_route, proxy_route) &&
					 a_eq_b_con(proxy_route, dim_route) &&
					 a_eq_b_con(dim_proxy_bottleneck, proxy_bottleneck) &&
					 a_eq_b_con(proxy_bottleneck, dim_bottleneck)
					 ){
					//all three types of routes equal, in length and bottleneck
					//with equal probability, use one of the three best routes
					switch(rnd){
					case 0://use proxy-dim route
						list_destroy(&b_list);// destroy the dimensional route
						list_concat(r_list,&c_list);// use the dim_proxy route
						res.node=best_route;
						res.port=best_bottleneck;// AE: temporary value
						return res;
						break;
					case 1://use dim route
						list_destroy(&c_list);// destroy proxy-dim route
						list_concat(r_list,&b_list);// use the dim_proxy route
						res.node=best_bottleneck;
						return res;
						break;
						//default: fall through and do best_p proxy route
					}
				}else if(a_eq_b_con(proxy_route,dim_proxy_route) &&
								 a_eq_b_con(proxy_bottleneck,dim_proxy_bottleneck)){
					if(rnd<1){
						//in this case we may have equal dim-proxy and proxy (we know proxy at least as
						//good)
						list_destroy(&b_list);// destroy the dimensional route
						list_concat(r_list,&c_list);// use the dim_proxy route
						res.node=best_route;
						res.port=0;// AE: temporary value
						return res;
					}
				} else if(a_eq_b_con(proxy_route, dim_route) && a_con_le_b(proxy_bottleneck, dim_bottleneck)){
					if(rnd<1){
						list_destroy(&c_list);// destroy proxy-dim route
						list_concat(r_list,&b_list);// use the dim_proxy route
						res.node=best_route;
						res.port=0;// AE: temporary value
						return res;
					}
				}
				// Set flag for wet_call and goto before the while loop.
				list_destroy(&b_list);//delete dim_route if necessary
				list_destroy(&c_list);// delete dim_proxy route
				p=best_p;
				//intr=best_intr;
				dry_call=FALSE;
				goto DoAndQuit;
			}// end [goto] and quit for !dry_run
		} //end proxies at this level
	}// end !noproxies
	if(!a_con(best_route)){
		res.node=-1;
		res.port=-1;
		return res;
	}
	else if(!dry_run){
		// choose between dim_proxy_route and dim_route (at least one of these is a_con)
		//recall:  b_list is dim_route and c_list is dim_proxy_route
		if(a_better_than_b(dim_proxy_route , dim_route )){// best_route > 0 guaranteed
			list_destroy(&b_list);
			list_concat(r_list,&c_list);
		}else if(a_eq_b_con(dim_proxy_route,dim_route)){
			//AE: use the one with lower bottleneck edge.
			if(dim_proxy_bottleneck < dim_bottleneck ||
				 (dim_proxy_bottleneck == dim_bottleneck && ztm(2))){
				//destroy dim_route
				list_destroy(&b_list);
				//keep dim_proxy_route
				list_concat(r_list,&c_list);
			}else{
				list_destroy(&c_list);
				list_concat(r_list,&b_list);
			}
		}else{
			list_destroy(&c_list);
			list_concat(r_list,&b_list);
		}
		res.node=best_route;
		res.port=best_bottleneck;
		return res;
	}else{
		res.node=best_route;
		res.port=best_bottleneck;
		return res;
	}
}

static void shuffle_array(long* A, long n){
	long i,j,tmp;
	if(n > 1){
		for (i = 0; i < n - 1; i++) {
			j = i + rand() / (RAND_MAX / (n-i) + 1);
			#ifdef DEBUG
			if(j>=n){
				printf("ERROR: buffer overflow in shuffle array.\n");
				exit(-1);
			}
			#endif
			tmp = A[j];
			A[j] = A[i];
			A[i] = tmp;
		}
	}
}

long intervals_print(long * intrs,long intrn){
	long i;
	printf("%ld intervals\n",intrn);
	for (i=0; i<intrn; i++) {
    printf("%ld %ld\n",intrs[2*i],intrs[2*i+1]);
	}
	return 1;
}

/**
 * @brief Compute a set of proxy D_{m-1}^c for src in D_{m-1}^a and dst in D_{m-1}^b, with
 * a,b,c pairwise distinct.
 *
 * Writes a list of intervals [a0,a1],[b0,b1] ... to the array intrs as <a0,a1,b0,b1, ...> and
 * returns the number of intervals written.  See file description for extended explanation of
 * this function.
 *
 * @param intrs an array of size at least 6.
 * @param intrs_length length of intrs.
 * @param src src node in some D_{m-1}^{label[src][m]} (distinct from that of dst).
 * @param dst node in some D_{m-1}^{label[dst][m]}.
 * @param m level m (lowest level D_{m} containing both src and dst).
 * @return long.
 */
static long compute_proxies(long * intrs,long intrs_length, long src, long dst, long m){
	long A[2],B[2],intrn;
	long src_d,dst_d,tt;
	#ifdef DEBUG
	if(intrs_length<MAX_INTRS){
		printf("Error in compute_proxies.  Input array must be of length at least 6.\n");
		exit(-1);
	}
	#endif
	if(label[src][m]>label[dst][m])
		return compute_proxies(intrs,intrs_length,dst,src,m);

	// OLD tt=(tight_proxy?0:1);
	tt=min(m-1,param_t);
	src_d=uid_i[src][m-1]-uid_i[src][tt];
	dst_d=uid_i[dst][m-1]-uid_i[dst][tt];
	intrn=0;
	switch(alpha){
	case FICONN:

		if(src_d-two_pow[m-1]+1>0)
			intrs[0]=iceil((src_d-two_pow[m-1]+1),two_pow[m]);
		else
			intrs[0]=0;

		if(src_d+tk[tt]-1+two_pow[m-1]+1<tk[m])
			intrs[1]= (src_d - two_pow[m-1]+1+tk[tt]-1)/two_pow[m];
		else
			intrs[1]=gk[m]-1;

		intrn+=(intrs[1]>=intrs[0])?1:0;

		if(dst_d-two_pow[m-1]+1>0)
			intrs[2*intrn]=iceil((dst_d-two_pow[m-1]+1),two_pow[m]);
		else
			intrs[2*intrn]=0;

		if(dst_d+tk[tt]-1+two_pow[m-1]+1<tk[m])
			intrs[2*intrn+1]= (dst_d - two_pow[m-1]+1+tk[tt]-1)/two_pow[m];
		else
			intrs[2*intrn+1]=gk[m]-1;

		intrn+=(intrs[2*intrn+1]>=intrs[2*intrn])?1:0;

		break;
	case DCELL_ALPHA:
		intrs[0]=src_d;
		intrs[1]=src_d+tk[tt];
		intrs[2]=dst_d;
		intrs[3]=dst_d+tk[tt];
		intrn=2;
		break;
	case DCELL_BETA:
		if(label[src][m]>0){
			A[0]=src_d+label[src][m]-tk[m-1];
			A[1]=A[0]+tk[tt]-1;
			B[0]=0;
			B[1]=label[src][m]-1;
			intrn+=(intervals_intersect(intrs+intrn*2,A,B)>0);
		}

		if(label[src][m]<gk[m]-1){
			A[0]=src_d+label[src][m]+1;
			A[1]=A[0]+tk[tt]-1;
			B[0]=label[src][m]+1;
			B[1]=gk[m]-1;
			intrn+=(intervals_intersect(intrs+intrn*2,A,B)>0);
		}

		if(label[dst][m]>0){
			A[0]=dst_d+label[dst][m]-tk[m-1];
			A[1]=A[0]+tk[tt]-1;
			B[0]=0;
			B[1]=label[dst][m]-1;
			intrn+=(intervals_intersect(intrs+intrn*2,A,B)>0);
		}

		if(label[dst][m]<gk[m]-1){
			A[0]=dst_d+label[dst][m]+1;
			A[1]=A[0]+tk[tt]-1;
			B[0]=label[dst][m]+1;
			B[1]=gk[m]-1;
			intrn+=(intervals_intersect(intrs+intrn*2,A,B)>0);
		}
		break;
	default:
		printf("ERROR: unknown network type in compute_proxies.\n");
		break;
	}
#ifdef DEBUG
	long i;
	long *debug_intrs;
	debug_intrs=malloc(intrn*2*sizeof(long));
	for(i=0;i<intrn;i++){
		debug_intrs[2*i]=intrs[2*i];
		debug_intrs[2*i+1]=intrs[2*i+1];
		if(intrs[2*i]>intrs[2*i+1] || intrs[2*i]<0 || intrs[2*i+1]<0){
			printf("ERROR: problem with compute intervals: %ld [%ld, %ld].\n",i,intrs[2*i],intrs[2*i+1]);
			exit(-1);
		}
	}
	long debug_res=intervals_union(debug_intrs,intrn);
			for (i = 0; i < 2*debug_res; i+=2) {
				if(debug_intrs[i]<0 || debug_intrs[i+1]<0 || debug_intrs[i]>debug_intrs[i+1]){
					printf("ERROR: problem with intervals after union. %ld %ld, %ld\n",i/2,
								 debug_intrs[i],debug_intrs[i+1]);
					intervals_print(debug_intrs,debug_res);
					intervals_print(intrs,intrn);
					exit(-1);
				}
			}
			free(debug_intrs);
#endif
	return intervals_union(intrs,intrn);
}

/**
 * @brief Compute the intersection of closed intervals A and B, store in C.
 *
 * A and B are intervals [A[0],A[1]], [B[0],B[1]].  Puts the intersection of A and B in A, if
 * non-empty, and returns the length of the interval.  If intersection is empty, then set
 * C[0]=-1 C[1]=0.
 *
 * @param C long[2].
 * @param A long[2] first interval.
 * @param B long[2] second interval.
 * @return long.  max(0,C[1]-C[0])
 */
static long intervals_intersect(long * C,long *A,long *B){
	if(A[0]>B[0])
		return intervals_intersect(C,B,A);
	if(B[0]>A[1]){
		C[0]=-1;
		C[1]=0;
		return 0;
	}
	C[0]=B[0];
	C[1]=min(A[1],B[1]);
	return C[1]-C[0]+1;
}

/**
 * @brief Sort list of closed intervals and take unions where possible.
 *
 * WARNING: uses a quadratic sort, under the assumption that we'll
 * never use this for many intervals.
 *
 * @param intrs array of intervals {a[0],a[1]}, {a[2],a[3]}, ...
 * @param intrn number of intervals in array.
 * @return long.
 */
static long intervals_union(long * intrs,long intrn){
	long i=-1,j=-1,tmp=-1,res=intrn;
	if(intrn<=1) return intrn;
	for(i=0;i<2*intrn;i+=2){
		for(j=i+2;j<2*intrn;j+=2){
			if(intrs[j]<intrs[i]){
				tmp=intrs[j];
				intrs[j]=intrs[i];
				intrs[i]=tmp;
				tmp=intrs[j+1];
				intrs[j+1]=intrs[i+1];
				intrs[i+1]=tmp;
			}
		}
	}// intervals ordered now
	for(i=0;i<2*(intrn-1);i+=2){
		if(intrs[i+1] < intrs[i+2]-1){
			continue;
		}
		if(intrs[i+1]<intrs[i+3]){
			intrs[i+2]=intrs[i];
		}else{
			intrs[i+2]=intrs[i];
			intrs[i+3]=intrs[i+1];
		}
		intrs[i]=-1;
		intrs[i+1]=-1;
		res--;
	}
	j=0;
	for(i=0;i<2*intrn;i+=2){
		if(intrs[i]==-1)
			continue;
		if(i!=j){
			intrs[j]=intrs[i];
			intrs[j+1]=intrs[i+1];
		}
		j+=2;
	}
	return res;
}

/**
 * @brief If we are proxying between src in D_{lvl-1}^a and dst in D_{lvl-1}^b return TRUE,
 * else FALSE.
 *
 * @param lvl Current level
 * @return bool_t.
 */
static bool_t proxy_this_lvl(long lvl){
	return proxy_array[lvl];
}

/**
 * writes to abcd but should return label[b][m]
 * @param
 * @param
 * @param the previous D_{lvl-1}^prev was from 'intr'th interval
 * @param previously we returned D_{lvl-1}^prev.  if <-1 then panic.
 * @param
 * @param index of dst substructure D_{lvl-1}^{dst_d}
 * @param level of operation
 */


/**
 * @brief Populate global variable long tk[0:param_k+1].
 *
 * Constructs array (t_0,t_1,...,t_k), where t_i is the number of servers
 * in a gdcficonn_{i,n}.
 *
 * Assume two_pow[] has been populated (at init_topo_gdcficonn())
 */
static void make_tk(){
	long i=0;
	tk=malloc((param_k+1)*sizeof(long));
	tk[0]=param_n;
	if(alpha==DCELL_ALPHA || alpha == DCELL_BETA){
		for(i=0;i<param_k;i++)
			tk[i+1]=tk[i]*(tk[i]+1);
	}else if(alpha==FICONN){
		for(i=0;i<param_k;i++)
			tk[i+1]=tk[i]*(tk[i]/two_pow[i+1]+1);
	}
}

/**
 * @brief Populate global variable long gk[0:param_k+1].
 *
 * Constructs array (g_1,...,g_k), where g_i is the number of gdcficonn_{i-1,n} in a
 * gdcficonn_{i,n}
 *
 * Assume make_tk() has been called.
 */
static void make_gk(){
	long i=0;
		gk=malloc((param_k+1)*sizeof(long));
		gk[0]=tk[0];
		for(i=1;i<param_k+1;i++){
			gk[i]=tk[i]/tk[i-1];
		}
}

/**
 * @brief Populate global variables long labels[0:servers+switches], uid_i[0:servers].
 *
 * See file description for details.
 */
static void construct_labels(){
	long i=-1,j=-1,sw=-1,a=-1;
	label=malloc((servers+switches)*sizeof(long*));
	uid_i=malloc(servers*sizeof(long*));
	for(i=0;i<servers;i++){
		label[i]=malloc((param_k+1)*sizeof(long));
		uid_i[i]=malloc((param_k+1)*sizeof(long));
		a=i;
		label[i][0]=a%tk[0];
		uid_i[i][0]=label[i][0];
		a-=label[i][0];
		for(j=1;j<param_k+1;j++){
			label[i][j]=(a%tk[j])/tk[j-1];
			a-=label[i][j]*tk[j-1];
			uid_i[i][j] = uid_i[i][j-1]+label[i][j]*tk[j-1];
		}
#ifdef DEBUG
		// check if uid_i and label are consistent
		//label[i]=(x_0,x_1, ..., x_k)
		//and uid_i =  x_0+x_1*t_0+x_2*t_1+ ... + x_k*t_{k-1}
		long uid=label[i][0];
		for(j=1;j<param_k+1;j++){
			uid+=label[i][j]*tk[j-1];
				if(uid_i[i][j]!=uid){
					printf("bad uid build, j=%ld, uid_i=%ld, uid=%ld\n",j,uid_i[i][j],uid);
					exit(-1);
				}
		}
#endif
		if(i%param_n==0){// label a corresponding switch
			sw=i/param_n+servers;// this is the mapping from servers to their switches
			label[sw]=malloc((param_k)*sizeof(long));
			for(j=0;j<param_k;j++){
				label[sw][j]=label[i][j+1];
			}
		}
	}
}

/**
 * Prints the coordinates for a given node (server or switch) in gdcficonn
 *
 * Assumes 'label' has been populated
 */
void print_coordinates(long node, long port){
    long k,n;

    if(is_server_gdcficonn(node)){
        printf("Server %ld port %ld < ",node, port);
        for (k=0; k<param_k+1; k++){
            printf("%ld, ", label[node][k]);
        }
        printf("> port %ld ::: %ld\n", port, network[node].port[port].flows);
    } else {
        printf("Switch %ld port %ld < ",node, port);
        n=network[node].port[0].neighbour.node; // the id of a switch are all the coordinates of any of its attached servers, except coordinate 0.
        for (k=1; k<param_k+1; k++){
            printf("%ld, ", label[n][k]);
        }
        printf("> port %ld ::: %ld\n", port, network[node].port[port].flows);
    }
}

/**
 * @brief Access and populate 'extern long **network' using connection rule.
 *
 * @return long.
 */
static long make_connections(){
	long i=-1;
	usedports=malloc(servers*sizeof(long));
	// connect switches with servers.
	for(i=0;i<servers;i++){
		usedports[i]=1;
		network[i].port[0].neighbour.node=i/param_n+servers;
		network[i].port[0].neighbour.port=i%param_n;
		network[i/param_n+servers].port[i%param_n].neighbour.node=i;
		network[i/param_n+servers].port[i%param_n].neighbour.port=0;
	}
	rec_connections(0,param_k);

#ifdef DEBUG
	long _ports=0,j=-1;
	for(i=0;i<servers+switches;i++){
		for(j=0;j<get_radix_gdcficonn(i);j++){
			if(network[i].port[j].neighbour.node!=-1){
				_ports++;
			}
		}
	}
	if(_ports!=ports){
		printf("we are leaving some things disconnected.\n");
		exit(-1);
		}
		printf("We have made the expected number of connections.\n");
#endif

	free(usedports);
	return 1;
}

/**
 * @brief Recursive companion to make_connections().
 *
 * @param suffix_id the label of the D_{lvl-1}s we are connecting
 * @param lvl We are doing level-lvl connections.
 * @return long.
 */
static long rec_connections(long suffix_id, long lvl){
	long i=-1,j=-1,a=-1,b=-1;
	if(lvl==0){// we could do the switch connections here if we wanted to
		return 1;
	}
	for(i=0;i<gk[lvl];i++){
		for(j=i+1;j<gk[lvl];j++){
			// Connect D_{lvl-1}^i with D_{lvl-1}^j < using getlink
			get_link(i,j,lvl);
			a=suffix_id+i*tk[lvl-1]+link[0];
			b=suffix_id+j*tk[lvl-1]+link[1];
#ifdef DEBUG
			if(alpha==DCELL_ALPHA && usedports[a]!=param_k-lvl+1){
				printf("expected usedports[a]==param_k-lvl+1, got %ld, %ld.\n",usedports[a],param_k-lvl+1);
				exit(-1);
			}
#endif
			network[a].port[usedports[a]].neighbour.node=b;
			network[a].port[usedports[a]].neighbour.port=usedports[b];
			network[b].port[usedports[b]].neighbour.node=a;
			network[b].port[usedports[b]].neighbour.port=usedports[a];
			usedports[a]++;
			usedports[b]++;
		}
		rec_connections(suffix_id+i*tk[lvl-1],lvl-1);
	}
	return 1;
}

/**
 * @brief Populate global variable 'link' with level-'lvl' bridge link, where link[0] in
 * D_{lvl-1}^a and link[1] in D_{lvl-1}^b.
 *
 * link[0] and link[1] are are uid_i[*][lvl-1]s, and are therefore ignorant of which
 * D_{lvl-1}s they are contained in, and must be added a suffix_id.  See the file description
 * for details.
 *
 * @param a index of D_{lvl-1}^a
 * @param b  index of D_{lvl-1}^b
 * @param lvl we are computing a level-lvl link
 * @return long always 1
 */
static long get_link(long a, long b, long lvl){
	long tmp=-1;
	if(b<a){
		get_link(b,a,lvl);
		tmp=link[0];
		link[0]=link[1];
		link[1]=tmp;
		return 1;
	}
#ifdef DEBUG
	if(b==a){
		printf("get_link was called with a==b.\n");
		exit(-1);
	}
#endif
	switch(alpha){
	case DCELL_ALPHA:
		link[0]=b-1;
		link[1]=a;
		//printf("DCELL_ALPHA getlink\n %ld %ld",a,b);
		break;
	case DCELL_BETA:
		link[0]=b-a-1;
		link[1]=tk[lvl-1]-b+a;
		break;
	case FICONN:
		link[0]=(b-1)*two_pow[lvl]+two_pow[lvl-1]-1;
		link[1]=two_pow[lvl-1]-1+a*two_pow[lvl];
		break;
	default:
		printf("Unrecognised connection rule %ld\n",alpha);
		exit(-1);
		break;
	}
	return 1;
}
