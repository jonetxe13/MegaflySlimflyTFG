/** @mainpage
 *  Generator of the random graph
 */

#include "rg_gen.h"
#include "jellyfish.h"
#include "../inrflow/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long total_nodes;

/**
 * Load graph from file.
 */
void load_graph_from_file(graph_t *rg, char *fname){

    FILE * fd;
    long o,d;

    if((fd = fopen(fname, "r")) == NULL) {
        printf("WARNING: graph file %s cannot be opened for read\n", fname);
        exit(-1);
    }

    while(!feof(fd)){
        if(fscanf(fd,"%ld %ld\n",&o, &d) < 2){
             printf("Format of workload file is not correct.\n");
             exit(-1);
        }
//        printf("O: %ld, D: %ld \n", o, d);
        connect_nodes(rg, o, d);

    }

    fclose(fd);
}

/**
 * Generate the random graph that represents the network.
 */
void generate(graph_t *rg, long n, long p, long seed)
{

    long i, j, k, l;
    char statenew[256];
    char *stateold;

    stateold = initstate(seed, statenew, 256);

    total_nodes = n;

    while(total_nodes > 1) {
        i = random() % n;
        j = random() % n;

        if(i == j || rg[i].fedges == 0 || rg[j].fedges == 0)
            continue;

        if(are_connected(rg, i, j)) {
            if(is_full_connected(rg,n)) {
                k = random() % n;
                while(k == i || k == j || are_connected(rg, i, k) || !is_difference(rg, k, j ,i, n)) {
                    k = random() % n;
                }
                l = rg[k].edge[random() % rg[k].nedges].neighbour.node;
                while(l == -1 || l == i || l == j || are_connected(rg, l, j))
                    l = rg[k].edge[random() % rg[k].nedges].neighbour.node;

                disconnect_nodes(rg, k, l);
                connect_nodes(rg, k, i);
                connect_nodes(rg, l, j);
            } 
            else {
                continue;
            }
        } else {
            connect_nodes(rg, i, j);
        }
    }
    setstate(stateold);
}

/**
 * Checks if a current subgraph is fully connected.
 */
long is_full_connected(graph_t *rg, long n)
{

    long i, j;

    for(i = 0; i < n; i++) {
        if(rg[i].fedges != 0) {
            for(j = i + 1; j < n; j++) {
                if(rg[j].fedges != 0) {
                    if(!are_connected(rg, i,  j)) {
                        return 0;
                    }
                }
            }
        }
    }
    return 1;
}

/**
 * Check if the difference (A - B) is empty.
 */
long is_difference(graph_t *rg, long node_ref, long node_ref2, long node, long n)
{

    long i, j;
    long node_ref_aux;

    for(i = 0; i < rg[node_ref].nedges; i++) {
        node_ref_aux = rg[node_ref].edge[i].neighbour.node;
        for(j = 0; j < rg[node_ref2].nedges; j++) {
            if( node_ref_aux != -1 && node_ref_aux != node_ref2 && node_ref_aux != node && node_ref_aux != rg[node_ref].edge[j].neighbour.node) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Check if two nodes are connected.
 */
long are_connected(graph_t *rg, long n1, long n2)
{

    long i;
    long connected;

    connected = 0;
    for(i = 0; i < rg[n1].nedges; i++) {
        if(rg[n1].edge[i].neighbour.node == n2) {
            connected = 1;
            break;
        }
    }
    return(connected);
}

/**
 * Connect two nodes.
 */
void connect_nodes(graph_t *rg, long n1, long n2)
{

    long src_edge, dst_edge;
    src_edge = random() % rg[n1].nedges;
    while(rg[n1].edge[src_edge].neighbour.edge != -1) {
        src_edge = random() % rg[n1].nedges;
    }
    rg[n1].fedges--;
    if(rg[n1].fedges == 0)
        total_nodes--;
    dst_edge = random() % rg[n2].nedges;
    while(rg[n2].edge[dst_edge].neighbour.edge != -1) {
        dst_edge = random() % rg[n2].nedges;
    }
    rg[n2].fedges--;
    if(rg[n2].fedges == 0)
        total_nodes--;
    rg[n1].edge[src_edge].neighbour.node = n2;
    rg[n1].edge[src_edge].neighbour.edge = dst_edge;
    rg[n2].edge[dst_edge].neighbour.node = n1;
    rg[n2].edge[dst_edge].neighbour.edge = src_edge;
}

/**
 * Disconnect two nodes.
 */
void disconnect_nodes(graph_t *rg, long n1, long n2)
{

    long i;
    long edge_n1_n2 = -1;

    for(i = 0; i < rg[n1].nedges; i++) {
        if(rg[n1].edge[i].neighbour.node == n2) {
            edge_n1_n2 = i;
            break;
        }
    }
    rg[n2].edge[rg[n1].edge[edge_n1_n2].neighbour.edge].neighbour.node = -1;
    rg[n2].edge[rg[n1].edge[edge_n1_n2].neighbour.edge].neighbour.edge = -1;
    if(rg[n2].fedges == 0)
        total_nodes++;
    rg[n2].fedges++;

    rg[n1].edge[edge_n1_n2].neighbour.node = -1;
    rg[n1].edge[edge_n1_n2].neighbour.edge = -1;
    if(rg[n1].fedges == 0)
        total_nodes++;
    rg[n1].fedges++;
}

void deactivate_edge(graph_t *rg, long node_src, long node_dst){

    long i;

    for(i = 0; i < rg[node_src].nedges; i++) {
        if(rg[node_src].edge[i].neighbour.node == node_dst) {
            rg[node_src].edge[i].active = 0;
            rg[node_dst].edge[rg[node_src].edge[i].neighbour.edge].active = 0;
            break;
        }
    }
}

void activate_edge(graph_t *rg, long node_src, long node_dst){

    long i;

    for(i = 0; i < rg[node_src].nedges; i++) {
        if(rg[node_src].edge[i].neighbour.node == node_dst) {
            rg[node_src].edge[i].active = 1;
            rg[node_dst].edge[rg[node_src].edge[i].neighbour.edge].active = 1;
            break;
        }
    }
}

void deactivate_node(graph_t *rg, long node){

            rg[node].active = 0;
}

void activate_node(graph_t *rg, long node){

            rg[node].active = 1;
}

/**
 * Print the network structure (without node) as a an adjacency matrix. Just for testing purposes.
 */
void export_graph_adjacency(graph_t *rg, long switches)
{

    long i, j, k;
    for(i = 0; i < switches; i++) {
        for(j = 0; j < switches; j++) {
            if(i == j) {
                printf("0 ");
            } else {
                for(k = 0; k < rg[i].nedges; k++) {
                    if(rg[i].edge[k].neighbour.node == j) {
                        printf("1 ");
                        break;
                    }
                    if(k == rg[i].nedges-1)
                        printf("0 ");
                }
            }
        }
        printf("\n");
    }
}

/**
 * Print the network structure (without node) as a an adjacency matrix. Just for testing purposes.
 */
void export_graph_edges(graph_t *rg, long switches, char *topo_name)
{

    long i, j, k;
        
    printf("graph jellyfish_%s{\n", topo_name);
    for(i = 0; i < switches; i++) {
        for(j = 0; j < switches; j++) {
                for(k = 0; k < rg[i].nedges; k++) {
                    if(rg[i].edge[k].neighbour.node == j) {
                        printf("%ld -- %ld\n", i,j);
                        break;
                    }
                }
        }
    }
    printf("}\n");
}

