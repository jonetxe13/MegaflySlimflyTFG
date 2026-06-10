#ifndef _routing_jellyfish
#define _routing_jellyfish

#include "rg_gen.h"

// Generate routing table
void generate_routing_table();

//Destroy the routing table
void destroy_routing_table();

//Print the routing table
void print_routing_table();

typedef struct k_paths {
    long n_paths;
    long *last_used;
    long **paths;
} k_paths;

typedef struct edge_removed_t{
    long src;
    long dst;
} edge_removed_t;

void find_shortest_path(graph_t *rg, long switches, long node_src, long *path, long *path_distances);

long shortest_path(list_t *l, long *paths, int start, int end);

long shortest_path2(long *l, long *paths, int start, int end, long length);

void mark_shortest_route_sp(long start, long end, long *paths, k_paths **routing_table_jellyfish, long current);

void shortest_route_ksp_ecmp(long start, long *length, long *paths, k_paths **routing_table_jellyfish, int K, int ecmp, long switches);

void mark_shortest_route_ksp_ecmp(long **k_path, k_paths **routing_table_jellyfish, long paths, long start, long end);

void shortest_route_llksr(long start, long *length, long *paths, k_paths **routing_table_jellyfish, long M, long K, long H, long ths, long switches);

void ksp_paths(graph_t *rg, long switches, k_paths **routing_table_jellyfish, long ecmp);

void llksr_paths(graph_t *rg, long switches, k_paths **routing_table_jellyfish, long r, long p);

void print_routing_table();

void destroy_routing_table();

void generate_routing_table(long switches, long port_servers);

#endif
