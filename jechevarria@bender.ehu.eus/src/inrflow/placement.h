#ifndef _placement
#define _placement

placement_t placement;

long placement_nparam;    ///< Number of parameters passed to the placement

long placement_params[MAX_PLACEMENT_PARAMS];    ///< Parameters passed to the placement

char placement_file[100];

void init_placement(long ntasks);

long task_to_server(long task_id);

void finish_placement();
#endif
