/** @mainpage
HCN(n,h) topology
*
* Note: We have changed the terms master and slave, used in the literature, to leader and follower.
*
  [Guo] Deke Guo, Tao Chen, Dan Li, Mo Li, Yunhao Liu, and Guihai Chen.  **Expandable
  and Cost-Effective Network Structures for Data Centers Using Dual-Port
  Servers**.  _IEEE Trans. Comput., _ 62, 1303-1317, 2013.
  <sup>[http://dx.doi.org/10.1109/TC.2012.90](http://dx.doi.org/10.1109/TC.2012.90)</sup>

* [Stewart] Iain A. Stewart.  **Improved Routing in the Data Centre Networks HCN and
*  BCN**.  _Computing and Networking (CANDAR), 2014 Second International
* Symposium on, _ 212-218, 2014.
* <sup>[http://dx.doi.org/10.1109/CANDAR.2014.16](http://dx.doi.org/10.1109/CANDAR.2014.16)</sup>
*
  [Kliegl] Markus Kliegl, Jason Lee, Jun Li, Xinchao Zhang, Chuanxiong Guo, and David
  Rincón.  **Generalized DCell Structure for Load-Balanced Data Center
  Networks**.  _INFOCOM IEEE Conference on Computer Communications Workshops, _
  1--5, 2010.
  <sup>[http://dx.doi.org/10.1109/INFCOMW.2010.5466647](http://dx.doi.org/10.1109/INFCOMW.2010.5466647)</sup>

  [Erickson] Alejandro Erickson, Abbas Eslami Kiasari, Javier Navaridas, and Iain
  A. Stewart.  **Routing Algorithms for Recursively-Defined Data Centre
  Networks**.  _Trustcom/BigDataSE/ISPA, 2015 IEEE, _ 3, 84--91, 2015.
  <sup>[http://arxiv.org/abs/1509.01747](http://arxiv.org/abs/1509.01747)</sup>

* Usage:
*
* Topology has parameters a,b,h[,c,r].  If c and r are supplied, the topology
* BCN(a,b,h,c) with connectrion rule r is specified, else the topology
* HCN(a+b,h) is specified.  Must have c<=h, and c and r must be supplied
* together.
*
* Connection rule r=0 is the original DCell-like connection rule, and r=1 is
* DCell-beta from [Kliegl]
*
* Routing algorithms include all those described in [Stewart] as well as a
* variant of newbdim routing using the "tightness" concept from [Erickson],
* which reduces the number of copies of HCN(n,h) to search through.  The
* parameter of routing algorithm hcnbcn-newbdim_t routing is this tightness
* parameter and therefore it has the same meaning as in topology gdcficonn.
*
* Examples of relevant paramters:
topo=hcnbcn_4_1_3 routing=hcnbcn-fdim
topo=hcnbcn_4_1_3 routing=hcnbcn-newfdim
topo=hcnbcn_4_5_3_1_0 routing=hcnbcn-bdim
topo=hcnbcn_4_5_3_1_0 routing=hcnbcn-newbdim_1
topo=hcnbcn_4_5_3_1_1 routing=hcnbcn-bdim
topo=hcnbcn_4_5_3_1_1 routing=hcnbcn-newbdim_1
*
*
* Implementation notes:
*

* BCN(a,b,h,c) consists of one or more copies of HCN(a+b,h), labelled B_u (or
* Bu) where 0 \le u < s, in which there are one or more copies of HCN(a+b,c),
* B_u^v (or Buv), where 0\le v < max(a^(h-c),1).
*
* 'leaders' 'followers' 'servers' 'switches' 'ports' refer to total numbers for
* BCN(a,b,h,c).  'hcn_leaders' 'hcn_followers' 'hcn_servers' 'hcn_switches' refer
* to total numbers for Bu.
*
* In memory, the servers appear before all switches.  First, servers of B0, then
* B1, etc, followed by switches of B0, B1, etc.  The servers (switches, resp.)
* of each Bu are identical, modulo hcn_servers (resp. hcn_switches).
*
* In the documentation we use the following names for the indices of network
* nodes:
*
* network-index refers to node's index among all nodes of network
*
* follower-index refers to follower's index among all followers
*
* leader-index refers to leader's index among all leaders
*
* server-index refers to server's index among servers (server-index ==
* network-index)
*
* switch-index refers to switch's index among all switches
*
* hcn-*-index, where *=node|follower|leader|server|switch refers to node's index among
* nodes of type * within the canonical first HCN(n,h), B_0 (see below)
*
* The canonical HCN B0: Within B0 servers, leaders are listed first, followed by
* followers.  The index of a B0 leader is its level h id, more or less as given in
* [Stewart].  That is, 0 \le m_node < 'hcn_leaders' indexes the leader in B0
* with label [x0,x1,...,x_h], whos id is x0*a^0+x1*a^1+ ... + xh*a^h.  The follower
* node s_node, connected to the same switch as m_node has index, 'hcn_leaders'
* \le s_node < 'hcn_servers' and label [a+i,x1,x2,...xh], for some 0\le i < a+b.
* id'(s_node)=i+b*(x1*a^0+x2*a^1+ ... + xh*a^(h-1)) = s_node-hcn_leaders.  The
* switch 'h_switch' attached to 'm_node' and 's_node' has label [x1,x2,
* ... ,xh], and its switch-id is x1*a^0+x2*a^1+ ... + xh*a^(h-1) =
* h_switch-servers.
*
* 'hcnlabels' stores the above labels for B0, and
* 'uid_i'[m_node][i]=x0*a^0+x1*a^1+...+xi*a^i, for 0\le i \le h, for each leader
* node of B0
*
* In addition, we shall write all indexed vectors from left to right, as index
* increases.  For example, m_node, given above, corresponds to [Stewart]'s
* [i_h,i_{h-1},...,i_1,x] where 1 <= x <= a.
*
* The labels 'Buv': Each node in Buv is mapped to the corresponding node in B0,
* and u,v are given with 'Buv'[node].hcn, .u, and .v, respectively.
*
* TODO:
* - reimplement list_t with an array for speed?
* - avoid proxying on the most conjested links; rather, proxy on the least congested links.
*   + new data suggests that we won't solve the problem even if we do so, because the most
* congested links are not the follower links anyway.
* - refactor code for maintainability
*
* Version information:

* v0.0 development version and link usage testing experiment. Features include routing
* algorithsm fdim, newfdim, bdim, newbdim, where newbdim takes a tightness parameter param_t
* so that not all copies of Bu are tried as proxies, but only those near to src or near to
* dst, where the bridge link lies within an HCN(n,param_t).

*Also implemented individual link usage, as well as level usage, printed as histograms, but
*they are not histograms.  newbdim also prints result of bdim including hop length
*distribution.

* v0.1 first experiments done on this version. Disabled individual link usage to reduce output
* size, but can be reenabled by editing the max hist size function in this file.

* v0.1.1 bug fix for param_t, where should have param_t=min(param_c,param_h) if not given.

* v0.2.0 traffic aware routing

* v0.3.0 remove traffic aware routing from conf options but remains in code.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../inrflow/node.h"
#include "../inrflow/globals.h"
#include "hcnbcn.h"
#include "../inrflow/reporting.h"

#define MAXLEN_FILENAME_PARAMS 100
#define BCN_ALPHA 0 ///< Connection rule original DCell
#define BCN_BETA 1 ///< Connection rule beta-DCell

static long param_a;	///< parameter a (= alpha), the number of leaders adjacent to a switch in HCN(n,h)
static long param_b;	///< parameter b (= beta), the number of followers adjacent to a switch in HCN(n,h)
static long param_h;	///< parameter h, depth of recursion of HCN(n,h)
static long param_n; ///< radix of switch
static long param_c; ///< value of c (= gamma) in BCN(a,b,h,c)
static long param_r;///< connection type, 0 or 1
static long param_t;///< tightness parameter.

static long *a_pow;	///< an array with each a^i, useful for doing some calculations, where 0 <= i <= h
static long *two_pow; ///< precompute powers of 2
static long *RA;///<array buffer for non-determinism
static long *RAi;///<array buffer for non-determinism
static long *RAj;///<array buffer for non-determinism
static long *RAz;///<array buffer for non-determinism
static long *dstb;
static long leaders;	///< the total number of leaders: a^(h+1)
static long followers;	///< the total number of followers: a^h*b
static long servers; 	///< the total number of servers (= N): leaders + followers
static long switches;	///< the total number of switches: a^h
static long ports;///< total number of ports USED in network:
					 ///2*servers+n*switches-unused switches
// static long server_ports;	///< the number of ports on a server: 2
// static long switch_ports;	///< the number of ports on a switch: a + b
static long scopies; ///< number of copies of HCN(n,h) in this BCN, or 1
static long hcn_switches; ///< number of switches in each copy of HCN(n,h)
static long hcn_leaders;///< number of leaders in each copy of HCN(n,h)
static long hcn_followers;///< number of followers in each copy of HCN(n,h)
static long hcn_servers;///< number of servers in each copy of HCN(n,h)

//Extra stats
static long bdim_max_hops; ///< upper bound on length of bdim path (for
													 ///histogram)
static long *level_links_usage_not_hist; ///< fake histogram to measure link
																				 ///usage by level (level h+1 are
																				 ///follower-to-follower links)
static long level_links_not_divided=1;///< flag indicating that
																			///level_links_usage_not_hist has not been
																			///divided out by number of links at each
																			///level.
static long *bdim_hist;///< histogram of bdimrouting path lengths.
static long *nproxy_hist;///< histogram for number of proxies searched
static long *succ_nproxy_hist;///< histogram for number of proxies through which
															///there was a path at most as long as bdim
static const char* hist_docs[4]={
	"Not a histogram, but average (floor) usage of links at each level, where h+1 is follower"
	" level, and to-from switches at level 0, (level, floor of"
	" average flows). When nothing is reported it means that the avg. usage of links is almost 0",
	"Number of proxies tested per path (proxies tested during a route, number of occurrences)",
	"Number of successful proxies found",
	"BdimRouting route hop-length histogram (server hop path length of bdim route, number of oc"
	"currences)",
};///< documentation for histograms (and fake histograms)

///< Throughout this file, route list data is the port number, so just a long,
///and not a struct
static list_t route_list; ///< list that proxy_routing will append to on the top level
static long link[2]; ///<store the output of get_link

static long **hcnlabels; ///< coordinate labels for copies of HCN(a,h)
static long **uid_i;///< uid_i for hcn leaders (functions map to these for other nodes)
static struct Buv{
	long hcn;
	long u;
	long v;
}*Buv;///< for each node in copy B_u^v of HCN(n,h) store u,v, and hcn-node-index

static long *best_u_array;//array for newbdimrouting

static char* network_token="hcnbcn";
static char* routing_token;
static char* topo_version="v0.2.0";
static char* topo_param_tokens[5]= {"alpha","beta","h","gamma","connection"};
static char *routing_param_tokens[1]={"tightness"};
extern char filename_params[MAXLEN_FILENAME_PARAMS];

// These declarations do not belong in hcnbcn.h as they are not used with inrflow
// Documentation appears with definitions.
static void construct_labels();
static long is_bcn();

// get link
static long get_link_hcn(long a, long b, long lvl);
static long get_link_bcn(long au, long bu, long v);
static long get_flipbit(long node);

//queries to nodes
static long id_hcn_server(long *node_label);///< dummy function for id_hcn
static long id_hcn(long *node_label);///< label to hcn node id
static long is_leader(long node);///< true iff node is a leader
static long is_follower(long node);///< true iff node is a follower
static long get_u_index(long node);///< return u in B_u^v
static long get_v_index(long node);///< return v in B_u^v
static long get_s_index(long node);
static long get_canonical_hcn_index(long node);
static long follower_to_leader(long node);
static long hcn_follower_to_leader(long node);
static long get_follower_switch(long node);

//label coordinates
static long get_coord(long node, long i);
static long get_uid_i(long node, long lvl);
static long switch_of(long node);

//routing
static long comm_suff(long a, long b,long lvl);
static tuple_t append_to_route(list_t * r_list, long node, long port, long lvl, bool_t dry_run);
static tuple_t fdimrouting(list_t * r_list, long src, long dst, long lvl, bool_t dry_run);
static tuple_t newfdimrouting_rec(list_t * r_list, long src, long dst, bool_t dry_run);
static tuple_t newfdimrouting(list_t * r_list, long src, long dst, bool_t dry_run);
static tuple_t bdimrouting(long src, long dst, long dry_run);
static tuple_t newbdimrouting(long src, long dst, long dry_run);

//utility / print
static void shuffle_array(long* A, long n);
static long init_ra(long * A, long n);
#ifdef DEBUG
static void print_hcnlabel(long node);
static void print_hcnuids(long node);
#endif

////////////////////////////////////////////////////////
// Functions declared in hcnbcn.h, interface with inrflow They are documented in
// topo.h
////////////////////////////////////////////////////////

long init_topo_hcnbcn(long np, long* par){
	long i;

	if (np!=3&&np!=5) {
		printf("3 or 5 parameters are needed for HCN/BCN <a, b, h[, c, r]>\n");
		exit(-1);
	}
	param_a=par[0];
	param_b=par[1];
	param_h=par[2];
	if(np==5){
		param_c=par[3];
		param_r=par[4];
		if (param_r!=BCN_ALPHA && param_r!=BCN_BETA) {
			printf("Connection rule r must be %d or %d\n", BCN_ALPHA,BCN_BETA);
		}
		snprintf(filename_params, MAXLEN_FILENAME_PARAMS,"a%ldb%ldh%ldc%ldr%ld",param_a,param_b,param_h,param_c,param_r);
	}else{
		param_c=param_h+1;
		param_r=BCN_ALPHA;
		snprintf(filename_params, MAXLEN_FILENAME_PARAMS,"a%ldb%ldh%ld",param_a,param_b,param_h);
	}

	param_n=param_a+param_b; // set number of switch ports n = alpha + beta

	a_pow=malloc((param_h+2)*sizeof(long));
	a_pow[0]=1;
	two_pow=malloc((param_h+2)*sizeof(long));
	two_pow[0]=1;
	for (i=1; i<=param_h+1; i++) {
		a_pow[i]=a_pow[i-1]*param_a;
		two_pow[i]=two_pow[i-1]*2;
	} // powers of a will be useful throughout,so let's compute them just once.

	bdim_max_hops=two_pow[param_h+1]*2+6;

	switch (routing) {
		case HCNBCN_FDIM:
		routing_token = "fdim";
		break;
		case HCNBCN_NEWFDIM:
		routing_token = "newfdim";
		break;
		case HCNBCN_BDIM:
		routing_token = "bdim";
		break;
		case HCNBCN_NEWBDIM:
		routing_token = "newbdim";
		if(routing_nparam==1){
			param_t=routing_params[0];
			snprintf(filename_params+strlen(filename_params),
                     MAXLEN_FILENAME_PARAMS-strlen(filename_params),
                     "_t%ld",param_t);
		}else if(routing_nparam==0){
			param_t=min(param_c,param_h);
		}else{
			printf("Number of routing params must be 0 or 1\n");
			exit(-1);
		}
		break;
		default:
		printf("Specify a routing algorithm.\n");
		exit(-1);
		break;
	}

	if (param_h < param_c)
	/*  Case HCN(a+b,h):
	a^h switches
	a^h*a leaders
	a^h*b (unused) followers
	*/
	scopies=1;
	else
	/*  Case BCN(a+b,h,h):
	a^h   *(a^h*b+1) switches
	a^h*a *(a^h*b+1) leaders
	a^h*b *(a^h*b+1) followers

	Case BCN(a+b,h,c) with c<h:
	a^h   *(a^c*b+1) switches
	a^h*a *(a^c*b+1) leaders
	a^h*b *(a^c*b+1) followers */
	scopies=a_pow[param_c]*param_b+1;
	hcn_switches=a_pow[param_h];
	hcn_leaders=a_pow[param_h+1];
	hcn_followers=a_pow[param_h]*param_b;
	switches=hcn_switches*scopies;
	leaders=hcn_leaders*scopies;
	followers=hcn_followers*scopies;

	hcn_servers=hcn_leaders+hcn_followers;
	servers=leaders+followers;
	//every server has 2 ports, used or not.  this is used ports, so subtract the unused leader
	//ports, and if this is only HCN, unused follower ports
	ports=2*servers-scopies*param_a+switches*param_n;
	if(scopies==1){
		ports-=followers;
	}
	RA=malloc(scopies*sizeof(long));
	RAi=malloc(param_a*sizeof(long));
	RAj=malloc(param_a*sizeof(long));
	RAz=malloc(param_a*sizeof(long));
	dstb=malloc((param_h+1)*sizeof(long));
	best_u_array=malloc(scopies*sizeof(long));
	list_initialize(&route_list,sizeof(long));
	init_hist_array(&level_links_usage_not_hist,param_h+2);
	init_hist_array(&bdim_hist,bdim_max_hops);
	init_hist_array(&nproxy_hist, scopies+1);
	init_hist_array(&succ_nproxy_hist, scopies+1);
	construct_labels();
	return 1; // return status, not used here
}

void finish_topo_hcnbcn(){
	long i;
	free(a_pow);
	free(two_pow);
	free(RA);
	free(RAi);
	free(RAj);
	free(RAz);
        free(dstb);
	free(best_u_array);
	for (i=0; i<hcn_servers+hcn_switches; i++) {
		free(hcnlabels[i]);
	}
	free(hcnlabels);
	for (i=0;i<hcn_leaders;i++){
		free(uid_i[i]);
	}
	free(uid_i);
	free(Buv);
	finish_hist_array(&level_links_usage_not_hist);
	finish_hist_array(&bdim_hist);
	finish_hist_array(&nproxy_hist);
	finish_hist_array(&succ_nproxy_hist);
}

long get_servers_hcnbcn(){
	return servers;
}
long get_switches_hcnbcn(){
	return switches;
}
long get_ports_hcnbcn(){
	return ports;
}

long is_server_hcnbcn(long i){
	return (i<servers);
}
long get_server_i_hcnbcn(long i){
	return i;
}
long get_switch_i_hcnbcn(long i){
	return servers+i;
}
long node_to_server_hcnbcn(long i){
	return i;
}
long node_to_switch_hcnbcn(long i){
	return i-servers;
}

char * get_network_token_hcnbcn(){
	return network_token;
}
char * get_routing_token_hcnbcn(){
	return routing_token;
}
char * get_topo_version_hcnbcn(){
	return topo_version;
}
char * get_topo_param_tokens_hcnbcn(long i){
	return topo_param_tokens[i];
}
char * get_filename_params_hcnbcn(){
	return filename_params;
}
char* get_routing_param_tokens_hcnbcn(long i){
	if(i!=0 || routing != HCNBCN_NEWBDIM){
		printf("Requested bad routing param token\n");
		exit(-1);
	}
	return routing_param_tokens[0];
}

long get_radix_hcnbcn(long n){
	if (n<servers) return 2;
	return param_n;
}

tuple_t connection_hcnbcn(long node, long port){
	tuple_t res;
	long i;
	long hcn_node;
	long flipbit;
	long s_id,u,v;
	//convert to an hcn node and then convert neighbouring node back to the
	//correct copy of B_{u,v}

	hcn_node=get_canonical_hcn_index(node);
	long offset=node-hcn_node;

	// hcn_node indices comprise three consecutive blocks: leaders, followers,
	// switches.  They are arranged so that leaders are grouped into contiguous
	// sets of param_a servers that connect to the same switch, and likewise,
	// followers into sets of param_b servers that connect to the same switch In
	// addition, the contiguous groups occur in the same order as their respective
	// switches in the switch block.

	// on a server, port 0 connects to the switch, and port 1 may connect to
	// another server.  on a switch, the first param_a ports connect to leaders,
	// in the order of the corresponding contiguous sequence of leaders, and
	// similarly for the next param_b ports of this server.

	// recall there are hcn_leaders leaders, indexed 0 to hcn_leaders-1 in
	// HCN(a+b,h) there are hcn_followers followers, indexed hcn_followers to
	// hcn_followers+hcn_leaders-1 there are hcn_switches switches, indexed
	// hcn_servers to hcn_servers+hcn_switches-1

	if(is_leader(hcn_node)){
		if(port==0){
			offset/=param_n;
			hcn_node=hcn_node/(param_a)+servers;
			res.port=hcn_node%(param_a);
		}else{
			//port==1
			res.port=1;
			flipbit = get_flipbit(hcn_node);

			#ifdef DEBUG
			if (flipbit>param_h) {
				printf("get_flipbit screwed up %ld\n", flipbit);
				exit(-1);
			}
			#endif

			if(flipbit < param_h){
				for(i=param_h; i>flipbit+1; i--)
				dstb[i] = hcnlabels[hcn_node][i];
				dstb[flipbit+1] = hcnlabels[hcn_node][flipbit];
				for (i=flipbit; i>=0; i--)
				dstb[i] = hcnlabels[hcn_node][flipbit+1];
				hcn_node=id_hcn_server(dstb);

				#ifdef DEBUG
				if (!is_leader(hcn_node)) {
					printf("id_hcn_server created a bad node."
					"  hcn_node %ld hcn_leaders %ld.\n",hcn_node,hcn_leaders );
					for (i=param_h; i>=0; i--) {
						printf("%ld ", dstb[i]);
					}
					printf("\n");
				}
				#endif

			}else{
				hcn_node=-1;
				res.port=-1;
			}
		}
		res.node=hcn_node+offset;
	}else if(is_follower(hcn_node)){
		if (port==0) {
			res.port=get_coord(node,0);//(hcn_node-hcn_leaders)%(param_b)+param_a;
			res.node=get_follower_switch(node);
		}else{
			//port==1
			if(is_bcn()){
				u=get_u_index(node);
				v=get_v_index(node);
				s_id=get_s_index(node);
				switch (param_r) {
				case BCN_ALPHA:
				if(u<=s_id)
				res.node=u+v*a_pow[param_c]*param_b+(s_id+1)*hcn_servers+hcn_leaders;
				else
				res.node=(u-1)+v*a_pow[param_c]*param_b+(s_id)*hcn_servers+hcn_leaders;
					break;
				case BCN_BETA:
					res.node= scopies-2- s_id + v*a_pow[param_c]*param_b + ((u+s_id+1)%(scopies))*hcn_servers+hcn_leaders;
				default:
					break;
				}

				#ifdef DEBUG
				if (!is_follower(res.node)) {
					printf("follower bad connection to res.node %ld.  u %ld, s_id %ld\n", res.node,u,s_id);
					exit(-1);
				}
				if (get_v_index(res.node)!=get_v_index(node)) {
					printf("bad follower connection %ld %ld %ld %ld\n",res.node,get_v_index(res.node),node,get_v_index(node));
				}
				#endif
				res.port=1;
			}else{
				res.node=-1;
				res.port=-1;
			}
		}
	}else{
		//is_switch
		if (port<param_a) {
			//connect to a leader server
			hcn_node=(hcn_node-servers)*param_a+port;
		}else{
			//connect to a follower server
			hcn_node=(hcn_node-servers)*param_b+(port-param_a)+hcn_leaders;
		}
		offset*=param_n;
		res.node=hcn_node+offset;
		res.port=0;
	}
	#ifdef DEBUG
	printf("connected %ld.%ld to %ld.%ld\n",node,port,res.node,res.port);
	#endif

	return res;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_hcnbcn(long src, long dst){
    return(1);
}

long init_routing_hcnbcn(long src, long dst){
	switch (routing) {
		case HCNBCN_FDIM:
		fdimrouting(&route_list,src,dst,param_h,FALSE);
		break;
		case HCNBCN_NEWFDIM:
			init_ra(RAj,param_a);
			shuffle_array(RAj,param_a);
			init_ra(RAi,param_a);
			shuffle_array(RAi,param_a);
			init_ra(RAz,param_a);
			shuffle_array(RAz,param_a);

			newfdimrouting(&route_list,src,dst,FALSE);
		break;
		case HCNBCN_BDIM:
		bdimrouting(src,dst,FALSE);
		break;
		case HCNBCN_NEWBDIM:
		newbdimrouting(src,dst,FALSE);
		break;
		default:
		return -1;
		break;
	}
	return 0;
}

void finish_route_hcnbcn(){
	list_destroy(&route_list);
}

long route_hcnbcn(long current, long destination){
	/* if(!is_leader(src) || !is_leader(dst)) return -1; */
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

long get_topo_nstats_hcnbcn(){
	if(routing==HCNBCN_NEWBDIM){
		return 18;
	}else{ return 0;}
}
struct key_value get_topo_key_value_hcnbcn(long i){
	switch(i){
	case 0:
		return make_key_value_node("min.n.proxies","%ld",min_hist_array(nproxy_hist,scopies+1));
		break;
	case 1:
		return make_key_value_node("max.n.proxies","%ld",max_hist_array(nproxy_hist,scopies+1));
		break;
	case 2:
		return make_key_value_node("mean.n.proxies","%f",mean_hist_array(nproxy_hist,scopies+1));
		break;
	case 3:
		return make_key_value_node("median.n.proxies","%f",median_hist_array(nproxy_hist,scopies+1));
		break;
	case 4:
		return make_key_value_node("var.n.proxies","%f",var_hist_array(nproxy_hist,scopies+1));
		break;
	case 5:
		return make_key_value_node("std.n.proxies","%f",std_dev_hist_array(nproxy_hist,scopies+1));
		break;
	case 6:
		return make_key_value_node("min.succ.n.proxies","%ld",min_hist_array(succ_nproxy_hist,scopies+1));
		break;
	case 7:
		return make_key_value_node("max.succ.n.proxies","%ld",max_hist_array(succ_nproxy_hist,scopies+1));
		break;
	case 8:
		return make_key_value_node("mean.succ.n.proxies","%f",mean_hist_array(succ_nproxy_hist,scopies+1));
		break;
	case 9:
		return make_key_value_node("median.succ.n.proxies","%f",median_hist_array(succ_nproxy_hist,scopies+1));
		break;
	case 10:
		return make_key_value_node("var.succ.n.proxies","%f",var_hist_array(succ_nproxy_hist,scopies+1));
		break;
	case 11:
		return make_key_value_node("std.succ.n.proxies","%f",std_dev_hist_array(succ_nproxy_hist,scopies+1));
		break;
	case 12:
		return make_key_value_node("min.bdim.hop","%ld",min_hist_array(bdim_hist,bdim_max_hops));
		break;
	case 13:
		return make_key_value_node("max.bdim.hop","%ld",max_hist_array(bdim_hist,bdim_max_hops));
		break;
	case 14:
		return make_key_value_node("mean.bdim.hop","%f",mean_hist_array(bdim_hist,bdim_max_hops));
		break;
	case 15:
		return make_key_value_node("median.bdim.hop","%f",median_hist_array(bdim_hist,bdim_max_hops));
		break;
	case 16:
		return make_key_value_node("var.bdim.hop","%f",var_hist_array(bdim_hist,bdim_max_hops));
		break;
	case 17:
		return make_key_value_node("std.bdim.hop","%f",std_dev_hist_array(bdim_hist,bdim_max_hops));
		break;
	default:
		printf("ERROR: invalid key_value requested i=%ld.\n",i);
		exit(-1);
	}
}

long get_topo_nhists_hcnbcn(){
	//divide each level
	long i;
#ifdef DEBUG
	long lnks;
#endif
	if(level_links_not_divided){
		level_links_not_divided=0;
			//2 bidirectional links at each switch, of which there are servers*2
		level_links_usage_not_hist[0]/=servers*2;
		for (i=1; i < param_h+1; i++) {
			//note, 2 bidirectional links counted below.
			level_links_usage_not_hist[i]/=(param_a)*(param_a-1)*a_pow[param_h-i]*scopies;

#ifdef DEBUG
			//these should add up to 'leaders'
			lnks+=(param_a)*(param_a-1)*a_pow[param_h-i]*scopies;
#endif
		}

#ifdef DEBUG
		if(lnks!=leaders-scopies*param_a){
			printf("lnks %ld dividing inter-leader flows don't add up to number of leaders-scopies*param_a %ld.\n",lnks,leaders-scopies*param_a);
			exit(-1);
		}
#endif

		level_links_usage_not_hist[param_h+1]/=followers;
	}
	switch (routing) {
	case HCNBCN_FDIM:
	case HCNBCN_NEWFDIM:
	case HCNBCN_BDIM:
		return 4;
	case HCNBCN_NEWBDIM:
		return 4;
	default:
		return 0;
	}
}
char get_topo_hist_prefix_hcnbcn(long i){
	switch (i) {
	case 0:
		return 'c';//level
	case 1:
		return 'r';//proxies
	case 2:
		return 's';//success
	case 3:
		return 'b';//bdim
	}
	return -1;
}
const char* get_topo_hist_doc_hcnbcn(long i){
	return hist_docs[i];
}
//AE: Temporary suppression of individual link "histograms"
long get_topo_hist_max_hcnbcn(long i){
	switch(i){
	case 0:
		return param_h+2;
                break;
	case 1:
	case 2:
		return scopies+1;
		break;
	case 3:
		return bdim_max_hops;
		break;
	}
	return -1;
}
void get_topo_hist_hcnbcn(long *topo_hist, long i){
		switch(i){
	case 0:
		memcpy(topo_hist,level_links_usage_not_hist,(param_h+2)*sizeof(long));
		break;
	case 1:
		memcpy(topo_hist,nproxy_hist,(scopies+1)*sizeof(long));
		break;
	case 2:
		memcpy(topo_hist,succ_nproxy_hist,(scopies+1)*sizeof(long));
		break;
	case 3:
		memcpy(topo_hist,bdim_hist,bdim_max_hops*sizeof(long));
		break;
	default:
		printf("ERROR: invalid histogram requested i=%ld.\n",i);
		exit(-1);
	}
}

////////////////////////////////////////////////////////
// End Inrflow topo.h functions and begin static functions.
////////////////////////////////////////////////////////

/**
* @brief compute level lvl link from one HCN(n,lvl-1) to another
*
* Compute the indices of leaders in HCN(n,lvl-1)s that link these two HCNs and
* store in link[0] link[1], such that (link[0],link[1]) is the bridge link.
*
* Leaders a,b must be in distinct HCN(n,lvl-1)s inside the same HCN(n,lvl) for this
* to be valid.
*
* @param a index of an HCN(n,lvl-1)
* @param b index of an HCN(n,lvl-1)
* @return long
*/
static long get_link_hcn(long a, long b, long lvl){
	long i;
	link[0]=0;
	link[1]=0;
	for (i=0; i < lvl; i++) {
		link[0]+=a_pow[i]*b;
		link[1]+=a_pow[i]*a;
	}
	return -1;
}

/**
* @brief compute link from Bav to Bbv
*
* Compute the indices of followers in Bav and Bbv and store in link[0] link[1], such that
* (link[0],link[1]) is the bridge link from Bav to Bbv.
*
* @param a index of Ba
* @param b index of Bb, different from a
* @return long
*/
static long get_link_bcn(long au, long bu, long v){
	long tmp;
	if(bu<au){
		get_link_bcn(bu,au,v);
		tmp=link[0];
		link[0]=link[1];
		link[1]=tmp;
		return 1;
	}
	#ifdef DEBUG
	if(bu==au){
		printf("get_link_bcn was called with au==bu.\n");
		printf("RA bug: ");
		for(tmp=0;tmp<scopies;tmp++){
			printf("%ld ", RA[tmp]);
		}
		printf("\n");
		exit(-1);
	}
	#endif
	switch (param_r) {
	case BCN_ALPHA:
	link[0]=bu-1+v*a_pow[param_c]*param_b+au*hcn_servers+hcn_leaders;
	link[1]=au+v*a_pow[param_c]*param_b+bu*hcn_servers+hcn_leaders;
    break;
	case BCN_BETA:
		link[0]=bu-au-1+v*a_pow[param_c]*param_b+au*hcn_servers+hcn_leaders;
		link[1]=scopies-1-bu+au+v*a_pow[param_c]*param_b+bu*hcn_servers+hcn_leaders;
default:
    break;
	}
	#ifdef DEBUG
	if (get_v_index(link[0])!=get_v_index(link[1])) {
		printf("get_link_bcn v index\n");
		exit(-1);
	}
	if(get_u_index(link[0])!=au || get_u_index(link[1])!=bu){
		printf("get_link_bcn u index\n");
		exit(-1);
	}
	#endif
	return 1;
}


/**
* @brief construct labels hcnlabels, 'Buv' and 'uid_i'
*
* See documentation in preamble.
* hcn_leaders are labelled: <i_h,i_{h-1},...,i_1,x>, where 0\le x < a
*
* hcn_followers are labelled: <i_h,i_{h-1},...,i_1,y>, where a\le y < a+b
*
* hcn_switches are labelled as follows: <i_h,i_{h-1},...,i_1,param_n>
*
*/
static void construct_labels(){
	long i,j, node, node_index,v;
	hcnlabels=malloc((hcn_servers+hcn_switches)*sizeof(long*));
	Buv=malloc((servers+switches)*sizeof(struct Buv));
	uid_i=malloc((hcn_leaders)*sizeof(long*));
	// leader servers

	#ifdef DEBUG
	printf("leaders\n");
	#endif
	for (node_index = 0; node_index < hcn_leaders; node_index++) {
		node = node_index;
		hcnlabels[node_index]=malloc((param_h+1)*sizeof(long));
		uid_i[node_index]=malloc((param_h+1)*sizeof(long));
		for(i=param_h; i>=0; i--) {
			hcnlabels[node_index][i] = node/a_pow[i];
			uid_i[node_index][i]=node;
			node = node - hcnlabels[node_index][i]*a_pow[i];
		}
		if(is_bcn())
		v=(uid_i[node_index][param_h]-uid_i[node_index][param_c])/a_pow[param_c+1];
		else v=0;
		for (j=0; j<scopies; j++) {
			Buv[j*hcn_servers+node_index].hcn=node_index;
			Buv[j*hcn_servers+node_index].u=j;
			Buv[j*hcn_servers+node_index].v=v;
			#ifdef DEBUG
			print_hcnlabel(node_index);
			print_hcnuids(node_index);
			printf("Buv.hcn %ld, .u %ld, .v %ld\n",
			Buv[j*hcn_servers+node_index].hcn,
			Buv[j*hcn_servers+node_index].u,
			Buv[j*hcn_servers+node_index].v);
			#endif
		}

		#ifdef DEBUG
		long test_uid=0;
		for (i = 0; i<param_h; i++) {
			test_uid+=a_pow[i]*hcnlabels[node_index][i];
			if (test_uid!=uid_i[node_index][i]){
				printf("uid_i messed up. test_uid %ld uid_i %ld node %ld i %ld.\n",
				test_uid,uid_i[node_index][i],node_index,i);
				exit(-1);
			}
		}
		#endif

	}
	#ifdef DEBUG
	printf("followers\n");
	#endif
	// follower servers
	for (node_index = hcn_leaders; node_index < hcn_leaders + hcn_followers; node_index++) {
		hcnlabels[node_index]=malloc((param_h+1)*sizeof(long));
		node=node_index - hcn_leaders;
		hcnlabels[node_index][0] = node - (node/param_b)*param_b + param_a;
		// note that hcnlabels[node_index][0] in {a,a+1,...,a+b-1}
		node = hcn_follower_to_leader(node_index);
		//AE: slightly redundant...
		for(i=param_h; i>=1; i--) {
			hcnlabels[node_index][i] = hcnlabels[node][i];
		}
		if(is_bcn())
		v=(uid_i[node][param_h]-uid_i[node][param_c])/a_pow[param_c+1];
		else v=0;
		for (j=0; j<scopies; j++) {
			Buv[j*hcn_servers+node_index].hcn=node_index;
			Buv[j*hcn_servers+node_index].u=j;
			Buv[j*hcn_servers+node_index].v=v;
			#ifdef DEBUG
			print_hcnlabel(node_index);
			printf("Buv.hcn %ld, .u %ld, .v %ld\n",
			Buv[j*hcn_servers+node_index].hcn,
			Buv[j*hcn_servers+node_index].u,
			Buv[j*hcn_servers+node_index].v);
			#endif
		}


	}
	// switches
	//AE: making labels for switches 'servers' to 'servers+hcn_switches'
	// which are indexed in hcnlabels from hcn_servers to hcn_servers+hcn_switches
	for (node_index = hcn_servers; node_index < hcn_servers + hcn_switches; node_index++) {
		hcnlabels[node_index]=malloc((param_h+1)*sizeof(long));
		node=param_a*(node_index-hcn_servers);
		//AE: redundant
		for(i=param_h; i>=1; i--) {
			hcnlabels[node_index][i] = hcnlabels[node][i];
		}
		hcnlabels[node_index][0]=param_n;
		if(is_bcn())
		v=(uid_i[node][param_h]-uid_i[node][param_c])/a_pow[param_c+1];
		else v=0;
		for (j=0; j<scopies; j++) {
			Buv[get_switch_i_hcnbcn(node_index-hcn_servers)+j*hcn_switches].hcn=
			get_switch_i_hcnbcn(node_index-hcn_servers);
			Buv[get_switch_i_hcnbcn(node_index-hcn_servers)+j*hcn_switches].u=j;
			Buv[get_switch_i_hcnbcn(node_index-hcn_servers)+j*hcn_switches].v=v;
			#ifdef DEBUG
			printf("Buv.hcn %ld, .u %ld, .v %ld\n",
			Buv[get_switch_i_hcnbcn(node_index-hcn_servers)+j*hcn_switches].hcn,
			Buv[get_switch_i_hcnbcn(node_index-hcn_servers)+j*hcn_switches].u,
			Buv[get_switch_i_hcnbcn(node_index-hcn_servers)+j*hcn_switches].v);
			#endif
		}
	}
	#ifdef DEBUG
	long u,srv;
	for (u=0;u<scopies;u++) {
		v=0;
		for(srv=u*hcn_servers+hcn_leaders;srv<(u+1)*hcn_servers;srv+=a_pow[param_c]*param_b){
			if (get_v_index(srv)!=v++) {
				printf("problem with v index label %ld %ld %ld\n", srv,v-1,u);
				exit(-1);
			}
		}
	}
	#endif
}

/**
 * @brief True iff current topo is a BCN
 *
 * @return long
 */
static long is_bcn(){
	return !(param_c>param_h);
}

// get link

/**
*  @brief return largest index in lowest contiguous block of coordinates in
*  leader label.  Used for WK-recursive networks connection.
*
*  Where hcnlabels[node]=<i_h,i_{h-1}, ..., i_1, x> for a leader server, we
*  return the largest index of last contiguous block of equal i_js.
*
*  @param long node an hcn_leader node
*  @return long
*/
static long get_flipbit(long node){
	long i;
	//print_hcnlabel(node);
	for (i = 0; i < param_h; i++) {
		if(hcnlabels[node][i+1] != hcnlabels[node][i]){
			break;
		}
	}
	return i;
}

//queries to nodes

/**
 * @brief dummy wrapper for id_hcn(...)
 *
 * @param node_label
 * @return long
 */
static long id_hcn_server(long *node_label){
	return id_hcn(node_label);
}

/**
 * @brief converts server label to hcn server
 *
 * @param node_label
 * @return long
 */
static long id_hcn(long *node_label){
	long i,res = 0;
	for (i=1; i<=param_h; i++) {
		res = res + a_pow[i-1]*node_label[i];
	}

	// hcn_switch
	if (node_label[0] == param_n)
	return res+hcn_servers;

	// hcn_leader
	if (node_label[0] < param_a)
	return(res*param_a + node_label[0]);

	// hcn_follower
	return hcn_leaders + res*param_b + node_label[0] - param_a;
}

/**
* @brief true iff the server is a leader
*
* @param node server
* @return long
*/
static long is_leader(long node){
	return (get_canonical_hcn_index(node)<hcn_leaders);
}

/**
* @brief true iff the server is a follower
*
* @param node  server
* @return long
*/
static long is_follower(long node){
	return (get_canonical_hcn_index(node)<hcn_servers && hcn_leaders<=get_canonical_hcn_index(node));
}

/**
* @brief index u of this server's B_u
*
* @param node server
* @return long
*/
static long get_u_index(long node){
	return Buv[node].u;
}

/**
*  @brief compute index v in B_u^v of this node
*
*  First compute the index of the corresponding hcn node, and then of the HCN(n,c) within
*  HCN(n,h), where we assume that c\le h
*
*  @param a server
*  @return return type
*/
static long get_v_index(long node){
	return Buv[node].v;
}

/**
 *  @brief index of follower within its B0v
 *
 *  Serves to build the follower-to-follower connections
 *
 *  @param param
 *  @return return type
 */
static long get_s_index(long node){
	#ifdef DEBUG
	if (!is_follower(node)) {
		printf("get_s_index called with non-follower %ld\n",node);
		exit(-1);
	}
	#endif
	return (get_canonical_hcn_index(node)-hcn_leaders)%(a_pow[param_c]*param_b);
}

/**
* @brief server index within hcn B_u
*
* Given a server-node in BCN, return the corresponding server-node x in
* canonical hcn, 0 \le x \le hcn_servers
*
* @param node server
* @return long
*/
static long get_canonical_hcn_index(long node){
	return Buv[node].hcn;
}

/**
 * @brief return first leader at same switch
 *
 * @param node  follower
 * @return long
 */
static long follower_to_leader(long node){
	long hcn_node;
	hcn_node=get_canonical_hcn_index(node);
	return hcn_follower_to_leader(hcn_node)+(node-hcn_node);
}

/**
* @brief first leader of this follower in canonical hcnbcn
*
* Given an HCN follower server, return the leader at port 0 of the same switch.
*
* @param node follower in canonical HCN
* @return long
*/
static long hcn_follower_to_leader(long node){
	return (node-hcn_leaders)/param_b*param_a;
}

/**
 * @brief Get the network-index of switch attached to this follower
 *
 * @param node follower
 * @return long
 */
static long get_follower_switch(long node){
	return (get_canonical_hcn_index(node)-hcn_leaders)/param_b+servers+hcn_switches*get_u_index(node);
}

//label coordinates

/**
* @brief ith coordinate of id of 'node'.
*
*
* @param node
* @param i
* @return long
*/
static long get_coord(long node, long i){
	return hcnlabels[get_canonical_hcn_index(node)][i];
}

/**
 * @brief get uid_i(node,lvl) of this node or 0th leader at same switch
 *
 * @param node server
 * @param lvl level in HCN(param_n,param_h)
 * @return long
 */
static long get_uid_i(long node, long lvl){
	long node_hcn;
	node_hcn=get_canonical_hcn_index(node);
	if(is_follower(node_hcn)){
		return uid_i[hcn_follower_to_leader(node_hcn)][lvl];
	}else{
		return uid_i[node_hcn][lvl];
	}
}

/**
 * @brief index of switch connected to server 'node'
 *
 * @param node  a server node
 * @return long
 */
static long switch_of(long node){
	return servers+(get_uid_i(node, param_h)-get_uid_i(node,0))/param_a;
}

//routing

/**
* @brief Return lowest level m of HCN(n,h) containing both servers 'a' and 'b' within an
* HCN(n,h).
*
* Return m, where hcnlabels[a][m]!=hcnlabels[b][m], but all larger hcnlabels[a][i] hcnlabels[b][i] are
* equal. We assume that hcnlabels[a][lvl+1:]=hcnlabels[b][lvl+1:] See file description for details.
*
* @param a leader_server in HCN(n,lvl)
* @param b  leader_server in HCN(n,lvl)
* @param lvl  lvl of HCN(n,lvl) containing servers 'a' and 'b'
* @return long
*/
static long comm_suff(long a, long b,long lvl){
	long i;
	#ifdef DEBUG
	for(i=param_h;i>lvl;i--){
		if(hcnlabels[a][i]!=hcnlabels[b][i]){
			printf("ERROR: we assume hcnlabels[a][lvl+1:]==hcnlabels[b][lvl+1:]"
			" in common suffix.\n(a,b,lvl)=(%ld,%ld,%ld)",a,b,lvl);
			exit(-1);
		}
	}
	#endif
	for(i=lvl;i>=0;i--){
		if(hcnlabels[a][i]!=hcnlabels[b][i])
		break;
	}
	return i;
}

/**
* @brief Return the next node and (if not dry run) load on link to reach it from
* 'node' at 'port'.
*
* If not a dry run append the next (node) to r_list and update histograms.
*
* @param r_list a list_t of longs (or NULL)
* @param node current node
* @param port from 'node' to next node
* @param dry_run TRUE iff we do not append to r_list
* @return tuple_t <next node,load (if not dry_run)>
*/
static tuple_t append_to_route(list_t * r_list, long node, long port, long lvl, bool_t dry_run){
	tuple_t res;
	if(!dry_run){
		list_append(r_list,&port);
		//count all links
		update_hist_array(lvl, &level_links_usage_not_hist,param_h+2);
	}
	res.node = network[node].port[port].neighbour.node;
	res.port = network[node].port[port].flows;
	return res;
}

/**
* @brief fdimrouting, return hop-length of path and...
*
* If not dry_run, build the path and return number of flows on bottleneck link.
*
* @param r_list append path to this r_list
* @param src a source server in B_0
* @param dst a destination server in B_0
* @param lvl the lowest level known to contain src and dst
* @param dry_run TRUE iff this is dry run
* @return tuple_t <hop-length, flows in bottleneck link>
*/
static tuple_t fdimrouting(list_t * r_list, long src, long dst, long lvl, bool_t dry_run){
	tuple_t l1_route_and_bottleneck,l2_route_and_bottleneck;
	tuple_t res;
	long m;
	long suffix_id;
	long a,b;
	tuple_t next_node_and_flows,next_node_and_flows2;
	long hop_length=0;
	long hcn_src,hcn_dst;
	#ifdef DEBUG
	if(get_u_index(src)!=get_u_index(dst)){
		printf("src and dst in different u in fdimrouting.\n");
		exit(-1);
	}
	#endif

	hcn_src=get_canonical_hcn_index(src);
	hcn_dst=get_canonical_hcn_index(dst);
	m=comm_suff(hcn_src,hcn_dst,lvl);
	if(m<0){// can be same switch different servers
		if(src==dst){
			res.node=0;
			res.port=0;
			return res;
		}else{
			if(!dry_run){
				next_node_and_flows=append_to_route(r_list,src,0,0,FALSE);
				next_node_and_flows2=append_to_route(r_list,next_node_and_flows.node,get_coord(dst,0),0,FALSE);
				res.port=max(next_node_and_flows.port,next_node_and_flows2.port);
			}else
				res.port=-1;
			res.node=1;
			return res;
		}
	}
	if(m==0){//same switch
		if (!dry_run) {
			next_node_and_flows=append_to_route(r_list,src,0,0,FALSE);
			next_node_and_flows2=append_to_route(r_list,next_node_and_flows.node,get_coord(dst,0),0,FALSE);
			res.port=max(next_node_and_flows.port,next_node_and_flows2.port);
		}else
			res.port=-1;
		res.node=1;
		return res;
	}
	suffix_id=uid_i[(is_follower(src))?follower_to_leader(hcn_src):hcn_src][param_h]
		-uid_i[(is_follower(src))?follower_to_leader(hcn_src):hcn_src][m];
	get_link_hcn(get_coord(src,m),get_coord(dst,m),m);
	a=suffix_id+get_coord(src,m)*a_pow[m]+link[0];
	b=suffix_id+get_coord(dst,m)*a_pow[m]+link[1];
	//AE: bring a,b from B0 to Bu.
	a=a+(src-hcn_src);
	b=b+(src-hcn_src);

	l1_route_and_bottleneck=fdimrouting(r_list,src,a,m,dry_run);
	hop_length+=l1_route_and_bottleneck.node;
	next_node_and_flows=append_to_route(r_list,a,1,m,dry_run);
	hop_length++;
	l2_route_and_bottleneck=fdimrouting(r_list,b,dst,m,dry_run);
	hop_length+=l2_route_and_bottleneck.node;
	res.node=hop_length;
	res.port=max(l1_route_and_bottleneck.port,
							 max(next_node_and_flows.port,l2_route_and_bottleneck.port));
	return res;
}

/**
* @brief newfdimrouting from leaders in B_0 return hop-length and...
*
* If not dry_run, build the path and return number of flows on bottleneck link.
*
* @param r_list append path to this r_list
* @param src a source leader in B_0
* @param dst a destination leader in B_0
* @param dry_run TRUE iff this is dry run
* @return tuple_t <hop-length, flows in bottleneck link>
*/
static tuple_t newfdimrouting_rec(list_t * r_list, long src, long dst, bool_t dry_run){
	tuple_t res;
	long m,i;
	long bottleneck2,bottleneck;
  tuple_t l_route_bottleneck, l_route_bottleneck1, l_route_bottleneck2;
	long l,l1,l2,best_l;
	long z,best_z = 0;
	long suffix_id,a,b,c,d,hcn_src,hcn_dst;
	hcn_src=get_canonical_hcn_index(src);
	hcn_dst=get_canonical_hcn_index(dst);
	m=comm_suff(hcn_src,hcn_dst,param_h);
	l=best_l=fdimrouting(r_list,src,dst,m,TRUE).node;
	res.node=best_l;
	if (l>1){
		//init_ra(RAz,param_a);
		//shuffle_array(RAz,param_a);
		for (z=0; z<param_a; z++) {
			if (RAz[z]!=hcnlabels[hcn_src][m] && RAz[z]!=hcnlabels[hcn_dst][m]) {
				l1=l2=0;
				for (i=0; i<m; i++) {
					if (RAz[z]!=hcnlabels[hcn_src][i])
					l1+=two_pow[i];
					if (RAz[z]!=hcnlabels[hcn_dst][i])
					l2+=two_pow[i];
				}
				if(l1+l2+two_pow[m]+1<best_l){
					best_z=RAz[z];
					best_l=l1+l2+two_pow[m]+1;
				}
			}
		}
		res.node=best_l;
		if (!dry_run) {
			if(best_l<l){
				suffix_id=hcn_src-uid_i[hcn_src][m];
				get_link_hcn(hcnlabels[hcn_src][m],best_z,m);
				a=suffix_id+hcnlabels[hcn_src][m]*a_pow[m]+link[0];
				b=suffix_id+best_z*a_pow[m]+link[1];
				get_link_hcn(best_z,hcnlabels[hcn_dst][m],m);
				c=suffix_id+best_z*a_pow[m]+link[0];
				d=suffix_id+hcnlabels[hcn_dst][m]*a_pow[m]+link[1];
				a+=(src-hcn_src);
				b+=(src-hcn_src);
				c+=(src-hcn_src);
				d+=(src-hcn_src);
				l_route_bottleneck1=fdimrouting(r_list,src,a,m,FALSE);
				bottleneck=append_to_route(r_list,a,1,m,FALSE).port;
				l_route_bottleneck=fdimrouting(r_list,b,c,m,FALSE);
				bottleneck2=append_to_route(r_list,c,1,m,FALSE).port;
				l_route_bottleneck2=fdimrouting(r_list,d,dst,m,FALSE);
				l=l_route_bottleneck.node;
				l1=l_route_bottleneck1.node;
				l2=l_route_bottleneck2.node;

				#ifdef DEBUG
				if (l1+l+l2+2 != best_l) {
					printf("Wrong choice of srcb_id? src %ld dst %ld real l %ld best_l %ld\n",
					src,dst,l1+l2+l+2,best_l);
					printf("src ");
					print_hcnlabel(src);
					printf("\ndst ");
					print_hcnlabel(dst);
					printf("\na ");
					print_hcnlabel(a);
					printf("\nb ");
					print_hcnlabel(b);
					printf("\nc ");
					print_hcnlabel(c);
					printf("\nd ");
					print_hcnlabel(d);
					exit(-1);
				}
				#endif
				res.node=best_l;
				res.port=max(max(l_route_bottleneck.port,l_route_bottleneck1.port),
										 max(l_route_bottleneck2.port,max(bottleneck,bottleneck2)));
			}else{
				res=fdimrouting(r_list,src,dst,m,FALSE);
			}
		}
	}else if(l==1 && !dry_run){
		res=fdimrouting(r_list,src,dst,m,FALSE);
	}
	return res;
}

/**
* @brief newfdimrouting from servers in B__0 return hop-length and...
*
* If not dry_run, build the path and return number of flows on bottleneck link.
*
* @param r_list append path to this r_list
* @param src a source server in B_0
* @param dst a destination server in B_0
* @param dry_run TRUE iff this is dry run
* @return tuple_t <hop-length, flows in bottleneck link>
*/
static tuple_t newfdimrouting(list_t *r_list, long src, long dst, bool_t dry_run){
	tuple_t res;
	tuple_t next_node_and_flows,l;
	long i,j,src_leader=-1,dst_leader=-1,this_res,best_res=-1,best_src=-1,best_dst=-1;
#ifdef DEBUG
	if(get_u_index(src)!=get_u_index(dst)){
		printf("src and dst in different u\n");
		exit(-1);
	}
#endif
	if(src==dst){
		res.node=0;
		res.port=0;
		return res;
	}
	if(!is_leader(src)){
		src_leader=follower_to_leader(src);
	}
	if(!is_leader(dst)){
		dst_leader=follower_to_leader(dst);
	}

	/* if((src_leader==dst_leader && src_leader>-1) || src_leader==dst || src==dst_leader){ */
	if(switch_of(src)==switch_of(dst)){
		return fdimrouting(&route_list,src,dst,param_h,dry_run);
	}

	if(!is_leader(src)){
		//init_ra(RAi,param_a);
		//shuffle_array(RAi,param_a);
		for (i = 0; i < param_a; i++) {
			if (!is_leader(dst)) {
				//init_ra(RAj,param_a);
				//shuffle_array(RAj,param_a);
				for (j =0; j < param_a; j++) {
					this_res=newfdimrouting_rec(NULL,src_leader+RAi[i],dst_leader+RAj[j],TRUE).node;
					if (this_res<best_res || best_res==-1) {
						best_res=this_res;
						best_src=src_leader+RAi[i];
						best_dst=dst_leader+RAj[j];
					}
				}
			}else{
				this_res=newfdimrouting_rec(NULL,src_leader+RAi[i],dst,TRUE).node;
				if (this_res<best_res || best_res==-1) {
					best_res=this_res;
					best_src=src_leader+RAi[i];
					best_dst=dst;
				}
			}
		}
	}else if (!is_leader(dst)) {
		//init_ra(RAj,param_a);
		//shuffle_array(RAj,param_a);
		for (j =0; j < param_a; j++) {
			this_res=newfdimrouting_rec(NULL,src,dst_leader+RAj[j],TRUE).node;
			if (this_res<best_res || best_res==-1) {
				best_res=this_res;
				best_src=src;
				best_dst=dst_leader+RAj[j];
			}
		}
	}else{best_src=src;best_dst=dst;}

#ifdef DEBUG
	if (!is_leader(best_dst) || !is_leader(best_src)) {
		printf("best_src %ld or best_dst %ld not a leader.  src %ld dst %ld\n", best_src,best_dst,src,dst);
		exit(-1);
	}
#endif
	best_res=0;
	res.port=-1;
	if (!is_leader(src)) {
		best_res++;
		if(!dry_run){
			next_node_and_flows=append_to_route(&route_list,src,0,0,FALSE);
			if(next_node_and_flows.port > res.port){//check if this is bottleneck so far
				res.port=next_node_and_flows.port;
			}
			next_node_and_flows=append_to_route(&route_list,
                                                next_node_and_flows.node,
                                                get_coord(best_src,0),0,FALSE);
			if(next_node_and_flows.port > res.port){
				res.port=next_node_and_flows.port;
			}
		}
	}
	l=newfdimrouting_rec(&route_list,best_src,best_dst,dry_run);
	best_res+=l.node;
	if(!dry_run && l.port > res.port){
		res.port = l.port;
	}
	if (!is_leader(dst)) {
		best_res++;
		if(!dry_run){
			next_node_and_flows=append_to_route(&route_list,best_dst,0,0,FALSE);
			if(next_node_and_flows.port > res.port){//check if this is bottleneck so far
				res.port=next_node_and_flows.port;
			}
			next_node_and_flows=append_to_route(&route_list,
                                                next_node_and_flows.node,
                                                get_coord(dst,0),0,FALSE);
			if(next_node_and_flows.port > res.port){
				res.port=next_node_and_flows.port;
			}
		}
	}
#ifdef DEBUG
	if (best_res > fdimrouting(NULL,src,dst,param_h,TRUE).node) {
		printf("newfdimrouting bigger than fdimrouting src %ld dst %ld\n", src,dst);
		exit(-1);
	}
#endif

	res.node=best_res;
	return res;
}

/**
* @brief bdimrouting, return hop-length of path and...
*
* If not dry_run, build the path and return number of flows on bottleneck link.
*
* @param r_list append path to this r_list
* @param src a source server in B_0
* @param dst a destination server in B_0
* @param lvl the lowest level known to contain src and dst
* @param dry_run TRUE iff this is dry run
* @return tuple_t <hop-length, flows in bottleneck link>
*/
static tuple_t bdimrouting(long src, long dst, long dry_run){
	tuple_t res, l1_route_and_bottleneck,l2_route_and_bottleneck;
	long  bottleneck;
	long test_l1,test_l2,a,b,dst_u,src_u;
	if ((src_u=get_u_index(src))==(dst_u=get_u_index(dst))) {
		return fdimrouting(&route_list,src,dst,param_h,dry_run);
	} else {
		get_link_bcn(src_u,dst_u,get_v_index(src));
		a=link[0];
		b=link[1];
		l1_route_and_bottleneck=fdimrouting(&route_list,src,a,param_h,dry_run);
		test_l1=l1_route_and_bottleneck.node;
	  bottleneck=append_to_route(&route_list,a,1,param_h+1,dry_run).port;
		l2_route_and_bottleneck=fdimrouting(&route_list,b,dst,param_h,dry_run);
		test_l2=l2_route_and_bottleneck.node;
		#ifdef DEBUG
		if(get_v_index(src)!=get_v_index(a)) {
			printf("BOOO\n" );
		}
		if (test_l1>two_pow[param_c+1]+1 || test_l2>two_pow[param_h+1]+	1) {
			printf("route in Ba or Bu too long %ld 2^(c+1)-1 %ld %ld 2^(h+1)-1 %ld\n"
			"src %ld a %ld b %ld dst %ld srcv %ld av %ld bv %ld dstv %ld",
			test_l1,two_pow[param_c+1]-1,test_l2, two_pow[param_h+1]-1,
			src,a,b,dst,get_v_index(src),get_v_index(a),get_v_index(b),get_v_index(dst));
			exit(-1);
		}
		#endif
		res.node=test_l1+test_l2+1;
		res.port=max(bottleneck,max(l1_route_and_bottleneck.port,l2_route_and_bottleneck.port));
		return res;
	}
}

/**
* @brief newbdimrouting, return hop-length of path and...
*
* If not dry_run, build the path and return number of flows on bottleneck link.
*
* @param r_list append path to this r_list
* @param src a source server in B_0
* @param dst a destination server in B_0
* @param lvl the lowest level known to contain src and dst
* @param dry_run TRUE iff this is dry run
* @return tuple_t <hop-length, flows in bottleneck link>
*/
static tuple_t newbdimrouting(long src, long dst, long dry_run){
	long src_u,dst_u,best_l,a,b,best_u,x,xx,y,yy,
		l,u,n_proxies=0,n_succ_proxies=0,bdim_l,src_v,dst_v,src_uid_t_diff,dst_uid_t_diff;
			init_ra(RAj,param_a);
			shuffle_array(RAj,param_a);
			init_ra(RAi,param_a);
			shuffle_array(RAi,param_a);
			init_ra(RAz,param_a);
			shuffle_array(RAz,param_a);
	if ((src_u=get_u_index(src))==(dst_u=get_u_index(dst))) {
		best_l=newfdimrouting(&route_list,src,dst,dry_run).node;
		if(!dry_run){
			bdim_l=bdimrouting(src, dst, TRUE).node;
			update_hist_array(bdim_l, &bdim_hist, bdim_max_hops);
		#ifdef DEBUG
		if(best_l>bdim_l){
			printf("newbidimrouting bigger than bdimrouting src %ld dst %ld\n",src,dst );
			list_destroy(&route_list);
			newbdimrouting(src,dst,FALSE).node;
			exit(-1);
		}
		#endif
		}
	} else {
		dst_v=get_v_index(dst);
		src_v=get_v_index(src);
		src_uid_t_diff=get_uid_i(src,param_h)-get_uid_i(src,param_t);
		dst_uid_t_diff=get_uid_i(dst,param_h)-get_uid_i(dst,param_t);
		best_l=1;
		get_link_bcn(src_u,dst_u,src_v);
		a=link[0];
		b=link[1];
		best_l+=newfdimrouting(NULL,src,a,TRUE).node;
		//	append_to_route(NULL,a,1,dry_run);
		best_l+=newfdimrouting(NULL,b,dst,TRUE).node;
		if(!dry_run)
			bdim_l=bdimrouting(src, dst, TRUE).node;
		best_u=-1;
		init_ra(RA,scopies);
		shuffle_array(RA,scopies);
		for (u = 0; u < scopies; u++) {
			if(RA[u]==src_u||RA[u]==dst_u)
				continue;
			l=2;
			get_link_bcn(src_u,RA[u],src_v);
			x=link[0];
			xx=link[1];
			get_link_bcn(RA[u],dst_u,dst_v);
			y=link[0];
			yy=link[1];
			//This is a HACK:  Instead of computing the list of u where x in same HCN(n,c) as src,
			//or yy in same HCN(n,c) as dst, we are computing all of them and discarding bad ones.
			//We only count (with n_proxies, below) the ones we keep.
			if((get_uid_i(x,param_h)-get_uid_i(x,param_t))!=src_uid_t_diff
				 &&( get_uid_i(yy,param_h)-get_uid_i(yy,param_t))!=dst_uid_t_diff){
				continue;
			}
			n_proxies++;
			l+=newfdimrouting(NULL,src,x,TRUE).node;
			l+=newfdimrouting(NULL,xx,y,TRUE).node;
			l+=newfdimrouting(NULL,yy,dst,TRUE).node;
			if(!dry_run && l<bdim_l){//record when savings over bdimrouting are made
				n_succ_proxies++;
			}
			if(l<best_l){
				best_l=l;
				best_u=RA[u];
			}
		}
		if(!dry_run){
			update_hist_array(bdim_l, &bdim_hist, bdim_max_hops);
			update_hist_array(n_proxies, &nproxy_hist, scopies+1);
			update_hist_array(n_succ_proxies, &succ_nproxy_hist, scopies+1);
			if (best_u<0) {
				get_link_bcn(src_u,dst_u,get_v_index(src));
				a=link[0];
				b=link[1];
				best_l=1+newfdimrouting(&route_list,src,a,dry_run).node;
				append_to_route(&route_list,a,1,param_h+1,dry_run);
				best_l+=newfdimrouting(&route_list,b,dst,dry_run).node;
			}else{
				l=2;
				get_link_bcn(get_u_index(src),best_u,get_v_index(src));
				x=link[0];
				xx=link[1];
				get_link_bcn(best_u,get_u_index(dst),get_v_index(dst));
				y=link[0];
				yy=link[1];
				l+=newfdimrouting(&route_list,src,x,dry_run).node;
				append_to_route(&route_list,x,1,param_h+1,dry_run);
				l+=newfdimrouting(&route_list,xx,y,dry_run).node;
				append_to_route(&route_list,y,1,param_h+1,dry_run);
				l+=newfdimrouting(&route_list,yy,dst,dry_run).node;
#ifdef DEBUG
				if (l!=best_l) {
					printf("newBdimrouting wrong size\n");
					exit(-1);
				}
#endif
			}
#ifdef DEBUG
			if(best_l>bdim_l){
				printf("newbidimrouting bigger than bdimrouting src %ld dst %ld\n",src,dst );
				list_destroy(&route_list);
				newbdimrouting(src,dst,FALSE).node;
				exit(-1);
			}
#endif
		}
	}
	tuple_t res;
	res.node=best_l;
	res.port=0;
	return res;
}

/**
* @brief newbdimrouting traffic aware, return hop-length of path and...
*
* THIS FUNCTION IS DISABLED BUT CAN BE SWAPPED IN, IN PLACE OF NEWBDIMROUTING
* If not dry_run, build the path and return number of flows on bottleneck link.
*
* @param r_list append path to this r_list
* @param src a source server in B_0
* @param dst a destination server in B_0
* @param lvl the lowest level known to contain src and dst
* @param dry_run TRUE iff this is dry run
* @return tuple_t <hop-length, flows in bottleneck link>
static tuple_t newbdimrouting_TAR_DISABLED(long src, long dst, long dry_run){
	tuple_t res,l1_route_and_flows,l2_route_and_flows,l_route_and_flows;
	long dim_bottleneck;
	long n_bests,no_proxy_l;
	long best_bottleneck,best_b_u,i;
	long bottleneck,bottleneck2;
	long src_u,dst_u,best_l,a,b,best_u,x,xx,y,yy,
		l,u,n_proxies=0,n_succ_proxies=0,bdim_l,src_v,dst_v,src_uid_t_diff,dst_uid_t_diff;

			init_ra(RAj,param_a);
			shuffle_array(RAj,param_a);
			init_ra(RAi,param_a);
			shuffle_array(RAi,param_a);
			init_ra(RAz,param_a);
			shuffle_array(RAz,param_a);

	if ((src_u=get_u_index(src))==(dst_u=get_u_index(dst))) {
		res=newfdimrouting(&route_list,src,dst,dry_run);
		if(!dry_run){
			bdim_l=bdimrouting(src, dst, TRUE).node;
			update_hist_array(bdim_l, &bdim_hist, bdim_max_hops);
#ifdef DEBUG
			if(res.node>bdim_l){
				printf("newbidimrouting bigger than bdimrouting src %ld dst %ld\n",src,dst );
				list_destroy(&route_list);
				newbdimrouting(src,dst,FALSE);
				exit(-1);
			}
#endif
		}
	} else {
		//best_l_array
		dst_v=get_v_index(dst);
		src_v=get_v_index(src);
		src_uid_t_diff=get_uid_i(src,param_h)-get_uid_i(src,param_t);
		dst_uid_t_diff=get_uid_i(dst,param_h)-get_uid_i(dst,param_t);
		get_link_bcn(src_u,dst_u,src_v);
		a=link[0];
		b=link[1];
		best_l=1;
		best_l+=newfdimrouting(NULL,src,a,TRUE).node;
		//	append_to_route(NULL,a,1,dry_run);
		best_l+=newfdimrouting(NULL,b,dst,TRUE).node;
		no_proxy_l=best_l;//use this to check whether no-proxy should be considered later
		n_bests=1;
		if(!dry_run)
			bdim_l=bdimrouting(src, dst, TRUE).node;
		best_u=-1;
		init_ra(RA,scopies);
		shuffle_array(RA,scopies);
		for (u = 0; u < scopies; u++) {
			if(RA[u]==src_u||RA[u]==dst_u)
				continue;
			l=2;
			get_link_bcn(src_u,RA[u],src_v);
			x=link[0];
			xx=link[1];
			get_link_bcn(RA[u],dst_u,dst_v);
			y=link[0];
			yy=link[1];
			//This is a HACK:  Instead of computing the list of u where x in same HCN(n,c) as src,
			//or yy in same HCN(n,c) as dst, we are computing all of them and discarding bad ones.
			//We only count (with n_proxies, below) the ones we keep.
			if((get_uid_i(x,param_h)-get_uid_i(x,param_t))!=src_uid_t_diff
				 &&( get_uid_i(yy,param_h)-get_uid_i(yy,param_t))!=dst_uid_t_diff){
				continue;
			}
			n_proxies++;
			l+=newfdimrouting(NULL,src,x,TRUE).node;
			l+=newfdimrouting(NULL,xx,y,TRUE).node;
			l+=newfdimrouting(NULL,yy,dst,TRUE).node;
			if(!dry_run && l<bdim_l){//record when savings over bdimrouting are made
				n_succ_proxies++;
			}
			if (l<=best_l) {
				if(l<best_l){
					n_bests=1;
					best_l=l;
				}else{
					n_bests++;
				}
				best_u_array[n_bests-1]=RA[u];
				best_u=RA[u];
			}
		}
		res.node=best_l;
		res.port=-1;//will be overwritten if !dry_run
		//We shall store all of the 'best_l's and then chose one among those with a lowest
		//bottleneck.  This is slightly more computation.  Note that building paths that I will
		//not use later might result in bad stats collected in append_to_route()
		if(!dry_run){
			update_hist_array(bdim_l, &bdim_hist, bdim_max_hops);
			update_hist_array(n_proxies, &nproxy_hist, scopies+1);
			update_hist_array(n_succ_proxies, &succ_nproxy_hist, scopies+1);
			if (best_u<0) {
				get_link_bcn(src_u,dst_u,get_v_index(src));
				a=link[0];
				b=link[1];
				l2_route_and_flows=newfdimrouting(&route_list,src,a,FALSE);
				bottleneck=append_to_route(&route_list,a,1,param_h+1,FALSE).port;
				l1_route_and_flows=newfdimrouting(&route_list,b,dst,FALSE);
				l=1+l1_route_and_flows.node+l2_route_and_flows.node;
				res.port=bottleneck=max(l1_route_and_flows.port,max(l2_route_and_flows.port,bottleneck));
				//skip to return at end
			}else{
				//at least one proxy is a best length.
				dim_bottleneck=-1;
				if(best_l==no_proxy_l){
					//if no_proxy is still a best length then get its bottleneck
					//else dim_bottleneck is like a "disconnected" path, being -1
					get_link_bcn(src_u,dst_u,get_v_index(src));
					a=link[0];
					b=link[1];
					l2_route_and_flows=newfdimrouting(&route_list,src,a,FALSE);
					bottleneck=append_to_route(&route_list,a,1,param_h+1,FALSE).port;
					l1_route_and_flows=newfdimrouting(&route_list,b,dst,FALSE);
					//we don't care about the length yet, but this is how we would compute it
					//best_l=1+l1_route_and_flows.node+l2_route_and_flows.node;
					dim_bottleneck=max(l1_route_and_flows.port,max(l2_route_and_flows.port,bottleneck));
				}
				best_bottleneck=-1;
				for (i=((best_l==no_proxy_l)?1:0); i<n_bests; i++) {
					//We are now building paths to check bottlenecks.  Obviously this could be optimised
					//by not created the memory, but instead we just destroy the path each time
					list_destroy(&route_list);
					best_u=best_u_array[i];
					get_link_bcn(get_u_index(src),best_u,get_v_index(src));
					x=link[0];
					xx=link[1];
					get_link_bcn(best_u,get_u_index(dst),get_v_index(dst));
					y=link[0];
					yy=link[1];
					l_route_and_flows=newfdimrouting(&route_list,src,x,FALSE);
					bottleneck=append_to_route(&route_list,x,1,param_h+1,FALSE).port;
					l1_route_and_flows=newfdimrouting(&route_list,xx,y,FALSE);
					bottleneck2=append_to_route(&route_list,y,1,param_h+1,FALSE).port;
					l2_route_and_flows=newfdimrouting(&route_list,yy,dst,FALSE);
					//not interested in path length (we already know what it is)
					//l=2+l_route_and_flows.node+l1_route_and_flows.node+l2_route_and_flows.node;
					bottleneck=max(l_route_and_flows.port,
												 max(max(bottleneck,l1_route_and_flows.port),
														 max(bottleneck2,l2_route_and_flows.port)));
					if(a_better_than_b(bottleneck,best_bottleneck)){
						best_bottleneck=bottleneck;
						best_b_u=best_u;
					}
				}
				//best_b_u is the best of the proxy routes (if there are any) and its bottlneck is
				//best_bottleneck.
				if (a_con_le_b(best_bottleneck,dim_bottleneck)) {
					//do proxy, even if they are equal and "connected"
					//destroy path first
					list_destroy(&route_list);
					best_u=best_b_u;
					get_link_bcn(get_u_index(src),best_u,get_v_index(src));
					x=link[0];
					xx=link[1];
					get_link_bcn(best_u,get_u_index(dst),get_v_index(dst));
					y=link[0];
					yy=link[1];
					l_route_and_flows=newfdimrouting(&route_list,src,x,FALSE);
					bottleneck=append_to_route(&route_list,x,1,param_h+1,FALSE).port;
					l1_route_and_flows=newfdimrouting(&route_list,xx,y,FALSE);
					bottleneck2=append_to_route(&route_list,y,1,param_h+1,FALSE).port;
					l2_route_and_flows=newfdimrouting(&route_list,yy,dst,FALSE);

					//we already know what the path length and bottleneck are
#ifdef DEBUG
					l=2+l_route_and_flows.node+l1_route_and_flows.node+l2_route_and_flows.node;
					bottleneck=max(l_route_and_flows.port,
												 max(max(bottleneck,l1_route_and_flows.port),
														 max(bottleneck2,l2_route_and_flows.port)));
#endif
					//res.node=best_l;//done earlier
					res.port=best_bottleneck;
				}else{
					//do dim
					list_destroy(&route_list);
					get_link_bcn(src_u,dst_u,get_v_index(src));
					a=link[0];
					b=link[1];
					l2_route_and_flows=newfdimrouting(&route_list,src,a,FALSE);
					bottleneck=append_to_route(&route_list,a,1,param_h+1,FALSE).port;
					l1_route_and_flows=newfdimrouting(&route_list,b,dst,FALSE);
					l=1+l1_route_and_flows.node+l2_route_and_flows.node;
					bottleneck=max(l1_route_and_flows.port,max(l2_route_and_flows.port,bottleneck));
					//res.node=best_l;//done earlier
					res.port=dim_bottleneck;
				}
			}
			//from the above we should have l and bottleneck corresponding to the computed path,
			//while res.port and res.node were recorded earlier, according to our searches.

#ifdef DEBUG
			if (l!=best_l) {
				printf("newBdimrouting wrong size\n");
				exit(-1);
			}
			if (res.port != bottleneck) {
				printf("newBdimrouting wrong bottleneck\n");
				exit(-1);
			}
#endif
		}
#ifdef DEBUG
		if(res.node>bdim_l){
			printf("newbidimrouting bigger than bdimrouting src %ld dst %ld\n",src,dst );
			list_destroy(&route_list);
			newbdimrouting(src,dst,FALSE);
			exit(-1);
		}
#endif
	}
	return res;
}
*/
//utility / print

/**
 * @brief Random shuffle for the array A of length n
 *
 * @param A  array of longs
 * @param n  length of A
 */
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

/**
 * @brief initialise array A of length n to A[i]=i
 *
 * @param A  array of longs
 * @param n  length of A
 * @return long
 */
static long init_ra(long * A, long n){
	long i;
	for (i=0;i<n; i++) {
		A[i]=i;
	}
	return 1;
}

#ifdef DEBUG
/**
* @brief print hcn label of a BCN server to stdout
*
* @param node
*/
void print_hcnlabel(long node){
	long i;
	node=get_canonical_hcn_index(node);
	printf("label: ");
	for (i=param_h; i >= 0; i--) {
		printf("%ld ", hcnlabels[node][i]);
	}
	printf("\n");
}

/**
 * @brief print all uids for a server or follower's leader node
 *
 * maps server to hcn counterpart, and if follower maps to first leader at same
 * switch. For the mapped node it prints:
 *
 * uid_h, uid_{h-1}, ..., uid_{0}
 *
 * @param node
 */
static void print_hcnuids(long node){
	long i;
	node=get_canonical_hcn_index(node);
	if(is_follower(node)){
		node=hcn_follower_to_leader(node);
	}
	printf("uids: ");
	for (i=param_h; i >= 0; i--) {
		printf("%ld ", uid_i[node][i]);
	}
	printf("\n");
}
#endif
