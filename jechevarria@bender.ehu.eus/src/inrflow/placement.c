#include "io.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>

long *translation;
char *placement_name;

void init_placement(long ntasks){
    
    long i, k;    
    translation = malloc(sizeof(long) * ntasks);

    switch(placement){

        case SEQUENTIAL_PLC:
            for(i = 0; i < ntasks; i++)
                translation[i] = i;
            break;
        case RANDOM_PLC:
            for(i = 0; i < ntasks; i++)
                translation[i] = -1;
            i = 0;
            while(i < ntasks){
                k = random() % ntasks;
                if(translation[k] == -1){
                    translation[k] = i;
                    i++;
                }
            }
            break;
        case PATTERN_PLC:
            break;
        case FILE_PLC:
            k = load_mapping_from_file(placement_file, ntasks, translation);
            if(k != ntasks){
                printf("The number of nodes-tasks is lower than ntasks.\n");
                exit(-1);
            }
            break;
        default:
            printf("Unknown placement strategy\n");
            exit(-1);
    }
}

long task_to_server(long task_id){

    return(translation[task_id]);
}

void finish_placement(){

    free(translation);
}

