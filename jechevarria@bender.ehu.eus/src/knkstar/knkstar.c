/** @mainpage
 *    KnkStar topology
 */

#include <stdlib.h>
#include <stdio.h>
#include "knkstar.h"

#ifdef DEBUG
long global_src=-1;
long global_dst=-1;
#endif

static long param_k;	///< number of dimensions
static long param_n;	///< size of clique in each dimension
static long retries; ///< number of times routing algorithm tries to route via a random proxy server

static long *n_pow;	///< an array with the n^i, useful for doing some calculations.
static long servers; 	///< The total number of servers
static long switches;	///< The total number of switches

static long * RA; ///< array for randomising the dimension order in route_recursive sub-procedures.

static long** label; ///< array of labels of nodes

static char* network_token="knkstar";
static char* routing_token="interintra";
static char* topo_version="v0.2.0";
static char* topo_param_tokens[3]= {"k", "n", "retries"};
static char filename_params[100]; ///< a substring of the output filenames

static struct path_t *path=NULL, *path_tail=NULL, *path_penultimate;

/**
 * Compute and store route from 'src' to 'dst', if it exists, in
 * 'path'
 * @param
 * @param
 * @param maximum the number of times routing is re-attempted with a random proxy server
 * @return 1 if successful, otherwise -1
 */
static long route_full(long src, long dst, long retries);

/**
 * Recursively attempt to route from 'src' to 'dst'.
 *
 * @param a server-node
 * @param a server-node
 * @param if 1 then we try some extra stuff
 */
static long route_recursive(long src, long dst, long retry);

/**
 * Append the port to 'path'.
 *
 * @param the node whose port is tested for a fault
 * @param
 * @return node reached via 'port' from 'node'
 */
static long append_to_path(long node, long port);

/**
 * Populate 'labels' with coordinate labels of knkstar
 */
static void construct_labels();

/**
 * Returns the unique switch connected to this server
 */
static long switchOf(long server);

/**
 * Returns the port from 'src' to 'dst' where 'src' and 'dst' are
 * neighbouring nodes
 */
static long get_port(long src, long dst);

/**
 * Returns the port from switch 'u' to server '(u,i,uip)'.
 * uip is u'_i, so the code translates this to the correct port.
 *
 * @param switch-node
 * @param dimension
 * @param coordinate that differs on dimension 'i'
 */
static long get_port_to_server(long u, long i, long uip);


/**
 * Rank function: Turns coordinates into index.
 *
 * @param a pointer to label of node 'i'
 * @param isServer == 1 iff this is a server label.
 *
 * @return i
 */
static long c2i(long *c, long isServer);

static long init_RA(long param);///< Initialise 'RA' with RA[i]=i.
static long rand_RA(long param);///<  Shuffle 'RA'

#ifdef DEBUG
static void printarray(long *a,long n);
#endif

///< Begin knkstar.h functions

long init_topo_knkstar(long nparam, long* params)
{
	long i;

	if (nparam<3) {
		printf("3 parameters are needed for KNKSTAR <k, n, retries>\n");
		exit(-1);
	}
	param_k=params[0];
	param_n=params[1];
	retries=params[2];

	sprintf(filename_params,"k%ldn%ldr%ld",param_k,param_n,retries);
	n_pow=malloc((param_k+1)*sizeof(long));
	n_pow[0]=1;

	//precompute powers of param_n
	for (i=1; i<=param_k; i++) {
		n_pow[i]=n_pow[i-1]*param_n;
	}

	switches=n_pow[param_k];
	servers=switches*(param_n-1)*param_k;

	RA = malloc(param_k*param_n*sizeof(long));

	construct_labels();

	return 1; //return status, not used here.
}

void finish_topo_knkstar()
{
	//I SHOULD FREE LABELS
	//for i in labels free(label[i]);
	//free(label);
}

long get_servers_knkstar()
{
	return servers;
}

long get_switches_knkstar()
{
	return switches;
}

long get_ports_knkstar()
{
	return servers*2 + switches*get_radix_knkstar(0);
}

long is_server_knkstar(long i)
{
	if(i>=switches && i < switches+servers)
		return 1;
	else
		return 0;
}

long get_server_i_knkstar(long i)
{
	return switches + i;
}

long get_switch_i_knkstar(long i)
{
	return i;
}

long node_to_server_knkstar(long i)
{
	return i-switches;
}

long node_to_switch_knkstar(long i)
{
	return i;
}

char* get_network_token_knkstar()
{
	return network_token;
}

char* get_routing_token_knkstar()
{
	return routing_token;
}

char* get_topo_version_knkstar()
{
	return topo_version;
}

char* get_topo_param_tokens_knkstar(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_knkstar()
{
	return filename_params;
}

long get_radix_knkstar(long n)
{
	if (n>=switches)
		return 2;
	else
		return (param_n-1)*param_k;
}

tuple_t connection_knkstar(long node, long port)
{
	tuple_t res;
	long * thing;
	long j;
	if(node<switches) {
		res.node = node*(param_n-1)*param_k+switches+port;
		res.port = 0;
	} else {
		if(port==0) {
			//connect to switch
			node = node-switches;
			res.port = node%((param_n-1)*param_k);      // get_radix
			res.node = node/((param_n-1)*param_k);      // get_radix
		} else {
			//connect this server, (u,i,uip) to the other server, (up,i,ui),
			//where u and up are the neighbouring switch labels

			//i is label[node][param_k]
			//uip is label[node][param_k+1]

			//get neighbouring switch
			//c2iu = (node-switches)/((param_n-1)*param_k);

			//build label of neighbouring server
			thing = malloc((param_k+2)*sizeof(long));
			for(j=0; j<param_k+1; j++) {
				thing[j] = label[node][j];
			}
			j--;
			//(u',i,ui) changing ui' to ui on ith dimension
			thing[j+1]=thing[thing[j]];
			//change u to u'
			thing[thing[j]] = label[node][param_k+1];
			//use ranking function
			res.node = c2i(thing,1);
			free(thing);
			res.port = 1;
		}
	}
	return res;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_knkstar(long src, long dst){

    return(1);
}

long init_routing_knkstar(long src, long dst)
{
#ifdef DEBUG
	global_src=src;
	global_dst=dst;
#endif

	long route_result =  route_full(src,dst,retries);

#ifdef DEBUG
	if(route_result!=1 && !path_is_null(path)) {
		printf("route_result %ld,i=%ld and j=%ld but path is not null\n",route_result, src,dst);
		print_path(path);
		exit(-1);
	}
	if(route_result == 1 && src != dst && path_is_null(path)) {
		printf("Path is connected but null, i=%ld, j=%ld\n",src,dst);
		exit(-1);
	}

	// This checks the whole path, regardless of whether or not dst was
	//encountered more than once.  mark_route stops the first time.  we
	//don't need to check that route_result==1, since the path MUST be
	//valid if it is non-null.
	long current=global_src;
	struct path_t *current_path = path;
	while(current_path!=NULL) {
		if(network[current].port[current_path->port].faulty) {
			//There are no disconnected ports in knkstar ||
			//network[current].port[current_path->port].neighbour.node ==
			//-1){
			current=global_src;
			printf("      Node  is_server       port  is_faulty  neighbour\n");
			current_path=path;
			while(current_path!=NULL) {
				printf( "%10ld %10ld %10ld %10ld %10ld\n",
				        current, is_server_knkstar(current),current_path->port,
				        network[current].port[current_path->port].faulty,
				        network[current].port[current_path->port].neighbour.node);
				current = network[current].port[current_path->port].neighbour.node;
				current_path=current_path->next;
			}
			printf("Using a bad port in a reportedly good path in knkstar (but check the output above).\n"
			       "src=%ld dst=%ld.\n",global_src,global_dst);
			print_path(path);

			exit(-1);
		}
		current = network[current].port[current_path->port].neighbour.node;
		current_path=current_path->next;
	}
#endif //DEBUG

	return route_result;
}

void finish_route_knkstar()
{
	empty_path(&path,&path_tail);

}

long route_knkstar(long current, long destination)
{
	return path_dequeue(&path,&path_tail);
}

///< End of knkstar.h functions.  Begin static functions

static long route_full(long src, long dst, long retries)
{
	long i;
	long intermediate;

#ifdef DEBUG
	struct path_t *t1=path_tail;
#endif

	if(route_recursive(src, dst,0)==1)
		return 1;

	if(route_recursive(src,dst,1)==1)
		return 1;

#ifdef DEBUG
	if(t1!=path_tail) {
		printf("path_tail has changed after failing route_recursive");

		exit(-1);
	}
#endif

	for(i=0; i<retries; i++) {
		intermediate = rand()%servers + switches;
		// We won't necessarily have path_is_null(path) in second call
		if (route_full(src,intermediate,0)==1 && route_full(intermediate,dst,0)==1) {
			return 1;
		} else //we don't do any retries while retrying, so this is correct.
			//may have to empty the path from src to intermediate for next
			//iteration of loop.
			empty_path(&path,&path_tail);
	}
	return -1;
}

static long route_recursive(long src, long dst, long retry)
{
	long i=-1,j=-1,found_one=-1;
	long next_node=-1,port=-1,p=-1,tryport=-1,node_up=-1,node_vp=-1,node_u=-1,ua=-1,tmp=-1;
	struct path_t *t1, *t2;
	t1 = path_tail;
	if(src==dst)
		return 1;
	if((next_node = switchOf(src)) ==switchOf(dst)) {
		p = get_port(next_node,dst);//don't change. also used later
		if((next_node=append_to_path(src,0))!=-1 &&
		        (next_node=append_to_path(next_node,p))!=-1) {
			return 1;
		} else {
			path_tail = t1;
			delete_tail(&path,path_tail);
			if(!retry) {
				return -1;
			} else {
				if(label[src][param_k] == label[dst][param_k]) { //i==j
					//first try the node_up to node_vp path.

					i=label[src][param_k];
					t2 = path_tail;
					node_u = switchOf(src);
					//node_v = switchOf(dst);// not needed since node_v == node_u;
					node_up = switchOf(network[src].port[1].neighbour.node);
					node_vp = switchOf(network[dst].port[1].neighbour.node);
					//#define KNKSTAR_FLAG1
#ifdef KNKSTAR_FLAG1
					printf("i %ld,src %ld, dst %ld,node_u %ld,node_up %ld,node_vp %ld\n",i,src,dst,node_u,node_up,node_vp);
					printarray(label[src],param_k);
					printarray(label[dst],param_k);
					printarray(label[node_up],param_k);
					printarray(label[node_vp],param_k);
					// printf("node_up %ld, node_vp %ld, switch neighbours",are_switch_neighbours(node_up,node_vp,i));
#endif
					//case copenhagen
					if ((next_node=append_to_path(src,1))!=-1 &&
					        (next_node=append_to_path(next_node,0))!=-1 &&
					        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_vp][i])))!=-1 &&
					        (next_node=append_to_path(next_node,1))!=-1 &&
					        (next_node=append_to_path(next_node,0))!=-1 &&
					        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
					        (next_node=append_to_path(next_node,1))!=-1
					   ) {
#ifdef DEBUG
						if (next_node != dst) {
							printf("failing path construction in short case, next_node %ld, dst %ld",next_node,dst);
							exit(-1);
						}
#endif
						return 1;
					} else {
						path_tail=t2;
						delete_tail(&path,path_tail);
					}// short case failed, so continue
					//short case failed, so do case XDR or XDY or XDJ
					if (network[src].port[0].faulty != 0 && network[switchOf(dst)].port[network[dst].port[0].neighbour.port].faulty == 0) {
						//case XDR tokyo
						i=label[src][param_k];
						init_RA(param_n);
						rand_RA(param_n);
						for(ua = 0; ua < param_n; ua++) {
							t2 = path_tail;
							node_u = switchOf(src);
							//node_v = switchOf(dst);// not needed since node_v == node_u;
							node_up = switchOf(network[src].port[1].neighbour.node);
							node_vp = switchOf(network[dst].port[1].neighbour.node);
							if(RA[ua] != label[node_up][i] &&
							        RA[ua] != label[node_vp][i] &&
							        RA[ua] != label[node_u][i] &&
							        (next_node=append_to_path(src,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,RA[ua])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,p))!=-1 //recall that p is the port to dst
							  ) {
#ifdef DEBUG
								if (next_node != dst) {
									printf("failing path construction case XDR, next_node %ld, dst %ld",next_node,dst);
									exit(-1);
								}
#endif
								return 1;
							} else {
								path_tail = t2;
								delete_tail(&path,path_tail);
							}
						}
					}
					if (network[src].port[0].faulty == 0 && network[switchOf(dst)].port[network[dst].port[0].neighbour.port].faulty != 0) {
						//case XDY melbourne
						i=label[src][param_k];
						init_RA(param_n);
						rand_RA(param_n);
						for(ua = 0; ua < param_n; ua++) {
							t2 = path_tail;
							node_u = switchOf(src);
							//node_v = switchOf(dst);// not needed since node_v == node_u;
							node_up = switchOf(network[src].port[1].neighbour.node);
							node_vp = switchOf(network[dst].port[1].neighbour.node);
							if(RA[ua] != label[node_up][i] &&
							        RA[ua] != label[node_vp][i] &&
							        RA[ua] != label[node_u][i] &&
							        (next_node=append_to_path(src,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,RA[ua])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_vp][i])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1
							  ) {
#ifdef DEBUG
								if (next_node != dst) {
									printf("failing path construction case XDY, next_node %ld, dst %ld",next_node,dst);
									exit(-1);
								}
#endif
								return 1;
							} else {
								path_tail = t2;
								delete_tail(&path,path_tail);
							}
						}

					}
					//we actually want to do case XDJ if the above cases fail.
					//  if (network[src].port[0].faulty != 0 && network[switchOf(dst)].port[network[dst].port[0].neighbour.port].faulty != 0) {
					//case XDJ stockholm
					init_RA(param_n);
					rand_RA(param_n);
					i=label[src][param_k];
					for(ua = 0; ua < param_n; ua++) {
						t2 = path_tail;
						node_u = switchOf(src);
						//node_v = switchOf(dst);// not needed since node_v == node_u;
						node_up = switchOf(network[src].port[1].neighbour.node);
						node_vp = switchOf(network[dst].port[1].neighbour.node);
						if(RA[ua] != label[node_up][i] &&
						        RA[ua] != label[node_vp][i] &&
						        RA[ua] != label[node_u][i] &&
						        (next_node=append_to_path(src,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,RA[ua])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_vp][i])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1
						  ) {
#ifdef DEBUG
							if (next_node != dst) {
								printf("failing path construction case XDJ, next_node %ld, dst %ld",next_node,dst);
								exit(-1);
							}
#endif
							return 1;
						} else {
							path_tail = t2;
							delete_tail(&path,path_tail);
						}
					}
					return -1;// we won't try anything else.

					//  }

				} else { //end i==j case
					//i!=j.
					if (network[src].port[0].faulty != 0 && network[switchOf(dst)].port[network[dst].port[0].neighbour.port].faulty == 0) {
						//case LKY vienna (similar to XDR), bypass through dimension i
						i=label[src][param_k];
						init_RA(param_n);
						rand_RA(param_n);
						for(ua = 0; ua < param_n; ua++) {
							t2 = path_tail;
							node_u = switchOf(src);
							//node_v = switchOf(dst);// not needed since node_v == node_u;
							node_up = switchOf(network[src].port[1].neighbour.node);
							if(RA[ua] != label[node_up][i] &&
							        RA[ua] != label[node_u][i] &&
							        (next_node=append_to_path(src,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,RA[ua])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,p))!=-1 //recall that p is the port to dst
							  ) {
#ifdef DEBUG
								if (next_node != dst) {
									printf("failing path construction case XDR, next_node %ld, dst %ld",next_node,dst);
									exit(-1);
								}
#endif
								return 1;
							} else {
								path_tail = t2;
								delete_tail(&path,path_tail);
							}
						}
					}

					if (network[src].port[0].faulty == 0 && network[switchOf(dst)].port[network[dst].port[0].neighbour.port].faulty != 0) {
						//case LKX zurich (similar to XDY).  alternate switch node through dimension j (let i stand for j here
						j=label[dst][param_k];//LET i STAND FOR j
						init_RA(param_n);
						rand_RA(param_n);
						for(ua = 0; ua < param_n; ua++) {
							t2 = path_tail;
							node_u = switchOf(src);
							node_vp = switchOf(network[dst].port[1].neighbour.node);
							if(RA[ua] != label[node_vp][j] &&
							        RA[ua] != label[node_u][j] &&
							        (next_node=append_to_path(src,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,RA[ua])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_vp][j])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_u][j])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1
							  ) {
#ifdef DEBUG
								if (next_node != dst) {
									printf("failing path construction case XDY, next_node %ld, dst %ld",next_node,dst);
									exit(-1);
								}
#endif
								return 1;
							} else {
								path_tail = t2;
								delete_tail(&path,path_tail);
							}
						}

					}
					//the remaining cases should be tried regardless, with LKK first, since it is shortest.
					//case LKK (similar to short case).  uses the unique 4-cycle that node_up and node_vp are on.
					//node_up, (node_up,j,node_vp[j])

					i=label[src][param_k];
					j=label[dst][param_k];
					t2 = path_tail;
					node_u = switchOf(src);
					//node_v = switchOf(dst);// not needed since node_v == node_u;
					node_up = switchOf(network[src].port[1].neighbour.node);
					node_vp = switchOf(network[dst].port[1].neighbour.node);
					if ((next_node=append_to_path(src,1))!=-1 &&
					        (next_node=append_to_path(next_node,0))!=-1 &&
					        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_vp][j])))!=-1 &&
					        (next_node=append_to_path(next_node,1))!=-1 &&
					        (next_node=append_to_path(next_node,0))!=-1 &&
					        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
					        (next_node=append_to_path(next_node,1))!=-1 &&
					        (next_node=append_to_path(next_node,0))!=-1 &&
					        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_u][j])))!=-1 &&
					        (next_node=append_to_path(next_node,1))!=-1
					   ) {
#ifdef DEBUG
						if (next_node != dst) {
							printf("failing path construction in short case, next_node %ld, dst %ld",next_node,dst);
							exit(-1);
						}
#endif
						return 1;
					} else {
						path_tail=t2;
						delete_tail(&path,path_tail);
					}// short case failed, so continue


					//case LKW munich

					i=label[src][param_k];
					j=label[dst][param_k];
					init_RA(param_n);
					rand_RA(param_n);
					for(ua = 0; ua < param_n; ua++) {
						t2 = path_tail;
						node_u = switchOf(src);
						//node_v = switchOf(dst);// not needed since node_v == node_u;
						node_up = switchOf(network[src].port[1].neighbour.node);
						node_vp = switchOf(network[dst].port[1].neighbour.node);
						if (RA[ua] != label[node_vp][j] &&
						        RA[ua] != label[node_u][j] &&
						        (next_node=append_to_path(src,1))!=-1 && //direction i
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,RA[ua])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_vp][j])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_u][j])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1
						   ) {
#ifdef DEBUG
							if (next_node != dst) {
								printf("failing path construction in short case, next_node %ld, dst %ld",next_node,dst);
								exit(-1);
							}
#endif
							return 1;
						} else {
							path_tail=t2;
							delete_tail(&path,path_tail);
						}
					}
					//caase LKV kyoto
					i=label[src][param_k];
					j=label[dst][param_k];
					init_RA(param_n);
					rand_RA(param_n);
					for(ua = 0; ua < param_n; ua++) {
						t2 = path_tail;
						node_u = switchOf(src);
						//node_v = switchOf(dst);// not needed since node_v == node_u;
						node_up = switchOf(network[src].port[1].neighbour.node);
						node_vp = switchOf(network[dst].port[1].neighbour.node);
						if (RA[ua] != label[node_up][i] &&
						        RA[ua] != label[node_u][i] &&
						        (next_node=append_to_path(src,1))!=-1 && //direction i
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,RA[ua])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_vp][j])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1 &&
						        (next_node=append_to_path(next_node,0))!=-1 &&
						        (next_node=append_to_path(next_node,get_port_to_server(next_node,j,label[node_u][j])))!=-1 &&
						        (next_node=append_to_path(next_node,1))!=-1
						   ) {
#ifdef DEBUG
							if (next_node != dst) {
								printf("failing path construction in short case, next_node %ld, dst %ld",next_node,dst);
								exit(-1);
							}
#endif
							return 1;
						} else {
							path_tail=t2;
							delete_tail(&path,path_tail);
						}
					}
					return -1;
				}
			}//end src -- u -- dst retries.
		}
	}
	for(i=0; i<param_k; i++) {
		if(label[src][i] != label[dst][i]) {
			found_one = 0;
			//route from (u,j,ujp) along i to (next,i,ui).
			//if i=j and dst[i]=ujp this is one hop, and otherwise it is 2.
			if(label[src][param_k]==i &&
			        (label[dst][i]==label[src][param_k+1])) {
				if((next_node = append_to_path(src,1))==-1) {
					path_tail = t1;
					delete_tail(&path,path_tail);
					if(retry) {
						//WARNING: THIS BIT OF CODE WILL INTRODUCE AN EXTRA SERVER HOP
						//UNNECESSARILY if at the end, next_node is not equal to dst.
						//This means that we depend on the availability of that connection, but don't need it.
						//we should route over another dimension
						//case YYJ fukuoka
						node_u = switchOf(src);
						tmp = network[src].port[1].neighbour.node;//for error checking
						node_up = switchOf(tmp); ////careful.  we do not want dst here.
						init_RA(param_n);
						rand_RA(param_n);
						for (ua=0; ua<param_n; ua++) {
							//                        found_one = 0;
							if (RA[ua] != label[node_u][i] &&
							        RA[ua] != label[node_up][i] &&
							        (next_node=append_to_path(src,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,RA[ua])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        (next_node=append_to_path(next_node,0))!=-1 &&
							        (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_up][i])))!=-1 &&
							        (next_node=append_to_path(next_node,1))!=-1 &&
							        ((next_node == dst) || //deal with possibility that dst is on the reroute path
							         (
							             (next_node=append_to_path(next_node,0))!=-1 &&
							             (next_node=append_to_path(next_node,get_port_to_server(next_node,i,label[node_u][i])))!=-1
							         )
							        )
							   ) {
								if(next_node == dst)
									return 1;
#ifdef DEBUG
								if (next_node != tmp) {
									printf("failing path construction in case YYJ, next_node %ld, tmp %ld",next_node,tmp);
									exit(-1);
								}
#endif
								found_one = 1;
								break;
							} else {
								path_tail = t1;
								delete_tail(&path,path_tail);
								return -1;
							}
						}
					}
				} else {
					found_one = 1;
				}
			} else {
				//-src-(dimension i)-u--server--server.  u is a switch
				if((next_node=append_to_path(src,0))==-1) {
					path_tail = t1;
					delete_tail(&path,path_tail);
					j = label[src][param_k];
					if(path_is_null(path)) { // && i!=j){
						//we need to reroute to the switch through other dimensions.  a few special cases to consider.  It only makes sense to do this at the top level, since otherwise we would backtrack to try other approaches.
						init_RA(param_n);
						rand_RA(param_n);
						for(ua=0; ua<param_n; ua++) {
							//CASE YVR SYDNEY
							found_one = 0;
							t1=path_tail;
							tmp = network[
							          network[switchOf(src)].port[
							              get_port_to_server(switchOf(src),i,label[dst][i])
							          ].neighbour.node
							      ].port[1].neighbour.node;
							if(RA[ua] != label[src][j] && //RA[ua] != uj
							        RA[ua] != label[src][param_k+1] &&  //RA[ua] != ujp
							        (next_node = append_to_path(src, 1)) != -1 &&
							        (next_node = append_to_path(next_node, 0)) != -1 &&
							        (next_node =
							             append_to_path(
							                 next_node,
							                 get_port_to_server(next_node,j,RA[ua]))) != -1 &&
							        (next_node = append_to_path(next_node, 1)) != -1 &&
							        ((dst == next_node) ||// stop checking if dst==next_node
							         ((next_node = append_to_path(next_node, 0)) != -1 &&
							          (next_node =
							               append_to_path(
							                   next_node,
							                   get_port_to_server(next_node,j,label[src][j]))) != -1 &&
							          ((i==j && RA[ua] == label[dst][i]) ||    //if we have case C, stop
							           ((next_node = append_to_path(next_node, 1)) != -1 &&
							            (next_node = append_to_path(next_node, 0)) != -1 //go right to the switch
							           )
							          )
							         )
							        )
							  ) {
								// we have to do something special with case C!
								if(dst == next_node) return 1;
								if(i==j && RA[ua] == label[dst][i]) {
#ifdef DEBUG
									if(tmp!=next_node) {
										printf("Case YVR, next_node %ld not equal tmp %ld",next_node, tmp);
										exit(-1);
									}
#endif
									//this recursive call does not affect the array RA for this loop,
									//since we break the loop regardless of the result of the recursion.
									if(route_recursive(next_node,dst,retry)==1) {
										return 1;
									} else {
										path_tail = t1;
										delete_tail(&path,path_tail);
										if(!path_is_null(path)) {
											printf("boo");
											exit(-1);
										}
										return -1;
									}
									//we have returned
								}
								found_one = 1;
								break;
							} else {
								path_tail = t1;
								delete_tail(&path,path_tail);
							}
						}
						if (found_one==0) {
							return -1;
						} else {
							found_one = 0;
						}
					} else
						return -1;
				}//else we are at the switch u keep going

				//to server (u,i,uip), where u is switchOf(src) and uip is
				//ith pos of switchOf(dst), via port "port"
				port = get_port_to_server(next_node,i,label[dst][i]);
				t2 = path_tail;//bookmark our place in the list
				if((next_node = append_to_path(next_node,port))!=-1 &&
				        (next_node = append_to_path(next_node,1))!=-1) {
					found_one = 1;
				} else {
					//undo what was done in the if statement
					path_tail = t2;
					delete_tail(&path,path_tail);
					//set next_node.
					next_node = switchOf(src);//network[src].port[0].neighbour.node;
					// we need this here because next_node=-1
					if(retry) {
						//we have arrived successfuly at the switch, but travelling
						//directly toward the ideal server did not work.  try going
						//to other dimensions(or other values in same dimension).
						//reset next_node to the switch.
						init_RA(param_n-1);
						rand_RA(param_n-1);
						for(tryport=0; tryport<param_n-1; tryport++) {
							//bookmark our place
							//found_one = 0;
							t2=path_tail;
							next_node = network[src].port[0].neighbour.node;//already appended up to here
							if((RA[tryport] + i*(param_n-1)) != port &&
							        (next_node = append_to_path(next_node, RA[tryport] + i*(param_n-1))) != -1 &&
							        (next_node = append_to_path(next_node, 1)) != -1 &&
							        (next_node = append_to_path(next_node, 0)) != -1 &&
							        (next_node = append_to_path(next_node, get_port_to_server(next_node,i,label[dst][i]))) != -1 &&
							        (next_node = append_to_path(next_node, 1)) != -1 //&&
							        //check that the next step will be ok as well.
							        //(next_node==dst || network[next_node].port[0].faulty==0)
							        //&& 0
							  ) {
								found_one = 1;
								break;
							} else {
								path_tail = t2;
								delete_tail(&path,path_tail);
							}
						}
					}
				}
			}
			if(found_one == 1 && route_recursive(next_node,dst,retry)==1)
				return 1;
			else {
				path_tail = t1;
				delete_tail(&path,path_tail);
				t1 = path_tail;
			}
		}
	}
	path_tail = t1;
	delete_tail(&path,path_tail);
	return -1;
}

static long append_to_path(long node, long port)
{
	//#define KNKSTAR_FLAG2
#ifdef KNKSTAR_FLAG2
	printf("node %ld, neighbour %ld, faulty %ld, port %ld\n",
	       node,
	       network[node].port[port].neighbour.node,
	       network[node].port[port].faulty,
	       port);
	printarray(label[node],param_k);
	printarray(label[network[node].port[port].neighbour.node],param_k);
#endif

#ifdef DEBUG
	if (port<0 || port>get_radix_knkstar(node)) {
		printf("Invalid appendage, node %ld, port %ld",node,port);
		exit(-1);
	}
#endif
	if(network[node].port[port].faulty!=0)
		return -1;
	path_enqueue(&path,&path_tail, &path_penultimate,port);
	return network[node].port[port].neighbour.node;
}

static void construct_labels()
{
	long i;
	long j;
	long remainder;
	///< Allocate and add labels to the array of nodes n^0*u_0+ n^1*u_1 +
	///< n^2*u_2 + ... + n^{k-1}*u_{k-1}.  Server (u,i,uip), is server
	///< attached to switch u, on dimension i, on path to switch u', which
	///< differs on dimension i, so u'_i=uip.  Thus, server labels have 2
	///< extra coordinates, for i and uip.

	label=malloc((servers+switches) * sizeof(long*));
	//label the switches with k-tuples
	for(i=0; i<switches; i++) {
		label[i]=malloc((param_k)*sizeof(long));
		remainder = i;
		for(j=0; j<param_k; j++) {
			label[i][j] = remainder%param_n;
			remainder/=param_n;
		}
#ifdef DEBUG
		if(i != c2i(label[i],0)) {
			printf("c2i not working for %ld.  outputs %ld\n",i,c2i(label[i],0));
			printarray(label[i],param_k);
		}
#endif
	}
	//label the servers with (k+2)-tuples.  for switches u, u' which are
	//dimension r neighbours, the server x is labelled,
	//u,label[k],label[k+1], where label[k]=r, and label[k+1]=u'[r]
	for(; i<switches+servers; i++) {
		label[i]=malloc((param_k+2)*sizeof(long));
		remainder = i-switches;
		//(n-1)*k servers for each of the n^k switches
		//this value gets updated after we learn u_{label[param_k]}
		label[i][param_k+1] = remainder%(param_n-1);
		remainder /= param_n-1;
		label[i][param_k] = remainder%(param_k);
		remainder /= param_k;
		for(j=0; j<param_k; j++) {
			label[i][j] = label[remainder][j];
		}
		if(label[i][param_k+1]>=label[i][label[i][param_k]]) {
			(label[i][param_k+1])++;
		}
#ifdef DEBUG
		// check that this here "unrank" function is consistent with our
		// "rank" function, c2i
		if(i != c2i(label[i],1)) {
			printf("c2i not working for %ld. outputs %ld\n",i,c2i(label[i],1));
			printarray(label[i],param_k+2);
		}
#endif
	}
}

static long switchOf(long server)
{
#ifdef DEBUG
	if(server < switches)
		printf("Called switchOf with a switch instead of a sever: %ld\n",server);
	if(network[server].port[0].neighbour.node !=
	        (server-switches)/(param_n-1)/(param_k))
		printf("switchOf not working, reads %ld from network, calculates %ld in code\n",network[server].port[0].neighbour.node,
		       (server-switches)/(param_n-1)/(param_k));
#endif// DEBUG
	return network[server].port[0].neighbour.node;
}

static long get_port(long src, long dst)
{
	if(src>=switches) {
		//src is a server
		if(dst<switches)
			//dst is a switch
			return 0;
		else
			//dst is a server
			return 1;
	} else {
		//src is a switch
		return network[dst].port[0].neighbour.port;
	}
}

static long get_port_to_server(long u, long i, long uip)
{
	long port;
	port = i*(param_n-1)+uip;
	if(uip > label[u][i])
		port--;
	return port;
}

static long c2i(long *c, long isServer)
{
	long j;
	long i;
	i=0;
	for(j=param_k-1; j>=0; j--) {
		i = c[j]+param_n*i;
	}
	if(isServer>0) {
		i = i*(param_n-1)*param_k+c[param_k]*(param_n-1)+c[param_k+1];
		if(c[param_k+1]>=c[c[param_k]])
			i--;
		i += switches;
	}
	return i;
}

static long init_RA(long param)
{
	if (param==0) {
		return 1;
	} else {
		RA[param-1]=param-1;
		init_RA(param-1);
	}
	return 1;
}

static long rand_RA(long param)
{
	long tmp,i;
	if(param>0) {
		i = rand()%param;
		tmp = RA[param-1];
		RA[param-1] = RA[i];
		RA[i] = tmp;
		rand_RA(param-1);
	}
	return 1;
}

#ifdef DEBUG
static void printarray(long *a,long n)
{
	long i;
	for(i=0; i<n; i++) {
		printf("%ld ",a[i]);
	}
	printf("\n");
}
#endif
