#ifndef _allocation
#define _allocation

#include "globals.h"
#include "applications.h"

allocation_t allocation; ///< the allocation strategy we are using in the simulation

char* allocation_name; ///< the name of the used allocation strategy as a string

long allocation_nparam;    ///< Number of parameters passed to the allocation strategy

long allocation_params[MAX_SCHEDULING_PARAMS];    ///< Parameters passed to the allocation strategy

long allocate_application(application *next_app);

long allocate_application_tasks(application *app);

long allocate_application_storage(application *app); 

void release_application(application *app);

void assign_active_cores(application *app, long *cores);

void assign_servers_storage(application *app, long *servers_storage);

void release_servers_storage(application *app);

void assign_inactive_cores(application *app, long *cores_inactive, long n_cores_inactive);

void assign_inactive_switches(application *app, long *switches_inactive, long n_switches_inactive);

void assign_active_switches(application *app, long *switches_active, long n_switches_active);

void assign_switches_paths(application *app, long ports_switches);

void release_active_cores(application *app);

void release_inactive_cores(application *app);

void release_inactive_switches(application *app);

void release_active_switches(application *app);

void release_switches_paths(application *app, long ports_switches);
#endif

