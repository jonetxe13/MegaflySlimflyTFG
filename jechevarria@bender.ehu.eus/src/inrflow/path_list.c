#include <stdio.h>
#include <stdlib.h>
#include "path_list.h"


long delete_tail(struct path_t **path,struct path_t *path_tail)
{
	struct path_t *t1,*t2;
	if(*path == NULL && path_tail== NULL) return 1;
	//assume now that head is not null, since otherwise path_tail would
	//also be null and we would not fall through the above if statement

	if(path_tail != NULL) {
		t1=path_tail->next;
		path_tail->next = NULL;
	} else { // if(path!=NULL){
		t1=*path;
		*path = NULL;//prevents a dangling pointer
	}
	while(t1!=NULL) {
		t2=t1;
		t1=t1->next;
		free(t2);
	}
	return 1;
}

long empty_path(struct path_t **path,struct path_t **path_tail)
{
	*path_tail = NULL;
	delete_tail(path,NULL);
	*path = NULL;
	return 1;
}

long path_is_null(struct path_t *path)
{
	if(path==NULL)
		return 1;
	else
		return 0;
}

void print_path(struct path_t *path)
{
	struct path_t *curr;
	curr = path;
	while(curr!=NULL) {
		printf("%ld, ",curr->port);
		curr = curr->next;
	}
	printf(".\n");

}

void path_enqueue(struct path_t **path, struct path_t **path_tail, struct path_t **path_penultimate, long port)
{
	if(*path_tail != NULL) {
		(*path_tail)->next = malloc(sizeof(struct path_t));
		*path_penultimate = *path_tail;
		*path_tail = (*path_tail)->next;
	} else {
		*path = malloc(sizeof(struct path_t));
		*path_tail = *path;
		*path_penultimate = NULL;
	}
	(*path_tail)->port = port;

	(*path_tail)->next = NULL;
}

long path_dequeue(struct path_t **path, struct path_t **path_tail)
{
	long port;
	struct path_t *freeme;
	if(*path == NULL)
		return -1;
	else {
		//store head of path in result
		port = (*path)->port;
		//pop the head
		freeme = *path;
		*path = (*path)->next;
		free(freeme);
		if(*path==NULL)
			*path_tail = NULL;
		return port;
	}
}
