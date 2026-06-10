/** @mainpage
* FiConn topology tools
*/


#include <stdlib.h>
#include <stdio.h>

#include "routelist.h"

route_t* concat_nn(long a, long b)
{
	route_t *aux;

	aux=malloc(sizeof(route_t));
	aux->first=malloc(sizeof(listnode_t));
	aux->first->port=a;
	aux->last=malloc(sizeof(listnode_t));
	aux->last->port=b;
	aux->last->next=NULL;
	aux->first->next=aux->last;

	return aux;
}

route_t* newlist(long a)
{
	route_t *aux;

	aux=malloc(sizeof(route_t));
	aux->first=malloc(sizeof(listnode_t));
	aux->first->port=a;
	aux->last=aux->first;
	aux->first->next=NULL;

	return aux;
}

route_t* emptylist()
{
	route_t *aux;

	aux=malloc(sizeof(route_t));
	aux->first=NULL;
	aux->last=NULL;

	return aux;
}

route_t* concat_ll(route_t *r1, route_t *r2)
{
	r1->last->next=r2->first;
	r1->last=r2->last;

	return r1;
}

void start(route_t *r)
{
}

long get_next(route_t *r)
{
	listnode_t *aux;
	long res;

	aux=r->first;
	if (aux!=NULL) {
		res=aux->port;
		r->first=aux->next;
		if (r->first==NULL)
			r->last=NULL;
		free(aux);

		return res;
	} else
		return -1;
}
