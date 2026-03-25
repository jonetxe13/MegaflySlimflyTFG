/**
 * @file
 * @brief	Initialization of structures & functions required for simulation.

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

#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "misc.h"

/**
 * Initialization of functions.
 *
 * Instances the virtual functions to the ones required for the simulation
 *
 * @see run_network
 * @see neighbor
 * @see calc_rr
 * @see check_rr
 * @see request_port
 * @see consume
 * @see arbitrate_cons
 * @see arbitrate_select
 * @see select_input_port
 * @see data_movement
 */
void init_functions (void) {
    long i, j;

    if (shotmode) run_network = run_network_shotmode;
#if (TRACE_SUPPORT != 0)
    else if (pattern == TRACE) run_network = run_network_trc;
#endif
    else run_network = run_network_batch;

#if (EXECUTION_DRIVEN != 0)
    init_exd(fsin_cycle_relation, simics_cycle_relation, packet_size_in_phits,
            serv_addr, num_periodos_espera);
    run_network = run_network_exd;
#endif

    batch = alloc(sizeof(batch_t)*(samples+1));
    source_ports = alloc(sizeof(long)*n_ports);
    dest_ports = alloc(sizeof(long)*n_ports);
    port_utilization = alloc(sizeof(CLOCK_TYPE)*n_ports);

    if (plevel & 1) {
        destinations = alloc(sizeof(long *)*nprocs);
        sources = alloc(sizeof(long *)*nprocs);

        for (i=0; i<nprocs; i++) {
            destinations[i]=alloc(sizeof(long)*nprocs);
            sources[i]=alloc(sizeof(long)*nprocs);
            for (j=0; j<nprocs; j++){
                destinations[i][j]=0;
                sources[i][j]=0;
            }
        }
    }

    if (plevel & 4){
        if (topo<DIRECT)
            max_dst = nodes_x+nodes_y+nodes_z-2;
        else if (topo<=CUBE)
            max_dst = nodes_x+nodes_y+nodes_z;
        else if (topo==DRAGONFLY_ABSOLUTE || topo==DRAGONFLY_RELATIVE || topo==DRAGONFLY_RND || topo==DRAGONFLY_CIRCULANT || topo==DRAGONFLY_NAUTILUS || topo==DRAGONFLY_HELIX || topo==DRAGONFLY_OTHER || topo==MEGAFLY)
            max_dst = 7;
        else if(topo==SLIMFLY)
                max_dst = 5;
        else if (topo == RRG || topo == EXA || topo == GDBG || topo == KAUTZ)
            max_dst = nswitches;
        else if (topo<INDIRECT)
            max_dst = (2*nstages)+3;

        inj_dst = alloc(sizeof(long)*max_dst);
        con_dst = alloc(sizeof(long)*max_dst);
        for (i=0; i<max_dst; i++)
            inj_dst[i] = con_dst[i] = 0;
    }

    for (i=0; i<n_ports; i++) {
        source_ports[i]=0;
        dest_ports[i]=0;
        port_utilization[i]= (CLOCK_TYPE) 0L;
    }

    if (topo>DIRECT && routing<CUBE_ROUTING) {
        printf("WARNING: DOR only for direct topologies. Switching to Adaptive Routing\n");
        routing=ADAPTIVE_ROUTING;
    }
    // Not the best place to put this. Need to check.
    get_next_hop = get_next_hop_rr;
    get_next_router_hop = get_next_router_hop_rr;

    switch (routing) {
        case DIMENSION_ORDER_ROUTING:
            check_rr = check_rr_dim_o_r;
            break;
        case DIRECTION_ORDER_ROUTING:
            check_rr = check_rr_dir_o_r;
            break;
        case STATIC_ROUTING:
        case ARITH_ROUTING:
        case ADAPTIVE_ROUTING:
        case RANDOM_ROUTING:
		case SRC_ROUTING:
		case DST_ROUTING:
            if (topo<DIRECT){
                printf("WARNING: Inadequate routing for k-ary n-cubes. Switching to Dimension Order Routing\n");
                check_rr = check_rr_dim_o_r;
            }
            break;
        case ICUBE_1M_ROUTING:
            calc_rr=icube_1mesh_rr;
            break;
        case ICUBE_4M_ROUTING:
            calc_rr=icube_4mesh_rr;
            break;
        case CAM_ROUTING:
            if(cam_policy == CAM_SP){
                fill_cam = fill_cam_sp;
                cam_ports = CAM_RR; // Not adaptive for SP
            } else if(cam_policy == CAM_ECMP){
                fill_cam = fill_cam_ecmp;
            } else if(cam_policy == CAM_KSP){
                fill_cam = fill_cam_ksp;
            } else if(cam_policy == CAM_LLSKR){
                fill_cam = fill_cam_llskr;
            } else if(cam_policy == CAM_ALLPATH){
                fill_cam = fill_cam_allpath;
            } else panic("Unsupported cam policy");

            if(cam_ports == CAM_RR){
                init_cams = init_cams_rr;
                finish_cams = finish_cams_rr;
                get_cam_port = get_cam_port_rr;
                calc_rr = graph_rr_static;
                get_next_hop = get_next_hop_rr;
                get_next_router_hop = get_next_router_hop_cam;
                set_mpaths = set_mpaths_rr;
                set_mpaths_reverse = set_mpaths_reverse_rr;
            } else if(cam_ports == CAM_RND){
                init_cams = init_cams_rr;
                finish_cams = finish_cams_rr;
                get_cam_port = get_cam_port_rr;
                calc_rr = graph_rnd_static;
                get_next_hop = get_next_hop_rr;
                get_next_router_hop = get_next_router_hop_cam;
                set_mpaths = set_mpaths_rr;
                set_mpaths_reverse = set_mpaths_reverse_rr;
            } else if(cam_ports == CAM_ADAPTIVE){
                init_cams = init_cams_adaptive;
                finish_cams = finish_cams_adaptive;
                calc_rr = graph_rr_adaptive;
                get_next_hop = get_next_hop_cam;
                set_mpaths = set_mpaths_adaptive;
                set_mpaths_reverse = set_mpaths_reverse_adaptive;
                if(vc_management == GRAPH_NODE_MANAGEMENT)
                    get_cam_port = get_cam_port_adaptive_node;
                else if(vc_management == GRAPH_PORT_MANAGEMENT)
                    get_cam_port = get_cam_port_adaptive_port;
                else if(vc_management == GRAPH_NODE_PORT_MANAGEMENT)
                    get_cam_port = get_cam_port_adaptive_node_port;

            }
            else panic("Unsupported port-selection policy");
            break;
        case VALIANT:
        case QUICK_VALIANT_DUAL:
        case QUICK_VALIANT_LOCAL:
        case QUICK_VALIANT_REMOTE:
		case QUICK_VALIANT_PRIVATE:
		case QUICK_VALIANT_QUASIPRIVATE:
            break;
        case SPANNING_TREE_ROUTING:
            break;
        default:
            panic("Unsupported routing");
    }

    switch (topo) {
        case MESH:
            neighbor = torus_neighbor;
            calc_rr = mesh_rr;
            break;
        case CIRC_PK:
            neighbor = circ_pk_neighbor;
            calc_rr = circ_pk_rr;
            break;
        case CIRCULANT:
            neighbor = circulant_neighbor;
            calc_rr = circulant_rr;
            break;
        case MIDIMEW:
            neighbor = midimew_neighbor;
            calc_rr = midimew_rr;
            break;
        case TORUS:
            neighbor = torus_neighbor;
            if (nways != 1) calc_rr = torus_rr;
            else calc_rr = torus_rr_unidir;
            break;
        case SPINNAKER:
            neighbor = spinnaker_neighbor;
            calc_rr = spinnaker_rr;
            break;
        case TWISTED:
            neighbor = dtt_neighbor;
            if (nways != 1) calc_rr = dtt_rr;
            else calc_rr = dtt_rr_unidir;
            break;
        case FATTREE:
            if (routing==ARITH_ROUTING){
                check_rr = check_rr_fattree_arithmetic;
				calc_rr = fattree_rr_arithmetic;
            }
            else if (routing==ADAPTIVE_ROUTING){
                check_rr = check_rr_fattree_adaptive;
				calc_rr = fattree_rr_arithmetic;
            }
			else if (routing==STATIC_ROUTING){
				if(vc_management == VOQ_MANAGEMENT){
					check_rr = check_rr_arbitrary_voq;
				} else {
					check_rr = check_rr_arbitrary_dummy;
				}
				calc_rr = fattree_rr;
			}
            break;
        case THINTREE:
            if (routing==ARITH_ROUTING){
                check_rr = check_rr_thintree_arithmetic;
				calc_rr = thintree_rr_arithmetic;
            }
            else if (routing==ADAPTIVE_ROUTING){
                check_rr = check_rr_thintree_adaptive;
				calc_rr = thintree_rr_arithmetic;
            }
			else {
				if(vc_management == VOQ_MANAGEMENT){
					check_rr = check_rr_arbitrary_voq;
				} else {
					check_rr = check_rr_arbitrary_dummy;
				}
				if (routing==STATIC_ROUTING){
					calc_rr = thintree_rr;
				} else if (routing==SRC_ROUTING){
					calc_rr = thintree_rr_src;
				} else if (routing==DST_ROUTING){
					calc_rr = thintree_rr_dst;
				} else if (routing==RANDOM_ROUTING){
					calc_rr = thintree_rr_rnd;
				}
			}
            break;
        case SLIMTREE:
            calc_rr = slimtree_rr_arithmetic;
            if (routing==STATIC_ROUTING)
                check_rr = check_rr_slimtree_arithmetic;
            else
                check_rr = check_rr_slimtree_adaptive;
            break;
        case SLIMFLY:
            // check_rr_slimfly_dally
            calc_rr = slimfly_rr;
            // check_rr = check_rr_slimfly_dally;
            break;
        case MEGAFLY:
            // check_rr_slimfly_dally
            calc_rr = megafly_rr;
            check_rr = check_rr_megafly_dally;
            break;
        case DRAGONFLY_ABSOLUTE:
        case DRAGONFLY_RELATIVE:
        case DRAGONFLY_CIRCULANT:
		case DRAGONFLY_NAUTILUS:
		case DRAGONFLY_HELIX:
		case DRAGONFLY_RND:
		case DRAGONFLY_OTHER:
			calc_rr = dragonfly_rr;
			if(vc_management == GRAPH_NODE_MANAGEMENT){
				check_rr = check_rr_arbitrary_node;
			} else if(vc_management == GRAPH_PORT_MANAGEMENT){
				check_rr = check_rr_arbitrary_port;
			} else if(vc_management == GRAPH_NODE_PORT_MANAGEMENT){
				check_rr = check_rr_arbitrary_node_port;
			} else if(vc_management == VOQ_MANAGEMENT){
				check_rr = check_rr_arbitrary_voq;
			} else if(vc_management == SPANNING_TREE_MANAGEMENT){
				calc_rr = spanning_tree_rr;
				check_rr = check_rr_spanning_tree;
			} else if(vc_management == DF_DALLY_MANAGEMENT){
				if (routing==VALIANT || routing==QUICK_VALIANT_PRIVATE || routing==QUICK_VALIANT_QUASIPRIVATE ||
					routing==QUICK_VALIANT_LOCAL || routing==QUICK_VALIANT_REMOTE || routing==QUICK_VALIANT_DUAL){
					escape_vcs=3; // Proxy routing requires an extra VC.
				} else
					escape_vcs=2;
				if (nchan<escape_vcs)
					perror("Not enough VCs for Dally VC management!");
				else if (nchan==escape_vcs)
					check_rr = check_rr_dragonfly_dally;
				else // nchan> scape_vcs, thus there are extra VCs; let's use adaptive vc management
					check_rr = check_rr_dragonfly_dally_adap;
			} else {//if(vc_management == GRAPH_DUMMY_MANAGEMENT){
				check_rr = check_rr_arbitrary_dummy;
			}
			break;
        case ICUBE:
            if (routing==STATIC_ROUTING){
                check_rr = check_rr_icube_static;
                calc_rr = icube_rr;
            } else if (routing==ADAPTIVE_ROUTING){
                check_rr = check_rr_icube_adaptive;
                calc_rr = icube_rr;
            } else {
                check_rr = check_rr_icube_static_IB;
            }
            neighbor = torus_neighbor;
            break;
        case RRG:
		case EXA:
        case GDBG:
        case KAUTZ:
            if(vc_management == GRAPH_NODE_MANAGEMENT){
                check_rr = check_rr_arbitrary_node;
            } else if(vc_management == GRAPH_PORT_MANAGEMENT){
                check_rr = check_rr_arbitrary_port;
            } else if(vc_management == GRAPH_NODE_PORT_MANAGEMENT){
                check_rr = check_rr_arbitrary_node_port;
            } else if(vc_management == VOQ_MANAGEMENT){
                check_rr = check_rr_arbitrary_voq;
            } else if(vc_management == GRAPH_INC_HOP_MANAGEMENT){
                check_rr = check_rr_arbitrary_inc_hop;
            } else if(vc_management == SPANNING_TREE_MANAGEMENT){
                calc_rr = spanning_tree_rr;
                check_rr = check_rr_spanning_tree;
            } else {
                check_rr = check_rr_arbitrary_dummy;
            }
            break;
        default:
            panic("Unsupported Topology");
    }

    switch (req_mode) {
        case BUBBLE_OBLIVIOUS_REQ:
            request_port = request_port_bubble_oblivious;
            break;
        case BUBBLE_ADAPTIVE_RANDOM_REQ:
            request_port = request_port_bubble_adaptive_random;
            break;
        case BUBBLE_ADAPTIVE_SHORTEST_REQ:
            request_port = request_port_bubble_adaptive_shortest;
            break;
        case BUBBLE_ADAPTIVE_SMART_REQ:
            request_port = request_port_bubble_adaptive_smart;
            break;
        case DOUBLE_OBLIVIOUS_REQ:
            request_port = request_port_double_oblivious;
            break;
        case DOUBLE_ADAPTIVE_REQ:
            request_port = request_port_double_adaptive;
            break;
        case HEXA_OBLIVIOUS_REQ:
            request_port = request_port_hexa_oblivious;
            break;
        case HEXA_ADAPTIVE_REQ:
            request_port = request_port_hexa_adaptive;
            break;
        case DALLY_TRC_REQ:
            request_port = request_port_dally_trc;
            break;
        case DALLY_BASIC_REQ:
            request_port = request_port_dally_basic;
            break;
        case DALLY_IMPROVED_REQ:
            request_port = request_port_dally_improved;
            break;
        case DALLY_ADAPTIVE_REQ:
            request_port = request_port_dally_adaptive;
            break;
#if (BIMODAL_SUPPORT != 0)
        case BIMODAL_REQ:
            request_port = request_port_bimodal_random;
            break;
#endif /* BIMODAL */
        case ARBITRARY_REQ:
            request_port = request_port_arbitrary;
            break;
        case ICUBE_REQ:
            if (routing==ICUBE_1M_ROUTING || routing==ICUBE_4M_ROUTING)
                request_port = request_port_icube_IB;
            else
                request_port = request_port_icube;
            break;
        default:
            panic("Unsupported request mode");
    }

    switch (cons_mode) {
        case SINGLE_CONS:
            consume = consume_single;
            arbitrate_cons = arbitrate_cons_single;
            break;
        case MULTIPLE_CONS:
            consume = consume_multiple;
            arbitrate_cons = arbitrate_cons_multiple;
            break;
        default:
            panic("Unsupported consumption mode");
    }

    switch (arb_mode) {
        case ROUNDROBIN_ARB:
            arbitrate_select = arbitrate_select_round_robin;
            break;
        case FIFO_ARB:
            arbitrate_select = arbitrate_select_fifo;
            break;
        case LONGEST_ARB:
            arbitrate_select = arbitrate_select_longest;
            break;
		case HIGHEST_ARB:
            arbitrate_select = arbitrate_select_highest;
            break;
        case RANDOM_ARB:
            arbitrate_select = arbitrate_select_random;
            break;
        case AGE_ARB:
            arbitrate_select = arbitrate_select_age;
            break;
        default:
            panic("Unsupported arbitration mode");
    }

    switch (inj_mode) {
        case SHORTEST_INJ:
            select_input_port = select_input_port_shortest;
            break;
        case DOR_INJ:
            select_input_port = select_input_port_dor_only;
            break;
        case DOR_SHORTEST_INJ:
            select_input_port = select_input_port_dor_shortest;
            break;
        case SHORTEST_PROFITABLE_INJ:
            select_input_port = select_input_port_shortest_profitable;
            break;
        case LONGEST_PATH_INJ:
            select_input_port = select_input_port_lpath;
            break;
        default:
            panic("Unsupported injection mode");
    }

    if (topo<DIRECT){
        data_movement = data_movement_direct;
        arbitrate = arbitrate_direct;
    } else {
        data_movement = data_movement_indirect;
        if (topo==ICUBE)
            arbitrate = arbitrate_icube;
        else
            arbitrate = arbitrate_arbitrary;
    }

	fflush(stdout);
}

void finish_functions(void){

    long i;

    free(batch);
    free(source_ports);
    free(dest_ports);
    free(port_utilization);

    if (plevel & 1) {
        for (i=0; i<nprocs; i++) {
            free(destinations[i]);
            free(sources[i]);
        }
        free(destinations);
        free(sources);
    }

    if (plevel & 4){
        free(inj_dst);
        free(con_dst);
    }
}
