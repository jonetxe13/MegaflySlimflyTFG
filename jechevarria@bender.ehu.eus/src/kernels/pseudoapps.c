#include "../inrflow/gen_trace.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * GUPS benchmark
 * Generates many random writes.
 *
 *@param app            The data structure storing the application information
 */
void gups(application *app)
{
    long i;   ///< The iteration.
    long s;  ///< The source node.
    long d; ///< The destination node.
    long seed;
    char statenew[256];
    char *stateold;

    if(app->phases < 1)
        app->phases = 1;

    seed = rand();
    stateold = initstate(seed, statenew, 256);

    for (i=0; i<app->phases; i++)
    {
        do{
            s=random() % app->size_tasks;
            d=random() % app->size_tasks;
        } while(s==d);
        send(app, s, d, 0, app->packets);
    }

    setstate(stateold);
    stateold = initstate(seed, statenew, 256);
    for (i=0; i<app->phases; i++)
    {
        do{
            s=random() % app->size_tasks;
            d=random() % app->size_tasks;
        } while(s==d);
        receive(app, s, d, 0, app->packets);
    }
    setstate(stateold);
}

/**
 * n-bodies application
 * Generates a kernel simulating a n-bodies application based on a ring arrangement
 *
 *@param app            The data structure storing the application information
 */
void nbodies(application *app)
{
    long l, i ,s;

    if(app->phases < 1)
        app->phases = 1;

    for (l = 0; l < app->phases; l++){
        for (s = 0; s < app->size_tasks; s++){
            for (i = 1; i < app->size_tasks; i=2*i){
                send(app, s, (s + i) % app->size_tasks, i, min(i,app->size_tasks-i)*app->packets);
                receive(app, (s + app->size_tasks - i) % app->size_tasks, s, i, min(i,app->size_tasks-i)*app->packets);
            }
        }
    }
}


/**
 * The Shift kernel requires a single parameter, the stride. Each node will communicate with the node whose address is src+stride.
 *
 *@param app            The data structure storing the application information
 */
void shift(application *app){

    long i, src, dst;
    long stride = app->pattern_params[1];

    if(app->phases < 1)
        app->phases = 1;

    for (i = 0; i < app->phases; i++){
        for(src = 0; src < app->size_tasks; src++){
            dst = (src + stride) % app->size_tasks;
            send(app, src, dst, i, app->packets);
        }
        for(src = 0; src < app->size_tasks; src++){
            dst = (src + stride) % app->size_tasks;
            receive(app, src, dst, i, app->packets);
        }
    }
}

/**
 * The incremental Shift kernel requires a single parameter, the stride. Each phase the stride is incremented
 * e.g., the 1st phase nodes send to stride*1, the 2nd send to stride*2 and the nth send to stride*n
 *
 *@param app            The data structure storing the application information
 */
void shift_incremental(application *app){

    long i, src, dst;
    long stride = app->pattern_params[1];

    if(app->phases < 1)
        app->phases = 1;

    for (i = 1; i <= app->phases; i++){
        for(src = 0; src < app->size_tasks; src++){
            dst = (src + (stride*i)) % app->size_tasks;
            send(app, src, dst, i, app->packets);
        }
        for(src = 0; src < app->size_tasks; src++){
            dst = (src + (stride*i)) % app->size_tasks;
            receive(app, src, dst, i, app->packets);
        }
    }
}

/**
 * A random Shift kernel in which the stride is selected randomly each phase.
 *
 *@param app            The data structure storing the application information
 */
void shift_random(application *app){

    long i, src, dst;
    long stride;

    if(app->phases < 1)
        app->phases = 1;

    for (i = 0; i < app->phases; i++){
        stride = rand() % app->size_tasks;

        for(src = 0; src < app->size_tasks; src++){
            dst = (src + stride) % app->size_tasks;
            send(app, src, dst, i, app->packets);
        }
        for(src = 0; src < app->size_tasks; src++){
            dst = (src + stride) % app->size_tasks;
            receive(app, src, dst, i, app->packets);
        }
    }
}

/**
 * Bisection kernel.
 * The network is partitioned into pairs of nodes randomly and pairs communicate. Each phase a new pair assignment is selected.
 *
 *@param app            The data structure storing the application information
 */
void bisection(application *app){

    long i, r, tmp, max_flows;
    long *shuffle;
    long curr_flows;

    max_flows= 2 * (app->size_tasks / 2);//integer division;

    if(app->phases < 1)
        app->phases = 1;

    shuffle=malloc(app->size_tasks * sizeof(long));

    for (i = 0; i < app->size_tasks; i++){
        shuffle[i]=i;
    }

    for (i = 0; i < app->phases; i++){
        for (i=0; i < app->size_tasks; i++){
            r = ztm(app->size_tasks - i);
            tmp = shuffle[i];
            shuffle[i] = shuffle[i + r];
            shuffle[i + r] = tmp;
        }

        curr_flows = 0;
        while(curr_flows < max_flows){
            if(curr_flows % 2){
                send(app, shuffle[curr_flows], shuffle[curr_flows - 1], i, app->packets);
                receive(app, shuffle[curr_flows], shuffle[curr_flows - 1], i, app->packets);
            } else{
                send(app, shuffle[curr_flows], shuffle[curr_flows + 1], i, app->packets);
                receive(app, shuffle[curr_flows], shuffle[curr_flows + 1], i, app->packets);
            }
            curr_flows++;
        }
    }
    free(shuffle);
}

/**
 * Hotregion kernel.
 * Random HotRegion communications where destinations in the first 1/8 of the network have a 1/4 probability of being chosen as destination
 *
 *@param app            The data structure storing the application information
 */
void hotregion(application *app){   ///Revision needed!!! what is a group in hotregion???

    long i, j, k, src, dst, seed;
    char statenew[256];
    char *stateold;
    long groups = app->pattern_params[2];
    long tag_s = 0;
    long tag_r = 0;
    long group_size;

    if(app->phases < 1)
        app->phases = 1;

    if(groups <= 0){
        printf("Groups size must be greater than 0.");
        exit(-1);
    }

    if(app->size_tasks<16){
        printf("At least 16 nodes required for hot region.\n");
        exit(-1);
    }

    group_size = app->pattern_params[1] / groups;

    for (k = 0; k < app->phases; k++){
        for(j =0; j < groups; j++){
            seed = rand();
            stateold = initstate(seed, statenew, 256);
            for(i = 0; i < group_size;i++){
                src = random() % (app->size_tasks);
                if(random() % 100 < 25){
                    dst = random() % (app->size_tasks / 8);
                    while(dst == src)
                        dst = random() % (app->size_tasks / 8);
                }
                else{
                    dst = random() % app->size_tasks;
                    while(dst == src)
                        dst = random() % app->size_tasks;
                }
                send(app, src, dst, tag_s++, app->packets);
            }
            setstate(stateold);
            stateold = initstate(seed, statenew, 256);
            for(i = 0; i < group_size;i++){
                src = random() % (app->size_tasks);
                if(random() % 100 < 25){
                    dst = random() % (app->size_tasks / 8);
                    while(dst == src)
                        dst = random() % (app->size_tasks / 8);
                }
                else{
                    dst = random() % app->size_tasks;
                    while(dst == src)
                        dst = random() % app->size_tasks;
                }
                receive(app, src, dst, tag_r++, app->packets);
            }
            setstate(stateold);
        }
    }
}

/**
 * MapReduce kernel.
 * The root node bcasts the data, then the workers perform a selective all-to-all, and finally the workers reduce the result into the root.
 *
 *@param app            The data structure storing the application information
 */
void mapreduce(application *app){

    long i, j, k, seed;
    char statenew[256];
    char *stateold;
    long percent = app->pattern_params[1];
    // Check whether th number of phases is given or not. If not, set to 1.
    if (app->phases < 1)
        app->phases = 1;

    for (i = 1; i < app->size_tasks; i++)	{
        send(app, 0, i, 0, app->packets);
        receive(app, 0, i, 0, app->packets);
    }

    for (k = 0; k < app->phases; k++){
            seed = rand();
            stateold = initstate(seed, statenew, 256);
        for (i = 1; i < app->size_tasks; i++){
            //insert_computation(app, i, app->comp_time);
            for (j = 1; j < app->size_tasks; j++){
                if(i != j && ((random() % 100) < percent))
                    send(app, i, j, k+1, app->packets);
            }
        }
        setstate(stateold);
        stateold = initstate(seed, statenew, 256);
        for (i = 1; i < app->size_tasks; i++){
            for (j = 1; j < app->size_tasks; j++){
                if(i != j && ((random() % 100) < percent))
                    receive(app, i, j, k+1, app->packets);
            }
        }
        setstate(stateold);
    }

    for (i = 1; i < app->size_tasks; i++)	{
        send(app, i, 0, app->phases+1, app->packets);
        receive(app, i, 0, app->phases+1, app->packets);
    }
}


/**
 * Point-to-point kernel.
 * 2 nodes send a message to each other and then wait for the other's message.
 *
 *@param app            The data structure storing the application information
 */
void ptp(application *app)
{
    long l;
    //long tag_c = 0;

    // Check whether th number of phases is given or not. If not, set to 1.
    if( app->phases < 1)
        app->phases = 1;

    for (l = 0; l < app->phases; l++){

        send(app, 0, 1, l, 10000);
        send(app, 1, 0, l, 10000);
        receive(app, 0, 1, l, 10000);
        receive(app, 1, 0, l, 10000);

//        write(app, 0, 64, &tag_c, 10000, 10);
//        read(app, 0, 64, &tag_c, 10, 10000);
        //send(app, 1, 2, tag_s++, 1000);
        //send(app, 1, 2, tag_s++, 10000);
        //send(app, 0, 9, tag_s++, 10000);
        //send(app, 1, 10, tag_s++, 100);
        //receive(app, 1, 2, tag_r++, 1000);
        //receive(app, 1, 2, tag_r++, 10000);
        //receive(app, 0, 9, tag_r++, 10000);
        //receive(app, 1, 10, tag_r++, 100);
    }
}

/**
 * Generates a kernel based on uniform random communications.
 * It differentiates from GUPS in that it includes causality whereas GUPS injects at will
 *
 *@param app            The data structure storing the application information
 */
void randomapp(application *app)  ///Revision needed!!! what is a group in randomapp???
{
    long i, j, l, tag_s, tag_r,seed, src, dst;
    char statenew[256];
    char *stateold;
    long groups = app->pattern_params[2];
    long group_size;

    if(groups <= 0){
        printf("Groups size must be greater than 0.");
        exit(-1);
    }
    group_size = app->pattern_params[1] / groups;

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    tag_s = 0;
    tag_r = 0;

    for (l = 0; l < app->phases; l++){
        for(j =0; j < groups; j++){
            seed = rand();
            stateold = initstate(seed, statenew, 256);
            for(i = 0; i < group_size;i++){
                src = random()%app->size_tasks;
                dst = random()%app->size_tasks;
                while(src == dst){
                    src = random()%app->size_tasks;
                    dst = random()%app->size_tasks;
                }
                send(app, src, dst, tag_s++, app->packets);
            }
            setstate(stateold);
            stateold = initstate(seed, statenew, 256);
            for(i = 0; i < group_size;i++){
                src = random()%app->size_tasks;
                dst = random()%app->size_tasks;
                while(src == dst){
                    src = random()%app->size_tasks;
                    dst = random()%app->size_tasks;
                }
                receive(app, src, dst, tag_r++, app->packets);
            }
            setstate(stateold);
        }
    }
}

/**
 * Generates a kernel based on uniform random communications, following typical DCN message size distributions
 *
 *@param app            The data structure storing the application information
 */
void randomappdcn(application *app) ///Revision needed!!! what is a group in randomappdcn???
{
    long i, j, l, tag_s, tag_r,seed, src, dst;
    char statenew[256];
    char *stateold;
    long groups = app->pattern_params[2];
    long group_size;
    long size;

    if(groups <= 0){
        printf("Groups size must be greater than 0.");
        exit(-1);
    }
    group_size = app->pattern_params[1] / groups;

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    tag_s = 0;
    tag_r = 0;
    for (l = 0; l < app->phases; l++){
        for(j =0; j < groups; j++){
            seed = rand();
            stateold = initstate(seed, statenew, 256);
            for(i = 0; i < group_size;i++){
                src = random()%app->size_tasks;
                dst = random()%app->size_tasks;
                while(src == dst){
                    src = random()%app->size_tasks;
                    dst = random()%app->size_tasks;
                }
                if((random() % 100) < 80){
                    size = (random() % 16) + 1;
                }
                else {
                    size = 16 + (random() % 1486);
                }
                send(app, src, dst, tag_s++, size * 8);
            }
            setstate(stateold);
            stateold = initstate(seed, statenew, 256);
            for(i = 0; i < group_size;i++){
                src = random()%app->size_tasks;
                dst = random()%app->size_tasks;
                while(src == dst){
                    src = random()%app->size_tasks;
                    dst = random()%app->size_tasks;
                }
                if((random() % 100) < 80){
                    size = (random() % 16) + 1;
                }
                else {
                    size = 16 + (random() % 1486);
                }

                receive(app, src, dst, tag_r++, size * 8);
            }
            setstate(stateold);
        }
    }
}

/**
 * Generates a kernel where nodes are split into groups and each group performs an all-to-all communication.
 * Groups are fixed and depend on task rank
 *
 *@param app            The data structure storing the application information
 */
void many_all2all(application *app){

    long j;
    long src = 0;
    long dst = 0;
    long curr_group = 0;
    long group_size = app->pattern_params[1];

    // Check whether the number of phases is given or not. If not, set to 1.
    if( app->phases < 1)
        app->phases = 1;

    for (j = 0; j < app->phases; j++){
        src = 0;
        dst = 0;
        curr_group = 0;
        while(1){
            send(app, src, dst, j, app->packets);
            src++;
            if(src % group_size == 0 || src == app->size_tasks) {
                dst++;
                if(dst == app->size_tasks) {
                    break;
                }
                else if(dst % group_size == 0) {
                    curr_group++;
                }
                src = curr_group * group_size;
            }
        }
        src = 0;
        dst = 0;
        curr_group = 0;
        while(1){
            receive(app, dst, src, j, app->packets);
            src++;
            if(src % group_size == 0 || src == app->size_tasks) {
                dst++;
                if(dst == app->size_tasks) {
                    break;
                }
                else if(dst % group_size == 0) {
                    curr_group++;
                }
                src = curr_group * group_size;
            }
        }

    }
}

/**
 * Generates a kernel where nodes are split into groups and each group performs an all-to-all communication.
 * Groups are selected randomly and vary each phase.
 *
 *@param app            The data structure storing the application information
 */
void many_all2all_rnd(application *app){

    long i, j, r, tmp;
    long src = 0;
    long dst = 0;
    long curr_group = 0;
    long group_size = app->pattern_params[1];
    long *translate;

    // Check whether th number of phases is given or not. If not, set to 1.
    if(app->phases < 1)
        app->phases = 1;

    translate=malloc(app->size_tasks * sizeof(long));

    for (i = 0; i < app->size_tasks; i++) {
        translate[i] = i;
    }

    for (j = 0; j < app->phases; j++){

        for (i = 0; i < app->size_tasks; i++) {
            r = rand() % app->size_tasks;
            tmp = translate[i];
            translate[i] = translate[r];
            translate[r] = tmp;
        }

        //for(i = 0; i < nodes; i++){
            //insert_computation(app, translate[i], app->comp_time);
        //}
        src = 0;
        dst = 0;
        curr_group = 0;
        while(1){
            send(app, translate[src], translate[dst], j, app->packets);
            src++;
            if(src % group_size == 0 || src == app->size_tasks) {
                dst++;
                if(dst == app->size_tasks) {
                    break;
                }
                else if(dst % group_size == 0) {
                    curr_group++;
                }
                src = curr_group * group_size;
            }
        }
        src = 0;
        dst = 0;
        curr_group = 0;
        while(1){
            receive(app, translate[dst], translate[src], j, app->packets);
            src++;
            if(src % group_size == 0 || src == app->size_tasks) {
                dst++;
                if(dst == app->size_tasks) {
                    break;
                }
                else if(dst % group_size == 0) {
                    curr_group++;
                }
                src = curr_group * group_size;
            }
        }
    }
    free(translate);
}
