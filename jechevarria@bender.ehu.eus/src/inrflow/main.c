/** @mainpage
  Copyright (2014)

  @author J. Navaridas.

  @section gnu License

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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "dynamic_engine.h"
#include "electric_engine.h"
#include "photonic_engine.h"

#include "node.h"
#include "misc.h"
#include "network.h"
#include "traffic.h"
#include "placement.h"
#include "workloads.h"
#include "globals.h"
#include "reporting.h"
#include "applications.h"
#include "scheduling.h"
#include "failures.h"
#include "io.h"
#include "list.h"

long r_seed;    ///< Random seed
int bfs_output; ///< Generate a bfs file with the topology? 0:no, other:yes
int mode; ///< Runtime mode: static or dynamic.

/**
 * Main function.
 *
 * Initializes network (and failures, if any).
 * Then routes among every pair of servers and captures statistics.
 * Finally writes the results to file and stdout.
 *
 * @param argc The number of parameters given in the command line.
 * @param argv Array that contains all the parameters.
 * @return The finalization code. Usually 0.
 */
int main(int argc, char **argv){

#ifdef DEBUG
    printf("DEBUG flag is on.\n");
#endif

    get_conf((long)(argc - 1), argv + 1);
    init_topology();
    srand(r_seed);
    init_topo(topo_nparam, topo_params);
    switch(mode){
        case STATIC:
        case DYNAMIC_ELECTRIC_FAST:
        case DYNAMIC_ELECTRIC_ACCURATE:
            construct_network_electric();
            break;
        case DYNAMIC_PHOTONIC:
            construct_network_photonic();
            break;
        default:
            printf("Execution mode not defined.\n");
            exit(-1);
    }

    reset_network();

    set_failures();

    if (bfs_output!=0)
        generate_bfs_file();

    switch(mode){
        case STATIC:
            init_patterns();
            run_static();
            break;
        case DYNAMIC_ELECTRIC_FAST:
            time_next_event = time_next_event_electric;
            insert_new_events = insert_new_events_electric;
            remove_flow = remove_flow_electric;
            min_links_bandwidth = min_links_bandwidth_fast_electric;
            run_dynamic();
            break;
        case DYNAMIC_ELECTRIC_ACCURATE:
            time_next_event = time_next_event_electric;
            insert_new_events = insert_new_events_electric;
            remove_flow = remove_flow_electric;
            min_links_bandwidth = min_links_bandwidth_accurate_electric;
            run_dynamic();
            break;
        case DYNAMIC_PHOTONIC:
            load_balancing = 0; // For now
            time_next_event = time_next_event_photonic;
            insert_new_events = insert_new_events_photonic;
            remove_flow = remove_flow_photonic;
            min_links_bandwidth = min_links_bandwidth_photonic;
            if(channel_assign_pol == STATIC_CHANNEL_ASSIGN &&
                    lambda_assign_pol == STATIC_LAMBDA_ASSIGN)
                explore_route = explore_route_static_static;
            else if(channel_assign_pol == ADAPTIVE_CHANNEL_ASSIGN &&
                  lambda_assign_pol == STATIC_LAMBDA_ASSIGN)
              explore_route = explore_route_adaptive_static;
            else if(channel_assign_pol == STATIC_CHANNEL_ASSIGN &&
                  lambda_assign_pol == ADAPTIVE_LAMBDA_ASSIGN)
              explore_route = explore_route_adaptive_static;
            else if(channel_assign_pol == ADAPTIVE_CHANNEL_ASSIGN &&
                  lambda_assign_pol == ADAPTIVE_LAMBDA_ASSIGN)
              explore_route = explore_route_adaptive_static;
            run_dynamic();
            break;
        default:
            printf("Execution mode not defined.\n");
            exit(-1);
    }
    finish_topo();
switch(mode){
        case STATIC:
        case DYNAMIC_ELECTRIC_FAST:
        case DYNAMIC_ELECTRIC_ACCURATE:
            release_network_electric();
            break;
        case DYNAMIC_PHOTONIC:
            release_network_photonic();
            break;
        default:
            printf("Execution mode not defined.\n");
            exit(-1);
    }

#ifdef DEBUG
    printf("DEBUG flag is on.\n");
#endif

    return(0);
}
