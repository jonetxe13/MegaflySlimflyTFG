/**
* @file
* @brief	Router management functions.

FSIN Functional Simulator of Interconnection Networks
Copyright (2003-2011) J. Miguel-Alonso, A. Gonzalez, J. Navaridas

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "globals.h"
#include "router.h"
#include "misc.h"

port_type p_inj_first,	///< The number of the first injection port.
		  p_inj_last;	///< The number of the last injection port.
port_type p_con;		///< The number of the consumption port.
port_type p_drop;       ///< The number of the dropping port, for dropping in-transit traffic.

static void port_coords(port_type e, dim *j, way *k, channel *l);

/**
* Initializes all the routers in the network.
*
* Prepares all structures needed for the simulation (routers & all their stuff for
* requesting, arbitring & stating). Event queues & occurred list are initilized here
* if compiled with the TRACE_SUPPORT != 0 .
*/
void router_init(void) {
	long i, j;

	network = alloc(sizeof(router) * NUMNODES);

	for(i = 0; i < NUMNODES; ++i) {

		// In topologies with NICs, these must be initialized with only one transit queue.
		// or define a data structure similar to the router for them.
		if (i<nprocs) { // Injection queues only in processors
			network[i].qi = alloc(sizeof(inj_queue) * ninj);
			for (j=0; j<ninj; j++)
				network[i].qi[j].pos = alloc(sizeof(phit) * inj_ql);
		}
		else {
			network[i].qi=NULL;
			network[i].source=NO_SOURCE;
		}

		network[i].p = alloc(sizeof(port) * (n_ports+1));
		for(j = 0; j < n_ports+1; ++j) {
			network[i].p[j].req = alloc(sizeof(CLOCK_TYPE) * n_ports+1);
			network[i].p[j].histo = alloc(sizeof(CLOCK_TYPE) * (buffer_cap + 1));
			network[i].p[j].faulty = 0;
		}

		network[i].op_i = alloc(sizeof(long) * radix);
		network[i].nbor = alloc(sizeof(long) * radix);
		network[i].nborp = alloc(sizeof(long) * radix);

                // Initializes to NULL_PORT. Required for random topologies. Should not have any effect on the deterministic ones.
                for(j = 0; j < radix; j++){
                        network[i].op_i[j] = NULL_PORT;
		        network[i].nbor[j] = NULL_PORT;
		        network[i].nborp[j] = NULL_PORT;
                }

		if (topo<DIRECT)
			coords(i, &network[i].rcoord[D_X], &network[i].rcoord[D_Y], &network[i].rcoord[D_Z]);
		else if (topo==ICUBE)
			coords_icube(i, &network[i].rcoord[D_X], &network[i].rcoord[D_Y], &network[i].rcoord[D_Z]);
		// Tree coordinates will be written when the topology is created.

		network[i].injecting_port = NULL_PORT;
		network[i].next_port = 0;
#if (PCOUNT!=0)
		network[i].pcount = 0;
#endif
		// Congestion with timeouts.
		network[i].timeout_counter = (CLOCK_TYPE) 0L;
		network[i].timeout_packet = NULL_PACKET;
		network[i].congested = B_FALSE;

		network[i].triggered=0;

#if (TRACE_SUPPORT == 1)
		if (i<nprocs){
			init_event(&network[i].events);
			init_occur(&network[i].occurs);
		}
		else
			network[i].source=NO_SOURCE;
#endif

#if (TRACE_SUPPORT > 1)
		if (i<nprocs){
			init_event(&network[i].events);
			network[i].occurs = alloc(sizeof(event_l) * nprocs);
			init_occur(&network[i].occurs);
		}
		else
			network[i].source=NO_SOURCE;
#endif
	}

	p_con = n_ports - 1;
	p_drop = n_ports;   // For transit dropping
	p_inj_first = radix*nchan;
	p_inj_last = n_ports - 2;

	if (topo<DIRECT){
		port_coord_dim = alloc(sizeof(dim) * p_inj_first);
		port_coord_way = alloc(sizeof(way) * p_inj_first);
		port_coord_channel = alloc(sizeof(channel) * p_inj_first);
		for (j = 0; j < p_inj_first; j++)
			port_coords(j, &port_coord_dim[j], &port_coord_way[j], &port_coord_channel[j]);
	}

	if (topo==ICUBE){
		port_coord_dim = alloc(sizeof(dim) * p_inj_first);
		port_coord_way = alloc(sizeof(way) * p_inj_first);
		port_coord_channel = alloc(sizeof(channel) * p_inj_first);
		for (j = 0; j < p_inj_first; j++){
			port_type a1 = ( j - (stDown*nchan) ) / (links_per_direction);
			port_coord_dim[j] = a1 / ( nways * nchan );
			a1 = a1 % ( nways * nchan );
			port_coord_way[j] = a1 / nchan;
			port_coord_channel[j] = a1 % nchan;
		}
	}

	if (shotmode)
	    for (i=0; i<nprocs; i++)
	        network[i].source=OTHER_SOURCE; // Bursty Source

	/* Allocates space for transit queues */
	for(i = 0; i < NUMNODES; ++i)
		for(j = 0; j < n_ports+1; ++j)
			network[i].p[j].q.pos = alloc(sizeof(phit) * tr_ql);
}

/**
 * Initializes transit and injection ports of a node.
 *
 * All port's structures (queues, arbitration, requesting, ...) are prepared for transit and
 * injection ports of the given router.
 *
 * @param i The node whose ports must be initilized.
 */
void init_ports(long i){
	port_type e;
	long f;

	for (e=0; e<p_con; e++) {
		init_queue(&network[i].p[e].q);
		network[i].p[e].utilization = (CLOCK_TYPE) 0L;
		network[i].p[e].bet = B_TRIAL_0;
		network[i].p[e].aop = P_NULL;
		network[i].p[e].tor = CLOCK_MAX;

		network[i].p[e].sip = P_NULL;
		network[i].p[e].ri = P_NULL;

		if(plevel & 8)
			for (f=0; f<buffer_cap+1; f++)
				network[i].p[e].histo[f] = (CLOCK_TYPE) 0L;
	}
	/* Init consumption port */
	network[i].p[p_con].sip = P_NULL;
	network[i].p[p_con].ri = P_NULL;
	/* Number of pending packets */
	network[i].pending_packet = 0;
}

/**
* Calculates the coordinates of a node (X, Y, Z)
*
* Each node have stored their own coordinates because they are used often.
*
* @param ad Address of the node.
* @param cx Coordinate X is returned here.
* @param cy Coordinate Y is returned here.
* @param cz Coordinate Z is returned here.
* @see router.rcoord
*/
void coords (long ad, long *cx, long *cy, long *cz) {
	long a1;
	*cz = ad/(nodes_x*nodes_y);
	a1 = ad%(nodes_x*nodes_y);
	*cy = a1/nodes_x;
	*cx = a1%nodes_x;
}

/**
* Calculates the coordinates of a node (X, Y, Z) in a icube.
*
* Each node have stored their own coordinates because they are used often.
*
* @param ad Address of the node.
* @param cx Coordinate X is returned here.
* @param cy Coordinate Y is returned here.
* @param cz Coordinate Z is returned here.
* @see router.rcoord
*/
void coords_icube (long ad, long *cx, long *cy, long *cz) {
	long a1;
	if (ad<nprocs){
		*cz = (ad/stDown)/(nodes_x*nodes_y);
		a1 = (ad/stDown)%(nodes_x*nodes_y);
		*cy = a1/nodes_x;
		*cx = a1%nodes_x;
	}
	else {
		*cz = (ad-nprocs)/(nodes_x*nodes_y);
		a1 = (ad-nprocs)%(nodes_x*nodes_y);
		*cy = a1/nodes_x;
		*cx = a1%nodes_x;
	}
}

/**
* Calculates the coordinates of a port.(Dimension, way, VC)
*
* Since port's coordinates are used often, they're stored instead of
* be calculated each time they are required.
*
* @param e A port number.
* @param j The dimension of the port are returned here.
* @param k The direction (way) of the port are returned here.
* @param l The virtual channel id of the port are returned here.
* @see port_coord_dim;
* @see port_coord_way;
* @see port_coord_channel;
*/
static void port_coords(port_type e, dim *j, way *k, channel *l) {
	port_type a1;

	if ((e>=p_inj_first) || (e<0))
		panic("Bad port number");
	*j = e/(nways*nchan);
	a1 = e%(nways*nchan);
	*k = a1/nchan;
	*l = a1%nchan;
}

/**
* Initializes the network.
*
* Gives the initial values to the simulation depending on the topology.
*/
void init_network(void) {
	long i;
	dim j; way k;	// for mesh-like topologies
	long n,p;	// router and port.
	long nr, np;	// neighbor router and port.

	if (topo < DIRECT){
		for (i=0; i<NUMNODES; i++) {
			for (j=0; j<ninj; j++) // Init injection queues
				inj_init_queue(&network[i].qi[j]);
			init_ports(i); // Init transit and injection ports
			for (j=0; j<ndim; j++) {
				for (k=0; k<nways; k++) {
					network[i].nbor[dir(j,k)] = neighbor(i, j, k);
					network[i].nborp[dir(j,k)] = dir(j,k);
					network[i].op_i[dir(j,k)] = ESCAPE;
				}
			}
		}
	}
	else if (topo==FATTREE)
		create_fattree();
	else if (topo==THINTREE)
		create_thintree();
	else if (topo==SLIMTREE)
		create_slimtree();
	else if (topo==ICUBE)
		create_icube();
        else if (topo == RRG || topo == EXA || topo == GDBG || topo == KAUTZ)
		create_graph();
	else if (topo==DRAGONFLY_ABSOLUTE || topo==DRAGONFLY_RELATIVE || topo==DRAGONFLY_RND || topo==DRAGONFLY_CIRCULANT || topo==DRAGONFLY_HELIX || topo==DRAGONFLY_NAUTILUS || topo==DRAGONFLY_OTHER)
		create_dragonfly();
	else if (topo==MEGAFLY)
		create_megafly();
	else if (topo==SLIMFLY)
		create_slimfly();

	for(i=0; i<faults;i++){
		do{
			n=rand()%NUMNODES;
			p=rand()%radix;
		}while (network[n].p[p].faulty!=0);
		nr=network[n].nbor[p];
		if (p%2)
			np=p-1;
		else
			np=p+1;
		printf("breaking link %ld.%ld->%ld.%ld\n",n,p,nr,np);
		network[n].p[p].faulty=1;
		//network[nr].p[np].faulty=1; // Broken link means two direction malfunction.
	}

	fflush(stdout);
}

void router_finish(){

    long i, j;


	for(i = 0; i < NUMNODES; ++i) {

		// In topologies with NICs, these must be initialized with only one transit queue.
		// or define a data structure similar to the router for them.
		if (i<nprocs) { // Injection queues only in processors
			for (j=0; j<ninj; j++)
                            free(network[i].qi[j].pos);
                        free(network[i].qi);
		}

		for(j = 0; j < n_ports+1; ++j) {
                    free(network[i].p[j].req);
                    free(network[i].p[j].histo);
                    free(network[i].p[j].q.pos);
		}
                free(network[i].p);

                free(network[i].op_i);
                free(network[i].nbor);
                free(network[i].nborp);


#if (TRACE_SUPPORT > 1)
		if (i<nprocs){
                        free(network[i].occurs);
		}
#endif
	}

	if (topo<DIRECT){
            free(port_coord_dim);
            free(port_coord_way);
            free(port_coord_channel);
	}

	if (topo==ICUBE){
            free(port_coord_dim);
            free(port_coord_way);
            free(port_coord_channel);
	}

        free(network);

}

void finish_network(){

    if (topo == RRG || topo == EXA || topo == GDBG || topo == KAUTZ)
        finish_graph();
    if (topo==DRAGONFLY_ABSOLUTE || topo==DRAGONFLY_RELATIVE || topo==DRAGONFLY_RND || topo==DRAGONFLY_CIRCULANT || topo==DRAGONFLY_HELIX || topo==DRAGONFLY_NAUTILUS || topo==DRAGONFLY_OTHER){
        finish_dragonfly();
    }
}
