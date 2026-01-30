#include "topologies.h"
#include "../knkstar/knkstar.h"
#include "../ficonn/ficonn.h"
#include "../gdcficonn/gdcficonn.h"
#include "../swcube/swcube.h"
#include "../bcube/bcube.h"
#include "../hcnbcn/hcnbcn.h"
#include "../dpillar/dpillar.h"
#include "../fattree/fattree.h"
#include "../thintree/thintree.h"
#include "../gtree/gtree.h"
#include "../jellyfish/jellyfish.h"
#include "../torus/torus.h"
#include "../euroexa/euroexa.h"
#include "../euroexa/euroexa_tl.h"
#include "../exatorus/exatorus.h"
#include "../exatree/exatree.h"
#include "../exanest/nesttree.h"
#include "../exanest/nestghc.h"
#include "../dragonfly/dragonfly.h"
#include "../megafly/megafly.h"

// Could use directly the defined values but will be useful in the future when accepting them as a parameter.
topo_t topo;    ///< the topology we are modeling
long topo_nparam;   ///< Number of parameters passed to the topology
long topo_params[MAX_TOPO_PARAMS];  ///< parameters passed to the topology

routing_t routing;  ///< The routing function we are using
long routing_nparam;    ///< Number of parameters passed to the routing
long routing_params[MAX_ROUTING_PARAMS];    ///< Parameters passed to the routing

/** Initializes the virtual functions to point to the ones needed, based on the input parameters.
*/
void init_topology(){

    switch(topo) {
        case FICONN:
            init_topo=init_topo_ficonn;
            finish_topo=finish_topo_ficonn;
            get_servers=get_servers_ficonn;
            get_switches=get_switches_ficonn;
            get_ports=get_ports_ficonn;
            is_server=is_server_ficonn;
            get_server_i=get_server_i_ficonn;
            get_switch_i=get_switch_i_ficonn;
            node_to_server=node_to_server_ficonn;
            node_to_switch=node_to_switch_ficonn;
            get_radix=get_radix_ficonn;
            connection=connection_ficonn;
            init_routing=init_routing_ficonn;
            finish_route=finish_route_ficonn;
            route=route_ficonn;
            get_n_paths_routing=get_n_paths_routing_ficonn;
            get_network_token=get_network_token_ficonn;
            get_routing_token=get_routing_token_ficonn;
            get_topo_version=get_topo_version_ficonn;
            get_topo_param_tokens=get_topo_param_tokens_ficonn;
            get_filename_params=get_filename_params_ficonn;
            break;
        case HCNBCN:
            init_topo=init_topo_hcnbcn;
            finish_topo=finish_topo_hcnbcn;
            get_servers=get_servers_hcnbcn;
            get_switches=get_switches_hcnbcn;
            get_ports=get_ports_hcnbcn;
            is_server=is_server_hcnbcn;
            get_server_i=get_server_i_hcnbcn;
            get_switch_i=get_switch_i_hcnbcn;
            node_to_server=node_to_server_hcnbcn;
            node_to_switch=node_to_switch_hcnbcn;
            get_radix=get_radix_hcnbcn;
            connection=connection_hcnbcn;

            init_routing=init_routing_hcnbcn;
            finish_route=finish_route_hcnbcn;
            route=route_hcnbcn;
            get_n_paths_routing=get_n_paths_routing_hcnbcn;

            get_network_token=get_network_token_hcnbcn;
            get_routing_token=get_routing_token_hcnbcn;
            get_topo_version=get_topo_version_hcnbcn;
            get_topo_param_tokens=get_topo_param_tokens_hcnbcn;
            get_filename_params=get_filename_params_hcnbcn;
            get_routing_param_tokens=get_routing_param_tokens_hcnbcn;

            get_topo_nstats=get_topo_nstats_hcnbcn;
            get_topo_key_value=get_topo_key_value_hcnbcn;
            get_topo_nhists=get_topo_nhists_hcnbcn;
            get_topo_hist_prefix=get_topo_hist_prefix_hcnbcn;
            get_topo_hist_doc=get_topo_hist_doc_hcnbcn;
            get_topo_hist_max=get_topo_hist_max_hcnbcn;
            get_topo_hist=get_topo_hist_hcnbcn;
            break;
        case GDCFICONN:
            init_topo=init_topo_gdcficonn;
            finish_topo=finish_topo_gdcficonn;
            get_servers=get_servers_gdcficonn;
            get_switches=get_switches_gdcficonn;
            get_ports=get_ports_gdcficonn;
            is_server=is_server_gdcficonn;
            get_server_i=get_server_i_gdcficonn;
            get_switch_i=get_switch_i_gdcficonn;
            node_to_server=node_to_server_gdcficonn;
            node_to_switch=node_to_switch_gdcficonn;
            get_radix=get_radix_gdcficonn;
            connection=connection_gdcficonn;

            init_routing=init_routing_gdcficonn;
            finish_route=finish_route_gdcficonn;
            route=route_gdcficonn;
            get_n_paths_routing=get_n_paths_routing_gdcficonn;

            get_network_token=get_network_token_gdcficonn;
            get_routing_token=get_routing_token_gdcficonn;
            get_topo_version=get_topo_version_gdcficonn;
            get_topo_param_tokens=get_topo_param_tokens_gdcficonn;
            get_filename_params=get_filename_params_gdcficonn;
            get_routing_param_tokens=get_routing_param_tokens_gdcficonn;
            get_topo_nstats=get_topo_nstats_gdcficonn;
            get_topo_key_value=get_topo_key_value_gdcficonn;
            get_topo_nhists=get_topo_nhists_gdcficonn;
            get_topo_hist_prefix=get_topo_hist_prefix_gdcficonn;
            get_topo_hist_doc=get_topo_hist_doc_gdcficonn;
            get_topo_hist_max=get_topo_hist_max_gdcficonn;
            get_topo_hist=get_topo_hist_gdcficonn;

            break;
        case KNKSTAR:
            init_topo=init_topo_knkstar;
            finish_topo=finish_topo_knkstar;
            get_servers=get_servers_knkstar;
            get_switches=get_switches_knkstar;
            get_ports=get_ports_knkstar;
            is_server=is_server_knkstar;
            get_server_i=get_server_i_knkstar;
            get_switch_i=get_switch_i_knkstar;
            node_to_server=node_to_server_knkstar;
            node_to_switch=node_to_switch_knkstar;
            get_radix=get_radix_knkstar;
            connection=connection_knkstar;
            init_routing=init_routing_knkstar;
            finish_route=finish_route_knkstar;
            route=route_knkstar;
            get_n_paths_routing=get_n_paths_routing_knkstar;
            get_network_token=get_network_token_knkstar;
            get_routing_token=get_routing_token_knkstar;
            get_topo_version=get_topo_version_knkstar;
            get_topo_param_tokens=get_topo_param_tokens_knkstar;
            get_filename_params=get_filename_params_knkstar;
            break;
        case DPILLAR:
            init_topo=init_topo_dpillar;
            finish_topo=finish_topo_dpillar;
            get_servers=get_servers_dpillar;
            get_switches=get_switches_dpillar;
            get_ports=get_ports_dpillar;
            is_server=is_server_dpillar;
            get_server_i=get_server_i_dpillar;
            get_switch_i=get_switch_i_dpillar;
            node_to_server=node_to_server_dpillar;
            node_to_switch=node_to_switch_dpillar;
            get_radix=get_radix_dpillar;
            connection=connection_dpillar;
            init_routing=init_routing_dpillar;
            finish_route=finish_route_dpillar;
            get_n_paths_routing=get_n_paths_routing_dpillar;
            route=route_dpillar;
            get_network_token=get_network_token_dpillar;
            get_routing_token=get_routing_token_dpillar;
            get_topo_version=get_topo_version_dpillar;
            get_topo_param_tokens=get_topo_param_tokens_dpillar;
            get_filename_params=get_filename_params_dpillar;
            break;
        case SWCUBE:
            init_topo=init_topo_swcube;
            finish_topo=finish_topo_swcube;
            get_servers=get_servers_swcube;
            get_switches=get_switches_swcube;
            get_ports=get_ports_swcube;
            is_server=is_server_swcube;
            get_server_i=get_server_i_swcube;
            get_switch_i=get_switch_i_swcube;
            node_to_server=node_to_server_swcube;
            node_to_switch=node_to_switch_swcube;
            get_radix=get_radix_swcube;
            connection=connection_swcube;
            init_routing=init_routing_swcube;
            finish_route=finish_route_swcube;
            get_n_paths_routing=get_n_paths_routing_swcube;
            route=route_swcube;
            get_network_token=get_network_token_swcube;
            get_routing_token=get_routing_token_swcube;
            get_topo_version=get_topo_version_swcube;
            get_topo_param_tokens=get_topo_param_tokens_swcube;
            get_filename_params=get_filename_params_swcube;
            break;
        case BCUBE:
            init_topo=init_topo_bcube;
            finish_topo=finish_topo_bcube;
            get_servers=get_servers_bcube;
            get_switches=get_switches_bcube;
            get_ports=get_ports_bcube;
            is_server=is_server_bcube;
            get_server_i=get_server_i_bcube;
            get_switch_i=get_switch_i_bcube;
            node_to_server=node_to_server_bcube;
            node_to_switch=node_to_switch_bcube;
            get_radix=get_radix_bcube;
            connection=connection_bcube;
            init_routing=init_routing_bcube;
            finish_route=finish_route_bcube;
            get_n_paths_routing=get_n_paths_routing_bcube;
            route=route_bcube;
            get_network_token=get_network_token_bcube;
            get_routing_token=get_routing_token_bcube;
            get_topo_version=get_topo_version_bcube;
            get_topo_param_tokens=get_topo_param_tokens_bcube;
            get_filename_params=get_filename_params_bcube;
            break;
        case EXATORUS:
            init_topo=init_topo_exatorus;
            finish_topo=finish_topo_exatorus;
            get_servers=get_servers_exatorus;
            get_switches=get_switches_exatorus;
            get_ports=get_ports_exatorus;
            is_server=is_server_exatorus;
            get_server_i=get_server_i_exatorus;
            get_switch_i=get_switch_i_exatorus;
            node_to_server=node_to_server_exatorus;
            node_to_switch=node_to_switch_exatorus;
            get_radix=get_radix_exatorus;
            connection=connection_exatorus;
            init_routing=init_routing_exatorus;
            finish_route=finish_route_exatorus;
            get_n_paths_routing=get_n_paths_routing_exatorus;
            route=route_exatorus;
            get_network_token=get_network_token_exatorus;
            get_routing_token=get_routing_token_exatorus;
            get_routing_param_tokens=get_routing_param_tokens_exatorus;
            get_topo_version=get_topo_version_exatorus;
            get_topo_param_tokens=get_topo_param_tokens_exatorus;
            get_filename_params=get_filename_params_exatorus;
            break;
        case EXATREE:
            init_topo=init_topo_exatree;
            finish_topo=finish_topo_exatree;
            get_servers=get_servers_exatree;
            get_switches=get_switches_exatree;
            get_ports=get_ports_exatree;
            is_server=is_server_exatree;
            get_server_i=get_server_i_exatree;
            get_switch_i=get_switch_i_exatree;
            node_to_server=node_to_server_exatree;
            node_to_switch=node_to_switch_exatree;
            get_radix=get_radix_exatree;
            connection=connection_exatree;
            init_routing=init_routing_exatree;
            finish_route=finish_route_exatree;
            get_n_paths_routing=get_n_paths_routing_exatree;
            route=route_exatree;
            get_network_token=get_network_token_exatree;
            get_routing_token=get_routing_token_exatree;
            get_routing_param_tokens=get_routing_param_tokens_exatree;
            get_topo_version=get_topo_version_exatree;
            get_topo_param_tokens=get_topo_param_tokens_exatree;
            get_filename_params=get_filename_params_exatree;
            break;
        case EUROEXA_BASE:
        case EUROEXA_SINGLE:
        case EUROEXA_RND:
        case EUROEXA_RS:
        case EUROEXA_MULTI:
            init_topo=init_topo_euroexa;
            finish_topo=finish_topo_euroexa;
            get_servers=get_servers_euroexa;
            get_switches=get_switches_euroexa;
            get_ports=get_ports_euroexa;
            is_server=is_server_euroexa;
            get_server_i=get_server_i_euroexa;
            get_switch_i=get_switch_i_euroexa;
            node_to_server=node_to_server_euroexa;
            node_to_switch=node_to_switch_euroexa;
            get_radix=get_radix_euroexa;
            connection=connection_euroexa;
            init_routing=init_routing_euroexa;
            finish_route=finish_route_euroexa;
            get_n_paths_routing=get_n_paths_routing_euroexa;
            route=route_euroexa;
            get_network_token=get_network_token_euroexa;
            get_routing_token=get_routing_token_euroexa;
            get_routing_param_tokens=get_routing_param_tokens_euroexa;
            get_topo_version=get_topo_version_euroexa;
            get_topo_param_tokens=get_topo_param_tokens_euroexa;
            get_filename_params=get_filename_params_euroexa;
            break;
        case EUROEXA_TL:
            init_topo=init_topo_euroexa_tl;
            finish_topo=finish_topo_euroexa_tl;
            get_servers=get_servers_euroexa_tl;
            get_switches=get_switches_euroexa_tl;
            get_ports=get_ports_euroexa_tl;
            is_server=is_server_euroexa_tl;
            get_server_i=get_server_i_euroexa_tl;
            get_switch_i=get_switch_i_euroexa_tl;
            node_to_server=node_to_server_euroexa_tl;
            node_to_switch=node_to_switch_euroexa_tl;
            get_radix=get_radix_euroexa_tl;
            connection=connection_euroexa_tl;
            init_routing=init_routing_euroexa_tl;
            finish_route=finish_route_euroexa_tl;
            get_n_paths_routing=get_n_paths_routing_euroexa_tl;
            route=route_euroexa_tl;
            get_network_token=get_network_token_euroexa_tl;
            get_routing_token=get_routing_token_euroexa_tl;
            get_routing_param_tokens=get_routing_param_tokens_euroexa_tl;
            get_topo_version=get_topo_version_euroexa_tl;
            get_topo_param_tokens=get_topo_param_tokens_euroexa_tl;
            get_filename_params=get_filename_params_euroexa_tl;
            break;
        case NESTTREE:
            init_topo=init_topo_nesttree;
            finish_topo=finish_topo_nesttree;
            get_servers=get_servers_nesttree;
            get_switches=get_switches_nesttree;
            get_ports=get_ports_nesttree;
            is_server=is_server_nesttree;
            get_server_i=get_server_i_nesttree;
            get_switch_i=get_switch_i_nesttree;
            node_to_server=node_to_server_nesttree;
            node_to_switch=node_to_switch_nesttree;
            get_radix=get_radix_nesttree;
            connection=connection_nesttree;
            init_routing=init_routing_nesttree;
            finish_route=finish_route_nesttree;
            get_n_paths_routing=get_n_paths_routing_nesttree;
            route=route_nesttree;
            get_network_token=get_network_token_nesttree;
            get_routing_token=get_routing_token_nesttree;
            get_routing_param_tokens=get_routing_param_tokens_nesttree;
            get_topo_version=get_topo_version_nesttree;
            get_topo_param_tokens=get_topo_param_tokens_nesttree;
            get_filename_params=get_filename_params_nesttree;
            break;
        case NESTGHC:
            init_topo=init_topo_nestghc;
            finish_topo=finish_topo_nestghc;
            get_servers=get_servers_nestghc;
            get_switches=get_switches_nestghc;
            get_ports=get_ports_nestghc;
            is_server=is_server_nestghc;
            get_server_i=get_server_i_nestghc;
            get_switch_i=get_switch_i_nestghc;
            node_to_server=node_to_server_nestghc;
            node_to_switch=node_to_switch_nestghc;
            get_radix=get_radix_nestghc;
            connection=connection_nestghc;
            init_routing=init_routing_nestghc;
            finish_route=finish_route_nestghc;
            get_n_paths_routing=get_n_paths_routing_nestghc;
            route=route_nestghc;
            get_network_token=get_network_token_nestghc;
            get_routing_token=get_routing_token_nestghc;
            get_routing_param_tokens=get_routing_param_tokens_nestghc;
            get_topo_version=get_topo_version_nestghc;
            get_topo_param_tokens=get_topo_param_tokens_nestghc;
            get_filename_params=get_filename_params_nestghc;
            break;
        case TORUS:
            init_topo=init_topo_torus;
            finish_topo=finish_topo_torus;
            get_servers=get_servers_torus;
            get_switches=get_switches_torus;
            get_ports=get_ports_torus;
            is_server=is_server_torus;
            get_server_i=get_server_i_torus;
            get_switch_i=get_switch_i_torus;
            node_to_server=node_to_server_torus;
            node_to_switch=node_to_switch_torus;
            get_radix=get_radix_torus;
            connection=connection_torus;
            init_routing=init_routing_torus;
            finish_route=finish_route_torus;
            get_n_paths_routing=get_n_paths_routing_torus;
            route=route_torus;
            get_network_token=get_network_token_torus;
            get_routing_token=get_routing_token_torus;
            get_routing_param_tokens=get_routing_param_tokens_torus;
            get_topo_version=get_topo_version_torus;
            get_topo_param_tokens=get_topo_param_tokens_torus;
            get_filename_params=get_filename_params_torus;
            break;
        case MESH:
            init_topo=init_topo_torus;
            finish_topo=finish_topo_torus;
            get_servers=get_servers_torus;
            get_switches=get_switches_torus;
            get_ports=get_ports_torus;
            is_server=is_server_torus;
            get_server_i=get_server_i_torus;
            get_switch_i=get_switch_i_torus;
            node_to_server=node_to_server_torus;
            node_to_switch=node_to_switch_torus;
            get_radix=get_radix_torus;
            connection=connection_torus;
            init_routing=init_routing_mesh;
            finish_route=finish_route_torus;
            get_n_paths_routing=get_n_paths_routing_torus;
            route=route_torus;
            get_network_token=get_network_token_torus;
            get_routing_token=get_routing_token_torus;
            get_topo_version=get_topo_version_torus;
            get_topo_param_tokens=get_topo_param_tokens_torus;
            get_filename_params=get_filename_params_torus;
            break;
        case FATTREE:
            init_topo=init_topo_fattree;
            finish_topo=finish_topo_fattree;
            get_servers=get_servers_fattree;
            get_switches=get_switches_fattree;
            get_ports=get_ports_fattree;
            is_server=is_server_fattree;
            get_server_i=get_server_i_fattree;
            get_switch_i=get_switch_i_fattree;
            node_to_server=node_to_server_fattree;
            node_to_switch=node_to_switch_fattree;
            get_radix=get_radix_fattree;
            connection=connection_fattree;
            init_routing=init_routing_fattree;
            finish_route=finish_route_fattree;
            get_n_paths_routing=get_n_paths_routing_fattree;
            route=route_fattree;
            get_network_token=get_network_token_fattree;
            get_routing_token=get_routing_token_fattree;
            get_routing_param_tokens=get_routing_param_tokens_fattree;
            get_topo_version=get_topo_version_fattree;
            get_topo_param_tokens=get_topo_param_tokens_fattree;
            get_filename_params=get_filename_params_fattree;
            break;
        case THINTREE:
            init_topo=init_topo_thintree;
            finish_topo=finish_topo_thintree;
            get_servers=get_servers_thintree;
            get_switches=get_switches_thintree;
            get_ports=get_ports_thintree;
            is_server=is_server_thintree;
            get_server_i=get_server_i_thintree;
            get_switch_i=get_switch_i_thintree;
            node_to_server=node_to_server_thintree;
            node_to_switch=node_to_switch_thintree;
            get_radix=get_radix_thintree;
            connection=connection_thintree;
            init_routing=init_routing_thintree;
            finish_route=finish_route_thintree;
            get_n_paths_routing=get_n_paths_routing_thintree;
            route=route_thintree;
            get_network_token=get_network_token_thintree;
            get_routing_token=get_routing_token_thintree;
            get_routing_param_tokens=get_routing_param_tokens_thintree;
            get_topo_version=get_topo_version_thintree;
            get_topo_param_tokens=get_topo_param_tokens_thintree;
            get_filename_params=get_filename_params_thintree;
            break;
        case GTREE:
            init_topo=init_topo_gtree;
            finish_topo=finish_topo_gtree;
            get_servers=get_servers_gtree;
            get_switches=get_switches_gtree;
            get_ports=get_ports_gtree;
            is_server=is_server_gtree;
            get_server_i=get_server_i_gtree;
            get_switch_i=get_switch_i_gtree;
            node_to_server=node_to_server_gtree;
            node_to_switch=node_to_switch_gtree;
            get_radix=get_radix_gtree;
            connection=connection_gtree;
            init_routing=init_routing_gtree;
            finish_route=finish_route_gtree;
            get_n_paths_routing=get_n_paths_routing_gtree;
            route=route_gtree;
            get_network_token=get_network_token_gtree;
            get_routing_token=get_routing_token_gtree;
            get_routing_param_tokens=get_routing_param_tokens_gtree;
            get_topo_version=get_topo_version_gtree;
            get_topo_param_tokens=get_topo_param_tokens_gtree;
            get_filename_params=get_filename_params_gtree;
            break;
        case JELLYFISH:
            init_topo=init_topo_jellyfish;
            finish_topo=finish_topo_jellyfish;
            get_servers=get_servers_jellyfish;
            get_switches=get_switches_jellyfish;
            get_ports=get_ports_jellyfish;
            is_server=is_server_jellyfish;
            get_server_i=get_server_i_jellyfish;
            get_switch_i=get_switch_i_jellyfish;
            node_to_server=node_to_server_jellyfish;
            node_to_switch=node_to_switch_jellyfish;
            get_radix=get_radix_jellyfish;
            connection=connection_jellyfish;
            init_routing=init_routing_jellyfish;
            finish_route=finish_route_jellyfish;
            get_n_paths_routing=get_n_paths_routing_jellyfish;
            route=route_jellyfish;
            get_network_token=get_network_token_jellyfish;
            get_routing_token=get_routing_token_jellyfish;
            get_topo_version=get_topo_version_jellyfish;
            get_topo_param_tokens=get_topo_param_tokens_jellyfish;
            get_routing_param_tokens=get_routing_param_tokens_jellyfish;
            get_filename_params=get_filename_params_jellyfish;
            break;
        case DRAGONFLY_ABSOLUTE:
        case DRAGONFLY_RELATIVE:
        case DRAGONFLY_CIRCULANT:
        case DRAGONFLY_NAUTILUS:
        case DRAGONFLY_HELIX:
            init_topo=init_topo_dragonfly;
            finish_topo=finish_topo_dragonfly;
            get_servers=get_servers_dragonfly;
            get_switches=get_swithes_dragonfly;
            get_ports=get_ports_dragonfly;
            is_server=is_server_dragonfly;
            get_server_i=get_server_i_dragonfly;
            get_switch_i=get_switch_i_dragonfly;
            node_to_server=node_to_server_dragonfly;
            node_to_switch=node_to_switch_dragonfly;
            get_radix=get_radix_dragonfly;
            connection=connection_dragonfly;
            init_routing=init_routing_dragonfly;
            finish_route=finish_route_dragonfly;
            get_n_paths_routing=get_n_paths_routing_dragonfly;
            route=route_dragonfly;
            get_network_token=get_network_token_dragonfly;
            get_routing_token=get_routing_token_dragonfly;
            get_topo_version=get_topo_version_dragonfly;
            get_topo_param_tokens=get_topo_param_tokens_dragonfly;
            get_routing_param_tokens=get_routing_param_tokens_dragonfly;
            get_filename_params=get_filename_params_dragonfly;
            break;
        case MEGAFLY:
            init_topo=init_topo_megafly;
            finish_topo=finish_topo_megafly;
            get_servers=get_servers_megafly;
            get_switches=get_swithes_megafly;
            get_ports=get_ports_megafly;
            is_server=is_server_megafly;
            get_server_i=get_server_i_megafly;
            get_switch_i=get_switch_i_megafly;
            node_to_server=node_to_server_megafly;
            node_to_switch=node_to_switch_megafly;
            get_radix=get_radix_megafly;
            connection=connection_megafly;
            init_routing=init_routing_megafly;
            finish_route=finish_route_megafly;
            get_n_paths_routing=get_n_paths_routing_megafly;
            route=route_megafly;
            get_network_token=get_network_token_megafly;
            get_routing_token=get_routing_token_megafly;
            get_topo_version=get_topo_version_megafly;
            get_topo_param_tokens=get_topo_param_tokens_megafly;
            get_routing_param_tokens=get_routing_param_tokens_megafly;
            get_filename_params=get_filename_params_megafly;
            break;
        default:
            break;
    }
}

