#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "globals.h"
#include "applications.h"
#include "misc.h"
#include "literal.h"
#include "list.h"

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

long load_mapping_from_file(char *file, long ntasks, long *translation)
{
    FILE *fd;
    long node;
    long i = -1;

    if((fd = fopen(file, "r")) == NULL){
        printf("Error opening the mapping file.\n");
        exit(-1);
    }

    while (!feof (fd) && fscanf(fd, "%ld", &node) && i++ < ntasks ){
        translation[i] = node;
    }
    fclose(fd);
    return(i);
}

/**
 * Outputs the topology in an adjacency list format (for BFS).
 * @return The finalization code. -1 if there was any error, otherwise 0
 */
long generate_bfs_file()
{
    long i, j, k;
    long s; ///< server
    long nnode;

    FILE *bfs_file;
    char bfs_filename[100];

    sprintf(bfs_filename,"%s_%s_%s_fr%.2f_seed%ld.bfs",
            get_network_token(),
            get_filename_params(),
            get_routing_token(),
            failure_rate,
            r_seed);

    if((bfs_file = fopen(bfs_filename,"w")) == NULL) {
        perror("Unable to open bfs file for writing");
        return -1;
    }

    if (topo<SERVERCENTRIC) {// For servercentric networks we consider server-to-server adjacency only.
        fprintf(bfs_file,"%ld\n",servers);
        for (i=0; i<servers; i++) {
            s = get_server_i(i);
            fprintf(bfs_file,"%ld",i);
            for(j=0; j<network[s].nports; j++) {
                if (network[s].port[j].neighbour.node!=-1 && network[s].port[j].faulty!=1) { // the server is connected and the link is not faulty
                    nnode=network[s].port[j].neighbour.node;
                    if (is_server(nnode)) { // if the neighbour is a server, add it directly to the adjacency list.
                        fprintf(bfs_file,", %ld",node_to_server(nnode));
                    } else { // this is a switch; check the neighbours at every port
                        for (k=0; k<network[nnode].nports; k++) {
                            if (network[nnode].port[k].neighbour.node!=-1 && network[nnode].port[k].faulty!=1) {
                                if (network[nnode].port[k].neighbour.node!=s ) {// not the original node
                                    if(is_server(network[nnode].port[k].neighbour.node))
                                        fprintf(bfs_file,", %ld",node_to_server(network[nnode].port[k].neighbour.node));
                                    else
                                        printf("Switches (%ld and %ld) interconnected in a server-centric topology. Check topology definition is correct.\n",nnode,network[nnode].port[k].neighbour.node);
                                }
                            }
                        }
                    }
                }
            }
            fprintf(bfs_file,"\n");
        }
    } else {
        fprintf(bfs_file,"%ld\n",servers+switches);
        for (i=0; i<servers+switches; i++) {
            fprintf(bfs_file,"%ld",i);
            for(j=0; j<network[i].nports; j++) {
                if (network[i].port[j].neighbour.node!=-1 && network[i].port[j].faulty!=1) { // the server is connected and the link is not faulty
                    fprintf(bfs_file,", %ld",network[i].port[j].neighbour.node);
                }
            }
            fprintf(bfs_file,"\n");
        }
    }
    fclose(bfs_file);
    printf("wrote topology model for bfs in file %s\n",bfs_filename);
    exit(0);
    return 0; //Everything went ok
}


