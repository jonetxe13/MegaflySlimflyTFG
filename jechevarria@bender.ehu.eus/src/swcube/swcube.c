/** @mainpage
 * SWCube topology
 */

#include <stdio.h>
#include <stdlib.h>
#include "swcube.h"

//#define SHOWLABELS
#define DETERMINISTIC
#define DET1

static long param_k;	///< number of dimensions
static long param_n;	///< size of clique in each dimension

static long *n_pow;	///< an array with the n^i, useful for doing some calculations.
static long n_choose_2;
static long servers; 	///< The total number of servers
static long switches;	///< The total number of switches

static long** label; ///< array of labels of nodes

static char* network_token="swcube";
static char* routing_token="dimensional";
static char* topo_version="v0.2.1";
static char* topo_param_tokens[2]= {"k", "n"};
static char filename_params[100]; ///< a substring of the output filenames

static struct path_t *path=NULL, *path_tail=NULL, *path_penultimate;

/**
 * Compute and store route from 'src' to 'dst', if it exists, in
 * 'path'
 * @param
 * @param
 * @return 1 if successful, otherwise -1
 */
static long route_full(long src, long dst);

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
 *
 * Regarding the server labels of swcube: servers are labelled
 * (u,j,up[j]), where u=(u[0],u[1],...,u[param_k-1]) and u[j]<up[j].
 * The switch u is on port 0 and the switch up is on port 1.
 */
static void construct_labels();

static long get_j(long server); ///< Return j of server (u,j,ujp)
static long get_upj(long server); ///< Return upj of server (u,j,ujp)
static long get_uj(long server); ///< Return uj of  server (u,j,ujp)


/**
 * Return a random element from the intersection of src and dst in
 * dimension i.  required that src and dst be servers, and that 0\le i
 * < param_k
 *
 * @param server
 * @param server
 * @param coordinate
 */
static long coordinates_intersect(long src, long dst, long i);

/**
 * Make a choice in deterministic routing based on precompiler switches
 */
static long make_det(long src, long dst);

/**
 * Return the port from switch 'u' to server '(u,i,uip)'.
 * uip is u'_i, so the code translates this to the correct port.
 *
 * @param switch-node
 * @param dimension
 * @param coordinate that differs on dimension 'i'
 */
static long get_port_to_server(long u, long i, long uip);

/**
 * Compute and return dimension 'j' from port number 'port'
 */
static long get_j_from_port(long port);

/**
 * Compute and return 'ujp' from 'port' and 'uj' between switch u
 * and server (u,j,ujp)
 */
static long get_ujp_from_port(long port,long uj);

/**
 * Rank the pair (a,b) as a 2-combination from {0,1,..., n-1}
 * require a<b tested in test_n_choose_2(long n)
 */
static long rank_n_choose_2(long a, long b, long n);

/**
 * Unrank i to a 2-combination from /{0,1,..., n-1} (a,b) where a<b.
 * expect first call to have *a=*b=0.
 *
 */
static void unrank_n_choose_2(long *a, long *b, long i, long n);

/**
 * Unrank node to label.  Servers always unrank u to the switch on port 0,
 * so u[j]<up[j]
 */
static void unrank(long **res, long i);

/**
 * Rank function: Turns coordinates into index.
 *
 * @param a pointer to label of node 'i'
 * @param isServer == 1 iff this is a server label.
 *
 * @return i
 */
static long c2i(long *c, long isServer);

#ifdef DEBUG
static void printarray(long *a,long n);
#endif

///< Begin swcube.h functions

long init_topo_swcube(long nparam, long* params)
{
	long i;

	if (nparam<2) {
		printf("2 parameters are needed for SWCUBE <k, n>\n");
		exit(-1);
	}
	param_k=params[0];
	param_n=params[1];

	sprintf(filename_params,"k%ldn%ld",param_k,param_n);

	n_pow=malloc((param_k+1)*sizeof(long));
	n_pow[0]=1;

	//precompute powers of param_n
	for (i=1; i<=param_k; i++) {
		n_pow[i]=n_pow[i-1]*param_n;
	}
	n_choose_2 = param_n*(param_n-1)/2;
	switches=n_pow[param_k];
	servers=switches*(param_n-1)*param_k/2;

	construct_labels();

	return 1; // return status, not used here.
}

void finish_topo_swcube()
{
	long i;
	for(i=0; i<switches+servers; i++) {
		free(label[i]);
	}
	free(label);
}

long get_servers_swcube()
{
	return servers;
}

long get_switches_swcube()
{
	return switches;
}

long get_ports_swcube()
{
	return servers*2 + switches*get_radix_swcube(0);
}

long is_server_swcube(long i)
{
	if(i>=switches && i < switches+servers)
		return 1;
	else
		return 0;
}

long get_server_i_swcube(long i)
{
	return(switches + i);
}

long get_switch_i_swcube(long i)
{
	return(i);
}

long node_to_server_swcube(long i)
{
	return(i-switches);
}
long node_to_switch_swcube(long i)
{
	return(i);
}

char* get_network_token_swcube()
{
	return network_token;
}

char* get_routing_token_swcube()
{
	return routing_token;
}

char* get_topo_version_swcube()
{
	return topo_version;
}

char* get_topo_param_tokens_swcube(long i)
{
	return topo_param_tokens[i];
}

char * get_filename_params_swcube()
{
	return filename_params;
}

long get_radix_swcube(long n)
{
	if (n>=switches)
		return 2;	// If this is a server it has 2 ports
	else
		return (param_n-1)*param_k; // If this is a switch it has (n-1)*k ports
}

/**
 * Calculates connections
 *
 * swcube convention for server ports is that ui<uip iff u is on port 0
 * and up is on port 1.
 */
tuple_t connection_swcube(long node, long port)
{
	tuple_t res;
	long * thing;
	long i,j,ujp,uj;
	if(node<switches) {
		//build server label from port number and switch label, then rank
		//it.  note that if we are connecting to ujp in jth dimension,
		//then port== get_port_to_server(u,j,ujp)
		j=get_j_from_port(port);
		uj=label[node][j];
		ujp=get_ujp_from_port(port,uj);
		thing=malloc((param_k+2)*sizeof(long));
		for(i=0; i<param_k; i++)
			thing[i]=label[node][i];
		//externalise this if we use it a lot later
		thing[i++]=j;
		thing[i]=ujp;
		if(uj<ujp)
			res.port=0;
		else { //change to canonical label
			res.port=1;
			thing[i]=uj;
			thing[j]=ujp;
		}
		res.node=c2i(thing,1);
	} else {
		unrank(&thing,node);//always unranks to (u,j,ujp), where u is the
		//server on port 0, so uj<ujp
		ujp=thing[param_k+1];
		j=thing[param_k];
		uj=thing[j];
		if(port==1) {
			thing[j]=ujp;
			thing[param_k+1]=uj;
		}
		res.node=c2i(thing,0);
		res.port=get_port_to_server(res.node,j,thing[param_k+1]);
	}
	free(thing);
	return res;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_swcube(long src, long dst){
    return(1);
}

// Initializes the routing for a given path. Can be used for source routing.
// In local-routing this might do nothing
// returns -1 if there is no route to the destination.
long init_routing_swcube(long src, long dst)
{
	long route_result =  route_full(src,dst);
#ifdef DEBUG
	if(route_result!=1 && !path_is_null(path)) {
		printf("route_result %ld,src=%ld and dst=%ld but path is not null\n",route_result, src,dst);
		print_path(path);
		exit(-1);
	}
	if(route_result == 1 && src != dst && path_is_null(path)) {
		printf("Path is connected but null, src=%ld, dst=%ld\n",src,dst);
		exit(-1);
	}
#endif //DEBUG
	return route_result;
}

// Finishes a route, checks route is complete and frees stuff if needed.
//this is a cleanup function
void finish_route_swcube()
{
#ifdef DEBUG
	if(!path_is_null(path)) {
		printf("mark_route failed to empty the path\n");
		print_path(path);
		exit(-1);
	}
#endif //DEBUG
}

long route_swcube(long current, long destination)
{
	return path_dequeue(&path,&path_tail);
}

///< End swcube.h functions

static long route_full(long src, long dst)
{
	long next_node,tmp,tmp2,this_node,exit_switch,i;
	//for each dimension i, if the coordinates differ, then ammend them.
	//if i is not next_nodej and next_node and dst do not differ at
	//next_nodej then we need to leave next_node via the intersection of
	//next_node and dst at these coordinates; otherwise, we choose
	//arbitrarily, but (try to do it) in a way that balances the usage
	//of the links in the network.  Finally, settle into dstj coordinate
	//if necessary.
	next_node=src;
	for(i=0; i<param_k; i++) {
		if(coordinates_intersect(src,dst,i)==-1) {
			if(get_j(next_node)!=i) {
				//we are changing on dimension i, but if next_node and dst do
				//not differ at dimension get_j(next_node) (different from i),
				//then we may need to exit get_j(next_node) via a specific
				//switch.  otherwise pick one arbitrarily.
				if((exit_switch=
				            coordinates_intersect(next_node,dst,get_j(next_node)))==-1 ||
				        exit_switch==param_n) {
					//THIS DEALS WITH THE param_n return value
					//coordinates_intersect
#ifdef DEBUG
					if(src!=next_node) {
						printf("Expected next_node=src in this case\n");
						exit(-1);
					}
#endif
#ifdef DETERMINISTIC
					exit_switch=make_det(src,dst)?get_uj(next_node):get_upj(next_node);
#else
					exit_switch=(rand()%2)?get_uj(next_node):get_upj(next_node);
#endif
				}
				//having chosen an exit switch, we append this switch to the path
				if((next_node =
				            append_to_path(
				                next_node,
				                (exit_switch==get_uj(next_node))?0:1
				            )
				   )
				        == -1
				  ) {
					empty_path(&path,&path_tail);
					return -1;
				}
				//next_node is now a switch and we need to get to the server
				//on dimension i.
			} else { //we are changing on dimension get_j(next_node), so the
				//exit switch is arbitrary, since these dimensions differ.
#ifdef DEBUG
				if(src!=next_node) {
					printf("Expected next_node=src in this case\n");
					exit(-1);
				}
#endif
#ifdef DETERMINISTIC
				if((next_node =
				            append_to_path(next_node,
				                           ((
				                                make_det(src,dst)
				                            )?0:1)))==-1)
#else
				if((next_node = append_to_path(next_node,rand()%2))==-1)
#endif
				{
					empty_path(&path,&path_tail);
					return -1;
				}

			}
			//next_node is a switch here
			if((next_node =
			            append_to_path(
			                next_node,
			                get_port_to_server(
			                    next_node,
			                    i,
			                    (get_j(dst)==i)?
#ifdef DETERMINISTIC
			                    (make_det(src,dst)?
			                     get_uj(dst)
			                     :
			                     get_upj(dst))
#else
			                    ((rand()%2)?
			                     get_uj(dst)
			                     :
			                     get_upj(dst))
#endif


			                    :
			                    label[dst][i]
			                )
			            ))==-1) {
				empty_path(&path,&path_tail);
				return -1;
			}
		}
#ifdef DEBUG
		if(coordinates_intersect(next_node,dst,i)==-1) {
			empty_path(&path,&path_tail);
			return -1;
		}
#endif
	}
	//  Settling step.
	if(next_node==dst)
		return 1;
	//next_node and dst at same switch
	this_node=next_node;
	if(((next_node =  network[this_node].port[tmp=0].neighbour.node) ==
	        network[dst].port[tmp2=1].neighbour.node )
	        ||
	        (network[this_node].port[0].neighbour.node ==
	         network[dst].port[tmp2=0].neighbour.node )
	        ||
	        ((next_node =  network[this_node].port[tmp=1].neighbour.node) ==
	         network[dst].port[tmp2=1].neighbour.node )
	        ||
	        (network[this_node].port[1].neighbour.node) ==
	        network[dst].port[tmp2=0].neighbour.node ) {
		if( ((next_node=append_to_path(this_node,tmp))==-1) ||
		        ((next_node=
		              append_to_path(next_node,
		                             network[dst].port[tmp2].neighbour.port))==-1)
		  ) {
			empty_path(&path,&path_tail);
			return -1;
		}
		return 1;
	} else return 1;
}

//appends a port (and a link) to the variable path, at the tail
static long append_to_path(long node, long port)
{
#ifdef DEBUG
	if (port<0 || port>get_radix_swcube(node)) {
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
	///< Allocate and add labels to the array of nodes n^0*u_0+ n^1*u_1 +
	///< n^2*u_2 + ... + n^{k-1}*u_{k-1}.  Server (u,i,uip), is server
	///< attached to switch u, on dimension i, on path to switch u', which
	///< differs on dimension i, so u'_i=uip.  Thus, server labels have 2
	///< extra coordinates, for i and uip.

	///< Note that if ui=_i, then (u,i,uip)=(up,i,ui) (this is the
	///< difference between swcube and knkstar).
	label=malloc((servers+switches) * sizeof(long*));
	for(i=0; i<switches+servers; i++) {
		unrank(&(label[i]),i);
	}
}

static long get_j(long server)
{
	return label[server][param_k];
}

static long get_upj(long server)
{
	return label[server][param_k+1];
}

static long get_uj(long server)
{
	return label[server][get_j(server)];
}

static long coordinates_intersect(long src, long dst, long i)
{
	long res,res2;
	if(get_j(src)==i) { //test both get_upj(src) and get_uj(src)
		if(get_j(dst)==i) { //test both get_upj(dst) and get_uj(dst)
			if((res=get_upj(src))==get_upj(dst) &&
			        (res2=get_uj(src))==get_uj(dst))
				//The problem with determinism here is that we need to make a
				//choice dependent on the coordinate we want to change, which
				//is not this one.
#ifdef DETERMINISTIC
				//WARNING.  IF THE RETURN VALUE IS TO BE USED, YOU MUST CHOOSE
				//RES OR RES2 WHERE THIS FUNCTION IS CALLED, BECAUSE PARAM_N
				//IS AN INVALID COORDINATE VALUE.
				return param_n;
#else
				return (rand()%2)?res:res2;
#endif
#ifdef DEBUG
			//this next case does not occur, given the canonical labeling
			//where get_uj(src) < get_upj(src) and get_uj(dst) <
			//get_upj(dst).  We throw an exception if it does
			if((res=get_upj(src))==get_uj(dst) &&
			        (res2=get_uj(src))==get_upj(dst)) {
				printf("unexpected behaviour in coordinates_intersect\n");
				exit(-1);
			}
#endif
			if((res=get_upj(src))==get_upj(dst))
				return res;
			if((res=get_upj(src))==get_uj(dst))
				return res;
			if((res=get_uj(src))==get_uj(dst))
				return res;
			if((res=get_uj(src))==get_upj(dst))
				return res;
			else return -1;
		} else if(((res=label[dst][i])==get_upj(src)) ||
		          (label[dst][i]==get_uj(src)))
			return res;
		else return -1;
	} else if(get_j(dst)==i) {
		if(((res=label[src][i])==get_upj(dst)) ||
		        (label[src][i]==get_uj(dst)))
			return res;
		else return -1;
	}
	if((res=label[src][i]) == label[dst][i])
		return res;
	else return -1;
}

static long make_det(long src, long dst)
{

#ifdef DET1
	return (rank_n_choose_2(get_uj(src),
	                        get_upj(src),
	                        param_n) <
	        (param_n*(param_n-1)/4));
#endif
#ifdef DET2
	return ((src-dst)+servers)%servers < servers/2;
#endif
#ifdef DET3
	return (get_upj(src)-get_uj(src))<param_n/2;
#endif
#ifdef DET4
	return (get_uj(src))<param_n/2;
#endif
}

static long get_port_to_server(long u, long i, long uip)
{
	long port;
	port = i*(param_n-1)+uip;
	if(uip > label[u][i])
		port--;
	return port;
}

static long get_j_from_port(long port)
{
	return port/(param_n-1);
}

static long get_ujp_from_port(long port,long uj)
{
	long ujp=port%(param_n-1);
	if(ujp>=uj)
		ujp++;
	return ujp;
}

static long rank_n_choose_2(long a, long b, long n)
{
	if(a>=b)
		printf("ERROR in rank_n_choose_2: inputs, %ld should be less than %ld\n",a,b);
	if(a==0) {
		return b-1;
	}
	return n-1+rank_n_choose_2(a-1,b-1,n-1);
}

static void unrank_n_choose_2(long *a, long *b, long i, long n)
{
	if(i<(n-1)) {
		*a+=0;
		*b+=(i+1);
		return;
	}
	*a+=1;
	*b+=1;
	unrank_n_choose_2(a,b,i-n+1,n-1);
	return;
}

static void unrank(long **res, long i)
{
	long remainder,j;
	if(i<switches) {
		*res=malloc((param_k)*sizeof(long));
		remainder = i;
		for(j=0; j<param_k; j++) {
			(*res)[j] = remainder%param_n;
			remainder/=param_n;
		}
#ifdef SHOWLABELS
		printf("rank %ld switch: ",i);
		printarray(*res,param_k);
#endif
#ifdef DEBUG
		if(i != c2i(*res,0)) {
			printf("c2i not working for switch %ld.  outputs %ld\n",i,c2i(*res,0));
			printarray(*res,param_k);
		}
#endif
		return;
	} else {
		//label the servers with (k+2)-tuples.  for switches u, up which
		//are dimension r neighbours, the server x is labelled,
		//u,label[k],label[k+1], where label[k]=r, and label[k+1]=up[r]
		//note that in swcube, up,r,u[r] is the same server, but the
		//canonical label has u[r]<up[r]
		*res=malloc((param_k+2)*sizeof(long));
		remainder = i-switches;
		//swcube: k*(n choose 2) servers for each of n^(k-1) (k-1)-tuples in lex order:
		// for a given (k-1)-tuple, and dimensions in order from 0 to k we have pairs
		// (0,1), (0,2), ..., (0,n-1),(1,2),(1,3), ..., (1,n-1)

		//first find out which (k-1)-tuple it is

		//    (*res)[param_k+1] = remainder%(param_n-1);

		//this is an intermediate value for getting the K_n pair
		long tmp = remainder%(n_choose_2);
		remainder /= n_choose_2;
		//now we can discover dimension r of this server's edge
		(*res)[param_k] = remainder%(param_k);
		remainder /= param_k;
		//now remainder unranks to the (k-1)-tuple
		//(u[0],u[1],...,u[r-1],u[r+1], ..., u[k]) but we want the
		//switch's k-tuple after inserting either u[r] encoded in the K_n
		//pair.  It's easiest just to build the switch label again.
		for(j=0; j<(*res)[param_k]; j++) {
			(*res)[j] = remainder%param_n;
			remainder/=param_n;
		}
		//unrank the K_2 pair.  Note that
		//(*res)[j]=uj<ujp=(*res)[param_k+1]
		(*res)[j]=0;
		(*res)[param_k+1]=0;
		unrank_n_choose_2(&((*res)[j]),
		                  &((*res)[param_k+1]),
		                  tmp,param_n);
#ifdef DEBUG
		if((*res)[j]>=(*res)[param_k+1])
			printf("ERROR in unrank: (*res)[j]>=(*res)[param_k+1] for ((*res)[j],j,(*res)[param_k+1])=(%ld,%ld,%ld)\n",(*res)[j],j,(*res)[param_k+1]);
#endif
		j++;
		for(; j<param_k; j++) {
			(*res)[j] = remainder%param_n;
			remainder/=param_n;
			//      (*res)[j] = label[remainder][j];
		}
#ifdef SHOWLABELS
		printf("rank %ld server: ",i);
		printarray(*res,param_k+2);
		printf("server reranked as %ld\n",c2i(*res,1));
#endif
#ifdef DEBUG
		// check that this here "unrank" function is consistent with our
		// "rank" function, c2i
		if(i != c2i(*res,1)) {
			printf("c2i not working for server %ld. outputs %ld\n",i,c2i(*res,1));
			printarray(*res,param_k+2);
		}
#endif
	}
}

static  long c2i(long *c, long isServer)
{
	long j;
	long i;
	///< Note that switch labels are (u[0],u[1],..., u[k-1])=n^0*u_0+
	///< n^1*u_1 + n^2*u_2 + ... + n^{k-1}*u_{k-1}.  Server labels are
	///< (u[0],u[1],..., u[k-1],j,up[j]) for neighbouring switch up, such
	///< that u[j]<up[j]
	i=0;
	if(isServer<=0) {
		for(j=param_k-1; j>=0; j--) {
			i = c[j]+param_n*i;
		}
	} else {
		//if(isServer>0){
		//need to skip coordinate r=c[c[param_k]] and use it later
		for(j=param_k-1; j>c[param_k]; j--) {
			i = c[j]+param_n*i;
		}
		j--;
		for(; j>=0; j--) {
			i = c[j]+param_n*i;
		}
#ifdef DEBUG
		if(c[c[param_k]]>=c[param_k+1])
			printf("ERROR in c2i: up[j]>=u[j], but should have u[j]<up[j].\n(u[j],j,up[j])=(%ld,%ld,%ld)\n",c[c[param_k]],c[param_k],c[param_k+1]);
#endif
		//now i is the rank of a (k-1)-tuple...
		i = i*n_choose_2*param_k+//the ith (k-1)-tuple
		    c[param_k]*n_choose_2+//the c[param_k]th coordinate
		    //the rank of the combination of u[j],up[j]
		    rank_n_choose_2(c[c[param_k]],c[param_k+1],param_n);
		i += switches;
	}
	return i;
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
