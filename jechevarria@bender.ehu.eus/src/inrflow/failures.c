#include "globals.h"
#include <stdlib.h>
#include <stdio.h>

#define SHOWCONNECTIONS 1

extern long servers; ///< The total number of servers
extern long switches;///< The total number of switches
extern long radix;	  ///< radix of the switches - assuming all switches have the same
extern long ports;	///< The total number of ports in the topology

float failure_rate;  ///< rate of the network which is faulty updates n_failures
long n_failures;  ///< number of failures

/**
 * Counts the number of failures in the network.
 */
long check_failures()
{
    long count,i,j;
    count = 0;
    for(i=0; i<servers+switches; i++)
        for(j=0; j<network[i].nports; j++)
            count+=network[i].port[j].faulty;
    return count;
}


/**
 * Selects at random which links to break
 */
void set_failures()
{
    long n;
    long i,j; // node, port

    //use probability to decide number of failures.  cast it to long
    if (n_failures==-1) // undefined
        n_failures = (long)((ports/2)*failure_rate);//ports is the number of ports (twice the number of links)


    for(n=0; n<n_failures; n++) {
        do {
            i=rand()%(servers+switches);
            j=rand()%network[i].nports;
            // Choose a different link if the link is already faulty or it is disconnected
        } while (network[i].port[j].faulty==1 ||
                network[i].port[j].neighbour.node==-1);
        network[i].port[j].faulty=1;
        network[network[i].port[j].neighbour.node].port[network[i].port[j].neighbour.port].faulty=1;
#ifdef SHOWCONNECTIONS
        printf("node %ld fails on port %ld\n",i,j);
#endif
    }

#ifdef DEBUG
    long c_failures;
    if((c_failures=check_failures()/2)!=n_failures)
        printf("n_failures = %ld but check_failures = %ld",n_failures,c_failures);
#endif //DEBUG


}

/**
 * return true if and only if either the number of failures is non-zero or the failure rate is
 * non-zero.  This works before calling init_topo().  We assume get_conf() has been called.
 * declared in globals.h
 */
bool_t are_there_failures(){
    if(n_failures>0) return TRUE;
    if(n_failures==0) return FALSE;
    if(failure_rate>0.0) return TRUE;
    return FALSE;
}



