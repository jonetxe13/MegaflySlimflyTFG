#include "applications.h"
#include "literal.h"
#include "globals.h"
#include "../kernels/trace.h"
#include "../kernels/collectives.h"
#include "../kernels/neighbours.h"
#include "../kernels/pseudoapps.h"
#include "../kernels/storageapps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/**
* Insert a CPU event in an application.
*
*@param app         The application.
*@param t_id        The task where the CPU event is inserted.
*@param computation The length of the CPU event.
*/
void insert_computation(application *app, long t_id, long computation)
{
    event ev;

    ev.type = COMPUTATION;
    ev.id = 0;
    ev.count = computation;
    ev.length = computation;
    ev.pid = t_id;
    ev.pid2 = -1;
    ev.total_subflows = 0;
    ev.subflows_aux = 0;
    ev.app = app;
    ev.type_flow = -1;

    if (computation>0)
        list_append(app->task_events[t_id], &ev);
}

/**
* Insert a SEND event in an application.
*
*@param app     The application.
*@param src     The source task.
*@param dst     The destination task.
*@param tag     The MPI tag for this message.
*@param size    The size of the message in bytes.
*@param type    The type of message (inter-processor, I/O read, I/O write, ...).
*/
void insert_send(application *app, long src, long dst, long tag, long size, int type)
{
    event ev;

    ev.type = SENDING;
    ev.id = tag;
    ev.pid = src;
    ev.length = size;
    ev.count = size;
    ev.pid2 = dst;
    ev.total_subflows = 0;
    ev.subflows_aux = 0;
    ev.app = app;
    ev.dflow.speed = FLT_MAX;
    ev.dflow.san_link = -1;
    ev.dflow.max_flows = 0;
    ev.dflow.n_max_flows = 0;
    ev.dflow.min_flows = INT_MAX;
    ev.dflow.n_min_flows = 0;
    if(mode == DYNAMIC_PHOTONIC)
        ev.dflow.allocated = 0;
    else
        ev.dflow.allocated = 1;
    ev.dflow.path = malloc(sizeof(list_t));
    ev.type_flow = type;
    //if(type == 1 || type == 3)
    //printf("S: (%ld --> %ld) %ld - %d\n", from, to, size,type);
    list_append(app->task_events[src], &ev);
}

/**
* Insert a RECV event in an application.
*
*@param app     The application.
*@param src     The source task.
*@param dst     The destination task.
*@param tag     The MPI tag for this message.
*@param size    The size of the message in bytes.
*@param type    The type of message (inter-processor, I/O read, I/O write, ...).
*/
void insert_recv(application *app, long src, long dst, long tag, long size, int type)
{
    event ev;

    ev.type = RECEPTION;
    ev.id = tag;
    ev.pid = dst;
    ev.length = size;
    ev.count = size;
    ev.pid2 = src;
    ev.total_subflows = 0;
    ev.subflows_aux = 0;
    ev.app = app;
    ev.dflow.speed = FLT_MAX;
    ev.dflow.san_link = -1;
    ev.type_flow = type;
    //if(type == 0)
    //printf("R: (%ld --> %ld) %ld\n", from, to, size);
    list_append(app->task_events[dst], &ev);
}

/**
* Insert an interprocessor SEND event in an application.
*
*@param app     The application.
*@param src     The source task.
*@param dst     The destination task.
*@param tag     The MPI tag for this message.
*@param size    The size of the message in bytes.
*/
void send(application *app, long src, long dst, long tag, long size)
{
    insert_send(app, src, dst, tag, size, 0);
}

/**
* Insert an interprocessor RECV event in an application.
*
*@param app     The application.
*@param src     The source task.
*@param dst     The destination task.
*@param tag     The MPI tag for this message.
*@param size    The size of the message in bytes.
*/
void receive(application *app, long src, long dst, long tag, long size)
{
    insert_recv(app, src, dst, tag, size, 0);
}

void gen_trace(application *app){

    switch(app->pattern){
        case ALL2ALL:
            all2all(app);
            break;
        case MANYALL2ALL:
            many_all2all(app);
            break;
        case MANYALL2ALLRND:
            many_all2all_rnd(app);
            break;
        case ONE2ALL:
            one2all(app);
            break;
        case ONE2ALLRND:
            one2all_rnd(app);
            break;
        case ALL2ONE:
            all2one(app);
            break;
        case ALL2ONERND:
            all2one_rnd(app);
            break;
        case BARRIER:
            barrier(app);
            break;
        case BCAST:
            bcast(app);
            break;
        case REDUCE:
            reduce(app);
            break;
        case ALLREDUCE: // Butterfly (higher bandwidth, but fewer steps). Another possibility is Reduce+Bcast implementation (lower bandwidth consumption, but similar to barrier)
            allreduce(app);
            break;
        case GATHER:
            gather(app);
            break;
        case SCATTER:
            scatter(app);
            break;
        case ALLGATHER:
            all2all(app);
            break;
    	case PTP:
            ptp(app);
            break;
        case RANDOM:
            randomapp(app);
            break;
        case FILE_PATTERN:
            read_trc(app);
            break;
        case MESH2DWOC:
            mesh2d_woc(app);
            break;
        case MESH2DWC:
            mesh2d_wc(app);
            break;
        case MESH3DWOC:
            mesh3d_woc(app);
            break;
        case MESH3DWC:
            mesh3d_wc(app);
            break;
        case TORUS2DWOC:
            torus2d_woc(app);
            break;
        case TORUS2DWC:
            torus2d_wc(app);
            break;
        case TORUS3DWOC:
            torus3d_woc(app);
            break;
        case TORUS3DWC:
            torus3d_wc(app);
            break;
        case WATERFALL:
            waterfall(app);
            break;
        case INVERSEBINARYTREE:
            inversebinarytree(app);
            break;
        case BINARYTREE:
            binarytree(app);
            break;
        case BUTTERFLY:
            butterfly(app);
            break;
        case GUPS:
            gups(app);
            break;
        case NBODIES:
            nbodies(app);
            break;
        case SHIFT:
            shift(app);
            break;
        case SHIFT_INCREMENTAL:
            shift_incremental(app);
            break;
        case SHIFT_RANDOM:
            shift_random(app);
            break;
        case BISECTION:
            bisection(app);
            break;
        case HOTREGION:
            hotregion(app);
            break;
        case MAPREDUCE:
            mapreduce(app);
            break;
        case RANDOMAPP:
            randomapp(app);
            break;
        case RANDOMAPPDCN:
            randomappdcn(app);
            break;
        case MARKOVAPP:
            markov_application(app);
            break;
       default:
            printf("Undefined application.\n");
	    exit(-1);
            break;
    }
}
