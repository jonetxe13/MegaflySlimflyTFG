#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../inrflow/list.h"
#include "../inrflow/gen_trace.h"

/**
 * Generates a kernel performing an all-to-all interchange.
 *
 *@param app            The data structure storing the application information
 */
void all2all(application *app)
{
    long i, j, k;

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    for (k = 0; k < app->phases; k++) {
        for (i = 0; i < app->size_tasks; i++){
            for (j = 0; j < app->size_tasks; j++){
                if(i != j)
                    send(app, i, j, k, app->packets);
            }
        }
        for (i = 0; i < app->size_tasks; i++){
            for (j = 0; j < app->size_tasks; j++){
                if(i != j)
                    receive(app, i, j, k, app->packets);
            }
        }
    }
}

/**
 * Generates a kernel performing a one-to-all interchange. (non-optimized bcast)
 *
 *@param app            The data structure storing the application information
 */
 void one2all(application *app)
{
    long i, l;

    // Check whether the number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    for (l = 0; l < app->phases; l++){
        for (i = 1; i < app->size_tasks; i++)
        {
            send(app, 0, i, l, app->packets);
            receive(app, 0, i, l, app->packets);
        }
    }
}

/**
 * Generates a kernel performing a one-to-all interchange (non-optimized bcast), with a random root in each phase
 *
 *@param app            The data structure storing the application information
 */
void one2all_rnd(application *app)
{
    long i, l;
    long src;

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    for (l = 0; l < app->phases; l++){
        src = rand() % app->size_tasks;
        for (i = 0; i < app->size_tasks; i++)
        {
            if(src != i){
                send(app, src, i, l, app->packets);
                receive(app,  src, i, l, app->packets);
            }
        }
    }
}

/**
 * Generates a kernel performing an all-to-one interchange
 *
 *@param app            The data structure storing the application information
 */
void all2one(application *app)
{
    long i, l;

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    for (l = 0; l < app->phases; l++){
        for (i = 1; i < app->size_tasks; i++)
        {
            send(app, i, 0, l, app->packets);
            receive(app, i, 0, l, app->packets);
        }
    }
}

/**
 * Generates a kernel performing an all-to-one interchange with a random root in each phase
 *
 *@param app            The data structure storing the application information
 */
void all2one_rnd(application *app)
{
    long i, l;
    long dst;

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    for (l = 0; l < app->phases; l++){
        dst = rand() % app->size_tasks;
        for (i = 0; i < app->size_tasks; i++)
        {
            if(dst != i){
                send(app, i, dst, l, app->packets);
                receive(app, i, dst, l, app->packets);
            }
        }
    }
}

/**
 * Generates a kernel performing an logarithmic, optimized all-to-one interchange
 *
 *@param app            The data structure storing the application information
 */
void binarytree(application *app)
{
    long i;
    long N=1;	///< 2 ^ nodes
    long k=2;		///< 2 ^ N
    long l;
    long n;					///< The iteration.
    long pn, pn1;				///< 2 ^ n & 2 ^ (n+1)

    while (k<app->size_tasks)
    {
        N++;
        k=2*k;
    }

    if(app->phases < 1)
        app->phases = 1;

    for (l=0; l<app->phases; l++){
        for (i=0; i<app->size_tasks; i++)
        {
            pn=1;
            pn1=2;
            for (n=0; n<N; n++)
            {
                if ((i%pn1) && !(i % pn))
                    send(app, i, i - pn, l, app->packets);
                if ( !(i%pn1) && (i+pn)<app->size_tasks)
                    receive(app, i + pn, i, l, app->packets);
                pn=pn1;
                pn1=2*pn1;
            }
        }
    }
}

/**
 * Generates a kernel performing an logarithmic, optimized one-to-all interchange
 *
 *@param app            The data structure storing the application information
 */
void inversebinarytree(application *app)
{
    long i;
    long N=1;	///< 2 ^ nodes
    long k=2;		///< 2 ^ N
    long l;
    long n;					///< The iteration.
    long pn, pn1;				///< 2 ^ n & 2 ^ (n+1)

    while (k<app->size_tasks)
    {
        N++;
        k=2*k;
    }

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    for (l=0; l<app->phases; l++){
        for (i=0; i<app->size_tasks; i++)
        {
            pn=k/2;
            pn1=k;
            for (n=0; n<N; n++)
            {
                if ((i%pn1) && !(i % pn))
                    receive(app, i - pn, i, l, app->packets);
                pn1=pn;
                pn=pn1/2;
            }
            pn=k/2;
            pn1=k;
            for (n=0; n<N; n++)
            {
                if ( !(i%pn1) && (i+pn)<app->size_tasks)
                    send(app, i, i + pn, l, app->packets);
                pn1=pn;
                pn=pn1/2;
            }
        }
    }
}

/**
 * Generates a kernel performing an optimised, logarithmic all-to-all
 *
 *@param app            The data structure storing the application information
 */
void butterfly(application *app)
{
    long N=0; ///< log2 nodes
    long k=1; ///< 2 ^ N
//    long tag_s = 0;
//    long tag_r = 0;
    long l;
    long i;					///< The number of the node.
    long n;
    long pn, pn1;				///< 2 ^ n & 2 ^ (n+1)

    while (k<app->size_tasks){
        N++;
        k = 2 * k;
    }

    // Check whether the number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    for (l = 0; l < app->phases; l++){
        for (i = 0; i < app->size_tasks; i++){
            pn = 1;
            pn1 = 2;
            for(n = 0; n < N; n++)
            {
                if (i % pn1 < pn)
                {
                    if (i + pn < app->size_tasks){
                        send(app, i, i + pn, l, app->packets);
                        receive(app, i + pn, i, l, app->packets);
                    }
                }
                else
                {
                    if (i - pn >= 0){
                        send(app, i, i - pn, l, app->packets);
                        receive(app, i - pn, i, l, app->packets);
                    }
                }
                pn = pn1;
                pn1 = 2 * pn1;
            }
        }
    }
}

/**
 * Generates a kernel performing a barrier, based on logarithmic all-to-one and one-to-all collectives
 *
 *@param app            The data structure storing the application information
 */
void barrier(application *app)
{
    binarytree(app);
    inversebinarytree(app);
}

/**
 * Generates a kernel performing a bcast, based on logarithmic one-to-all collective
 *
 *@param app            The data structure storing the application information
 */
void bcast(application *app)
{
    inversebinarytree(app);
}

/**
 * Generates a kernel performing a reduction, based on logarithmic all-to-one collective
 *
 *@param app            The data structure storing the application information
 */
void reduce(application *app)
{
    binarytree(app);
}

/**
 * Generates a kernel performing an allreduce, based on logarithmic all-to-all collective
 *
 *@param app            The data structure storing the application information
 */
void allreduce(application *app)
{
    butterfly(app);
}

/**
 * Generates a kernel performing a bcast, based on logarithmic one-to-all collective
 *
 *@param app            The data structure storing the application information
 */
void gather(application *app)
{
    all2one(app);
}

/**
 * Generates a kernel performing a reduction, based on logarithmic all-to-one collective
 *
 *@param app            The data structure storing the application information
 */
void scatter(application *app)
{
    one2all(app);
}


