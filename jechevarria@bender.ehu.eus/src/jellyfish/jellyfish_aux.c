#include "../inrflow/list.h"
#include "../inrflow/scheduling.h"
#include "../inrflow/globals.h"
#include "jellyfish.h"
#include "jellyfish_aux.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

extern long ports_servers;
extern long ports_switches;
extern long switches; 

long arrays_eq(long *a1, long *a2, long i){

    long j;
    long eq = 1;

    if(i <= a1[0] && i<=a2[0]){
        for(j = 0;j <= i; j++){
            if(a1[j+1] != a2[j+1]){
                eq = 0;
                break;
            }         
        }
    }
    return(eq);
}


/**
 * Decides if two lists are equal until element i.
 * ONLY works with longs.
 */
long list_is_in(list_t *l, long *l1){

    long current = 0;
    long eq = 0;
    long i = 0;
    long *l2;
    long **l2_aux;
    node_list *l_curr;

    l_curr = l->head;

    while(i < l->length){
        l2_aux = (long**)l_curr->data;
        l2 = *l2_aux;
        if(l2[0] != l1[0]){
            l_curr = l_curr->next;
            i++;
            continue;
        }

        current = 1;

        while(current < l1[0] && l1[current] == l2[current]){
            current++;
        }
        if(current == l1[0]){
            eq = 1;
            break;
        }
        else{
            l_curr = l_curr->next;
            i++;
        }
    }
    return(eq);
}

long list_shorter_from_a_to_b(list_t *B, long **l_dst, long ecmp, long shortest_path_length){

    long i = 0;
    long ret = 0;
    long **l2_aux;
    long *l2;
    long *l_aux = NULL;
    node_list *l_curr = NULL;
    node_list *l_curr_aux = NULL;
    long min = LONG_MAX;

    l_curr = B->head;

    while(i < B->length){
        l2_aux = (long**)l_curr->data;
        l2 = *l2_aux;
        if(l2[0] < min){
            min = l2[0];
            l_aux = l2;
            l_curr_aux = l_curr;
        } 
        l_curr = l_curr->next;
        i++;
    } 
    if(l_aux[0] <= shortest_path_length || ecmp == 0 ){
        ret = 1;
        *l_dst = l_aux;
        list_rem_node(B, l_curr_aux);
    }
    return(ret);
}

long select_path(list_t *B, long **l_dst, long n_current, long H, long ths){

    long i = 0;
    long ret = 0;
    long **l2_aux;
    long *l2;
    long *l_aux = NULL;
    node_list *l_curr = NULL;
    node_list *l_curr_aux = NULL;
    long min = LONG_MAX;

    l_curr = B->head;

    while(i < B->length){
        l2_aux = (long**)l_curr->data;
        l2 = *l2_aux;
        if(l2[0] < min){
            min = l2[0];
            l_aux = l2;
            l_curr_aux = l_curr;
        } 
        l_curr = l_curr->next;
        i++;
    } 
    if(l_aux[0] <= H){
        ret = 1;
        *l_dst = l_aux;
        list_rem_node(B, l_curr_aux);
    }
    else if((l_aux[0] > H) && n_current < ths){
        ret = 2;
        *l_dst = l_aux;
        list_rem_node(B, l_curr_aux);
    }
    else{
        ret = 0;
    }
    return(ret);
}

void print_array(long *arr){

    long i;

    for(i = 0; i < arr[0]; i++){
        printf("%ld - ", arr[i+1]);
    }
    printf("\n");
}

long check_selected_set(long candidate, long n_selected, long *selected){

    long k = 0;
    long j = 0;
    long i = 0;
    long res = 1;
    long found = 0;
    long *path = NULL;

    //printf("Candidate: %ld Selected: %ld\n",candidate, n_selected);
    //print_vector_aux(n_selected,selected);
    while(i < n_selected){
        found = 0;
        path = get_path(candidate, selected[i], 0); 
        //print_vector_aux(path[0]+1, path);
        j = 0;
        while(!found && res && j < path[0]-2){
            k = 0;
            while(k < n_selected){
                //printf("(%ld , %ld) - ", selected[k], path[j+2]);
                if(selected[k] == path[j+2]){
                    found = 1;
                    break;
                } 
                //printf("**** %ld %ld ****\n",n_selected,k);
                k++;
                if(k == n_selected){
                    res = 0;
                }

            }
            //printf("\n");
            j++;
        }
        i++;
    }
    //printf("----- %ld -----\n",res);
    return(res);
}

long check_selected_set_if(long candidate, long n_selected, long *selected, long *switches_inactive){

    long res = 1;
    
    return(res);
}

long check_distance_set_switches(long max_d, long max_delta, long candidate, long n_selected, long *selected){

    long i, path_length;
    long res = 1;

    for(i = 0;i< n_selected;i++){
        path_length = get_path_length(candidate, selected[i], 0); 
        if(path_length > max_d + max_delta){
            res = 0;
            break;
        }
    }
    return(res);
}


void print_vector_aux(long size, long *vec){

    long i;

    for(i = 0; i < size; i++){
        printf("%ld ", vec[i]);
    }
    printf("\n");
}

