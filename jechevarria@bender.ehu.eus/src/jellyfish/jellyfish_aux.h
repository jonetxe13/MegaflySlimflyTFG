#ifndef _jellyfish_aux
#define _jellyfish_aux

#include "allocation_jellyfish_strategies.h"

long list_is_in(list_t *l, long *l1);

long arrays_eq(long *a1, long *a2, long i);

long list_shorter_from_a_to_b(list_t *B, long **l_dst, long ecmp, long shortest_path_length);

long select_path(list_t *B, long **l_dst, long n_current, long H, long ths);

void print_array(long *arr);

long check_selected_set(long candidate, long n_selected, long *selected);

long check_selected_set_if(long candidate, long n_selected, long *selected, long *switches_inactive);


long check_distance_set_switches(long max_d, long max_delta, long candidate, long n_selected, long *selected);

void print_vector_aux(long size, long *vec);

#endif
