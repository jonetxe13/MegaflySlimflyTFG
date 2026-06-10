#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "rg_gen.h"
#include "jellyfish_aux.h"
#include "routing_jellyfish.h"
#include "../inrflow/list.h"
#include "../inrflow/globals.h"

#include <unistd.h>
#include <time.h>

extern graph_t *random_graph;
extern k_paths **routing_table_jellyfish;
extern long n_kpaths;

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

edge_removed_t edges[1000];
long nodes_removed[1000];
long *root_path;
long *spur_path;
long *path_aux;
long *path_aux_distances;
long **k_path;
list_t B;

void mark_shortest_route_ksp_ecmp(long **k_path, k_paths **routing_table_jellyfish, long paths, long start, long end){

    long i,j;
    long *l_aux;

    if(start == end){
        routing_table_jellyfish[start][end].n_paths = paths;
        routing_table_jellyfish[start][end].paths = malloc(paths * sizeof(long*));
        for(i = 0; i < paths; i++){
            l_aux = k_path[i];
            routing_table_jellyfish[start][end].paths[i] = malloc((l_aux[0] + 1 + 3) * sizeof(long));
            routing_table_jellyfish[start][end].paths[i][0] = l_aux[0];
            for(j = 0; j < l_aux[0]; j++){
                routing_table_jellyfish[start][end].paths[i][j+1] = l_aux[j+1];
            }
            for(j = l_aux[0] + 1; j < l_aux[0] + 4; j++){
                routing_table_jellyfish[start][end].paths[i][j] = 0;
            }
        }

    }
    else{
        routing_table_jellyfish[start][end].n_paths = paths;
        routing_table_jellyfish[end][start].n_paths = paths;
        routing_table_jellyfish[start][end].paths = malloc(paths * sizeof(long*));
        routing_table_jellyfish[end][start].paths = malloc(paths * sizeof(long*));
//        printf("%ld --> %ld (%ld) \n", start, end, paths);
        for(i = 0; i < paths; i++){
            l_aux = k_path[i];
            routing_table_jellyfish[start][end].paths[i] = malloc((l_aux[0] + 1 + 3) * sizeof(long));
            routing_table_jellyfish[end][start].paths[i] = malloc((l_aux[0] + 1 + 3) * sizeof(long));
            routing_table_jellyfish[start][end].paths[i][0] = l_aux[0];
            routing_table_jellyfish[end][start].paths[i][0] = l_aux[0];
            for(j = 0; j < l_aux[0]; j++){
                routing_table_jellyfish[start][end].paths[i][j+1] = l_aux[j+1];
                routing_table_jellyfish[end][start].paths[i][j+1] = l_aux[l_aux[0] - j];
  //              printf("%ld - ", routing_table_jellyfish[start][end].paths[i][j+1] = l_aux[j+1]);
            }
    //        printf("\n");
            for(j = l_aux[0] + 1; j < l_aux[0] + 4; j++){
                routing_table_jellyfish[start][end].paths[i][j] = 0;
                routing_table_jellyfish[end][start].paths[i][j] = 0;
            }

        }
      //  printf("\n-------------------------------------------------------------------------\n\n");
    }
}

long shortest_path(list_t *l, long *paths, int start, int end){

    long done = 0;
    long current = end;

    if(start == end){
        list_insert(l, &current);
        current = start;
        list_insert(l, &current);
        done = 1;
    }
    else if(paths[current] != -1){
        while(!done){
            list_insert(l, &current);
            current = paths[current];
            if(current == start){
                list_insert(l, &current);
                done = 1;
            }
        }
    }
    return(done);
}

long shortest_path2(long *l, long *paths, int start, int end, long length){

    long done = 0;
    long current = end;
    long length_aux = length;

    if(start == end){
        l[0] = 2;
        l[1] = end;
        l[2] = end;
        done = 1;
    }
    else if(paths[current] != -1){
        while(!done){
            l[0] = length;
            l[length_aux] = current;
            current = paths[current];
            length_aux--;
            if(current == start){
                l[length_aux] = current;
                done = 1;
            }
        }
    }
    return(done);
}

/**
 * Auxiliar funtion to generate the routing table.
 */
void shortest_route_ksp_ecmp(long start, long *length, long *paths, k_paths **routing_table_jellyfish, int K, int ecmp, long switches){

    long k, i, j, p, eq, end, spur_node, shortest_path_length, cont, curr_path, n_edges, n_nodes_removed, K_aux;
    list_t **l_aux;
    long *total_path;

    for(end = start; end < switches;end++){
        k_path[0] = malloc((switches+1) * sizeof(long));
        curr_path = 0;
        K_aux = K;
        shortest_path2(k_path[0], paths, start, end, length[end]);
        shortest_path_length = k_path[0][0];
        k_path[0][k_path[0][0] + 1] = 0;
        if(start == end){
            K_aux=1;
        }
        for(k = 1; k < K_aux; k++){
            for(i = k_path[k-1][k_path[k-1][0]+1]; i < k_path[k-1][0] - 1; i++){
                n_edges = 0;
                n_nodes_removed = 0;
                spur_node = k_path[k-1][i+1];

                for(j = 0;j <= i;j++){
                    root_path[j+1] = k_path[k-1][j+1];
                }
                root_path[0] = i+1;
                for(p = 0; p <= curr_path; p++){
                    eq = arrays_eq(root_path, k_path[p], i);
                    if(k_path[p][0] > i && eq == 1){
                        edges[n_edges].src = k_path[p][i+1];
                        edges[n_edges].dst = k_path[p][i+2];
                        deactivate_edge(random_graph,  k_path[p][i+1], k_path[p][i+2]);
                        n_edges++;
                    }
                }
                for(j = 0; j < root_path[0]; j++){
                    if(root_path[j+1] == spur_node){
                        break;
                    }
                    else{
                        nodes_removed[n_nodes_removed] = root_path[j+1];
                        deactivate_node(random_graph, root_path[j+1]);
                        n_nodes_removed++;
                    }
                }
                find_shortest_path(random_graph, switches, spur_node, path_aux, path_aux_distances);
                if(shortest_path2(spur_path, path_aux, spur_node, end, path_aux_distances[end])){
                    total_path = malloc((root_path[0] + spur_path[0] + 1) *sizeof(long));
                    for(j = 0; j < root_path[0] - 1; j++){
                        total_path[j+1] = root_path[j+1];
                    }
                    for(j = 0; j < spur_path[0]; j++){
                        total_path[root_path[0] + j] = spur_path[j+1];
                    }
                    total_path[0] = root_path[0] + spur_path[0] - 1;

                    if(root_path[0] - 2 >= 0)
                        total_path[root_path[0] + spur_path[0]] = root_path[0] - 2;
                    else
                        total_path[root_path[0] + spur_path[0]] = 0;

                    if(!list_is_in(&B, total_path)){
                        list_append(&B, &total_path);
                    }
                    else{
                        free(total_path);
                    }
                }
                for(j = 0; j < n_edges; j++){
                    activate_edge(random_graph, edges[j].src, edges[j].dst);
                }
                for(j = 0; j < n_nodes_removed; j++){
                    activate_node(random_graph, nodes_removed[j]);
                }

            }
            if(B.length == 0){
                break;
            }
            cont = list_shorter_from_a_to_b(&B, &k_path[k], ecmp, shortest_path_length);
            if(!cont){
                break;
            }
            curr_path++;
        }
        mark_shortest_route_ksp_ecmp(k_path, routing_table_jellyfish, curr_path + 1, start, end);
        list_reset(&B);
        while(list_next(&B, (void*)&l_aux)){
            free((*l_aux));
            list_rem_elem(&B);
        }
        for(i = 0; i < curr_path + 1; i++){
            free(k_path[i]);
        }
    }
}

/**
 * Auxiliar funtion to generate the routing table.
 */
void shortest_route_llksr(long start, long *length, long *paths, k_paths **routing_table_jellyfish, long M, long K, long H, long ths, long switches){

    long k, i, j, p, eq, end, spur_node, cont, curr_path, n_edges, n_nodes_removed, K_aux;
    list_t **l_aux;
    long *total_path;

    for(end = start; end < switches;end++){
        k_path[0] = malloc((switches+1) * sizeof(long));
        curr_path = 0;
        K_aux = M;
        shortest_path2(k_path[0], paths, start, end, length[end]);
        //shortest_path_length = k_path[0][0];
        k_path[0][k_path[0][0] + 1] = 0;
        if(start == end){
            K_aux=1;
        }
        k = 1;
        while(k < K_aux){
            for(i = k_path[k-1][k_path[k-1][0]+1]; i < k_path[k-1][0] - 1; i++){
                n_edges = 0;
                n_nodes_removed = 0;
                spur_node = k_path[k-1][i+1];

                for(j = 0;j <= i;j++){
                    root_path[j+1] = k_path[k-1][j+1];
                }
                root_path[0] = i+1;
                for(p = 0; p <= curr_path; p++){
                    eq = arrays_eq(root_path, k_path[p], i);
                    if(k_path[p][0] > i && eq == 1){
                        edges[n_edges].src = k_path[p][i+1];
                        edges[n_edges].dst = k_path[p][i+2];
                        deactivate_edge(random_graph,  k_path[p][i+1], k_path[p][i+2]);
                        n_edges++;
                    }
                }
                for(j = 0; j < root_path[0]; j++){
                    if(root_path[j+1] == spur_node){
                        break;
                    }
                    else{
                        nodes_removed[n_nodes_removed] = root_path[j+1];
                        deactivate_node(random_graph, root_path[j+1]);
                        n_nodes_removed++;
                    }
                }
                find_shortest_path(random_graph, switches, spur_node, path_aux, path_aux_distances);
                if(shortest_path2(spur_path, path_aux, spur_node, end, path_aux_distances[end])){
                    total_path = malloc((root_path[0] + spur_path[0] + 1) *sizeof(long));
                    for(j = 0; j < root_path[0] - 1; j++){
                        total_path[j+1] = root_path[j+1];
                    }
                    for(j = 0; j < spur_path[0]; j++){
                        total_path[root_path[0] + j] = spur_path[j+1];
                    }
                    total_path[0] = root_path[0] + spur_path[0] - 1;

                    if(root_path[0] - 2 >= 0)
                        total_path[root_path[0] + spur_path[0]] = root_path[0] - 2;
                    else
                        total_path[root_path[0] + spur_path[0]] = 0;

                    if(!list_is_in(&B, total_path)){
                        list_append(&B, &total_path);
                    }
                    else{
                        free(total_path);
                    }
                }
                for(j = 0; j < n_edges; j++){
                    activate_edge(random_graph, edges[j].src, edges[j].dst);
                }
                for(j = 0; j < n_nodes_removed; j++){
                    activate_node(random_graph, nodes_removed[j]);
                }

            }
            if(B.length == 0){
                break;
            }
            cont = select_path(&B, &k_path[k], k - 1, H + 1, ths);
            if(!cont){
                break;
            }
            else if(cont == 2){
                K_aux = K;
            }
            curr_path++;
            k++;
        }
        mark_shortest_route_ksp_ecmp(k_path, routing_table_jellyfish, curr_path + 1, start, end);
        list_reset(&B);
        while(list_next(&B, (void*)&l_aux)){
            free((*l_aux));
            list_rem_elem(&B);
        }
        for(i = 0; i < curr_path + 1; i++){
            free(k_path[i]);
        }
    }
}

/**
 * Find the shortest path between all pairs of nodes.
 */
void find_shortest_path(graph_t *rg, long switches, long node_src, long *path, long *paths_distances){

    long i;
    long *queue;
    long *discovered;
    long *processed;
    long queue_insert, queue_extract;
    long v;

    queue = malloc(switches * sizeof(long));
    discovered = malloc(switches * sizeof(long));
    processed = malloc(switches * sizeof(long));

    for(i = 0; i < switches; i++) {
        queue[i] = -1;
        path[i] = -1;
        discovered[i] = 0;
        processed[i] = 0;
        paths_distances[i] = 0;
    }
    queue_insert = 0;
    queue_extract = 0;

    queue[queue_insert] = node_src;
    queue_insert++;
    discovered[node_src] = 1;
    paths_distances[node_src] = 1;

    while(queue_insert != queue_extract) {
        v = queue[queue_extract];
        queue_extract++;
        processed[v] = 1;
        for(i = 0; i < rg[v].nedges; i++) {
            if(rg[v].edge[i].neighbour.edge != -1 && rg[v].edge[i].active && rg[rg[v].edge[i].neighbour.node].active) {
                if(discovered[rg[v].edge[i].neighbour.node] == 0) {
                    queue[queue_insert] = rg[v].edge[i].neighbour.node;
                    queue_insert++;
                    discovered[rg[v].edge[i].neighbour.node] = 1;
                    paths_distances[rg[v].edge[i].neighbour.node] = paths_distances[v] + 1;
                    path[rg[v].edge[i].neighbour.node] = v;
                }
            }
        }
    }

    free(queue);
    free(discovered);
    free(processed);
}

void sp_paths(graph_t *rg, long switches, k_paths **routing_table_jellyfish){

    long i;
    long *path;
    long *paths_distances;
    path = malloc(switches * sizeof(long));
    paths_distances = malloc(switches * sizeof(long));
    root_path = malloc((switches+1) * sizeof(long));
    spur_path = malloc((switches+1) * sizeof(long));
    path_aux = malloc(sizeof(long) * switches);
    path_aux_distances = malloc(sizeof(long) * switches);

    k_path = malloc(sizeof(long*));

    list_initialize(&B, sizeof(long**));

    for(i = 0; i < switches; i++) {
        find_shortest_path(random_graph, switches, i, path, paths_distances);
        shortest_route_ksp_ecmp(i, paths_distances, path, routing_table_jellyfish, 1, 0, switches);
    }
    free(path);
    free(paths_distances);
    free(root_path);
    free(spur_path);
    free(path_aux);
    free(path_aux_distances);
    free(k_path);

}

void ksp_paths(graph_t *rg, long switches, k_paths **routing_table_jellyfish, long ecmp){

    long i,k;
    long *path;
    long *paths_distances;

    path = malloc(switches * sizeof(long));
    paths_distances = malloc(switches * sizeof(long));
    root_path = malloc((switches+1) * sizeof(long));
    spur_path = malloc((switches+1) * sizeof(long));
    path_aux = malloc(sizeof(long) * switches);
    path_aux_distances = malloc(sizeof(long) * switches);

    if(ecmp == 1){
        // Upper bound. Should be improved.
        k = switches;
    }
    else{
        k = routing_params[0];
    }
    k_path = malloc(k * sizeof(long*));

    list_initialize(&B, sizeof(long**));

    for(i = 0; i < switches; i++) {
        find_shortest_path(random_graph, switches, i, path, paths_distances);
        shortest_route_ksp_ecmp(i, paths_distances, path, routing_table_jellyfish, k, ecmp, switches);
    }
    free(path);
    free(paths_distances);
    free(root_path);
    free(spur_path);
    free(path_aux);
    free(path_aux_distances);
    free(k_path);

}

void llksr_paths(graph_t *rg, long switches, k_paths **routing_table_jellyfish, long r, long p){

    long i, h;
    long H = 0;
    long EM = 0;
    long *path;
    long *paths_distances;
    long K = routing_params[0];
    long thw = routing_params[1];
    long ths = routing_params[2];
    long M = K * (p * p);

    path = malloc(switches * sizeof(long));
    paths_distances = malloc(switches * sizeof(long));
    root_path = malloc((switches+1) * sizeof(long));
    spur_path = malloc((switches+1) * sizeof(long));
    path_aux = malloc(sizeof(long) * switches);
    path_aux_distances = malloc(sizeof(long) * switches);
    k_path = malloc(M * sizeof(long*));

    list_initialize(&B, sizeof(long**));

    // Calculate H
    for(h = 0; h < switches; h++){
        EM += r * pow(r - 1, h);
        if((EM / switches) > thw){
            H = h + 1;
            break;
        }
    }
    for(i = 0; i < switches; i++) {
        find_shortest_path(random_graph, switches, i, path, paths_distances);
        shortest_route_llksr(i, paths_distances, path, routing_table_jellyfish, M, K, H, ths, switches);
    }
    free(path);
    free(paths_distances);
    free(root_path);
    free(spur_path);
    free(path_aux);
    free(path_aux_distances);
    free(k_path);

}



/**
 * Generates the routing table.
 */
void generate_routing_table(long ports_switches, long ports_servers)
{

    long i, j, k;
    long size = 1;

    n_kpaths = 0;
    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
        case JELLYFISH_ECMP_ROUTING:
            break;
        case JELLYFISH_LLSKR_ROUTING:
            size = ports_servers;
            n_kpaths = routing_params[0];
            break;
        default:
            printf("Unknown routing.\n");
            exit(-1);
    }

    routing_table_jellyfish = malloc(switches * sizeof(k_paths*));
    for(i = 0; i < switches; i++) {
        routing_table_jellyfish[i] = malloc(switches * sizeof(k_paths));
        for(j = 0; j < switches; j++) {
            routing_table_jellyfish[i][j].n_paths = 1;
            routing_table_jellyfish[i][j].last_used = malloc(size * sizeof(long));;
            for(k = 0; k < size; k++){
                routing_table_jellyfish[i][j].last_used[k] = 0;
            }
        }
    }

    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
            sp_paths(random_graph, switches, routing_table_jellyfish);
            break;
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
            ksp_paths(random_graph, switches, routing_table_jellyfish, 0);
            break;
        case JELLYFISH_ECMP_ROUTING:
            ksp_paths(random_graph, switches, routing_table_jellyfish, 1);
            break;
        case JELLYFISH_LLSKR_ROUTING:
            llksr_paths(random_graph, switches, routing_table_jellyfish, ports_switches, ports_servers);
            break;
        default:
            printf("Unknown routing.\n");
            exit(-1);
    }
}

/**
 * Releases the resources reserved to store the routing table.
 */
void destroy_routing_table()
{
    long i, j, k;

    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
        case JELLYFISH_ECMP_ROUTING:
        case JELLYFISH_LLSKR_ROUTING:
            for(i = 0; i < switches; i++) {
                for(j = 0; j < switches; j++) {
                    for(k = 0; k < routing_table_jellyfish[i][j].n_paths;k++){
                        free(routing_table_jellyfish[i][j].paths[k]);
                    }
                    free(routing_table_jellyfish[i][j].paths);
                    free(routing_table_jellyfish[i][j].last_used);
                }
                free(routing_table_jellyfish[i]);
            }
            free(routing_table_jellyfish);
            break;
        default:
            printf("Unknown routing.\n");
            exit(-1);
    }
}

/**
 * Print the routing table.
 */
void print_routing_table()
{
    long i, j, k;

    switch(routing) {
        case JELLYFISH_SHORTEST_PATH_ROUTING:
        case JELLYFISH_K_SHORTEST_PATHS_ROUTING:
        case JELLYFISH_ECMP_ROUTING:
        case JELLYFISH_LLSKR_ROUTING:
            for(i = 0; i < switches; i++) {
                for(j = 0; j < switches; j++) {
                    for(k = 0; k < routing_table_jellyfish[i][j].n_paths; k++) {
                        printf(" %ld ",routing_table_jellyfish[i][j].paths[k][0]);
                    }
                }
                printf("\n");
            }
            break;
        default:
            printf("Unknown routing \n.");
            exit(-1);
    }
}


