#include "../inrflow/gen_trace.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * 2D Mesh without causality.
 * Generates a kernel in which nodes in a virtual 2D mesh send to all neighbours before waiting for their messages.
 *
 *@param app            The data structure storing the application information
 */
void mesh2d_woc(application *app)
{
    long dist;      ///< nodes ^ 1/2
    long i,j;         ///< The number of the node.

    if ((dist=(long)sqrt(app->size_tasks))!=(long)ceil(sqrt(app->size_tasks))){
        printf("Number of Nodes is not a quadratic value\n");
        exit(0);
    }
    else
    {
        if(app->phases < 1)
            app->phases = 1;

        for (j=0; j<app->phases; j++) {
            for (i=0; i<app->size_tasks; i++){
                if ((i%dist)<(dist-1))  // i Sends to +1
                    send(app, i, i + 1, j, app->packets);
                if ((i%dist)!=0)        // i Sends to -1
                    send(app, i, i - 1, j, app->packets);
                if ((i/dist)<(dist-1))  // i Sends to +dist
                    send(app, i, i + dist, j, app->packets);
                if ((i/dist)!=0)        // i Sends to -dist
                    send(app, i, i - dist, j, app->packets);
            }

            for (i=0; i<app->size_tasks; i++){
                if ((i%dist)<(dist-1))  // +X waits for i
                    receive(app, i, i + 1, j, app->packets);
                if ((i%dist)!=0)        // -X waits for i
                    receive(app, i, i - 1, j, app->packets);
                if ((i/dist)<(dist-1))  // +Y waits for i
                    receive(app, i, i + dist, j, app->packets);
                if ((i/dist)!=0)        // -Y waits for i
                    receive(app, i, i - dist, j, app->packets);
            }
        }
    }
}

/**
 * 2D Mesh with causality.
 * Generates a kernel in which nodes in a virtual 2D mesh perform a sweep pattern towards X+ & Y+.
 *
 *@param app            The data structure storing the application information
 */
void mesh2d_wc(application *app)
{
    long dist;      ///< nodes ^ 1/2
    long i,j;         ///< The number of the node.

    if ((dist=(long)sqrt(app->size_tasks))!=(long)ceil(sqrt(app->size_tasks))){
        printf("Number of Nodes is not a quadratic value\n");
        exit(0);
    }
    else
    {
        if(app->phases < 1)
            app->phases = 1;

        for (j=0; j<app->phases; j++) {
            for (i=0; i<app->size_tasks; i++){
                if ((i%dist)<(dist-1))  // Sends to +1
                    send(app, i, i + 1, j, app->packets);
                if ((i/dist)<(dist-1))  // Sends to +dist
                    send(app, i, i + dist, j, app->packets);

                if ((i%dist)<(dist-1))  // +X waits for i
                    receive(app, i, i + 1, j, app->packets);
                if ((i/dist)<(dist-1))  // +Y waits for i
                    receive(app, i, i + dist, j, app->packets);
            }
            // Last task synchronizes with root task (0)
            send(app, app->size_tasks - 1 , 0, j, app->packets);
            receive(app, app->size_tasks - 1 , 0, j, app->packets);
        }
    }
}

/**
 * 3D Mesh without causality.
 * Generates a kernel in which nodes in a virtual 3D mesh send to all neighbours before waiting for their messages.
 *
 *@param app            The data structure storing the application information
 */
void mesh3d_woc(application *app)
{
    long i,j;         ///< The number of the node.
    long dist;      ///< nodes ^ 1/3
    long dist2;     ///< nodes ^ 2/3
//    long tag_s = 0;       ///< The tag
//    long tag_r = 0;       ///< The tag


    dist=(long)pow(app->size_tasks, 1.0/3);
    while(dist*dist*dist<app->size_tasks)
        dist++;
    if (dist*dist*dist!=app->size_tasks){
        printf("Number of Nodes is not a cubic value.\n");
        exit(0);
    }
    else
    {
        if(app->phases < 1)
            app->phases = 1;

        dist2=dist*dist;
        for (j=0; j<app->phases; j++) {
            for (i=0; i<app->size_tasks; i++){
                if (i%dist<dist-1)              // Sends to +X
                    send(app, i, i + 1, j, app->packets);
                if (i%dist)                     // Sends to -X
                    send(app, i, i - 1, j, app->packets);
                if ((i/dist)%dist<dist-1)       // Sends to +Y
                    send(app, i, i + dist, j, app->packets);
                if ((i/dist)%dist)              // Sends to -Y
                    send(app, i, i - dist, j, app->packets);
                if (i/dist2<dist-1)             // Sends for +Z
                    send(app, i, i + dist2, j, app->packets);
                if (i/dist2)                    // Sends to -Z
                    send(app, i, i - dist2, j, app->packets);
            }

            for (i=0; i<app->size_tasks; i++){
                if (i%dist<dist-1)              // +X Waits for i
                    receive(app, i, i + 1, j, app->packets);
                if (i%dist)                     // -X Waits for i
                    receive(app, i, i - 1, j, app->packets);
                if ((i/dist)%dist<dist-1)       // +Y Waits for i
                    receive(app, i, i + dist, j, app->packets);
                if ((i/dist)%dist)              // -Y Waits for i
                    receive(app, i, i - dist, j, app->packets);
                if (i/dist2<dist-1)             // +Z Waits for i
                    receive(app, i, i + dist2, j, app->packets);
                if (i/dist2)                    // -Z Waits for i
                    receive(app, i, i - dist2, j, app->packets);
            }
        }
    }
}

/**
 * 3D Mesh with causality.
 * Generates a kernel in which nodes in a virtual 3D mesh perform a sweep pattern towards X+ & Y+ & Z+.
 *
 *@param app            The data structure storing the application information
 */
void mesh3d_wc(application *app)
{
    long i,j;         ///< The number of the node.
    long dist;      ///< nodes ^ 1/3
    long dist2;     ///< nodes ^ 2/3
//    long tag_s = 0;       ///< The tag
//    long tag_r = 0;       ///< The tag


    dist=(long)pow(app->size_tasks, 1.0/3);
    while(dist*dist*dist<app->size_tasks)
        dist++;
    if (dist*dist*dist!=app->size_tasks){
        printf("Number of Nodes is not a cubic value.\n");
        exit(0);
    }
    else
    {
        if(app->phases < 1)
            app->phases = 1;

        dist2=dist*dist;
        for (j=0; j<app->phases; j++) {
            for (i=0; i<app->size_tasks; i++){
                if (i%dist<dist-1)              // Sends to +X
                    send(app, i, i + 1, j, app->packets);
                if ((i/dist)%dist<dist-1)       // Sends to +Y
                    send(app, i, i + dist, j, app->packets);
                if (i/dist2<dist-1)             // Sends for +Z
                    send(app, i, i + dist2, j, app->packets);

                if (i%dist<dist-1)              // +X waits for i
                    receive(app, i, i + 1, j, app->packets);
                if ((i/dist)%dist<dist-1)       // +Y waits for i
                    receive(app, i, i + dist, j, app->packets);
                if (i/dist2<dist-1)             // +Z waits for i
                    receive(app, i, i + dist2, j, app->packets);
            }
            // Last task synchronizes with root task (0)
            send(app, app->size_tasks - 1 , 0, j, app->packets);
            receive(app, app->size_tasks - 1 , 0, j, app->packets);
        }
    }
}

/**
 * 2D torus without causality.
 * Generates a kernel in which nodes in a virtual 2D torus sends to all neighbours before waiting for their messages.
 *
 *@param app            The data structure storing the application information
 */
void torus2d_woc(application *app){

    long dist;      ///< nodes ^ 1/2
    long i, j;      ///< The number of the node.
    long x,y;       ///< The coordinates of the node

    if ((dist=(long)sqrt(app->size_tasks))!=(long)ceil(sqrt(app->size_tasks))){
        printf("Number of Nodes is not a quadratic value\n");
        exit(-1);
    }
    else
    {
        if (app->phases<1)
            app->phases=1;
        for (j = 0; j < app->phases; j++) {
            for (i = 0; i < app->size_tasks; i++){
                x=i%dist;
                y=i/dist;
                send(app, i, ((x + 1) % dist)+(y*dist), j, app->packets);
                send(app, i, ((x + dist - 1) % dist)+(y*dist), j, app->packets);
                send(app, i, x + (((y + 1) % dist)*dist), j, app->packets);
                send(app, i, x + (((y + dist - 1) % dist)*dist), j, app->packets);
            }
            for (i = 0; i < app->size_tasks; i++){
                x=i%dist;
                y=i/dist;
                receive(app, i, ((x + 1) % dist)+(y*dist), j, app->packets);
                receive(app, i, ((x + dist - 1) % dist)+(y*dist), j, app->packets);
                receive(app, i, x + (((y + 1) % dist)*dist), j, app->packets);
                receive(app, i, x + (((y + dist - 1) % dist)*dist), j, app->packets);
            }
        }
    }
}

/**
 * 2D torus with causality.
 * Generates a kernel in which nodes in a virtual 2D torus perform a flood pattern from node 0 in all directions.
 *
 *@param app            The data structure storing the application information
 */
void torus2d_wc(application *app){

    long dist;      ///< nodes ^ 1/2
    long i, j, t, n;         ///< The number of the node , the number of phase, the position of the tail of queue, and the neighbour node.
    long x,y;       ///< The coordinates of the node
    long *visited;	///< how many times a node has been visited (should be 4 at the end of the simulation)
    long *breadth_order;	///< a static implementation of a queue to which newly visited nodes are added to flood the system in order

    if ((dist=(long)sqrt(app->size_tasks))!=(long)ceil(sqrt(app->size_tasks))){
        printf("Number of Nodes is not a quadratic value\n");
        exit(-1);
    }
    else
    {
        if (app->phases<1)
            app->phases=1;

    	visited=malloc(app->size_tasks*sizeof(long));
    	breadth_order=malloc(app->size_tasks*sizeof(long));
        for (j = 0; j < app->phases; j++) {
			breadth_order[0]=0; // We could also start from a random node
			t=1; // The index of the breadth order queue tail
			visited[0]=1;
			for (i = 1; i < app->size_tasks; i++)
				visited[i]=0;

            for (i = 0; i < app->size_tasks; i++){
                x=breadth_order[i]%dist;
                y=breadth_order[i]/dist;
				// printf("visit node %ld (%ld, %ld)\n",breadth_order[i], x, y);

                // X+
                n=((x + 1) % dist)+(y*dist);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

                // X-
                n=((x + dist - 1) % dist)+(y*dist);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

				// Y+
                n=x + (((y + 1) % dist)*dist);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

				// Y-
                n=x + (((y + dist - 1) % dist)*dist);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);
            }
        }
        free(visited);
        free(breadth_order);
    }
}

/**
 * 3D torus without causality.
 * Generates a kernel in which nodes in a virtual 3D torus sends to all neighbours before waiting for their messages.
 *
 *@param app            The data structure storing the application information
 */
void torus3d_woc(application *app){

    long i, j;         ///< The number of the node.
    long dist;      ///< nodes ^ 1/3
    long dist2;     ///< nodes ^ 2/3
    long x,y,z;       ///< The coordinates of the node


    dist = (long)pow(app->size_tasks, 1.0 / 3);
    while(dist * dist * dist < app->size_tasks)
        dist++;
    if (dist * dist * dist != app->size_tasks){
        printf("Number of Nodes is not a cubic value.\n");
        exit(0);
    }
    else
    {
        if (app->phases<1)
            app->phases=1;
        dist2 = dist * dist;
        for (j = 0; j < app->phases; j++){
            for (i = 0; i < app->size_tasks; i++){
                x=i%dist;
                y=(i/dist)%dist;
                z=i/dist2;
                send(app, i, ((x + 1) % dist) + (y*dist) + (z*dist2), j, app->packets);
                send(app, i, ((x + dist - 1) % dist) + (y*dist) + (z*dist2), j, app->packets);
                send(app, i, x + (((y + 1) % dist)*dist) + (z*dist2), j, app->packets);
                send(app, i, x + (((y + dist - 1) % dist)*dist) + (z*dist2), j, app->packets);
                send(app, i, x + (y*dist) + (((z + 1) % dist)*dist2), j, app->packets);
                send(app, i, x + (y*dist) + (((z + dist - 1) % dist)*dist2), j, app->packets);
            }
            for (i = 0; i < app->size_tasks; i++){
                x=i%dist;
                y=(i/dist)%dist;
                z=i/dist2;
                receive(app, i, ((x + 1) % dist) + (y*dist) + (z*dist2), j, app->packets);
                receive(app, i, ((x + dist - 1) % dist) + (y*dist) + (z*dist2), j, app->packets);
                receive(app, i, x + (((y + 1) % dist)*dist) + (z*dist2), j, app->packets);
                receive(app, i, x + (((y + dist - 1) % dist)*dist) + (z*dist2), j, app->packets);
                receive(app, i, x + (y*dist) + (((z + 1) % dist)*dist2), j, app->packets);
                receive(app, i, x + (y*dist) + (((z + dist - 1) % dist)*dist2), j, app->packets);
            }

        }
    }
}

/**
 * 3D torus with causality.
 * Generates a kernel in which nodes in a virtual 3D torus perform a flood pattern from node 0 in all directions.
 *
 *@param app            The data structure storing the application information
 */
void torus3d_wc(application *app){

    long i, j, t, n;         ///< The number of the node.
    long dist;      ///< nodes ^ 1/3
    long dist2;     ///< nodes ^ 2/3
    long x,y,z;       ///< The coordinates of the node
	long *visited;	///< how many times a node has been visited (should be 6 at the end of the simulation)
	long *breadth_order;	///< a static implementation of a queue to which newly visited nodes are added to flood the system in order

    dist = (long)pow(app->size_tasks, 1.0 / 3);
    while(dist * dist * dist < app->size_tasks)
        dist++;
    if (dist * dist * dist != app->size_tasks){
        printf("Number of Nodes is not a cubic value.\n");
        exit(0);
    }
    else
    {
        if (app->phases<1)
            app->phases=1;
        dist2 = dist * dist;

    	visited=malloc(app->size_tasks*sizeof(long));
    	breadth_order=malloc(app->size_tasks*sizeof(long));
        for (j = 0; j < app->phases; j++) {
			breadth_order[0]=0; // We could also start from a random node
			t=1; // The index of the breadth order queue tail
			visited[0]=1;
			for (i = 1; i < app->size_tasks; i++)
				visited[i]=0;

            for (i = 0; i < app->size_tasks; i++){
                x=breadth_order[i]%dist;
                y=(breadth_order[i]/dist)%dist;
                z=breadth_order[i]/dist2;
				// printf("visit node %ld (%ld, %ld, %ld)\n",breadth_order[i], x, y,z);

                // X+
                n=((x + 1) % dist)+(y*dist)+(z*dist2);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

                // X-
                n=((x + dist - 1) % dist)+(y*dist)+(z*dist2);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

				// Y+
                n= x + (((y + 1) % dist)*dist)+(z*dist2);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

				// Y-
                n=x + (((y + dist - 1) % dist)*dist)+(z*dist2);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

                // Z+
                n= x +(y*dist) + (((z + 1) % dist)*dist2);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);

				// Z-
                n= x +(y*dist) + (((z + dist - 1) % dist)*dist2);
				if (visited[n]++ == 0)
					breadth_order[t++]=n;
				send(app, i, n, j, app->packets);
                receive(app, i, n, j, app->packets);
            }
        }
        free(visited);
        free(breadth_order);
    }
}


/**
 * Similar to the waterfalls in LU.
 * Generates a kernel in which nodes in a virtual 2D mesh perform many sweep patterns towards X+ & Y+.
 * Similar to mesh2d_wc, but waterfall generates all sweeps together
 *
 *@param app            The data structure storing the application information
 */
void waterfall(application *app)
{
    long i;                                 ///< The number of the node.
    long dist;      ///< nodes ^ 1/2
    long n;                                 ///< The total amount of bytes sent
    long size_aux;

    if ((dist=(long)sqrt(app->size_tasks))!=(long)ceil(sqrt(app->size_tasks))){
        printf("Number of Nodes is not a quadratic value\n");
        exit(0);
    }
    else
    {
        if(app->phases < 1)
            app->phases = 10;

        size_aux = app->packets / app->phases;

        for (i = 0; i < app->size_tasks; i++)
            for (n = 0;  n < app->phases; n++)
            {
                if (i % dist)             // Waits for -1
                    receive(app, i - 1, i, n, size_aux);
                if (i / dist)             // Waits for -dist
                    receive(app, i - dist, i, n, size_aux);
                if (i%dist<dist-1)      // Sends to +1
                    send(app, i, i + 1, n, size_aux);
                if (i/dist<dist-1)      // Sends to +dist
                    send(app, i, i + dist, n, size_aux);
            }
    }
}
