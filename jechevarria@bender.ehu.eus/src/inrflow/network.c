#include "topologies.h"
#include "applications.h"
#include "node.h"
#include <stdio.h>
#include <stdlib.h>

long servers; ///< The total number of servers
long switches;///< The total number of switches
long radix;	  ///< radix of the switches - assuming all switches have the same
long ports;	///< The total number of ports (i.e.links) in the topology

//Servers definition
long server_cores;
long server_memory;
long server_capacity;
long switch_capacity;
long san_capacity;
long memory_capacity;

node_t* network;    /// Data structure containing the model of the network.

/**
 * Check whether there are any nodes that are expected to be neighbours
 * but aren't - possibly due to errors in the connection function for
 * the topology.
 */
void check_network_consistency()
{
    long halt=0;
    long nnode;
    long nport;
    long i,j;

    for(i=0; i<servers+switches; i++) {
        for(j=0; j<network[i].nports; j++) {
            if(network[i].port[j].neighbour.node!=-1) {
                nnode=network[i].port[j].neighbour.node;
                nport=network[i].port[j].neighbour.port;

                if (network[nnode].port[nport].neighbour.node !=i || network[nnode].port[nport].neighbour.port !=j ) {
                    printf("Inconsistency!, %ld.%ld --> %ld.%ld but %ld.%ld --> %ld.%ld\n",
                            i, j,
                            network[i].port[j].neighbour.node, network[i].port[j].neighbour.port,
                            network[i].port[j].neighbour.node, network[i].port[j].neighbour.port,
                            network[nnode].port[nport].neighbour.node,
                            network[nnode].port[nport].neighbour.port);
                    halt=1;
                }
            } else if (network[i].port[j].neighbour.port!=-1) {
                printf("Warning!, %ld.%ld neighbour node is -1, but ports is not -1\n",i, j);
            }
        }
    }
    if (halt){
        exit(-1);
    } else
        printf("All connections in the topology seem consistent!!!\n");
}

/**
 * Initializes data structures and connects them to form the desired topology.
 */
void construct_network_electric()
{
    long i,j,k; // node, port
    tuple_t dst;

    printf("Constructing electric Network\n");

    servers=get_servers();
    switches=get_switches();
    ports=get_ports();
    radix=get_radix(get_switch_i(0));

    network=malloc(sizeof(node_t)*(servers+switches));

    for(i=0; i<servers+switches; i++) {
        network[i].nports=get_radix(i);
        network[i].port=malloc(sizeof(port_t)*network[i].nports);
        for(j=0; j<network[i].nports; j++) {
            network[i].port[j].neighbour.node=-1;
            network[i].port[j].neighbour.port=-1;
            network[i].port[j].bandwidth_capacity=0;
            for(k=0; k < MAX_CONCURRENT_APPS + 1;k++){
                network[i].port[j].link_info[k] = 0;
                network[i].port[j].link_info_apps[k] = 0;
            }

        }
    }

    for(i=0; i<servers+switches; i++) {
        for(j=0; j<network[i].nports; j++) {
            dst=connection(i,j);
            // Assess consistency of connections
            if (dst.node==-1 || dst.port==-1) { //this port is disconnected
                dst.node=-1;
                dst.port=-1;
                network[i].port[j].neighbour=dst;
            } else {
                network[i].port[j].neighbour=dst;
                //network[dst.node].port[dst.port].neighbour.node=i;
                //network[dst.node].port[dst.port].neighbour.port=j;
                list_initialize(&network[i].port[j].dflows, sizeof(dflow_t*));
                if (topo==EXATORUS){
                    if (i<servers){
                        if (j==0) //CRDBs
                            network[i].port[j].bandwidth_capacity=64*1E6;
                        else
                            network[i].port[j].bandwidth_capacity=32*1E6;
                    }
                    else if (i<servers+(servers/16)){ // Blade switch
                        if (j<16)
                            network[i].port[j].bandwidth_capacity=64*1E6;
                        else
                            network[i].port[j].bandwidth_capacity=switch_capacity;
                    }
                    else if (i<servers+(servers/8)){
                        if (j<6)
                            network[i].port[j].bandwidth_capacity=100*1E6;
                        else
                            network[i].port[j].bandwidth_capacity=switch_capacity;
                    }
                }
                else if (topo==EXATREE){

                    if (i<servers){
                        if (j==0)
                            network[i].port[j].bandwidth_capacity=64*1E6;
                        else
                            network[i].port[j].bandwidth_capacity=32*1E6;
                    }
                    else if (i<servers+(servers/16)){
                        if (j<16)
                            network[i].port[j].bandwidth_capacity=64*1E6;
                        else
                            network[i].port[j].bandwidth_capacity=switch_capacity;
                    }
                    else if (i<servers+(servers/8)){
                        if (j>0)
                            network[i].port[j].bandwidth_capacity=100*1E6;
                        else
                            network[i].port[j].bandwidth_capacity=switch_capacity;
                    }
                    else{
                        network[i].port[j].bandwidth_capacity=100*1E6;
                    }
                }
                else if (   topo==EUROEXA_BASE   || topo==EUROEXA_RND || topo==EUROEXA_MULTI
                         || topo==EUROEXA_SINGLE || topo==EUROEXA_RS  || topo==EUROEXA_TL){
                    if (i<servers){ // CRDBs
                        if (j==0) // uplink
                            network[i].port[j].bandwidth_capacity=64*1E6;
                        else // local quad link
                            network[i].port[j].bandwidth_capacity=32*1E6;
                    }
                    else if (i<servers+(servers/4)){// Lower local routers
                        if (j<4) // ports to nodes
                            network[i].port[j].bandwidth_capacity=64*1E6;
                        else // inter-switch links and uplinks
                            network[i].port[j].bandwidth_capacity=100*1E6;
                    }
                    else{ // Upper Switch and Eth switches
                        network[i].port[j].bandwidth_capacity=100*1E6;
                    }
                }
                else{
                    if(is_server(i) || is_server(network[i].port[j].neighbour.node)){
                        network[i].port[j].bandwidth_capacity=server_capacity;
                    }
                    else{
                        network[i].port[j].bandwidth_capacity=switch_capacity;
                    }
                }
            }
        }
    }
#ifdef DEBUG
    check_network_consistency();
#endif

#ifdef SHOWCONNECTIONS
    for(i=0; i<servers+switches; i++)
        for(j=0; j<network[i].nports; j++)
            printf("%ld.%ld <-> %ld.%ld\n", i, j, network[i].port[j].neighbour.node, network[i].port[j].neighbour.port);
#endif

}

/**
 * Initializes data structures and connects them to form the desired topology.
 */
void construct_network_photonic()
{
    long i,j,k; // node, port
    tuple_t dst;

    printf("Constructing photonic Network\n");

    servers=get_servers();
    switches=get_switches();
    ports=get_ports();
    radix=get_radix(get_switch_i(0));

    network=malloc(sizeof(node_t)*(servers+switches));

    for(i=0; i<servers+switches; i++) {
        network[i].nports=get_radix(i);
        network[i].port=malloc(sizeof(port_t)*network[i].nports);
        network[i].opt_port=malloc(sizeof(opt_port_t)*network[i].nports);
        for(j=0; j<network[i].nports; j++) {
            network[i].port[j].neighbour.node=-1;
            network[i].port[j].neighbour.port=-1;
            network[i].port[j].bandwidth_capacity=0;
            for(k=0; k < MAX_CONCURRENT_APPS + 1;k++){
                network[i].port[j].link_info[k] = 0;
                network[i].port[j].link_info_apps[k] = 0;
            }
            network[i].opt_port[j].n_channels = n_channels;
            network[i].opt_port[j].channel_bandwidth = channel_bandwidth;
            network[i].opt_port[j].channels = malloc(n_channels * sizeof(opt_channel_t));
            for(k=0; k < n_channels; k++){
                network[i].opt_port[j].channels[k].flows = 0;
                network[i].opt_port[j].channels[k].n_lambdas = n_lambdas;
                network[i].opt_port[j].channels[k].lambda_bandwidth = (channel_bandwidth / n_lambdas);
            }

        }
    }

    for(i=0; i<servers+switches; i++) {
        for(j=0; j<network[i].nports; j++) {
            dst=connection(i,j);
            // Assess consistency of connections
            if (dst.node==-1 || dst.port==-1) { //this port is disconnected
                dst.node=-1;
                dst.port=-1;
                network[i].port[j].neighbour=dst;
            } else {
                network[i].port[j].neighbour=dst;
                network[dst.node].port[dst.port].neighbour.node=i;
                network[dst.node].port[dst.port].neighbour.port=j;
                list_initialize(&network[i].port[j].dflows, sizeof(dflow_t*));
                if(is_server(i)){
                    network[i].port[j].bandwidth_capacity = (channel_bandwidth * n_channels);
                }
                else{
                    network[i].port[j].bandwidth_capacity = (channel_bandwidth * n_channels);
                }
            }
        }
    }
#ifdef DEBUG
    check_network_consistency();
#endif

#ifdef SHOWCONNECTIONS
    for(i=0; i<servers+switches; i++)
        for(j=0; j<network[i].nports; j++)
            printf("%ld.%ld --> %ld.%ld\n", i, j, network[i].port[j].neighbour.node, network[i].port[j].neighbour.port);
#endif

}

void release_network_electric(){

    long i;

    for(i=0; i<servers+switches; i++) {
        free(network[i].port);
    }
    free(network);
}

void release_network_photonic(){

    long i, j;

    for(i=0; i<servers+switches; i++) {
        for(j=0; j<network[i].nports; j++) {
            free(network[i].opt_port[j].channels);
        }
        free(network[i].opt_port);
        free(network[i].port);
    }
    free(network);
}

/**
 * Resets the statistics and failure, useful to run monte carlo experiments.
 */
void reset_network()
{
    long i,j;// node, port

    for(i=0; i<servers+switches; i++) {
        network[i].flows_storage_read_injected=0;
        network[i].flows_storage_write_injected=0;
        for(j=0; j<network[i].nports; j++) {
            network[i].port[j].faulty=0;
            network[i].port[j].flows=0;
            network[i].port[j].flows_storage_read=0;
            network[i].port[j].flows_storage_read_fault=0;
            network[i].port[j].flows_storage_write=0;
            network[i].port[j].flows_storage_write_fault=0;
        }
    }
}

