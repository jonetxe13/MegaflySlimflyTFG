/** @mainpage
 * Jellyfish topology
 */

#include <stdio.h>
#include <stdlib.h>
#include "jellyfish.h"
#include "rg_gen.h"
#include "routing_jellyfish.h"
#include "../inrflow/dynamic_engine.h"
#include "../inrflow/globals.h"

static long param_n;
static long param_k;
static long param_r;
static long param_seed;
static long param_routing_type;
static long param_routing_k;

static long src_aux;
static long dst_aux;
static long n_path;
static long c_path;
long n_kpaths;

static long servers;   ///< The total number of servers
static long switches;  ///< The total number of switches
static long ports_network;	///< The total number of links
static long total_ports;
static long ports_switches;
long ports_servers;

static char *network_token="jellyfish";
static char routing_token[50];
static char *topo_version="v0.2";
static char *topo_param_tokens[4]= {"n", "k", "r", "seed"};
static char *routing_param_tokens[3]= {"k1", "k2", "k3"};

extern char filename_params[100];

graph_t *random_graph;
k_paths **routing_table_jellyfish;

/**
 * Initializes the topology and sets the parameters n & k & r.
 */
long init_topo_jellyfish(long np, long *par)
{

    long i,j;

    if (np < 4) {
        printf("4 parameters are needed for JELLYFISH <n, k, r, seed>.\n");
        exit(-1);
    }
    param_n = par[0];
    param_k = par[1];
    param_r = par[2];
    param_seed = par[3];
    if (param_r < 3) {
        printf("The number of ports must be higher than 2.\n");
        exit(-1);
    }
    sprintf(filename_params,"n%ldk%ldr%lds%ld",param_n,param_k, param_r, param_seed);
    servers = param_n * (param_k - param_r);
    switches = param_n;
    ports_network = (param_n * param_k) + (param_n * param_r);
    total_ports = param_k;
    ports_switches = param_r;
    ports_servers = param_k - param_r;
    random_graph = malloc(sizeof(graph_t) * switches);

    for(i = 0; i < switches; i++) {
        random_graph[i].nedges = get_radix_jellyfish(i + servers) - ports_servers;
        random_graph[i].fedges = random_graph[i].nedges;
        random_graph[i].active = 1;
        random_graph[i].edge = malloc(sizeof(edge_t) * random_graph[i].nedges);
        for(j = 0; j < random_graph[i].nedges; j++) {
            random_graph[i].edge[j].neighbour.node = -1;
            random_graph[i].edge[j].neighbour.edge = -1;
            random_graph[i].edge[j].active = 1;
        }
    }
    generate(random_graph, switches, ports_switches, param_seed);
    //load_graph_from_file(random_graph, "graphs4/g8_106_13_0.25");
    //export_graph_edges(random_graph,switches, filename_params);
    //export_graph_adjacency(random_graph,switches);
    generate_routing_table(ports_switches, ports_servers);
    return(1);
}

/**
 * Releases the resources used by jellyfish (random graph and routing table).
 */
void finish_topo_jellyfish()
{

    long i;

    destroy_routing_table();

    for(i = 0; i < switches; i++) {
        free(random_graph[i].edge);
    }
    free(random_graph);
}


/**
 * Get the number of servers of the network.
 */
long get_servers_jellyfish(){

    return servers;
}

/**
 * Get the number of switches of the network
 */
long get_switches_jellyfish(){

    return switches;
}

/**
 * Get the number of ports of the network
 */
long get_ports_jellyfish(){

    return ports_network;
}

/**
 * Get the number of ports of a given node (either a server or a switch, see above)
 */
long get_radix_jellyfish(long i){

    if(i < servers)
        return 1;
    else
        return param_k;
}

/**
 * Checks if a node is a server.
 */
long is_server_jellyfish(long i)
{
    if(i < servers)
        return 1;
    else
        return 0;
}

/**
 * Get the id of a server.
 */
long get_server_i_jellyfish(long i)
{

    return i;
}

/**
 * Get the id of a switch.
 */
long get_switch_i_jellyfish(long i)
{

    return i + servers;
}

long node_to_server_jellyfish(long i)
{
    return(i);
}

long node_to_switch_jellyfish(long i)
{
    return(i-servers);
}

/**
 * Calculates connections of the network.
 */
tuple_t connection_jellyfish(long node, long port)
{

    long node_aux, port_aux;
    tuple_t res;

    if(node < servers) {
        res.node = (node / ports_servers) + servers;
        res.port = node % ports_servers;
    } else if(port < ports_servers) {
        res.node = ((node - servers) * ports_servers) + port;
        res.port = 0;
    } else {
        node_aux = random_graph[node - servers].edge[port-ports_servers].neighbour.node;
        port_aux = random_graph[node - servers].edge[port-ports_servers].neighbour.edge;
        if(node_aux != -1 || port_aux != -1) {
            res.node = node_aux + servers;
            res.port = port_aux + ports_servers;
        } else {
            res.node = node_aux;
            res.port = port_aux;
        }
    }

    return(res);
}

/**
 * Get the network type.
 */
char * get_network_token_jellyfish()
{

    return(network_token);
}

/**
 * Get the routing token.
 */
char * get_routing_token_jellyfish(){

    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
            snprintf(routing_token, 50, "sp");
            break;
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
            snprintf(routing_token, 50, "ksp-%ld",routing_params[0]);
            break;
        case JELLYFISH_ECMP_ROUTING:
            snprintf(routing_token, 50, "ecmp");
            break;
        case JELLYFISH_LLSKR_ROUTING:
            snprintf(routing_token, 50, "llskr-%ld-%ld-%ld",routing_params[0],routing_params[1],routing_params[2]);
            break;
        default:
            printf("Unknown routing %ld", param_routing_type);
            exit(-1);
    }

    return(routing_token);
}

char *get_routing_param_tokens_jellyfish(long i){

    return routing_param_tokens[i];
}

/**
 * Get the version of the topology.
 */
char * get_topo_version_jellyfish(){

    return(topo_version);
}

/**
 * Get the parameters of the topology.
 */
char * get_topo_param_tokens_jellyfish(long i)
{
    return(topo_param_tokens[i]);
}

/**
 * Get the filename in which return the results..
 */
char * get_filename_params_jellyfish()
{
    return(filename_params);
}

/**
* Get the number of paths between a source and a destination.
* @return the number of paths.
*/
long get_n_paths_routing_jellyfish(long src, long dst){

    long n_paths;
    long src_aux = (src / ports_servers);
    long dst_aux = (dst / ports_servers);

    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
        case JELLYFISH_ECMP_ROUTING:
            n_paths = routing_table_jellyfish[src_aux][dst_aux].n_paths;
            break;
        case JELLYFISH_LLSKR_ROUTING:
            n_paths = n_kpaths;
            break;
        default:
            printf("Unknown routing.\n");
            exit(-1);
    }
    return(n_paths);
}

/**
 * Init the routing each time a flow has to be send (Not required).
 */
long init_routing_jellyfish(long src, long dst, long app_id){

    long dst2_aux, dist;
    long i = 0;
    long inj = -1;
    long max, n_path_aux;

    //printf("%ld -- > %ld\n", src, dst);
    src_aux = (src / ports_servers);
    dst_aux = (dst / ports_servers);
    dst2_aux = (dst % ports_servers);
    c_path = 1;

    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
            n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
            routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[0] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
            inj = 0;
            break;
        case JELLYFISH_ECMP_ROUTING:
            if(1){
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[0] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                inj = n_path;
            }
            else if(routing_params[0] == 0 && routing_params[1] == 1){
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    //printf("(%ld --> %ld) (%ld --> %ld) NPATH: %ld PSERVERS: %ld NPATHS: %ld APPID: %ld NAPPS: %ld APPID2:: %ld -- \n", src_aux,dst_aux,src,dst,n_path, ports_servers,  routing_table_jellyfish[src_aux][dst_aux].n_paths, app_id, routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 2], routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 3]);
                    max = max_route_dynamic(src, dst);
                    c_path = 1;
                    if( ((max > 0) && (app_id == max))
                            || (max == 0)){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else if( (max > 0) && (app_id != max)){
                        n_path_aux = n_path;
                        i++;
                    }
                    else{
                        n_path = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
                if( inj == -1){
                    routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((n_path_aux + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                    inj = n_path_aux;
                }
                //printf("## %ld ## ----------------------------------------------------\n", inj);

            }
            else if(routing_params[0] == 1 && routing_params[1] == 0){
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    if( ((network[src].port[0].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1])) && ((network[dst].port[dst2_aux].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
            }
            else if(routing_params[0] == 1 && routing_params[1] == 1){
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    max = max_route_dynamic(src, dst);
                    c_path = 1;
                    if((((max > 0) && (app_id == max)) && ((network[src].port[0].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))  && ((network[dst].port[dst2_aux].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))) || (max == 0)){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
            }

            break;
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
            if(routing_params[1] == 0 && routing_params[2] == 0){
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[0] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                inj = n_path;
            }
            else if(routing_params[1] == 0 && routing_params[2] == 1){
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    //printf("(%ld --> %ld) (%ld --> %ld) NPATH: %ld PSERVERS: %ld NPATHS: %ld APPID: %ld NAPPS: %ld APPID2:: %ld -- \n", src_aux,dst_aux,src,dst,n_path, ports_servers,  routing_table_jellyfish[src_aux][dst_aux].n_paths, app_id, routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 2], routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 3]);
                    max = max_route_dynamic(src, dst);
                    c_path = 1;
                    if( ((max > 0) && (app_id == max))
                            || (max == 0)){

                        routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
                //printf("## %ld ## ----------------------------------------------------\n", inj);

            }
            else if(routing_params[1] == 1 && routing_params[2] == 0){
                dist =  routing_table_jellyfish[src_aux][dst_aux].paths[0][0];
                if(dist == 2){
                    dist = 3;
                }
                //printf("(%ld --> %ld) Dist: %ld \n", src_aux, dst_aux, dist);
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[0] + 1) % (dist - 2));
                inj = n_path;
                /*
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    if( ((network[src].port[0].flows) >= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1])) || ((network[dst].port[dst2_aux].flows) >= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
            */
            }
            else if(routing_params[1] == 1 && routing_params[2] == 1){
                n_path = routing_table_jellyfish[src_aux][dst_aux].last_used[0];
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    max = max_route_dynamic(src, dst);
                    c_path = 1;

                    if((((max > 0) && (app_id == max)) && ((network[src].port[0].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))  && ((network[dst].port[dst2_aux].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))) || (max == 0)){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[0] = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((n_path + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
            }
            break;
        case JELLYFISH_LLSKR_ROUTING:
            if(routing_params[3] == 0 && routing_params[4] == 0){
                n_path = ((dst2_aux * n_kpaths) + routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux]) % routing_table_jellyfish[src_aux][dst_aux].n_paths;
                routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                inj = 0;
            }
            else if(routing_params[3] == 0 && routing_params[4] == 1){
                n_path = ((dst2_aux * n_kpaths) + routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux]) % routing_table_jellyfish[src_aux][dst_aux].n_paths;
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){

                    max = max_route_dynamic(src, dst);
                    c_path = 1;
                    if( ((max > 0) && (app_id == max)) || (max == 0)){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
            }
            else if(routing_params[3] == 1 && routing_params[4] == 0){
                n_path = ((dst2_aux * n_kpaths) + routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux]) % routing_table_jellyfish[src_aux][dst_aux].n_paths;
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    if( ((network[src].port[0].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1])) && ((network[dst].port[dst2_aux].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }
                }
            }

            else if(routing_params[3] == 1 && routing_params[4] == 1){
                n_path = ((dst2_aux * n_kpaths) + routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux]) % routing_table_jellyfish[src_aux][dst_aux].n_paths;
                while(i < routing_table_jellyfish[src_aux][dst_aux].n_paths){
                    max = max_route_dynamic(src, dst);
                    c_path = 1;

                    if((((max > 0) && (app_id == max)) && ((network[src].port[0].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))  && ((network[dst].port[dst2_aux].flows) <= (routing_table_jellyfish[src_aux][dst_aux].paths[n_path][routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1]))) || (max == 0)){
                        routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] = ((routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        inj = n_path;
                        break;
                    }
                    else{
                        n_path = ((routing_table_jellyfish[src_aux][dst_aux].last_used[dst2_aux] + 1) % routing_table_jellyfish[src_aux][dst_aux].n_paths);
                        i++;
                    }

                }
            }
            break;
        default:
            printf("Unknown routing.\n");
            exit(-1);
    }
    return(inj);
}

/**
 * Finish the routing (Not required).
 */
void finish_route_jellyfish(long path_n_flows_max, long path_n_apps, long app_id){

    //long pos_max;

    //pos_max = routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0] + 1;
    //printf("(%ld --> %ld) SIZE: %ld PATH: %ld - %ld: %ld %ld\n", src_aux, dst_aux, routing_table_jellyfish[src_aux][dst_aux].paths[n_path][0],n_path,pos_max, path_n_flows_max, path_n_apps);
    //routing_table_jellyfish[src_aux][dst_aux].paths[n_path][pos_max] = path_n_flows_max;
    //routing_table_jellyfish[src_aux][dst_aux].paths[n_path][pos_max + 2] = app_id;
    //if(src_aux != dst_aux){
        //routing_table_jellyfish[src_aux][dst_aux].paths[n_path][pos_max + 1] = path_n_apps;
        //if(path_n_apps > 1){
        //    exit(0);
        //}
    //}

}

/**
 * Route a flow from a source to a destination.
 */
long route_jellyfish(long current, long destination)
{

    long i;
    long next = -1;
    long port = -1;

    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
            if(current < servers) {
                port = 0;
                c_path++;
            }
            else {
                next = routing_table_jellyfish[src_aux][dst_aux].paths[n_path][c_path++];
                if((current - servers) == dst_aux) {
                    port = (destination % ports_servers);
                } else {
                    for(i = 0; i < random_graph[current - servers].nedges; i++) {
                        if(random_graph[current - servers].edge[i].neighbour.node == next) {
                            port = i + ports_servers;
                            break;
                        }
                    }
                }
            }
            break;
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
            if(current < servers) {
                port = 0;
                c_path++;
            }
            else {
                next = routing_table_jellyfish[src_aux][dst_aux].paths[n_path][c_path++];
                if((current - servers) == dst_aux) {
                    port = (destination % ports_servers);
                } else {
                    for(i = 0; i < random_graph[current - servers].nedges; i++) {
                        if(random_graph[current - servers].edge[i].neighbour.node == next) {
                            port = i + ports_servers;
                            break;
                        }
                    }
                }
            }
            break;
        case JELLYFISH_ECMP_ROUTING:
            if(current < servers) {
                port = 0;
                c_path++;
            } else {
                next = routing_table_jellyfish[src_aux][dst_aux].paths[n_path][c_path++];
                if((current- servers) == dst_aux) {
                    port = (destination % ports_servers);
                } else {
                    for(i = 0; i < random_graph[current - servers].nedges; i++) {
                        if(random_graph[current - servers].edge[i].neighbour.node == next) {
                            port = i + ports_servers;
                            break;
                        }
                    }
                }
            }
            break;
        case JELLYFISH_LLSKR_ROUTING:
            if(current < servers) {
                port = 0;
                c_path++;
            } else {
                next = routing_table_jellyfish[src_aux][dst_aux].paths[n_path][c_path++];
                if((current- servers) == dst_aux) {
                    port = (destination % ports_servers);
                } else {
                    for(i = 0; i < random_graph[current - servers].nedges; i++) {
                        if(random_graph[current - servers].edge[i].neighbour.node == next) {
                            port = i + ports_servers;
                            break;
                        }
                    }
                }
            }
            break;
        default:
            printf("Unknown routing %ld", param_routing_type);
            exit(-1);
    }
    return(port);
}

/**
 * Search the set of switches at a given distance dist from switch src.
 * Returns the set of switches in dsts and the size of the set.
 */
long nodes_distance_jellyfish(long src, long *dsts, long dist){

    long i, j;
    long num = 0;

    if(dist == 0){
        dsts[num++] = src;
    }
    else{
        for(i = 0; i < switches; i++){
            for(j = 0; j < routing_table_jellyfish[src][i].n_paths; j++){
                if(i != src && ((dist + 1) == routing_table_jellyfish[src][i].paths[j][0])){
                    dsts[num++] = i;
                    break;
                }
            }
        }
    }
    return(num);
}

long *get_path(long src, long dst, long n_path){

    return(routing_table_jellyfish[src][dst].paths[n_path]);
}

long get_path_length(long src, long dst, long n_path){

    return(routing_table_jellyfish[src][dst].paths[n_path][0]);
}

