#ifndef _dpillar
#define _dpillar

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../inrflow/node.h"
#include "../inrflow/misc.h"
#include "dpillar.h"

extern node_t* network;
extern routing_t routing;

extern long long link_hops;
extern long long server_hops;
extern long* server_hop_histogram;
extern long* path_length_histogram;

extern long update_hist_array(long hops, long ** H, long n);

extern char filename_params[100];

static char* network_token="dpillar";
static char* routing_token;
static char* topo_version="v0.1";
static char* topo_param_tokens[2]= {"n","k"};

static long N_sr, N_sw, N, port, col, turn_back, diameter;

static long* path;

static long rout_alg, dir;	// used for DPillar network

static long** phys_address;
static long nxt_sw_port;

static long path_index;

static long all_path[100][50];
static long min_route_hist[10]= {0};
static long ver_index, hor_index;

static long *B, *I, *J;

long init_topo_dpillar(long np, long* par)
{

	port=par[0];
	col =par[1];

	sprintf(filename_params, "n%ldk%ld", port, col);

	N_sw = col*(long)pow(port/2.0, col-1);
	N_sr = N_sw * port/2;
	N = N_sr+N_sw;

	long i, m, n;
	phys_address = malloc(N * sizeof(long*));	// create nodes
	for(n=0; n<N_sr; n++) {
		phys_address[n]=malloc((col+1)*sizeof(long));
		m=n;				// calculate the physical address of servers
		for(i=0; i<col; i++) {
			phys_address[n][i]= m%(port/2);
			m/=(port/2);
		}
		phys_address[n][col]=m;
	}

	for(n=N_sr; n<N; n++) {
		phys_address[n]=malloc(col*sizeof(long));
		m=n-N_sr;			// calculate the physical address of switches
		for(i=0; i<col-1; i++) {
			phys_address[n][i]= m%(port/2);
			m/=(port/2);
		}
		phys_address[n][col-1]=m;
	}
	path = malloc((4*col+2)*sizeof(long));



	B=(long*)malloc(col*sizeof(long));
	I=(long*)malloc(col*sizeof(long));
	J=(long*)malloc(col*sizeof(long));

	return 1; // return status, not used here.
}

/**
* Release the resources used by the topology.
**/

void finish_topo_dpillar()
{

}



long get_servers_dpillar()
{
	return N_sr;
}

long get_switches_dpillar()
{
	return N_sw;
}

long get_ports_dpillar()
{
	return N_sr*2 + N_sw*port;
}

long get_radix_dpillar(long n)
{
	if(n < N_sr)
		return 2;
	else
		return port;
}

long is_server_dpillar(long i)
{
	if(i < N_sr)
		return 1;
	else
		return 0;
}

long get_server_i_dpillar(long i)
{
	return i;
}

long get_switch_i_dpillar(long i)
{
	return i+N_sr;
}

long node_to_server_dpillar(long i)
{
	return i;
}

long node_to_switch_dpillar(long i)
{
	return i-N_sr;
}

tuple_t connection_dpillar(long node, long _port)
{
	long i, col_num;
	tuple_t res;

	if(node<N_sr) {
		res.node=0;
		res.port=0;
	} else {                // if the node is a switch
		col_num = phys_address[node][col-1];
		res.node = node-N_sr;
		for(i=col_num; i<col; i++)
			res.node += - phys_address[node][i]*(long)pow(port/2.0, i)
			            + phys_address[node][i]*(long)pow(port/2.0, i+1);

		if(_port<port/2)
			res.node +=  _port*(long)pow(port/2.0, col_num);
		else
			res.node = res.node + (_port-port/2)*(long)pow(port/2.0, col_num) - (col_num - (col_num+1)%col )*(long)pow(port/2.0, col);

		if(_port<port/2)
			res.port = 0;
		else
			res.port = 1;
	}
	/// This is an ugly hack, should be fixed.
	network[res.node].port[res.port].neighbour.node=node;
	network[res.node].port[res.port].neighbour.port=_port;

	return res;
}

long route_dpillar(long current, long dst)
{
	return(path[path_index++]);	// return next port
}

void finish_route_dpillar()
{
	path_index = 0;
}

char* get_network_token_dpillar()
{
	return network_token;
}

char* get_routing_token_dpillar()
{
	switch(routing) {

	case DPILLAR_SINGLE_PATH_ROUTING:
		routing_token = "sp";
		break;

	case DPILLAR_RANDOM_DIRECTION_SP:
		routing_token = "sp-rnd";
		break;

	case DPILLAR_SHORTER_DIRECTION_SP:
		routing_token = "sp-shd";
		break;

	case DPILLAR_MULTI_PATH_ROUTING:
		routing_token = "mp";
		break;

	case DPILLAR_RANDOM_DIRECTION_MP:
		routing_token = "mp-rnd";
		break;

	case DPILLAR_SHORTER_DIRECTION_MP:
		routing_token = "mp-shd";
		break;

	case DPILLAR_MINIMAL_ROUTING:
		routing_token = "min";
		break;

	default:
		printf("Error: Unknown routing algorithm for DPillar\n");
		exit(-1);
		break;
	}
	return routing_token;
}

char* get_topo_version_dpillar()
{
	return topo_version;
}

char* get_topo_param_tokens_dpillar(long i)
{
	return topo_param_tokens[i];
}

char* get_filename_params_dpillar()
{
	return filename_params;
}

long forward_packet(long crn, long dst, char move_type, long hop)
{
	long i, crn_col, nxt_col, swp_index, crn_swp, dst_swp;

	for(i=0; i<hop; i++) {
		crn_col = phys_address[crn][col];

		if(move_type=='c')	nxt_col = (crn_col+1+col)%col;
		else if(move_type=='a')	nxt_col = (crn_col-1+col)%col;
		else			nxt_col = crn_col;

		if(move_type=='c' || move_type=='b')	swp_index = crn_col;
		else					swp_index = (crn_col-1+col)%col;

		crn_swp = phys_address[crn][swp_index];
		dst_swp = phys_address[dst][swp_index];

		long nxt_sr = crn + (nxt_col-crn_col)*(long)pow(port/2.0, col)
		              + (dst_swp-crn_swp)*(long)pow(port/2.0, swp_index);

		long nxt_sw_port = phys_address[nxt_sr][swp_index];
		if(move_type=='c' || move_type=='d')
			nxt_sw_port += port/2;

		if(move_type=='c' || move_type=='b')
			all_path[ver_index][hor_index++] = 0;
		else
			all_path[ver_index][hor_index++] = 1;

		all_path[ver_index][hor_index++] = nxt_sw_port;
		crn = nxt_sr;
		all_path[ver_index][0]=hor_index-1;
	}
	return crn;
}

long DPillarSP(long src, long dst, long dir)
{
	long hop=0;

	if(src != dst) {
		long crn=src;
		while(crn!=dst) {
			if(network[crn].port[(1-dir)/2].faulty) { // output channel is faulty
				path_index = 0;
				path[path_index]=-1;
				return -1;
			}
			long crn_col = phys_address[crn][col];
			long nxt_col = (crn_col+dir+col)%col;
			long swp_index = (dir==1)? crn_col : nxt_col;
			long crn_swp = phys_address[crn][swp_index];
			long dst_swp = phys_address[dst][swp_index];

			long nxt_sr = crn + (nxt_col-crn_col)*(long)pow(port/2.0, col)
			              + (dst_swp-crn_swp)*(long)pow(port/2.0, swp_index);
			long nxt_sr_port=(1-dir)/2; // dir=1: port 0 (cw), dir=-1: port 1 (ccw)

			if(network[nxt_sr].port[1-nxt_sr_port].faulty) {		// input channel of the next node is faulty
				path_index = 0;
				path[path_index]=-1;
				return -1;
			}

			long nxt_sw_port = phys_address[nxt_sr][swp_index];
			if(dir==1)
				nxt_sw_port += port/2;

			path[path_index++] = (1-dir)/2;
			path[path_index++] = nxt_sw_port;

			hop++;
			crn = nxt_sr;

			if ( turn_back==1 && (labs(dst-crn))%(long)pow(port/2.0, col)==0 ) {
				long hop_cw = (phys_address[dst][col]-phys_address[crn][col]+col)%col;
				long hop_ccw= (phys_address[crn][col]-phys_address[dst][col]+col)%col;

				if(hop_cw < hop_ccw)
					dir=1;
				else if(hop_ccw < hop_cw)
					dir=-1;
			}
		}		// while(crn!=dst) {
	}
	return hop;
}

long DPillarMP (long src, long dst, long dir)
{
	long i, j, hop, src_nbr_sw, dst_nbr_sw;
	long col_num, index_1, index_2;
	long *src_nbr, *dst_nbr;

	if (src==dst)
		return 0;
	else if(network[src].port[(1-dir)/2].faulty || network[dst].port[(1+dir)/2].faulty) {
		path_index = 0;
		path[path_index]=-1;
		return -1;
	}

	else {
		src_nbr=(long*)malloc((port/2)*sizeof(long));
		dst_nbr=(long*)malloc((port/2)*sizeof(long));

		src_nbr_sw = network[src].port[(1-dir)/2].neighbour.node;
		for(i=0; i<port/2; i++) {
			if(dir==1)	src_nbr[i] = network[src_nbr_sw].port[i+port/2].neighbour.node;
			else		src_nbr[i] = network[src_nbr_sw].port[    i   ].neighbour.node;
		}
		if(dir==1)	col_num = phys_address[   src    ][col];
		else		col_num = phys_address[src_nbr[0]][col];

		long s_prime = src_nbr[0] - phys_address[src_nbr[0]][col_num]*(long)pow(port/2.0, col_num)
		               + phys_address[    dst   ][col_num]*(long)pow(port/2.0, col_num);

		i=0;
		while(src_nbr[i] != s_prime)
			i++;
		for(j=i; j>0; j--)
			src_nbr[j] = src_nbr[j-1];
		src_nbr[0] = s_prime;

		dst_nbr_sw = network[dst].port[(1+dir)/2].neighbour.node;
		for(i=0; i<port/2; i++) {
			if(dir==1)	dst_nbr[i] = network[dst_nbr_sw].port[    i   ].neighbour.node;
			else		dst_nbr[i] = network[dst_nbr_sw].port[i+port/2].neighbour.node;
		}

		if(dir==1)		col_num = phys_address[dst_nbr[0]][col];
		else			col_num = phys_address[   dst    ][col];

		long d_prime = dst_nbr[0] - phys_address[dst_nbr[0]][col_num]*(long)pow(port/2.0, col_num)
		               + phys_address[    src   ][col_num]*(long)pow(port/2.0, col_num);

		i=0;
		while(dst_nbr[i] != d_prime)
			i++;
		for(j=i; j>0; j--)
			dst_nbr[j] = dst_nbr[j-1];
		dst_nbr[0] = d_prime;

		index_1=-1;
		do {
			index_1++;
			path_index = 2;
			hop = DPillarSP(src_nbr[index_1], dst_nbr[index_1], dir);
		} while(hop == -1 && index_1<port/2-1);
	}
	if(hop!=-1) {
		path[path_index++] = (1-dir)/2;
		if(dir==1)		index_2=port/2;
		else			index_2=0;

		while(network[dst_nbr_sw].port[index_2].neighbour.node != dst)
			index_2++;
		path[path_index++] = index_2;

		path[0] = (1-dir)/2;
		if(dir==1)		index_2=port/2;
		else			index_2=0;
		while(network[src_nbr_sw].port[index_2].neighbour.node != src_nbr[index_1])
			index_2++;
		path[1] = index_2;
	}

	free(src_nbr);
	free(dst_nbr);

	return (hop<0 ? hop : hop+2);
}

long minimal_routing(long src, long dst)
{
	long i, min_hop=1000, min_hop_index[10];
	long crn, L=0, r=0, s=0, DELTA_0=0, DELTA_x=0;
	long src_col = phys_address[src][col];
	long dst_col = phys_address[dst][col];
	long col_off = (dst_col-src_col+col)%col;

	for(i=0; i<col; i++) {
		if(phys_address[src][i]==phys_address[dst][i])
			B[i] = 0;
		else
			B[i] = 1;
	}

	if(B[src_col]==1)	DELTA_0 = 1;
	if(B[dst_col]==1)	DELTA_x = 1;

	for(i=1; i<=(src_col-dst_col+col-1)%col; i++) {
		long j = (dst_col+i)%col;
		if(B[j]==1) {
			r++;
			I[r]=j;
		}
	}

	for(i=1; i<=(dst_col-src_col+col-1)%col; i++) {
		long j = (dst_col-i+col)%col;
		if(B[j]==1) {
			s++;
			J[s]=j;
		}
	}

	if(dst != src) {
		if(dst_col != src_col) {		// src and dst are not in the same column
			ver_index = 0;
			hor_index = 1;
			crn = forward_packet(src, dst, 'c', col+col_off);
			if(crn != dst)
				printf("ERROR!\n");

			ver_index++;
			hor_index = 1;
			crn = forward_packet(src, dst, 'a', 2*col-col_off);
			if(crn != dst)
				printf("ERROR!\n");

			if(r==0) {						// case (a)
				if(DELTA_x == 0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'c', col_off);
					if(crn != dst)
						printf("ERROR! a1\n");
				} else {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'c', col_off);
					crn = forward_packet(crn, dst, 'b', 1);
					if(crn != dst)
						printf("ERROR! a2\n");
				}
			}

			if(s==0) {						// case (b)
				if(DELTA_0 == 0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-col_off);
					if(crn != dst)
						printf("ERROR! b1\n");
				} else {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', col-col_off);
					if(crn != dst)
						printf("ERROR b2\n");
				}
			}

			if(r==1) {					// case (c)
				ver_index++;
				hor_index = 1;
				crn = forward_packet(src, dst, 'c', (I[1]-src_col+col)%col);
				crn = forward_packet(crn, dst, 'b', 1);
				crn = forward_packet(crn, dst, 'a', (I[1]-dst_col+col)%col);
				if(crn != dst)
					printf("ERROR! c1\n");

				if(DELTA_x == 0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(I[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[1]-dst_col+col)%col);
					if(crn != dst)
						printf("ERROR! c2\n");
				} else {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(I[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[1]-dst_col+col)%col);
					crn = forward_packet(crn, dst, 'b', 1);
					if(crn != dst)
						printf("ERROR! c3\n");
				}
			}

			if(s==1) {					// case (d)
				if(DELTA_0 == 0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(J[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', (dst_col-J[1]+col)%col-1);
					if(crn != dst)
						printf("ERROR! d1\n");
				} else {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', col-1-(J[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', (dst_col-J[1]+col)%col-1);
					if(crn != dst)
						printf("ERROR! d2\n");
				}

				ver_index++;
				hor_index = 1;
				crn = forward_packet(src, dst, 'c', (J[1]-src_col+col)%col);
				crn = forward_packet(crn, dst, 'b', 1);
				crn = forward_packet(crn, dst, 'a', col-(dst_col-J[1]+col)%col);
				if(crn != dst)
					printf("ERROR! d3\n");
			}
			if(r>1) {						// case (e)
				for(i=1; i<r; i++) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(I[i+1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[i+1]-I[i]+col)%col);
					crn = forward_packet(crn, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', (I[i]-dst_col+col)%col);
					if(crn != dst)
						printf("ERROR! e1\n");
				}

				ver_index++;
				hor_index = 1;
				crn = forward_packet(src, dst, 'c', (I[r]-src_col+col)%col);
				crn = forward_packet(crn, dst, 'b', 1);
				crn = forward_packet(crn, dst, 'a', (I[r]-dst_col+col)%col);
				if(crn != dst)
					printf("ERROR! e2\n");

				if(DELTA_x==0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(I[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[1]-dst_col+col)%col);
					if(crn != dst)
						printf("ERROR! e3\n");
				} else {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(I[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[1]-dst_col+col)%col);
					crn = forward_packet(crn, dst, 'b', 1);
					if(crn != dst)
						printf("ERROR! e4\n");
				}
			}

			if(s>1) {						// case (f)
				for(i=1; i<s; i++) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'c', (J[i+1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', col-1-(J[i]-J[i+1]+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', (dst_col-J[i]+col)%col-1);
					if(crn != dst)
						printf("ERROR! f1\n");
				}

				ver_index++;
				hor_index = 1;
				crn = forward_packet(src, dst, 'c', (J[1]-src_col+col)%col);
				crn = forward_packet(crn, dst, 'b', 1);
				crn = forward_packet(crn, dst, 'a', col-(dst_col-J[1]+col)%col);
				if(crn != dst)
					printf("ERROR! f2\n");

				if(DELTA_0==0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(J[s]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', (dst_col-J[s]+col)%col-1);
					if(crn != dst)
						printf("ERROR! f3\n");
				} else {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', col-1-(J[s]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', (dst_col-J[s]+col)%col-1);
					if(crn != dst)
						printf("ERROR! f4\n");
				}
			}
		} else { 				// src and dst are in the same column
			ver_index = 0;
			hor_index = 1;
			crn = forward_packet(src, dst, 'c', col);
			if(crn != dst)
				printf("ERROR!\n");

			if(r==0) {						// case (a)
				ver_index++;
				hor_index = 1;
				crn = forward_packet(src, dst, 'b', 1);
				if(crn != dst)
					printf("ERROR! g1\n");
			} else if(r==1) {					// case (b)
				if((I[1]-src_col+col)%col==col-1 && DELTA_0==0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'd', 1);
					if(crn != dst)
						printf("ERROR! h1\n");
				}

				if((I[1]-src_col+col)%col==col-1 && DELTA_0==1) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'd', 1);
					if(crn != dst)
						printf("ERROR! h2\n");
				}

				if((I[1]-src_col+col)%col==1 && (I[1]-src_col+col)%col!=col-1) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'c', 1);
					crn = forward_packet(crn, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', 1);
					if(crn != dst)
						printf("ERROR! h3\n");
				}

				if((I[1]-src_col+col)%col!=1 && (I[1]-src_col+col)%col!=col-1) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'c', (I[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', (I[1]-src_col+col)%col);
					if(crn != dst)
						printf("ERROR! h4\n");

					if(DELTA_0==0) {
						ver_index++;
						hor_index = 1;
						crn = forward_packet(src, dst, 'a', col-1-(I[1]-src_col+col)%col);
						crn = forward_packet(crn, dst, 'd', 1);
						crn = forward_packet(crn, dst, 'c', col-1-(I[1]-src_col+col)%col);
						if(crn != dst)
							printf("ERROR! h5\n");
					} else {
						ver_index++;
						hor_index = 1;
						crn = forward_packet(src, dst, 'b', 1);
						crn = forward_packet(crn, dst, 'a', col-1-(I[1]-src_col+col)%col);
						crn = forward_packet(crn, dst, 'd', 1);
						crn = forward_packet(crn, dst, 'c', col-1-(I[1]-src_col+col)%col);
						if(crn != dst)
							printf("ERROR! h6\n");
					}
				}
			}
			else {						// case (C)
				for(i=1; i<r; i++) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(I[i+1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[i+1]-I[i]+col)%col);
					crn = forward_packet(crn, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', (I[i]-src_col+col)%col);
					if(crn != dst)
						printf("ERROR! i1\n");
				}

				ver_index++;
				hor_index = 1;
				crn = forward_packet(src, dst, 'c', (I[r]-src_col+col)%col);
				crn = forward_packet(crn, dst, 'b', 1);
				crn = forward_packet(crn, dst, 'a', (I[r]-src_col+col)%col);
				if(crn != dst)
					printf("ERROR! i2\n");

				if(DELTA_0==0) {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'a', col-1-(I[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[1]-src_col+col)%col);
					if(crn != dst)
						printf("ERROR! i3\n");
				} else {
					ver_index++;
					hor_index = 1;
					crn = forward_packet(src, dst, 'b', 1);
					crn = forward_packet(crn, dst, 'a', col-1-(I[1]-src_col+col)%col);
					crn = forward_packet(crn, dst, 'd', 1);
					crn = forward_packet(crn, dst, 'c', col-1-(I[1]-src_col+col)%col);
					if(crn != dst)
						printf("ERROR! i4\n");
				}
			}
		}
	}

	for(i=0; i<=ver_index; i++)
		if(all_path[i][0]<min_hop)
			min_hop = all_path[i][0];

	long j=0;
	for(i=0; i<=ver_index; i++) {
		if(all_path[i][0]==min_hop) {
			min_hop_index[j] = i;
			j++;
		}
	}

	long k=min_hop_index[0];
//  long k=min_hop_index[rand()%j];

	for(i=1; i<=min_hop; i++)
		path[i-1]=all_path[k][i];

//  min_route_hist[j]++;
//  if(src==N_sr-1 && dst==N_sr-1)
	//  for(i=0; i<10; i++)
	//  printf("%d  %d\n", i, min_route_hist[i]);

	return L;
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_dpillar(long src, long dst){

    return(1);
}

long init_routing_dpillar(long src, long dst)
{
	long hop=-1, hop_cw, hop_ccw;

	path_index = 0;

	switch(routing) {

	case DPILLAR_SINGLE_PATH_ROUTING:
		hop = DPillarSP(src, dst, 1);
		break;

	case DPILLAR_RANDOM_DIRECTION_SP:
		if(rand()%2==0) {
			hop = DPillarSP(src, dst, 1);
			if(hop==-1) {
				path_index = 0;	// reset for using it in route_dpillar()
				hop = DPillarSP(src, dst, -1);
			}
		} else {
			hop = DPillarSP(src, dst, -1);
			if(hop==-1) {
				path_index = 0;	// reset for using it in route_dpillar()
				hop = DPillarSP(src, dst, 1);
			}
		}
		break;

	case DPILLAR_SHORTER_DIRECTION_SP:
		hop_cw = (phys_address[dst][col]-phys_address[src][col]+col)%col;
		hop_ccw= (phys_address[src][col]-phys_address[dst][col]+col)%col;

		if(hop_cw<hop_ccw) {
			hop = DPillarSP(src, dst, 1);
			if(hop==-1)
				hop = DPillarSP(src, dst, -1);
		} else if (hop_ccw<hop_cw) {
			hop = DPillarSP(src, dst, -1);
			if(hop==-1)
				hop = DPillarSP(src, dst, 1);
		} else {
			if(rand()%2==0) {
				hop = DPillarSP(src, dst, 1);
				if(hop==-1)
					hop = DPillarSP(src, dst, -1);
			} else {
				hop = DPillarSP(src, dst, -1);
				if(hop==-1)
					hop = DPillarSP(src, dst, 1);
			}
		}
		break;

	case DPILLAR_MULTI_PATH_ROUTING:
		hop = DPillarMP(src, dst, 1);
		break;

	case DPILLAR_RANDOM_DIRECTION_MP:
		if(rand()%2==0) {
			hop = DPillarMP(src, dst, 1);
			if(hop==-1)
				hop = DPillarMP(src, dst, -1);
		} else {
			hop = DPillarMP(src, dst, -1);
			if(hop==-1)
				hop = DPillarMP(src, dst, 1);
		}
		break;

	case DPILLAR_SHORTER_DIRECTION_MP:
		hop_cw = (phys_address[dst][col]-phys_address[src][col]+col)%col;
		hop_ccw= (phys_address[src][col]-phys_address[dst][col]+col)%col;

		if(hop_cw<hop_ccw) {
			hop = DPillarMP(src, dst, 1);
			if(hop==-1)
				hop = DPillarMP(src, dst, -1);
		}

		else if (hop_ccw<hop_cw) {
			hop = DPillarMP(src, dst, -1);
			if(hop==-1)
				hop = DPillarMP(src, dst, 1);
		} else {
			if(rand()%2==0) {
				hop = DPillarMP(src, dst, 1);
				if(hop==-1)
					hop = DPillarMP(src, dst, -1);
			} else {
				hop = DPillarMP(src, dst, -1);
				if(hop==-1)
					hop = DPillarMP(src, dst, 1);
			}
		}
		break;

	case DPILLAR_MINIMAL_ROUTING:
		hop = minimal_routing(src, dst);
		break;

	default:
		printf("Error: Unknown routing algorithm for DPillar\n");
		exit(-1);
		break;
	}

	/*
	  if(parameter == 1)	// single-path routing (SP)
	    hop = DPillarSP(src, dst, 1);

	  if(parameter == 2) {	// random direction SP (SP_RND)
	  }

	  else if(parameter == 3) { // shorter direction SP (SP_SHD)
	  }

	  else if (parameter == 4)
	    hop = DPillarMP(src, dst, 1);


	  else if(parameter == 5) {		// random direction MP (MP_RND)
	  }

	  else if(parameter == 6) {		// shorter direction MP (MP_SHD)
	  }

	  else if(parameter == 7) 		// minimal routing
	*/

	path_index = 0;	// reset for using it in route_dpillar()
	return 0;
}

#endif
