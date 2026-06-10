#include "mapping.h"
#include "allocation.h"
#include "applications.h"
#include "scheduling.h"
#include "globals.h"
#include "../jellyfish/allocation_jellyfish.h"
#include <stdio.h>
#include <stdlib.h>

void map_application(application *app){

    app->translation = malloc(sizeof(long) * app->size);
    map_application_tasks(app);
    map_application_storage(app);
}

void map_application_tasks(application *app){

    long i, j;
    long *translation_aux = NULL;
    long core;

    switch(app->mapping){
        case CONSECUTIVE_MAP:
            for(i = 0; i < app->size_tasks; i++){
                app->translation[i] = app->cores_active[i];
            } 
            break;
        case RANDOM_MAP:
            translation_aux = calloc(app->size_tasks, sizeof(long));
            for(j = 0; j < app->size_tasks; j++){
            core = rand() % app->size_tasks;
                while(translation_aux[core] == 1){
                    core = rand() % app->size_tasks;
                }
                translation_aux[core] = 1;
                app->translation[j] = app->cores_active[core];
            }
            free(translation_aux);
            break;
        default:
            printf("Unknown mapping strategy.\n");
            exit(-1);
    }
}

void map_application_storage(application *app){

    long i;
    long n_tasks =  app->size_tasks;

    for(i = n_tasks; i < (n_tasks + app->size_storage); i++){
                app->translation[i] = app->servers_storage[i - n_tasks];
    } 
}

void release_mapping(application *app){

    free(app->translation);
}

long do_translation(application *app, long task, int type){

    long server = -1;;

    server = get_server_i(app->translation[task])/server_cores;

    return(server);
}
