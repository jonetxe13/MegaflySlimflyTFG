#ifndef _routelist
#define _routelist

//typedef struct listnode_t listnode_t;

typedef struct listnode_t {
	long port;
	//listnode_t* next;
	struct listnode_t* next;
} listnode_t;


/**
* Structure that defines a route list. Very inefficient but easy to implement, should be replaced by something better in the future.
*/
typedef struct route_t {
	listnode_t *first;	///< each step in the route.
	listnode_t *last;		///< the number of 'valid' steps.
} route_t;

route_t* concat_nn(long a, long b);

route_t* newlist(long a);

route_t* emptylist();

route_t* concat_ll(route_t *r1, route_t *r2);

void start(route_t *r);

long get_next(route_t *r);

#endif // _routelist
