#ifndef _mapping 
#define _mapping

#include "applications.h"

void map_application(application *next_app);

void map_application_tasks(application *app);

void map_application_storage(application *app);

void release_mapping(application *app);

long do_translation(application *app, long task, int type);

#endif

