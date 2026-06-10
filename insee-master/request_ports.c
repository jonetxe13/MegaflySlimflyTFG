/**
 * @file
 * @brief	All request port functions & tools.

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

#include <math.h>

#include "globals.h"

// Some declarations.
static bool_t check_rr_fully(packet_t * pkt);
static bool_t check_restrictions (long i, port_type s_p, port_type d_p, bool_t chkbub);
static void extract_packet (long i, port_type injector);
static bool_t preliminary_check(long i, port_type s_p, bool_t fully_check);

static queue *q;			///< An auxiliary queue that simplifies the code.
static phit *ph;			///< An auxiliary phit.
static dim d_d;				///< Destination dim.
static way d_w;				///< Destination way.
static port_type d_p;		///< Id of destination port.
static port_type curr_p;	///< Id of the current port.
static long id;				///< The id of the switching element.

static bool_t * mt;			///< A Matrix indicating all profitable directions/ways.
static bool_t * candidates;	///< An array containing all profitable output ports for a given input.

/**
 * Prepare arrays mt and candidates.
 *
 * mt[ dim * nways | way ] stores a list of boolean values, indicating all profitable directions
 * for a given input. When bidirectional links this means that the array is like the next:
 * [ X+ X- Y+ Y- .... ] in mesh-like topologies.
 * [ 0^ 0v 1^ 1v .... ] in tree-like topologies.
 *
 * candidates extends that list, and contains all profitable output ports for a given input.
 *
 * @see mt.
 * @see candidates.
 */
void request_ports_init(void) {
    if (topo<DIRECT){
        mt = alloc(sizeof(bool_t) * nways * ndim);
        candidates = alloc(sizeof(bool_t) * n_ports);
    }
}

void request_ports_finish(void) {
    if (topo<DIRECT){
        free(mt);
        free(candidates);
    }
}

/**
 * Requests an output port using bubble double oblivious routing.
 *
 * We have ndim escape channels. Each one using DOR routing in different order
 * of dimensions. For example for ndim=2 XY & YX.
 * We decide de channel when we inject & we cannot change it.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_double_oblivious(long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c;  // Destination channel
    dim j;		//
    way k;		// coords of port s_p making request
    dim ji;

    if (!preliminary_check (i, s_p, B_TRUE))
        return;
    // Will use array mt -- instead of d_d, d_w

    if (s_p < p_inj_first)
        d_c = port_coord_channel[s_p];
    else
        // This is a first attempt of injection. We need to choose a VC at random
        d_c = (channel)(1.0*nchan*rand()/(RAND_MAX+1.0));

    j = d_c;
    for (ji = 0; ji < nchan; ji++) {
        j = (d_c+ji)%ndim;
        for (k = UP; k < nways; k++) {
            if (mt[dir(j,k)]) {
                // This is a good way to advance!!
                d_p = port_address(dir(j,k), d_c);
                if (!check_restrictions(i, s_p, d_p, B_TRUE)) {
                    if (extract && s_p >= p_inj_first)
                        extract_packet(i, s_p);
                    return;
                }
                else {
                    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
                    return;
                }
            }
        }
    }
    panic("Request_port_bubble_doubleob: should not be here");
}

/**
 * Requests an output port using bubble double adaptive routing.
 *
 * Routes equal to bubble double but we are able to change our channel
 * when the bubble restriction allows us.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_double_adaptive (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c;  // Destination channel
    dim j; way k; // coords of port s_p making request
    long ji;
    long bets;

    if (!preliminary_check (i, s_p, B_TRUE)) return;
    // Will use array mt -- instead of d_d, d_w

    if (s_p < p_inj_first)	// in transit traffic try to continue in the same VC.
        d_c = port_coord_channel[s_p];
    else
        // This is a first attempt of injection. We need to choose a VC at random
        d_c = (channel)(1.0*nchan*rand()/(RAND_MAX+1.0));
    bets = 1;

another_attempt:
    for (ji = 0; ji < ndim; ji++) {
        j = (d_c+ji)%ndim;
        for (k = UP; k < nways; k++) {
            if (mt[dir(j,k)]) {
                // This is a good way to advance!!
                d_p = port_address(dir(j,k), d_c);
                if (!check_restrictions(i, s_p, d_p, B_TRUE)) {
                    // we can try another VC -- THE DIRTY WAY!
                    if (bets == nchan) {
                        // No more vchans to try
                        if (extract && s_p >= p_inj_first)
                            extract_packet(i, s_p);
                        return;
                    }
                    else {
                        bets++;
                        d_c = (d_c + 1) % nchan;
                        goto another_attempt;
                    }
                }
                else {
                    // Make reservation
                    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
                    return;
                }
            }
        }
    }
    panic("Request_port_bubble_doubleadap: should not be here");
}

/**
 * Requests an output port using bubble hexa??? oblivious routing.
 *
 * Equal to bubble double routing but with all the possible dimension orders XY XZ YX YZ ZX ZY.
 * The packets aren't allowed to move to another channel.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_hexa_oblivious(long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c;  // Destination channel
    dim j, ji;
    way k;		// coords of port s_p making request

    if (nchan != 6 || ndim != 3)
        panic("Bubble_hexa_oblivious only for 3-D and 6 VC");

    if (!preliminary_check (i, s_p, B_TRUE))
        return;
    // Will use array mt -- instead of d_d, d_w

    if (s_p < p_inj_first){
        j = port_coord_dim[s_p];
        k = port_coord_way[s_p];
        d_c = port_coord_channel[s_p];
    }
    else{
        // This is a first attempt of injection. We need to choose a VC at random
        j = INJ;
        d_c = (channel)(1.0*nchan*rand()/(RAND_MAX+1.0));
    }

    for (ji = 0; ji < ndim; ji++){
        if (d_c < ndim) // d_c < 3
            j = (d_c + ji) % ndim; // canales 0, 1 y 2 enrutan en orden alfab�ico (XYZ) (YZX) (ZXY)
        else // d_c > 2
            j = (d_c - ji) % ndim; // canales 3, 4 y 5 enrutan en orden a. inverso (XZY) (YXZ) (ZYX)

        for (k = UP; k < nways; k++){
            if (mt[dir(j,k)]){
                // This is a good way to advance!!
                d_p = port_address(dir(j,k), d_c);
                if (!check_restrictions(i, s_p, d_p, B_TRUE)){
                    if (extract && s_p >= p_inj_first)
                        extract_packet(i, s_p);
                    return;
                }
                else{
                    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
                    return;
                }
            }
        }
    }
    panic("Request_port_hexa_oblivious: should not be here");
}

/**
 * Requests an output port using bubble hexa??? adaptive routing.
 *
 * Equal to bubble hexa oblivious routing but packets can move to another channel when matchs
 * bubble restriction.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_hexa_adaptive(long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c;  // Destination channel
    dim j, ji;
    way k;
    channel l; // coords of port s_p making request
    long bets;

    if (nchan != 6 || ndim != 3)
        panic("Bubble hexa? adaptive only for 3-D and 6 VC");

    if (!preliminary_check (i, s_p, B_TRUE))
        return;
    // Will use array mt -- instead of d_d, d_w

    if (s_p < p_inj_first){
        j = port_coord_dim[s_p];
        k = port_coord_way[s_p];
        l = port_coord_channel[s_p];
    }
    else
        j = INJ;

    if (j != INJ)
        d_c = l;
    else // This is a first attempt of injection. We need to choose a VC at random
        d_c = (channel)(1.0*nchan*rand()/(RAND_MAX+1.0));


    for(bets=0; bets<nchan; bets++){
        j = d_c;
        for (ji = 0; ji < ndim; ji++){
            if (d_c < ndim) // d_c < 3
                j = (d_c + ji) % ndim; // canales 0, 1 y 2 enrutan en orden alfab�ico (XYZ) (YZX) (ZXY)
            else // d_c > 2
                j = (d_c - ji) % ndim; // canales 3, 4 y 5 enrutan en orden a. inverso (XZY) (YXZ) (ZYX)

            for (k = UP; k < nways; k++){
                if (mt[dir(j,k)]){
                    // This is a good way to advance!!
                    d_p = port_address(dir(j,k), d_c);
                    if (!check_restrictions(i, s_p, d_p, B_TRUE))
                        d_c = (d_c + 1) % nchan;
                    else{
                        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
                        return;
                    }
                }
            }
        }
    }
    if(extract && s_p >= p_inj_first)
        extract_packet(i, s_p);
}

#if (BIMODAL_SUPPORT != 0)

/**
 * Request port for bimodal traffic.
 *
 * Bubble adaptive random for short messages.
 * Escape channel for long messages.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_bimodal_random (long i, port_type s_p){
    // Let us work with port "s_p" at node "i"
    channel d_c;
    packet_t * pkt;

    q= &(network[i].p[s_p].q);
    if (queue_len(q))
        ph = head_queue(q);
    else
        return;

    pkt=&pkt_space[ph->packet];
    if (pkt->mtype == SHORT_MSG)
        request_port_bubble_adaptive_random(i, s_p);
    else if (pkt->mtype==LONG_MSG ||
            pkt->mtype==LONG_LAST_MSG){
        if (!preliminary_check(i, s_p, B_FALSE))
            return;

        // Long messages must be deliver in order
        d_c = ESCAPE;

        d_p = port_address(dir(d_d, d_w), d_c);
        if (!check_restrictions (i, s_p, d_p, B_TRUE)){
            if (extract && s_p >= p_inj_first)
                extract_packet(i, s_p);
            return;
        }
        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
    }
    else
        panic("Should not be here in request_port_bimodal_random");
}
#endif /* BIMODAL */

/**
 * Requests an output port using oblivious (DOR) routing.
 *
 * It is possible to have several oblivious channels in parallel. A static selection of
 * channel is done at injection time. After that, the same VC is used all the time.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_bubble_oblivious (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c;  // Destination channel
    dim j;
    channel l=-1; // coords of port s_p making request

    if (!preliminary_check (i, s_p, B_FALSE)) return;
    // Won't use array mt -- use d_d and d_w instead

    if (s_p < p_inj_first) {
        j = port_coord_dim[s_p];
        l = port_coord_channel[s_p];
    }
    else j = INJ;

    if (j == d_d)
        d_c = l;
    else
        // This is a first attempt of injection. We need to choose a VC at random
        d_c = (channel)(1.0*nchan*rand()/(RAND_MAX+1.0));

    d_p = port_address(dir(d_d, d_w), d_c);

    if (!check_restrictions (i, s_p, d_p, B_TRUE)) {
        if (extract && s_p >= p_inj_first)
            extract_packet(i, s_p);
        return;
    }
    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

/**
 * Smart Adaptive port request.
 *
 * At injection time, the packet is injected to a random channel.
 * It tries to continue in the same dim/way in an adaptive VC. If not possible, then
 * jump to another, profitable dim, still using adaptive VCs.
 * If no adaptive, profitable dim is found, go to ESCAPE VC.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_bubble_adaptive_smart(long i, port_type s_p) {
    channel d_c; // Destination channel
    dim j;
    way k=-1;
    channel l=-1; // coords of port s_p making request
    bet_type bt;

    if (!preliminary_check(i, s_p, B_TRUE)) return;

    // Packet is in transit
    bt = network[i].p[s_p].bet;
    if (s_p < p_inj_first) {
        j = port_coord_dim[s_p];
        k = port_coord_way[s_p];
        l = port_coord_channel[s_p];
    }
    else
        j = INJ;

    while ((bt>=B_TRIAL_0) && (bt<ndim)) {
        if (s_p >= p_inj_first)
            d_d = bt;			// D_X, D_Y OR D_Z for INJ
        else
            d_d = (j+bt)%ndim;  // (my dim + 1) OR (my dim + 2)
        if (mt[dir(d_d,UP)]) d_w = UP;
        else if ((nways > 1) && (mt[dir(d_d,DOWN)])) d_w = DOWN;
        else {
            if (bt < (ndim-1))
                bt = bt+1;
            else
                bt = B_ESCAPE;
            continue;
        }

        // destination dim d_d and way d_w already selected. Let us select channel
        if ((s_p >= p_inj_first) || (l == ESCAPE))
            // s_p is either a ESCAPE channel or the INJECTION port; select adaptive channel at random
            d_c = 1 + (rand()%(nchan-1)); // Candidate destination adaptive channel selected
        else {
            // s_p is an ADAPTIVE channel
            if ((j == d_d) && (k == d_w))
                // Continue in same adaptive channel
                d_c = l;
            else
                d_c = 1 + (rand()%(nchan-1));
        }
        d_p = port_address(dir(d_d, d_w), d_c);

        // Now let us check that space is we can move towards destination
        //d_n = network[i].nbor[dir(d_d, d_w)];
        if (!check_restrictions(i, s_p, d_p, B_TRUE)) {
            // Nope!! Go on trying...
            if (bt < (ndim-1))
                bt = bt+1;
            else
                bt = B_ESCAPE;
            continue;
        }

        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
        if (bt < (ndim-1))
            network[i].p[s_p].bet = bt+1;
        else
            network[i].p[s_p].bet = B_ESCAPE;
        return;
    }

    // We have tried several adaptive alternatives, now we should try the
    // ESCAPE channel
    if (bt == B_ESCAPE) {
        check_rr(&pkt_space[ph->packet], &d_d, &d_w);
        d_p = port_address(dir(d_d, d_w), ESCAPE);
        network[i].p[s_p].bet = B_TRIAL_0;
        if (!check_restrictions(i, s_p, d_p, B_TRUE)) {
            // Cannot request ESCAPE -- even this is full!!
            if (extract && s_p >= p_inj_first)
                extract_packet(i, s_p);
            return;
        }
        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
        // If not successful, next time we will start the round again
        return;
    }
    panic("No suitable option when reserving");
}

/**
 * Shortest Adaptive port request.
 *
 * If packet cannot continue in the same virtual channel, it tries to go to the adaptive
 * VC with most space in its queue. If no adaptive, profitable dim is found
 * due to a congestion control mechanism, then go to ESCAPE VC.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_bubble_adaptive_shortest (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    dim j; way k; channel l;
    long s_d_p; // space at selected output port's queue
    long c_d_n; // candidate destination node
    port_type c_p; // candidate destination port
    long s_c_p; // space at candidate destination port's queue

    if (!preliminary_check(i, s_p, B_TRUE)) return;

    // Packet is in transit
    s_d_p = -1;
    for (j=D_X; j<ndim; j++) {
        for (k=0; k<nways; k++) {
            if (!mt[dir(j,k)]) continue;
            c_d_n = network[i].nbor[dir(j,k)];
            for (l=1; l<nchan; l++) {
                c_p = port_address(dir(j,k), l);
                if (!check_restrictions(i, s_p, c_p, B_TRUE))
                    continue; // bubble check!!
                s_c_p = queue_space(&(network[c_d_n].p[c_p].q));
                if (s_c_p > s_d_p) {
                    // Port of choice
                    d_p = c_p;
                    s_d_p = s_c_p;
                }
            }
        }
    }
    if (s_d_p != -1) {
        // Let us make the request
        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
        return;
    }

    // At this point, no adaptive channel is available. Let us request escape, just in case
    check_rr(&pkt_space[ph->packet], &d_d, &d_w);
    d_p = port_address(dir(d_d, d_w), ESCAPE);
    if (!check_restrictions(i, s_p, d_p, B_TRUE)) {
        // Cannot request ESCAPE -- even this is full!!
        if (extract && s_p >= p_inj_first)
            extract_packet(i, s_p);
        return;
    }
    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

/**
 * Random Adaptive port request.
 *
 * It tries to continue in the same dim/way in an adaptive VC. If not possible, then jump
 * to another random adaptive VCs. If no adaptive, profitable dim is found, go to ESCAPE VC.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_bubble_adaptive_random (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    dim j; way k; channel l;
    long rp, ncand;

    if (!preliminary_check(i, s_p, B_TRUE))
        return;

    // Packet is in transit
    ncand = 0;
    for (d_p = 0; d_p<p_inj_first; d_p++) {
        j = port_coord_dim[d_p]; k = port_coord_way[d_p]; l = port_coord_channel[d_p];
        if ((l==ESCAPE) || !mt[dir(j,k)]) {
            candidates[d_p] = B_FALSE;
            continue;
        }
        if (!check_restrictions(i, s_p, d_p, B_TRUE)) {
            candidates[d_p] = B_FALSE;
            continue;
        }
        // A candidate that is viable: adaptive, right way, space & bubble available
        ncand++;
        candidates[d_p] = B_TRUE;
    }

    if (!ncand) {
        // At this point, no adaptive channel is available. Let us request escape, just in case
        check_rr(&pkt_space[ph->packet], &d_d, &d_w);
        d_p = port_address(dir(d_d, d_w), ESCAPE);
        if (!check_restrictions(i, s_p, d_p, B_TRUE)) {
            // Cannot request ESCAPE -- even this is full!!
            if (extract && s_p >= p_inj_first)
                extract_packet(i, s_p);
            return;
        }
        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
        return;
    }

    rp = (long)(1.0*ncand*rand()/(RAND_MAX+1.0));
    for (d_p=0; d_p<p_inj_first; d_p++) {
        if (!candidates[d_p])
            continue;
        if (rp == 0) {
            network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
            return;
        }
        else
            rp--;
    }
    panic("Should not be here in request_port_bubble_adaptive_random");
}

/**
 * Request a port using dally trc mechanism.
 *
 * In trc all packets are injected in Escape channel 0.
 * Packets remain in channel 0 until reaching a wrap-around
 * link where they switch to Escape channel 1.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_dally_trc(long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c;  // Destination channel
    dim j; channel l; // coords of port s_p making request
    long a_x, a_y, a_z;

    if (!preliminary_check(i, s_p, B_FALSE))
        return;
    // Won't use array mt -- use d_d and d_w instead

    if (s_p < p_inj_first) {
        j = port_coord_dim[s_p];
        //k = port_coord_way[s_p]; //Dead code
        l = port_coord_channel[s_p];
    }
    else
        j = INJ;

    if ((j == INJ) || (j != d_d))
        d_c = 1; // ONE
    else d_c = l; // "L"

    a_x=network[i].rcoord[D_X];
    a_y=network[i].rcoord[D_Y];
    a_z=network[i].rcoord[D_Z];

    switch (d_d) {  // only possible values are D_X, D_Y, D_Z
        case D_X:
            if (a_x == 0)
                d_c = 0;
            break;
        case D_Y:
            if (a_y == 0)
                d_c = 0;
            break;
        case D_Z:
            if (a_z == 0)
                d_c = 0;
            break;
        default:
            panic("Should not reach this point in request_port_dally_trc");
    }

    d_p = port_address(dir(d_d, d_w), d_c);
    if (!check_restrictions(i, s_p, d_p, B_FALSE)) {
        if (extract && s_p >= p_inj_first)
            extract_packet(i, s_p);
        return;
    }
    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

/**
 * Request a port using basic dally algorithm.
 *
 * In basic those packets that have to cross the wrap-around links of a dimension
 * circulate through Escape channel 0.
 * Those that do not have to use the wrap-around links use the Escape channel 1.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_dally_basic (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c=-1;  // Destination channel
    long a_x, a_y, a_z;
    packet_t * pkt;

    if (!preliminary_check(i, s_p, B_FALSE))
        return;
    // Won't use array mt -- use d_d and d_w instead

    a_x=network[i].rcoord[D_X];
    a_y=network[i].rcoord[D_Y];
    a_z=network[i].rcoord[D_Z];

    pkt=&pkt_space[ph->packet];
    switch (d_d) {   // only possible values are D_X, D_Y, D_Z
        case D_X:
            if ((a_x + pkt->rr.rr[d_d] >= nodes_x) || (a_x + pkt->rr.rr[d_d] < 0))
                d_c = 0;
            else
                d_c = 1;
            break;
        case D_Y:
            if ((a_y + pkt->rr.rr[d_d] >= nodes_y) || (a_y + pkt->rr.rr[d_d] < 0))
                d_c = 0;
            else
                d_c = 1;
            break;
        case D_Z:
            if ((a_z + pkt->rr.rr[d_d] >= nodes_z) || (a_z + pkt->rr.rr[d_d] < 0))
                d_c = 0;
            else
                d_c = 1;
            break;
        default:
            panic("Should not reach this point in request_port_dally_basic");
    }

    d_p = port_address(dir(d_d, d_w), d_c);
    if (!check_restrictions(i, s_p, d_p, B_FALSE)) {
        if (extract && s_p >= p_inj_first)
            extract_packet(i, s_p);
        return;
    }
    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

/**
 * Request a port using an improved dally algorithm.
 *
 * Similar to basic, but packets not using the wrap-around links can use
 * both Escape channels.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_dally_improved(long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    channel d_c;  // Destination channel
    long a_x, a_y, a_z;
    long tmp_dim=-1, passes=-1;

    if (!preliminary_check(i, s_p, B_FALSE))
        return;
    // Won't use array mt -- use d_d and d_w instead

    a_x=network[i].rcoord[D_X];
    a_y=network[i].rcoord[D_Y];
    a_z=network[i].rcoord[D_Z];

    switch (d_d) {
        case D_X:
            tmp_dim = a_x;
            passes = nodes_x;
            break;
        case D_Y:
            tmp_dim = a_y;
            passes = nodes_y;
            break;
        case D_Z:
            tmp_dim = a_z;
            passes = nodes_z;
            break;
        default:
            panic("Should not reach this point in request_port_dally_improved");
    }

    if ((tmp_dim + pkt_space[ph->packet].rr.rr[d_d] < 0) ||
            (tmp_dim + pkt_space[ph->packet].rr.rr[d_d] >= passes))
        d_c = 0;
    else {
        if (rand() >= (RAND_MAX/2))
            d_c = 1;
        else
            d_c = 0;
        d_p = port_address(dir(d_d, d_w), d_c);
        if (!check_restrictions(i, s_p, d_p, B_FALSE)){
            if (d_c == 1)
                d_c = 0;
            else
                d_c = 1;
        }
    }

    d_p = port_address(dir(d_d, d_w), d_c);
    if (!check_restrictions(i, s_p, d_p, B_FALSE)) {
        if (extract && s_p >= p_inj_first)
            extract_packet(i, s_p);
        return;
    }
    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

/**
 * Request a port using the dally algorithm + some VCs.
 *
 * Adaptive port request. Channels 0 and 1 are the Escape channels,
 * following the improved algorithm. Other virtual channels can adapt freely.
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_dally_adaptive (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
    dim j;
    way k;
    channel l;
    long rp, ncand;

    if (!preliminary_check(i, s_p, B_FALSE))
        return;

    // Packet is in transit
    ncand = 0;
    for (d_p = 0; d_p<p_inj_first; d_p++) {
        j = port_coord_dim[d_p];
        k = port_coord_way[d_p];
        l = port_coord_channel[d_p];
        if ((l<=1) || !mt[dir(j,k)]) {
            candidates[d_p] = B_FALSE;
            continue;
        }
        if (!check_restrictions(i, s_p, d_p, B_FALSE)) {
            candidates[d_p] = B_FALSE;
            continue;
        }
        // A candidate that is viable: adaptive, right way, space available
        ncand++;
        candidates[d_p] = B_TRUE;
    }

    if (!ncand) {
        // At this point, no adaptive channel is available. Let us request escape, just in case
        request_port_dally_improved(i, s_p);
        return;
    }

    rp = (long)(1.0*ncand*rand()/(RAND_MAX+1.0));
    for (d_p=0; d_p<p_inj_first; d_p++) {
        if (!candidates[d_p])
            continue;
        if (rp == 0) {
            network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
            return;
        }
        else
            rp--;
    }
    panic("Should not be here in request_port_dally_adaptive");
}

/**
 * Checks if a port can be used.
 *
 * Checks if a request of an output port is required or not. It returns FALSE if the
 * queue is empty, the port is already assigned, or if the packet at the head of the queue
 * has reached its final destination.
 *
 * @param i The node in which the checking is performed.
 * @param s_p The port we are checking.
 * @param fully_check If the direction matrix must be altered this must be TRUE.
 * @return TRUE if the packet can continue in this port, FALSE otherwise.
 * @see mt
 */
bool_t preliminary_check(long i, port_type s_p, bool_t fully_check) {

    q = &(network[i].p[s_p].q); // Local queue
    if (!queue_len(q))
        return B_FALSE; // Nothing to be scheduled
    ph = head_queue(q); // Let us check head of queue...
    if ((ph->pclass != RR) && (ph->pclass != RR_TAIL))
        return B_FALSE; // It is NOT a routing record

    // At this point, we have something to route
    if ((d_p = network[i].p[s_p].aop) != P_NULL) {
        if (network[i].p[d_p].sip != s_p)
            panic("Output port should be reserved for me");
        return B_FALSE; // I've got the port already assigned
    }

    if (network[i].p[s_p].tor == CLOCK_MAX)
        network[i].p[s_p].tor = sim_clock; // Time of first reservation attempt

    if (fully_check){
        if (check_rr_fully(&pkt_space[ph->packet])) {
            network[i].p[p_con].req[s_p] = network[i].p[s_p].tor;
            return B_FALSE;
        }
    } else
        if (check_rr(&pkt_space[ph->packet], &d_d, &d_w)) {
            network[i].p[p_con].req[s_p] = network[i].p[s_p].tor;
            return B_FALSE;
        }
    return B_TRUE;
}

/**
 * Extract the head packet from a transit queue.
 *
 * If a packet is the cause of head-of-line blocking at a transit queue then
 * removes it from the queue's head.
 *
 * @param i The node in which the extraction will be performed.
 * @param injector The injection queue which extract from.
 */
void extract_packet (long i, port_type injector) {
    long pl;
    phit *p;

    network[i].p[injector].tor = CLOCK_MAX; // A new packet will be waiting
    p=head_queue(&(network[i].p[injector].q));
    free_pkt(p->packet);
    for (pl = 0; pl < pkt_len; pl++)
        rem_head_queue(&(network[i].p[injector].q));
}

/**
 * Routes using Dimesion order routing.
 *
 * Checks a routing record and indicates the direction/way of choice, following the
 * dimension order routing algorithm. (order: x+ x- y+ y- z+ z-)
 *
 * @param pkt The packet to route.
 * @param d The desired dimesion is returned here.
 * @param w The elected direction (way) is returned here.
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_dim_o_r(packet_t * pkt, dim *d, way *w) {
    dim j;

    // p.pclass == RR. LET US ANALYZE THE ROUTING RECORD
    // returns TRUE if the phit is to be consumed
    // Checking order: x+ x- y+ y- z+ z-
    for (j=D_X; j<ndim; j++) {
        if (pkt->rr.rr[j] > 0){
            *d = j;
            *w = UP;
            return B_FALSE;
        }
        else if (pkt->rr.rr[j] < 0){
            *d = j;
            *w = DOWN;
            return B_FALSE;
        }
    }
    return B_TRUE;
}

/**
 * Routes using Direction order routing.
 *
 * Checks a routing record and indicates the direction/way of choice, following the
 * direction order routing algorithm. (order: x+ y+ z+ x- y- z-)
 *
 * @param pkt The packet to route.
 * @param d The selected dimesion is returned here.
 * @param w The chosen direction (way) is returned here.
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_dir_o_r(packet_t * pkt, dim *d, way *w) {
    dim j;

    // p.pclass == RR. LET US ANALYZE THE ROUTING RECORD
    // returns TRUE if the phit is to be consumed
    // Checking order: x+ y+ z+ x- y- z-
    for (j=D_X; j<ndim; j++)
        if (pkt->rr.rr[j] > 0){
            *d = j;
            *w = UP;
            return B_FALSE;
        }
    for (j=D_X; j<ndim; j++)
        if (pkt->rr.rr[j] < 0) {
            *d = j;
            *w = DOWN;
            return B_FALSE;
        }
    return B_TRUE;
}

/**
 * Updates mt Matrix using a routing record.
 *
 * @param pkt The packet to route.
 * @return TRUE if the phit has reached its final destination.
 */
bool_t check_rr_fully (packet_t * pkt) {
    dim d_d;
    bool_t res;

    res = B_TRUE;
    for (d_d=D_X; d_d<ndim; d_d++) {
        if (pkt->rr.rr[d_d] > 0) {
            mt[dir(d_d,UP)] = B_TRUE;
            res = B_FALSE;
            if (nways > 1)
                mt[dir(d_d,DOWN)] = B_FALSE;
        }
        else if (pkt->rr.rr[d_d] < 0) {
            mt[dir(d_d,UP)] = B_FALSE;
            if (nways > 1) {
                mt[dir(d_d,DOWN)] = B_TRUE;
                res = B_FALSE;
            }
            else
                panic("Cannot have negative rr when unidirectional");
        }
        else {
            mt[dir(d_d,UP)] = B_FALSE;
            if (nways > 1)
                mt[dir(d_d,DOWN)] = B_FALSE;
        }
    }
    return res;
}

/**
 * Checks VCT and bubble restriction.
 *
 * The virtual cut-through restrictions (room for at least one packet in the destination)
 * are always checked. The bubble restriction is only checked when chkbub is TRUE.
 * It considers these scenarios:
 * (1) If req_mode == OBLIVIOUS_REQ, meaning that several channels in parallel are being used in a oblivious mode.
 *     Then the bubble restriction is applied to all channels.
 * (2) If (bub_adap) then a bubble is required to INJECT into the adaptive channels.
 *
 * @param i The node in which we are checking.
 * @param s_p Source port (The packet is currently here).
 * @param d_p Destination output port (The room is checked here).
 * @param chkbub Must the bubble restriction be checked?
 * @return TRUE if the restrictions are matched.
 */
bool_t check_restrictions (long i, port_type s_p, port_type d_p, bool_t chkbub) {
    dim j; way k; channel l;
    long d_n; // Destination node. THIS node is "i"

    if (d_p == p_con)
        return B_TRUE; // Easy: no restriction when consuming. At any rate, should not be testing this...
    // Let us check VCT restriction
    j = port_coord_dim[d_p];
    k = port_coord_way[d_p];
    l = port_coord_channel[d_p];
    d_n = network[i].nbor[dir(j,k)];

    if (queue_space(&(network[d_n].p[d_p].q)) < pkt_len || network[i].p[d_p].faulty)
        return B_FALSE; // No space at destination / broken link

    if (!chkbub)
        return B_TRUE; // Simple VCT check

    // Let us check additional restrictions
    if ((req_mode == BUBBLE_OBLIVIOUS_REQ) || (req_mode == DOUBLE_OBLIVIOUS_REQ) ||
            (req_mode == DOUBLE_ADAPTIVE_REQ) || (l == ESCAPE)) {
        switch (j) {
            case D_X:
                if (!bub_x)
                    return B_TRUE;
                if ((s_p != d_p) && (queue_space(&(network[i].p[d_p].q)) < (bub_x*pkt_len)))
                    return B_FALSE;
                break;
            case D_Y:
                if (!bub_y)
                    return B_TRUE;
                if ((s_p != d_p) && (queue_space(&(network[i].p[d_p].q)) < (bub_y*pkt_len)))
                    return B_FALSE;
                break;
            case D_Z:
                if (!bub_z)
                    return B_TRUE;
                if ((s_p != d_p) && (queue_space(&(network[i].p[d_p].q)) < (bub_z*pkt_len)))
                    return B_FALSE;
                break;
            default: break;
        }
    }

    // Destination channel is adaptive. VCT allows pass. What about bubble to adaptive (bub_adap)? Only used when injecting
    if (!bub_adap[network[i].congested])
        return B_TRUE;
    if (s_p >= p_inj_first) {
        if (queue_space(&(network[i].p[d_p].q)) < (bub_adap[network[i].congested]*pkt_len))
            return B_FALSE;
    }
    return B_TRUE;
}

/**
 * Checks if a port can be used.
 *
 * Checks if a request of an output port is required or not. It returns FALSE if the
 * queue is empty, the port is already assigned, or if the packet at the head of the queue
 * has reached its final destination.
 *
 * @param i The node in which the checking is performed.
 * @param s_p The port we are checking.
 * @return TRUE if the packet can continue in this port, FALSE otherwise.
 */
bool_t preliminary_check_arbitrary(long i, port_type s_p) {
    q = &(network[i].p[s_p].q);	// Local queue
    if (!queue_len(q))
        return B_FALSE;	// Nothing to be scheduled
    ph = head_queue(q);	// Let us check head of queue...
    if ((ph->pclass != RR) && (ph->pclass != RR_TAIL))
        return B_FALSE;	// It is NOT a routing record

    // At this point, we have something to route
    if ((d_p = network[i].p[s_p].aop) != P_NULL) {
        if (network[i].p[d_p].sip != s_p)
            panic("Output port should be reserved for me");
        return B_FALSE; // I've got the port already assigned
    }

    id=i;	// The id of the local router/switch :: GLOBAL
    if (network[i].p[s_p].tor == CLOCK_MAX)
        network[i].p[s_p].tor = sim_clock; // Time of first reservation attempt

    curr_p=s_p;	//source port :: GLOBAL
    if ( check_rr(&pkt_space[ph->packet], &d_d, &d_w) ){
        network[i].p[p_con].req[s_p] = network[i].p[s_p].tor;
        return B_FALSE;
    }
    return B_TRUE;
}

/**
 * Extract the head packet from a transit queue.
 *
 * If a packet is the cause of head-of-line blocking at a queue then
 * removes it from the queue's head.
 *
 * @param i The node in which the extraction will be performed.
 * @param injector The injection queue which extract from.
 */
void extract_packet_arbitrary (long i, port_type injector) {
    long pl;

    network[i].p[injector].tor = CLOCK_MAX; // A new packet will be waiting
    for (pl = 0; pl < pkt_len; pl++)
        rem_head_queue(&(network[i].p[injector].q));
}

/**
 * Checks VCT restriction.
 *
 * The virtual cut-through restriction (room for at least one packet in the destination)
 * is always checked.
 *
 * @param i The node in which we are checking.
 * @param s_p Source VC port (The packet is currently here).
 * @param d_p Destination VC port (The room is checked here).
 * @return TRUE if the restrictions are matched.
 */
bool_t check_restrictions_arbitrary (long i, port_type s_p, port_type d_p, bool_t check_bubble){
    dim p; channel vc;
    long d_n; // Destination node. THIS node is "i"
    port_type aux;

    if (d_p == p_con)
        return B_TRUE; // Easy: no restriction when consuming. At any rate, should not be testing this...
    // Let us check VCT restriction
    p = d_p/nchan; // physichal port
    vc = d_p%nchan; // VC id
    if ((d_n = network[i].nbor[p]) == NULL_PORT){
        panic("Trying to transmit through a disconnected link");
        return B_FALSE;
    }
    aux=(network[i].nborp[p]*nchan)+vc;
    if (queue_space(&(network[d_n].p[aux].q)) < pkt_len)
        return B_FALSE; // No space at destination

    if (check_bubble)
	if (queue_space(&(network[d_n].p[aux].q)) < (bub_adap[network[i].congested]*pkt_len))
		return B_FALSE; // Bubble restriction not meet

    return B_TRUE;
}

/**
 * Requests an output port in an arbitrary topology (trees, dragonflies, rrgs)
 *
 * @param i The switch in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_arbitrary (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"
	bool_t check;
    //		+-> check packet availability.
    //		+-> writes time of request information.
    //		+-> check if have arrived.
    //		+-> prepares d_d calculating routing record.
    //		|
    if (!preliminary_check_arbitrary (i, s_p))
		return;

    d_p = d_d;

    check = (s_p < stDown * nchan);

    if (!check_restrictions_arbitrary (i, s_p, d_p, check)) {
        if (extract && s_p >= p_inj_first)
            extract_packet_arbitrary(i, s_p);
        return;
    }
    network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

/**
 * Return the port whose neigbor's queue has more free space (more credits).
 * To perform a better load-balancing there is a random selection between all the
 * queues with the same amount of credits
 *
 * @param first The first port to search in.
 * @param last The first port to not search in.
 * @return The selected port.
 */
port_type min_queue_occupation(port_type first, port_type last){
    long min=tr_ql;		                // Minimum value
    long ql;							// Queue length
    port_type p, nbp[last-first], nm=0;	// Neighbor port

    if (first==last)
        return first;
    for (p=first; p<last; p++){
        ql=queue_len(&network[network[id].nbor[p/nchan]].p[network[id].nborp[p/nchan]*nchan+(p%nchan)].q);
        if (ql<min){
            min=ql;
            nm=1;
            nbp[0]=p;
        }
        if (ql==min)
            nbp[nm++]=p;
    }
    return nbp[rand()%nm];
}

/**
 * Routes in a fattree.
 *
 * Adaptive routing in fattrees.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_fattree_adaptive(packet_t * pkt, dim *d, way *w) {
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->rr.size == pkt->n_hops)
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    // in a fattree: stDown == k == stUp;
    if (pkt->n_hops==0)	// NIC
        *d=min_queue_occupation(0, nchan);
    else if (pkt->n_hops < pkt->rr.size /2) //going Up, (adaptive)
        *d=min_queue_occupation(0, stDown*nchan); // We will search in all the Upward links.
    else 	// going down static.
        *d=min_queue_occupation(
                (((pkt->to / (long)pow(stDown, network[id].rcoord[STAGE])) % stDown )+ stDown)*nchan,
                (((pkt->to / (long)pow(stDown, network[id].rcoord[STAGE])) % stDown )+ stDown + 1)*nchan );
    return B_FALSE;
}

/**
 * Routes in a fattree.
 *
 * Static routing in fattrees. Upward path depends on the source.
 * Downward path depends on the destination(as always).
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_fattree_arithmetic(packet_t * pkt, dim *d, way *w) {
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->rr.size == pkt->n_hops)
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    // in a fattree: stDown == k == stUp;
    if (pkt->n_hops==0)	// NIC
        *d=rand()%nchan;
    else if (pkt->n_hops < pkt->rr.size /2) //going Up
        *d=((((pkt->from / (long)pow(stDown, network[id].rcoord[STAGE]))+(curr_p%nchan)) % stDown)*nchan) + (curr_p%nchan) ;
    else	// going down static.
        *d=((((pkt->to / (long)pow(stDown, network[id].rcoord[STAGE])) % stDown )+ stDown)*nchan) + (curr_p%nchan);
    return B_FALSE;
}

/**
 * Routes in a thintree.
 *
 * Adaptive routing in thintrees.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination.
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_thintree_adaptive(packet_t * pkt, dim *d, way *w) {
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->rr.size == pkt->n_hops)
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    if (pkt->n_hops==0)	// NIC
        *d=min_queue_occupation(0, nchan);
    else if (pkt->n_hops < pkt->rr.size /2) //going Up, (adaptive)
        *d=min_queue_occupation(0, stUp*nchan); // We will search in all the Upward links.
    else 	// going down static.
        *d=min_queue_occupation(
                (((pkt->to/(long)pow(stDown, network[id].rcoord[STAGE]))%stDown)+stUp)*nchan,
                (((pkt->to/(long)pow(stDown, network[id].rcoord[STAGE]))%stDown)+stUp+1)*nchan);
    return B_FALSE;
}

/**
 * Routes in a thintree.
 *
 * Static routing in thintrees. Upward path depends on the source.
 * Downward path depends on the destination(as always).
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_thintree_arithmetic(packet_t * pkt, dim *d, way *w) {
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->rr.size == pkt->n_hops)
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    if (pkt->n_hops==0) // NIC
        *d=rand()%nchan;
    else if (pkt->n_hops < pkt->rr.size /2) //going Up, (adaptive)
        *d=((((pkt->to/(long)pow(stDown, network[id].rcoord[STAGE]))+(curr_p%nchan))%stUp)*nchan) + (curr_p%nchan);
    else // going down static.
        *d=((((pkt->to/(long)pow(stDown, network[id].rcoord[STAGE]))%stDown)+stUp)*nchan) +(curr_p%nchan);
    return B_FALSE;
}

/**
 * Routes in a slimtree.
 *
 * Adaptive routing in slimtrees.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_slimtree_adaptive(packet_t * pkt, dim *d, way *w) {
    long n;
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->rr.size == pkt->n_hops)
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    if (pkt->n_hops==0)	// NIC
        *d=min_queue_occupation(0, nchan);
    else if (pkt->n_hops < pkt->rr.size /2) //going Up, (adaptive)
        *d=min_queue_occupation(0, stUp*nchan); // We will search in all the Upward links.
    else if (network[id].rcoord[STAGE]>1) { // Going down (adaptive)
        n=((pkt->to / (stDown*stDown*(long)pow(stDown/stUp,network[id].rcoord[STAGE]-2))) %(stDown/stUp))*stUp;
        *d=min_queue_occupation((stUp+n)*nchan, (stUp+stUp+n)*nchan);
    }
    else if (network[id].rcoord[STAGE]==1)	// To the last switch (static)
        *d=min_queue_occupation(
                (((pkt->to / stDown) % stDown) + stUp)*nchan,
                (((pkt->to / stDown) % stDown) + stUp + 1)*nchan);
    else if (network[id].rcoord[STAGE]==0)		// Last step to the destination (static)
        *d=min_queue_occupation(
                ((pkt->to % stDown) + stUp)*nchan,
                ((pkt->to % stDown) + stUp +1)*nchan);
    return B_FALSE;
}

/**
 * Routes in a slimtree.
 *
 * Static routing in slimtrees. Not implemented yet
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_slimtree_arithmetic(packet_t * pkt, dim *d, way *w) {
    // a hop for each routing record value, so we have arrive to our destination:)
    abort_sim("Not implemented yet");

    /*	if (pkt->rr.size == pkt->n_hops)
        return TRUE;
     *w=0;	// Way has no sense in multistage.

     if (pkt->n_hops==0) // NIC
     *d=0;
     else if (pkt->n_hops < pkt->rr.size /2) //going Up, (adaptive)
     *d=(pkt->to/(long)pow(stDown, network[id].rcoord[STAGE]))%stUp ;
     else if (network[id].rcoord[STAGE]>1) { // Going down (adaptive)
     n=((pkt->to / (stDown*stDown*(long)pow(stDown/stUp,network[id].rcoord[STAGE]-2))) %(stDown/stUp))*stUp;
     *d=min_queue_occupation((stUp+n)*nchan, (stUp+stUp+n)*nchan);
     }
     else if (network[id].rcoord[STAGE]==1) // To the last switch (static)
     *d=min_queue_occupation(
     (((pkt->to / stDown) % stDown) + stUp)*nchan,
     (((pkt->to / stDown) % stDown) + stUp + 1)*nchan);
     else if (network[id].rcoord[STAGE]==0) // Last step to the destination (static)
     *d=min_queue_occupation(
     ((pkt->to % stDown) + stUp)*nchan,
     ((pkt->to % stDown) + stUp +1)*nchan);
     */	return B_FALSE;
}

/**
 * Extract the head packet from a transit queue.
 *
 * If a packet is the cause of head-of-line blocking at a queue then
 * removes it from the queue's head.
 *
 * @param i The node in which the extraction will be performed.
 * @param injector The injection queue which extract from.
 */
void extract_packet_icube (long i, port_type injector) {
    long pl;

    network[i].p[injector].tor = CLOCK_MAX; // A new packet will be waiting
    for (pl = 0; pl < pkt_len; pl++)
        rem_head_queue(&(network[i].p[injector].q));
}

/**
 * Checks if a port can be used.
 *
 * Checks if a request of an output port is required or not. It returns FALSE if the
 * queue is empty, the port is already assigned, or if the packet at the head of the queue
 * has reached its final destination.
 *
 * @param i The node in which the checking is performed.
 * @param s_p The port we are checking.
 * @return TRUE if the packet can continue in this port, FALSE otherwise.
 */
bool_t preliminary_check_icube(long i, port_type s_p) {
    q = &(network[i].p[s_p].q);     // Local queue
    if (!queue_len(q))
		return B_FALSE;	    // Nothing to be scheduled
    ph = head_queue(q);				// Let us check head of queue...
    if ((ph->pclass != RR) && (ph->pclass != RR_TAIL))
		return B_FALSE;	// It is NOT a routing record

    // At this point, we have something to route
    if ((d_p = network[i].p[s_p].aop) != P_NULL) {
        if (network[i].p[d_p].sip != s_p)
            panic("Output port should be reserved for me");
        return B_FALSE; // I've got the port already assigned
    }

    if (network[i].p[s_p].tor == CLOCK_MAX)
        network[i].p[s_p].tor = sim_clock; // Time of first reservation attempt

    id=i; 		//id of the switch. GLOBAL
    curr_p=s_p;	//source port.     GLOBAL

    if (check_rr(&pkt_space[ph->packet], &d_d, &d_w)) {
        network[i].p[p_con].req[s_p] = network[i].p[s_p].tor;
        return B_FALSE;
    }
    return B_TRUE;
}

/**
 * Checks VCT and bubble restriction.
 *
 * The virtual cut-through restrictions (room for at least one packet in the destination)
 * are always checked. The bubble restriction is only checked when chkbub is TRUE.
 * It considers these scenarios:
 * (1) If req_mode == OBLIVIOUS_REQ, meaning that several channels in parallel are being used in a oblivious mode.
 *     Then the bubble restriction is applied to all channels.
 * (2) If (bub_adap) then a bubble is required to INJECT into the adaptive channels.
 *
 * @param i The node in which we are checking.
 * @param s_p Source port (The packet is currently here).
 * @param d_p Destination port (The room is checked here).
 * @return TRUE if the restrictions are matched.
 */
bool_t check_restrictions_icube (long i, port_type s_p, port_type d_p) {
    long d_n; // Destination node. THIS node is "i"
    long d_np;// Port in the destination node. s_p and d_p are from THIS node.
    long credit=0; //The number of credits needed to advance. 1 for VCT, bub_x for bubble

    if (d_p == p_con ) return B_TRUE; // Easy: no restriction when consuming. At any rate, should not be testing this...
    // Let us check VCT restriction
    d_n = network[i].nbor[d_p/nchan];
    d_np= (network[i].nborp[d_p/nchan]*nchan)+(d_p%nchan);

    if (!(i<nprocs || d_n<nprocs) && (s_p != d_np) && ((d_p % nchan)==0))	// If the packet doesn't come/go to a NIC, doesn't keep the VC and goes to ESCAPE.
        credit=bub_x;	// Check the bubble, not just the VCT.
    if (i>=nprocs && ((d_p % nchan) != 0) && s_p < (stDown*nchan)) // if this is an adaptive channel and comes from a NIC (inyection in the switch)
        credit=pkt_len*bub_adap[network[i].congested];

    if (credit<1)
        credit=1;	// To check VCT

    if (queue_space(&(network[d_n].p[d_np].q)) < (pkt_len*credit)) // check restriction (VCT and/or bubble)
        return B_FALSE; // Not enough space at destination

    return B_TRUE;
}

/**
 * Fully adaptive routing in a icube using Virtual channels.
 *
 * Each physical has an ESCAPE (Bubble) subchannel plus 'nchan-1' adaptive VC.
 * Adaptive routing selects between all the profitable VC in all physical links.
 * Checks a routing record and indicates the direction/way of choice, following
 * the less congested channel, but using minimal paths.
 * It chooses the profitable channel with more credits.
 *
 * @param pkt The packet to route.
 * @param d The desired dimesion is returned here.
 * @param w The elected direction (way) is returned here.
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_icube_adaptive(packet_t * pkt, dim *d, way *w) {
    dim j;
    routing_r prr=pkt->rr;	// the routing record of the packet.
    long max=0;	// Maximum queue space (minimum occupancy);
    long qs;	// Space available on the queue in use.
    port_type p, nbp[radix*nchan], nm=0;	// Neighbor port
    long DOR=0;	// this should be 0 only for the first profitable dimension, this way the ESCAPE chanel should be asked in DOR fashion.
    long pt, n; // ports in an icube

    *w=0; // let's forget the way.

    // p.pclass == RR. LET US ANALYZE THE ROUTING RECORD
    // returns TRUE if the phit has to be consumed
    // Checking order: x+ x- y+ y- z+ z-
    if (pkt->n_hops==0){ // First jump out of the NIC
        for (p=0; p<nchan; p++){
            qs=queue_space(&network[network[id].nbor[0]].p[(network[id].nborp[0]*nchan)+p].q);
            if (qs>=pkt_len){
                if (qs>max ){
                    max=qs;
                    nm=1;
                    nbp[0]=p;
                } else if (qs==max)
                    nbp[nm++]=p;
            }
        }
        if (nm<1)
            *d = NULL_PORT;
        else
            *d = nbp[rand()%nm];
        return B_FALSE;
    }

    if (pkt->n_hops==prr.size-1){		// Just a jump to the destination
        for (p=((pkt->to % stDown)*nchan); p<((pkt->to % stDown)*nchan)+nchan; p++){
            qs=queue_space(&network[network[id].nbor[p/nchan]].p[network[id].nborp[p/nchan]+(p%nchan)].q);
            if (qs>=pkt_len){
                if (qs>max){
                    max=qs;
                    nm=1;
                    nbp[0]=p;
                } else if (qs==max)
                    nbp[nm++]=p;
            }
        }
        if (nm<1)
            *d = NULL_PORT;
        else
            *d = nbp[rand()%nm];
        return B_FALSE;
    }

    if (prr.size==pkt->n_hops)	// We have arrived to the NIC
        return B_TRUE;

    for (j=D_X; j<ndim; j++) {
        if (prr.rr[j] > 0){
            for (n=0; n<links_per_direction;n++)
                for (p=DOR; p<nchan; p++){
                    pt=stDown+(2*j*links_per_direction)+n;
                    if(check_restrictions_icube (id, curr_p, (pt*nchan)+p)){
                        qs=queue_space(&network[network[id].nbor[pt]].p[(network[id].nborp[pt]*nchan)+p].q);
                        if (qs>max){
                            max=qs;
                            nm=1;
                            nbp[0]=(pt*nchan)+p;
                        } else if (qs==max)
                            nbp[nm++]=(pt*nchan)+p;
                    }
                }
            DOR=1;
        }
        else if (prr.rr[j] < 0) {
            for (n=0; n<links_per_direction;n++)
                for (p=DOR; p<nchan; p++){
                    pt=stDown+(((2*j)+1)*links_per_direction)+n;
                    if(check_restrictions_icube (id, curr_p, (pt*nchan)+p)){
                        qs=queue_space(&network[network[id].nbor[pt]].p[(network[id].nborp[pt]*nchan)+p].q);
                        if (qs>max){
                            max=qs;
                            nm=1;
                            nbp[0]=(pt*nchan)+p;
                        } else if (qs==max)
                            nbp[nm++]=(pt*nchan)+p;
                    }
                }
            DOR=1;
        }
    }
    if(nm>0)
        *d = nbp[rand()%nm];
    else
        *d = NULL_PORT;
    return B_FALSE;
}

/**
 * Routes using dimesion order routing in a icube (adaptive between parallel links).
 *
 * Checks a routing record and indicates the direction/way of choice, following the
 * dimension order routing algorithm. (order: x+ x- y+ y- z+ z-).
 * It also chooses the physichal channel with more credits.
 * Bubble is used to avoid deadlock.
 *
 * @param pkt The packet to route.
 * @param d The desired dimesion is returned here.
 * @param w The elected direction (way) is returned here.
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_icube_static(packet_t * pkt, dim *d, way *w) {
    dim j;
    routing_r prr=pkt->rr;	// the routing record of the packet.
    *w=0; // let's forget the way.

    // p.pclass == RR. LET US ANALYZE THE ROUTING RECORD
    // returns TRUE if the phit has to be consumed
    // Checking order: x+ x- y+ y- z+ z-
    if (pkt->n_hops==0){		// First jump out of the NIC
        if (check_restrictions_icube (id, curr_p, 0))
            *d = 0;
        else
            *d=NULL_PORT;
        return B_FALSE;
    } else if (prr.size==pkt->n_hops)	// We have arrived to the NIC
        return B_TRUE;
    else if (pkt->n_hops==prr.size-1){	// Just a jump to the destination
        *d=pkt->to % stDown;
        if (!check_restrictions_icube (id, curr_p, *d))
            *d=NULL_PORT;
        return B_FALSE;
    }

    for (j=D_X; j<ndim; j++) {
        if (prr.rr[j] > 0){
            *d = stDown+(links_per_direction*2*j)+(pkt->from % links_per_direction);
            if (!check_restrictions_icube (id, curr_p, *d))
                *d=NULL_PORT;
            return B_FALSE;
        }
        else if (prr.rr[j] < 0) {
            *d = stDown+(links_per_direction*((2*j)+1))+(pkt->from % links_per_direction);
            if (!check_restrictions_icube (id, curr_p, *d))
                *d=NULL_PORT;
            return B_FALSE;
        }
    }
    panic ("Should not be here in check_rr_icube_adaptive_static");

    return B_FALSE; //Remove warning
}

/**
 * Requests an output port in an indirect cube .
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_icube (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"

    //		+-> check packet availabilty.
    //		+-> writes time of request information.
    //		+-> check if have arrived.
    //		+-> prepares d_d calculating routing record.
    //		|
    if (!preliminary_check_icube (i, s_p)) return;
    // Won't use array mt -- use d_d and d_w instead

    d_p=d_d;

    if (d_p == (port_type) NULL_PORT) {
        if (extract && s_p >= p_inj_first)
            extract_packet_icube(i, s_p);
        return;
    }
    else
        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

/**
 * Checks if a port can be used (for IB-based simul.).
 *
 * Checks if a request of an output port is required or not. It returns FALSE if the
 * queue is empty, the port is already assigned, or if the packet at the head of the queue
 * has reached its final destination.
 *
 * @param i The node in which the checking is performed.
 * @param s_p The port we are checking.
 * @return TRUE if the packet can continue in this port, FALSE otherwise.
 */
bool_t preliminary_check_icube_IB (long i, port_type s_p) {
    q = &(network[i].p[s_p].q);     // Local queue
    if (!queue_len(q))
        return B_FALSE;	    // Nothing to be scheduled
    ph = head_queue(q);				// Let us check head of queue...
    if ((ph->pclass != RR) && (ph->pclass != RR_TAIL))
        return B_FALSE;	// It is NOT a routing record

    // At this point, we have something to route
    if ((d_p = network[i].p[s_p].aop) != P_NULL) {
        if (network[i].p[d_p].sip != s_p)
            panic("Output port should be reserved for me");
        return B_FALSE; // I've got the port already assigned
    }

    if (network[i].p[s_p].tor == CLOCK_MAX)
        network[i].p[s_p].tor = sim_clock; // Time of first reservation attempt

    id=i; 		//id of the switch. GLOBAL
    curr_p=s_p;	//source port.     GLOBAL

    if (check_rr(&pkt_space[ph->packet], &d_d, &d_w)) {
        network[i].p[p_con].req[s_p] = network[i].p[s_p].tor;
        return B_FALSE;
    }
    return B_TRUE;
}

/**
 * Checks VCT restriction (for IB-based simul.).
 *
 * The virtual cut-through restrictions (room for at least one packet in the destination)
 * are always checked.
 *
 * @param i The node in which we are checking.
 * @param s_p Source port (The packet is currently here).
 * @param d_p Destination port (The room is checked here).
 * @return TRUE if the restrictions are matched.
 */
bool_t check_restrictions_icube_IB (long i, port_type s_p, port_type d_p) {
    long d_n; // Destination node. THIS node is "i"
    long d_np;// Port in the destination node. s_p and d_p are from THIS node.
    //	long credit=0; //The number of credits needed to advance. 1 for VCT, bub_x for bubble

    if (d_p == p_con ) return B_TRUE; // Easy: no restriction when consuming. At any rate, should not be testing this...
    // Let us check VCT restriction
    d_n = network[i].nbor[d_p/nchan];
    d_np= (network[i].nborp[d_p/nchan]*nchan)+(d_p%nchan);

    if (queue_space(&(network[d_n].p[d_np].q)) < pkt_len) // check restriction (VCT)
        return B_FALSE; // Not enough space at destination

    return B_TRUE;
}

/**
 * Routes using dimesion order routing in an indirect cube  split into independent meshes (for IB-based simul.)
 *
 * Checks a routing record and indicates the direction/way of choice, following the
 * dimension order routing algorithm. (order: x+ x- y+ y- z+ z-).
 * Mesh + DOR is used to avoid deadlock.
 *
 * @param pkt The packet to route.
 * @param d The desired dimesion is returned here.
 * @param w The elected direction (way) is returned here.
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_icube_static_IB (packet_t * pkt, dim *d, way *w) {
    dim j;
    routing_r prr=pkt->rr;	// the routing record of the packet.
    *w=0; // let's forget the way.

    // p.pclass == RR. LET US ANALYZE THE ROUTING RECORD
    // returns TRUE if the phit has to be consumed
    // Checking order: x+ x- y+ y- z+ z-
    if (pkt->n_hops==0){		// First jump out of the NIC
        if (check_restrictions_icube_IB (id, curr_p, 0))
            *d = 0;
        else
            *d=NULL_PORT;
        return B_FALSE;
    } else if (prr.size==pkt->n_hops)	// We have arrived to the NIC
        return B_TRUE;
    else if (pkt->n_hops==prr.size-1){	// Just a jump to the destination
        *d=pkt->to % stDown;
        if (!check_restrictions_icube_IB (id, curr_p, *d))
            *d=NULL_PORT;
        return B_FALSE;
    }

    for (j=D_X; j<ndim; j++) {
        if (prr.rr[j] > 0){
            *d = stDown+(links_per_direction*2*j)+(prr.rr[ndim]);
            if (!check_restrictions_icube_IB (id, curr_p, *d))
                *d=NULL_PORT;
            return B_FALSE;
        }
        else if (prr.rr[j] < 0) {
            *d = stDown+(links_per_direction*((2*j)+1))+(prr.rr[ndim]);
            if (!check_restrictions_icube_IB (id, curr_p, *d))
                *d=NULL_PORT;
            return B_FALSE;
        }
    }
    panic ("Should not be here in check_rr_icube_adaptive_static");
    return B_FALSE; //Remove warning
}

/**
 * Requests an output port in an indirect cube (for IB-based simul.)
 *
 * @param i The node in which the request is performed.
 * @param s_p The source (input) port which is requesting the output port.
 *
 * @see init_functions
 * @see request_port
 */
void request_port_icube_IB (long i, port_type s_p) {
    // Let us work with port "s_p" at node "i"

    //		+-> check packet availabilty.
    //		+-> writes time of request information.
    //		+-> check if have arrived.
    //		+-> prepares d_d calculating routing record.
    //		|
    if (!preliminary_check_icube_IB (i, s_p)) return;
    // Won't use array mt -- use d_d and d_w instead

    d_p=d_d;

    if (d_p == (port_type) NULL_PORT) {
        if (extract && s_p >= p_inj_first)
            extract_packet_icube(i, s_p);
        return;
    }
    else
        network[i].p[d_p].req[s_p] = network[i].p[s_p].tor;
}

long get_first_vc(long length){
    long nvc = 0;

    if(vc_inj == VC_INJ_ZERO){
        nvc = 0;
    }
    else if(vc_inj == VC_INJ_DIST){
        if(length == 2){
            nvc = diameter_r - 1;
        }
        else
            nvc = diameter_r - (length - 2);
    }
    else if(vc_inj == VC_INJ_DIST_RND){
        if(length == 2){
            nvc = diameter_r - 1;
        }
        else
            nvc = diameter_r - (length - 2);

        nvc = rand() % (nvc + 1);
    }
    if(nvc > used_chan)
        used_chan = nvc;
    if (nvc == nchan)
        panic("Number of virtual channels exceeded during deadlock avoidance!");

    return(nvc);
}

/**
 * Routes deadlock-free in any arbitrary topology.
 *
 * To remove cycles, the VCs are arranged in such a way that when moving
 * into a lower id node, the VC is increased. Worst case scenario D vcs are
 * needed (D=diameter)
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_arbitrary_node(packet_t * pkt, dim *d, way *w) {
    long nd, nvc, n_id;

    if (pkt->to == id){
        return B_TRUE;
    }

    *w = 0;

    if (curr_p >= p_inj_first) {   // injection
        *d = (get_next_hop(pkt) * nchan) + get_first_vc(pkt->rr.size); // inject based on distance
    }
    else if(pkt->n_hops == 1){
        nvc=curr_p % nchan;
        nd=get_next_hop(pkt);//next port to calculate next node
        *d=(nd * nchan) + nvc;
    }
    else { // if next_hop ID >= current_hop ID keep the VC, otherwise increase the VC number
        nd=get_next_hop(pkt);//next port to calculate next node
        nvc=curr_p%nchan;// current vc
        n_id = network[id].nbor[nd];
        if ((n_id < id) && (n_id != pkt->to)){
            nvc++;
            if(nvc > used_chan)
                used_chan = nvc;
            if (nvc == nchan)
                panic("Number of virtual channels exceeded during deadlock avoidance!");
        }
        *d=(nd*nchan)+nvc;//compute next port to be used
    }

    return B_FALSE;
}

/**
 * Routes deadlock-free in any arbitrary topology.
 *
 * To remove cycles, the VCs are arranged in such a way that when moving
 * into a lower port, the VC is increased. Worst case scenario D vcs are
 * needed (D=diameter).
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_arbitrary_port(packet_t * pkt, dim *d, way *w) {
    long cp, fp, nd, nvc, n_id;

    if (pkt->to == id){
        return B_TRUE;
    }

    *w=0;

    if (curr_p >= p_inj_first){    // injection
        *d = (get_next_hop(pkt) * nchan) + get_first_vc(pkt->rr.size); // inject based on distance
    }
    else if(pkt->n_hops == 1){
        nvc=curr_p % nchan;
        nd=get_next_hop(pkt);//next port to calculate next node
        *d=(nd * nchan) + nvc;
    }
    else { // if next_port > previous_hop ID keep the VC, otherwise increase the VC number
        cp=curr_p/nchan; // current port
        fp=network[id].nborp[cp]; // from port
        nvc=curr_p%nchan; // current vc
        nd=get_next_hop(pkt);//next port to calculate next node
        n_id = network[id].nbor[nd];
        if ((nd <= fp) && (n_id != pkt->to)){// could check the node as well and save a few vc changes...
            nvc++;
            if(nvc > used_chan)
                used_chan = nvc;
            if (nvc == nchan)
                panic("Number of virtual channels exceeded during deadlock avoidance!");
        }
        *d=(nd*nchan)+nvc;
    }
    return B_FALSE;
}

/**
 * Routes deadlock-free in any arbitrary topology.
 *
 * To remove cycles, the VCs are arranged in such a way that when moving
 into a lower port, the VC is increased. Worst case scenario D vcs are
 needed (D=diameter).
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_arbitrary_node_port(packet_t * pkt, dim *d, way *w) {
    long cp, fp, nd, n_id, nvc;

    if (pkt->to == id){
        return B_TRUE;
    }

    *w=0;

    if (curr_p >= p_inj_first){    // injection
        *d = (get_next_hop(pkt) * nchan) + get_first_vc(pkt->rr.size); // inject based on distance
    }
    else if(pkt->n_hops == 1){
        nvc=curr_p % nchan;
        nd=get_next_hop(pkt);//next port to calculate next node
        *d=(nd * nchan) + nvc;
    }
    else { // if next_port > previous_hop ID keep the VC, otherwise increase the VC number
        cp=curr_p/nchan; // current port
        fp=network[id].nborp[cp]; // from port
        nvc=curr_p%nchan; // current vc
        nd=get_next_hop(pkt);//next port to calculate next node
        n_id = network[id].nbor[nd];
        if ((((n_id < id) && (nd == fp)) || (nd < fp)) && (n_id != pkt->to)){// could check the node as well and save a few vc changes...
            nvc++;
            if(nvc > used_chan)
                used_chan = nvc;
            if (nvc == nchan)
                panic("Number of virtual channels exceeded during deadlock avoidance!");
        }
        *d=(nd*nchan)+nvc;
    }
    return B_FALSE;
}

bool_t check_rr_arbitrary_dummy(packet_t * pkt, dim *d, way *w) {
    long nd, nvc;
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->to == id){
        return B_TRUE;
    }

    *w=0;

    if (curr_p>=p_inj_first){    // injection
        *d=(get_next_hop(pkt)*nchan) + rand()%nchan; // inject in  random VC
    }
    else { // keep the VC number
        nd=get_next_hop(pkt);//next port to calculate next node
        nvc=curr_p%nchan;// current vc
        *d=(nd*nchan)+nvc;//compute next port to be used
    }
    return B_FALSE;
}

bool_t check_rr_arbitrary_inc_hop(packet_t * pkt, dim *d, way *w) {
    long nd, n_id, nvc;
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->to == id){
        return B_TRUE;
    }

    *w=0;

    if (curr_p>=p_inj_first || pkt->n_hops == 1){    // injection
        *d=get_next_hop(pkt)*nchan; // inject in VC 0
    }
    else { // keep the VC number
        nd=get_next_hop(pkt);//next port to calculate next node
        nvc=curr_p%nchan;// current vc
        n_id = network[id].nbor[nd];
        if(n_id != pkt->to){
            nvc++;
            if(nvc > used_chan)
                used_chan = nvc;
            if (nvc == nchan)
                panic("Number of virtual channels exceeded during deadlock avoidance!");
        }
        *d=(nd * nchan) + nvc;//compute next port to be used
    }
    return B_FALSE;
}

/**
 * VC management using Virtual output queues.
 *
 * Chooses the VC based on the next hop that needs to be taken.
 * Does not solve deadlock by itself.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_arbitrary_voq(packet_t * pkt, dim *d, way *w) {
    long nd, nvc;
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->to == id){
        return B_TRUE;
    }

    *w=0;

    nd=get_next_hop(pkt);//next port to calculate next node
    nvc=get_next_router_hop(pkt);// current vc

    if(nvc > used_chan)
        used_chan = nvc;
    if (nvc>=nchan)
        panic("not enough virtual channels for VOQ!");
    *d=(nd*nchan)+nvc;//compute next port to be used

    return B_FALSE;
}

long get_next_hop_rr(packet_t*pkt) {
    return(pkt->rr.rr[pkt->n_hops]);
}

long get_next_router_hop_rr(packet_t*pkt) {
    if (pkt->n_hops < pkt->rr.size-1){ // is not the last hop
        return (pkt->rr.rr[(pkt->n_hops)+1]);
    }
    else
        return rand()%nchan;
}

long get_next_router_hop_cam(packet_t*pkt) {
    // to be implemented!!!!
    return(-1);
}

long get_next_hop_cam(packet_t*pkt) {
    long np;
    long to = (pkt->to/stDown);

    if(pkt->n_hops == 0){
        np = 0;
    }
    else if((to + nprocs) == id){
        np = (pkt->to % stDown);
    }
    else{
        np = get_cam_port(pkt);
    }

    return(np);
}

long get_cam_port_rr(packet_t*pkt){
    long src, dst, np;

    src = ((pkt->from / stDown) + nprocs);
    dst = (pkt->to / stDown);

    np = network[src].cam[dst].ports[pkt->path_id][pkt->n_hops];

    return(np);
}

/*
   long get_cam_port_adaptive(packet_t*pkt){

   long dst, np;

   dst = (pkt->to / stDown);
   np = network[id].cam[dst].ports[0][0];

   return(np);
   }
   */

long get_cam_port_adaptive_node(packet_t*pkt){
    long min, min_d;		                // Minimum value
    long i, d, ql, dst, n_paths, nvc_aux, n_id;							// Queue length
    long *nbp;
    port_type p, nm=0;	// Neighbor port

    min = tr_ql;
    min_d = nswitches;
    dst = (pkt->to / stDown);
    n_paths = network[id].cam[dst].n_paths;
    nbp = alloc(n_paths * sizeof(long));

    if(pkt->n_hops != pkt->rr.size){
        pkt->rr.size = pkt->n_hops; // Ther packet has advanced
    }

    i = 0;
    while(i < n_paths){
        p = network[id].cam[dst].ports[0][i];
        nvc_aux = curr_p % nchan;
        n_id = network[id].nbor[p];
        if(pkt->n_hops != 1){
            if ((n_id < id) && (n_id != pkt->to)){
                nvc_aux++;
            }
        }
        d = network[id].cam[dst].ports[1][i];
        ql = queue_len(&network[n_id].p[(network[id].nborp[p] * nchan)+ nvc_aux].q);

        if ((d < pkt->rr.rr[pkt->rr.size - 1]) && (ql < min || ((ql == min) && d < min_d))){
            min = ql;
            min_d = d;
            nm = 1;
            nbp[0] = p;
        } else if ((d < pkt->rr.rr[pkt->rr.size - 1]) && ql == min && d == min_d){
            nbp[nm++]=p;
        }
        i++;
    }
    p = nbp[rand() % nm];
    free(nbp);

    pkt->rr.rr[pkt->rr.size] = min_d;

    return(p);
}

long get_cam_port_adaptive_port(packet_t*pkt){
    long min, min_d;		                // Minimum value
    long i, d, ql, dst, n_paths, nvc_aux, n_id, cp, fp;							// Queue length
    long *nbp;
    port_type p, nm=0;	// Neighbor port

    min = tr_ql;
    min_d = nswitches;
    dst = (pkt->to / stDown);
    n_paths = network[id].cam[dst].n_paths;
    nbp = alloc(n_paths * sizeof(long));

    if(pkt->n_hops != pkt->rr.size){
        pkt->rr.size = pkt->n_hops; // Ther packet has advanced
    }

    i = 0;
    while(i < n_paths){
        p = network[id].cam[dst].ports[0][i];

        cp = curr_p / nchan; // current port
        fp = network[id].nborp[cp]; // from port

        nvc_aux = curr_p % nchan;
        n_id = network[id].nbor[p];

        if(pkt->n_hops != 1){
            if ((p <= fp) && (n_id != pkt->to)){// could check the node as well and save a few vc changes...
                nvc_aux++;
            }
        }
        d = network[id].cam[dst].ports[1][i];
        ql = queue_len(&network[n_id].p[(network[id].nborp[p] * nchan)+ nvc_aux].q);

        if ((d < pkt->rr.rr[pkt->rr.size - 1]) && (ql < min || ((ql == min) && d < min_d))){
            min = ql;
            min_d = d;
            nm = 1;
            nbp[0] = p;
        } else if ((d < pkt->rr.rr[pkt->rr.size - 1]) && ql == min && d == min_d){
            nbp[nm++]=p;
        }
        i++;
    }
    p = nbp[rand() % nm];
    free(nbp);

    pkt->rr.rr[pkt->rr.size] = min_d;

    return(p);
}

long get_cam_port_adaptive_node_port(packet_t*pkt){
    long min, min_d;		                // Minimum value
    long i, d, ql, dst, n_paths, nvc_aux, n_id, cp, fp;							// Queue length
    long *nbp;
    port_type p, nm=0;	// Neighbor port

    min = tr_ql;
    min_d = nswitches;
    dst = (pkt->to / stDown);
    n_paths = network[id].cam[dst].n_paths;
    nbp = alloc(n_paths * sizeof(long));

    if(pkt->n_hops != pkt->rr.size){
        pkt->rr.size = pkt->n_hops; // Ther packet has advanced
    }

    i = 0;
    while(i < n_paths){
        p = network[id].cam[dst].ports[0][i];
        nvc_aux = curr_p % nchan;
        n_id = network[id].nbor[p];
        cp = curr_p / nchan; // current port
        fp = network[id].nborp[cp]; // from port

        if(pkt->n_hops != 1){
            if ((((n_id < id) && (p == fp)) || (p < fp)) && (n_id != pkt->to)){// could check the node as well and save a few vc changes...
                nvc_aux++;
            }
        }
        d = network[id].cam[dst].ports[1][i];
        ql = queue_len(&network[n_id].p[(network[id].nborp[p] * nchan)+ nvc_aux].q);

        if ((d < pkt->rr.rr[pkt->rr.size - 1]) && (ql < min || ((ql == min) && d < min_d))){
            min = ql;
            min_d = d;
            nm = 1;
            nbp[0] = p;
        } else if ((d < pkt->rr.rr[pkt->rr.size - 1]) && ql == min && d == min_d){
            nbp[nm++]=p;
        }
        i++;
    }
    p = nbp[rand() % nm];
    free(nbp);

    pkt->rr.rr[pkt->rr.size] = min_d;

    return(p);
}

/**
 * Routes in Dragonfly.
 *
 * Arithmetic routing in Dragonfly. The next hop is calculated in each router. No deadlock avoidance is provided.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_dragonfly_arithmetic(packet_t * pkt, dim *d, way *w) {
    // a hop for each routing record value, so we have arrive to our destination:)
    if (pkt->to == id) // arrived into our destination
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    if (pkt->n_hops==0)	// NIC, just choose a VC at random
        *d=rand()%nchan;
    else
        *d=(route_dragonfly(id,pkt->to,pkt->rr.size)*nchan) + (curr_p%nchan) ; // Remember rr.size stores the proxy group for simplicity

    return B_FALSE;
}

/**
 * Routes in Dragonfly.
 *
 * Static routing in Dragonfly using Dally deadlock avoidance.
 * The routing record stores the port that needs to be taken in each hop.
 * Dally mechanisms is used in order to ensure deadlock-freedom.
 * This version is used if there are no adaptive VCs.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_dragonfly_dally(packet_t * pkt, dim *d, way *w) {
    long npt, nvc;

    if (pkt->to == id) // arrived into our destination
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    nvc = curr_p % nchan;

	if (pkt->n_hops==0){
//		if (pkt->rr.rr[7] == 0 && escape_vcs>2) // routes without proxy could be injected in VC1 when mixed with routes that use a proxy
//			*d = 1;
//		else
			*d = 0;
	} else {
        npt = get_next_hop(pkt);
        if(npt >= param_p + intra_ports){
            nvc++;
            if(nvc > used_chan)
                used_chan = nvc;
            if (nvc == nchan)
                panic("Number of virtual channels exceeded during deadlock avoidance!");
        }
        *d= (npt * nchan) + nvc; // Remember rr.size stores the proxy group for simplicity
    }
    return B_FALSE;
}

/**
 * Routes in Dragonfly.
 *
 * Adaptive routing in Dragonfly using Dally deadlock avoidance + extra adaptive VCs.
 * The routing record stores the port that needs to be taken in each hop.
 * Dally mechanisms is used as the escape network to ensure deadlock-freedom.
 * Bubble mechanism is used to allow packets entering the escape network.
 * Packets are injected randomly in an adaptive VC. They stay in the same VC if there is enough credit.
 * If there is no space, try another adaptive VC at random and if its credit not enough, then try to move to the escape channel.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_dragonfly_dally_adap(packet_t * pkt, dim *d, way *w) {
    long lpt, lvc; // local port and vc
    long npt, nvc, nnd, ntry; // next port, vc, node and try for a new vc

    if (pkt->to == id) // arrived into our destination
        return B_TRUE;
    *w=0;	// Way has no sense in multistage.

    lvc = curr_p % nchan;

	if (pkt->n_hops==0){
//		if (pkt->rr.rr[7] == 0 && escape_vcs>2) // routes without proxy could be injected in VC1 when mixed with routes that use a proxy
//			*d = 1;
//		else
			*d = escape_vcs+(rand()%(nchan-escape_vcs)); // selects a VC at random when injecting. There is an implicit addition of (pkt->rr.rr[0]*nchan) which is 0 for regular dragonfly
	} else {
        lpt = get_next_hop(pkt);
        npt = network[id].nborp[lpt];
        nnd = network[id].nbor[lpt];
        nvc = lvc; // the default option is to stay in the same VC.

        if (queue_space(&(network[nnd].p[npt*nchan+nvc].q)) < pkt_len) {// No space at destination
			// let's try an adaptive VC at random
			ntry=escape_vcs+(rand()%(nchan-escape_vcs));
			if (queue_space(&(network[nnd].p[npt*nchan+ntry].q)) < pkt_len) {// No space in this try either
				// Awww! We'll use the escape channel then.
				if (lvc<escape_vcs){ // we're already in the escape channel
					if(lpt >= param_p + intra_ports){
						nvc=lvc+1;
					}
				} else {
					if(queue_space(&(network[nnd].p[npt*nchan].q)) >= 2*pkt_len)// We check for one extra space (bubble restriction)
						nvc=0;
				}
			} else {
				nvc=ntry;
			}
        }

		if(nvc > used_chan)
			used_chan = nvc;
		if (nvc == nchan)
			panic("Number of virtual channels exceeded during deadlock avoidance!");
        *d= (lpt * nchan) + nvc; // Remember rr.size stores the proxy group for simplicity
    }
    return B_FALSE;
}

/**
 statick routing para megafly
**/
bool_t check_rr_megafly_arithmetic(packet_t *pkt, dim *d, way *w) {
    if (pkt->to == id)
        return B_TRUE;

    *w = 0; // Way has no sense in multistage

    if (pkt->n_hops == 0) // NIC: choose VC at random
        *d = rand() % nchan;
    else
        *d = (route_megafly(id, pkt->to, pkt->rr.size) * nchan) + (curr_p % nchan);
        // rr.size stores the proxy group for non-minimal routing

    return B_FALSE;
}
// bool_t check_rr_megafly_arithmetic(packet_t * pkt, dim *d, way *w) {
//     // Si el nodo actual (id) es el destino final del paquete
//     if (pkt->to == id) 
//         return B_TRUE;
//
//     *w = 0; // Sin uso en estas topologías indirectas
//
//     if (pkt->n_hops == 0) {
//         // Estamos en la NIC (servidor): inyectamos en un VC basado en el ID o azar
//         *d = 0; // O rand() % nchan;
//     } else {
//         // Obtenemos el puerto de salida que guardamos en el paso anterior en megafly_rr
//         long npt = get_next_hop(pkt);
//         
//         // El puerto real en el simulador es (Puerto * canales_virtuales) + canal_actual
//         // Usamos curr_p % nchan para mantener el mismo VC o distribuir
//         *d = (npt * nchan) + (curr_p % nchan);
//     }
//
//     return B_FALSE;
// }

/**
 statick routing para slimfly
**/
// bool_t check_rr_slimfly_arithmetic(packet_t * pkt, dim *d, way *w) {
//     if (pkt->to == id)
//         return B_TRUE;
//
//     *w = 0;
//
//     if (pkt->n_hops == 0) {
//         // Inyección desde servidor
//         *d = 0;
//     } else {
//         // En Slim Fly, el ruteo suele ser: Switch Local -> Switch Destino -> Servidor
//         // get_next_hop lee el puerto calculado por tu función slimfly_rr
//         long npt = get_next_hop(pkt);
//         
//         *d = (npt * nchan) + (curr_p % nchan);
//     }
//
//     return B_FALSE;
// }
bool_t check_rr_slimfly_arithmetic(packet_t *pkt, dim *d, way *w) {
    if (pkt->to == id)
        return B_TRUE;

    *w = 0; // Way has no sense in multistage

    if (pkt->n_hops == 0) // NIC: choose VC at random
        *d = rand() % nchan;
    else
        *d = (route_slimfly(id, pkt->to, pkt->rr.size) * nchan) + (curr_p % nchan);
        // rr.size stores the proxy switch (valiant) or 0 (minimal)

    return B_FALSE;
}

/**
 * Spanning tree management for arbitrary topologies.
 *
 * Static routing through the defined spanning trees.
 * The first entry in the routing record stores the number of spanning tree that is being used; i.d. the VC to use.
 *
 * @param pkt The packet to route.
 * @param d The desired port is returned here.
 * @param w The elected direction (way) is returned here (always UP).
 * @return TRUE if the phit has reached its final destination
 *
 * @see init_functions
 * @see check_rr
 */
bool_t check_rr_spanning_tree(packet_t *pkt, dim *d, way *w){
    if (pkt->to == id) // arrived into our destination
        return B_TRUE;

    *w=0;	// Way has no sense in multistage.

    if (pkt->n_hops == 0)
        *d = pkt->rr.rr[0];
    else
        *d = (pkt->rr.rr[pkt->n_hops + 1] * nchan) + pkt->rr.rr[0];

    return B_FALSE;
}
