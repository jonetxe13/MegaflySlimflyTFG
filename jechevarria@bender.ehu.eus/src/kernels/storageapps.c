#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "storageapps.h"
#include "../inrflow/list.h"
#include "../inrflow/gen_trace.h"
#include "../inrflow/storage.h"
#include "../inrflow/globals.h"

struct properties props;

/**
 * Adds a block of computation to all nodes in the application
 *
 *@param app            The data structure storing the application information
 */
void computation_block(application *app){

    long i;
    float lambda=100.0;
    long computation;

    for(i = 0; i < app->size_tasks; i++){
        computation = lround(expon(1.0 / (1.0 / lambda))) + 1;
        insert_computation(app, i, computation);
    }
}

void communications_block(application *app, long size, long nflows, long groups, long *tag_s, long *tag_r){

    long i, j, src, dst;
    char statenew[256];
    char *stateold;
    long group_size;
    long seed;

    if(groups <= 0){
        printf("Groups size must be greater than 0.");
        exit(-1);
    }
    group_size = nflows / groups;

    for(j =0; j < groups; j++){
        seed = random();
        stateold = initstate(seed, statenew, 256);
        for(i = 0; i < group_size;i++){
            src = random() % app->size_tasks;
            dst = random()% app->size_tasks;;
            while(src == dst){
                src = random() % app->size_tasks;
                dst = random() % app->size_tasks;
            }
            if((random() % 100) < 80){
                    size = random() % 100 + 1;
                }
                else {
                    size = random() % 1000000 + 1;
                }

            send(app, src, dst, (*tag_s)++, size * 8);
        }
        setstate(stateold);
        stateold = initstate(seed, statenew, 256);
        for(i = 0; i < group_size;i++){
            src = random() % app->size_tasks;
            dst = random() %app->size_tasks;
            while(src == dst){
                src = random() % app->size_tasks;
                dst = random() % app->size_tasks;
            }
            if((random() % 100) < 80){
                    size = random() % 100 + 1;
                }
                else {
                    size = random() % 100000 + 1;
                }

            receive(app, src, dst, (*tag_r)++, size * 8);
        }
        setstate(stateold);
    }
}

void io_read_block(application *app, long total_read_size, long ntasks,long *tag_s, long *tag_r){

    long i, j;
    long read_size, io_tasks_read_size;
    long io_tasks;
    long ntasks_aux;
    long seed;

    read_size = ((((random() % 100) + 1) * total_read_size) / 100);
    seed = random();

    switch(app->data_access_mode){
        case SAN_DATA_ACCESS_MODE:
            j = 0;
            io_tasks_read_size = read_size / app->size_tasks;
            io_tasks = app->size_tasks / ntasks;
            ntasks_aux = ntasks;

            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    if(ntasks_aux== 1){
                        input_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size) + ((app->size_tasks % ntasks) * io_tasks_read_size));
                    }
                    else{
                        input_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size));
                    }
                    j += io_tasks;
                }
            }
            ntasks_aux = ntasks;
            j = -io_tasks;
            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    j += io_tasks;
                }
                if(i != j){
                    send(app, j, i, (*tag_s)++, io_tasks_read_size);
                    receive(app, j, i,(*tag_r)++, io_tasks_read_size);
                }
            }
            break;
        case RND_DATA_ACCESS_MODE:
        case CONSECUTIVE_DATA_ACCESS_MODE:
        case CACHED_DATA_ACCESS_MODE:
            j = 0;
            io_tasks_read_size = read_size / app->size_tasks;

            //for(i = 0; i < app->size_tasks; i++){
                read_data(app, tag_s, tag_r, io_tasks_read_size, 0, seed, app->size_tasks);
            //}
            break;
        default:
            printf("Unknown data access mode\n");
            exit(-1);

    }

}

void io_write_block(application *app, long total_read_size, long ntasks,long *tag_s, long *tag_r){

    long i, j;
    long read_size, io_tasks_read_size;
    long io_tasks;
    long ntasks_aux;
    long seed;

    seed = random();

    read_size = ((((random() % 100) + 1) * total_read_size) / 100);
    //printf("%ld\n", read_size);

    switch(app->data_access_mode){
        case SAN_DATA_ACCESS_MODE:
            io_tasks_read_size = read_size / app->size_tasks;
            io_tasks = app->size_tasks / ntasks;
            ntasks_aux = ntasks;
            j = -io_tasks;
            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    j += io_tasks;
                }
                if(i != j){
                    send(app, i, j, (*tag_s)++, io_tasks_read_size);
                    receive(app, i, j,(*tag_r)++, io_tasks_read_size);
                }
            }
            j = 0;
            ntasks_aux = ntasks;
            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    if(ntasks_aux== 1){
                        output_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size) + ((app->size_tasks % ntasks) * io_tasks_read_size));
                    }
                    else{
                        output_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size));
                    }
                    j += io_tasks;
                }
            }

            break;
        case RND_DATA_ACCESS_MODE:
        case CONSECUTIVE_DATA_ACCESS_MODE:
        case CACHED_DATA_ACCESS_MODE:
            j = 0;
            io_tasks_read_size = read_size / app->size_tasks;
            //for(i = 0; i < app->size_tasks; i++){
                write_data(app, tag_s, tag_r, io_tasks_read_size, 0, seed, app->size_tasks);
            //}
            break;
        default:
            printf("Unknown data acces mode\n");
            exit(-1);

    }

}


void io_input_block(application *app, long read_size, long ntasks,long *tag_s, long *tag_r){

    long i;
    long j;
    long io_tasks_read_size;
    long io_tasks_read_size_aux;
    long io_tasks;
    long ntasks_aux;
    long seed;

    seed = random();



    switch(app->data_access_mode){
        case SAN_DATA_ACCESS_MODE:
            j = 0;
            io_tasks_read_size = read_size / app->size_tasks;
            io_tasks = app->size_tasks / ntasks;
            ntasks_aux = ntasks;

            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    if(ntasks_aux== 1){
                        input_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size) + ((app->size_tasks % ntasks) * io_tasks_read_size));
                    }
                    else{
                        input_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size));
                    }
                    j += io_tasks;
                }

            }
            ntasks_aux = ntasks;
            j = -io_tasks;
            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    j += io_tasks;
                }
                if(i != j){
                    send(app, j, i, (*tag_s)++, io_tasks_read_size);
                    receive(app, j, i,(*tag_r)++, io_tasks_read_size);
                }
            }
            break;
        case RND_DATA_ACCESS_MODE:
        case CONSECUTIVE_DATA_ACCESS_MODE:
        case CACHED_DATA_ACCESS_MODE:
            j = 0;
            io_tasks_read_size = read_size / app->size_storage;
            io_tasks_read_size_aux = read_size / app->size_tasks;

            for(i = 0; i < app->size_storage; i++){
                //input_data(app, app->servers_storage[i], tag_s, tag_r, io_tasks_read_size);
                input_data(app, i, tag_s, tag_r, io_tasks_read_size);
            }
            //for(i = 0; i < app->size_tasks; i++){
                read_data(app, tag_s, tag_r, io_tasks_read_size_aux, 1, seed, app->size_tasks);
            //}
            break;
        default:
            printf("Unknown data acces mode\n");
            exit(-1);

    }
}

void io_output_block(application *app, long read_size, long ntasks,long *tag_s, long *tag_r){

    long i;
    long j;
    long io_tasks_read_size;
    long io_tasks_read_size_aux;
    long io_tasks;
    long ntasks_aux;
    long seed;

    seed = random();

    switch(app->data_access_mode){
        case SAN_DATA_ACCESS_MODE:
            io_tasks_read_size = read_size / app->size_tasks;
            io_tasks = app->size_tasks / ntasks;
            ntasks_aux = ntasks;
            j = -io_tasks;
            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    //input_data(app, j, tag_s, tag_r, io_tasks_read_size);
                    j += io_tasks;
                }
                if(i != j){
                    send(app, i, j, (*tag_s)++, io_tasks_read_size);
                    receive(app, i, j,(*tag_r)++, io_tasks_read_size);
                }
            }
            j = 0;
            ntasks_aux = ntasks;

            for(i = 0; i < app->size_tasks; i++){
                if(i % io_tasks == 0 && ntasks_aux > 0){
                    ntasks_aux--;
                    if(ntasks_aux== 1){
                        output_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size) + ((app->size_tasks % ntasks) * io_tasks_read_size));
                    }
                    else{
                        output_data(app, j, tag_s, tag_r, (io_tasks * io_tasks_read_size));
                    }
                    j += io_tasks;
                }

            }
            break;
        case RND_DATA_ACCESS_MODE:
        case CONSECUTIVE_DATA_ACCESS_MODE:
        case CACHED_DATA_ACCESS_MODE:
            j = 0;
            io_tasks_read_size = read_size / app->size_storage;
            io_tasks_read_size_aux = read_size / app->size_tasks;
            //for(i = 0; i < app->size_tasks; i++){
                write_data(app, tag_s, tag_r, io_tasks_read_size_aux, 1, seed, app->size_tasks);
            //}
            for(i = 0; i < app->size_storage; i++){
                //output_data(app, app->servers_storage[i], tag_s, tag_r, io_tasks_read_size);
                output_data(app, i, tag_s, tag_r, io_tasks_read_size);
            }
            break;
        default:
            printf("Unknown data acces mode\n");
            exit(-1);

    }

}

void transition_matrix(application *app, long n_states, long **states_matrix){

    long i, j, cum;

    for(i = 0; i < n_states; i++){
        cum = 0;
        for(j = 0; j < n_states; j++){
            cum += app->pattern_params[j];
            states_matrix[i][j] = cum;
        }
    }
}

long next_state(long current_state, long n_states, long **states_matrix, long n_current, long n_min){

    long n_state= -1;
    long i;
    long n_rand;
    long finish = 0;

    while(finish == 0){
        finish = 1;
    n_rand = random() % 100;

    for(i = 0; i < n_states; i++){
        if(states_matrix[current_state][i] > n_rand){
            n_state = i;
            if(((n_state + 1) == n_states) && (n_current < n_min)){
                finish = 0;
            }
            break;
        }
    }
    }
    return(n_state);
}

void markov_application(application *app){

    char statenew[256];
    char *stateold;
    int n_states = 5;
    long n_min = 0;
    int current_state = 0;
    long tag_s = 0;
    long tag_r = 0;
    long seed = app->seed;
    long i;
    long n_current = 0;

    long **states_matrix = malloc(sizeof(long*) * n_states);
    for (i = 0; i < n_states; i++){
        states_matrix[i] = malloc(sizeof(long) * n_states);
    }
    //srandom(seed);
    rand_val(seed);
    stateold = initstate(seed, statenew, 256);
    transition_matrix(app, n_states, states_matrix);
    set_properties(app);

    io_input_block(app, props.read_input_size, props.ntasks_input, &tag_s, &tag_r);

    while(current_state != 4){
        if(current_state == 0){
            if(verbose >=2){
                printf("COMP - ");
            }
            computation_block(app);
        }
        else if(current_state == 1){
            if(verbose >=2){
                printf("COMMS - ");
            }
            communications_block(app, props.read_input_size, props.nflows, props.groups, &tag_s, &tag_r);
        }
        else if(current_state == 2){
            if(verbose >=2){
                printf("READ - ");
            }
            io_read_block(app, props.read_size, props.ntasks_input, &tag_s, &tag_r);
        }

        else if(current_state == 3){
            if(verbose >=2){
                printf("WRITE - ");
            }
            io_write_block(app, props.write_size, props.ntasks_output, &tag_s, &tag_r);
        }
        current_state = next_state(current_state, n_states, states_matrix, n_current++, n_min);
    }

    if(verbose >=2){
        printf("\n");
    }
    io_output_block(app, props.write_output_size, props.ntasks_output, &tag_s, &tag_r);
    setstate(stateold);
    for (i = 0; i < n_states; i++){
        free(states_matrix[i]);
    }
    free(states_matrix);
}

double expon(double x){

  double z;                     // Uniform random number (0 < z < 1)
  double exp_value;             // Computed exponential value to be returned

  // Pull a uniform random number (0 < z < 1)
  do
  {
    z = rand_val(0);
  }
  while ((z == 0) || (z == 1));

  // Compute exponential random variable using inversion method
  exp_value = -x * log(z);

  return(exp_value);
}

double rand_val(int seed){

  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  static long x;               // Random int value
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // Set the seed if argument is non-zero and then return zero
  if (seed > 0)
  {
    x = seed;
    return(0.0);
  }

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;

  // Return a random value between 0.0 and 1.0
  return((double) x / m);
}

void set_properties(application *app){

    props.ntasks_input = app->pattern_params[6];
    props.ntasks_output = app->pattern_params[7];
    props.read_input_size = app->pattern_params[8];
    props.write_output_size = app->pattern_params[9];
    props.read_size = props.read_input_size;
    props.write_size = props.write_output_size;
    props.nflows = app->size_tasks * ((random() % 10) + app->size_tasks);
    props.groups = (random() % 10) + 1;
}



