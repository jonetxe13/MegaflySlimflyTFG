/**
 * @file
 * @brief Linked-list for storing routed path.
 *
 * Nodes queue at the path tail and dequeue from the path head
 */
#ifndef _path_list
#define _path_list

struct path_t { ///< linked-list for path storage used in mark_route
	long port;
	struct path_t * next;
};

/**
 * Determine whether 'path' is empty
 *
 * @return 1 if 'path' is NULL, 0 otherwise
 */
long path_is_null(struct path_t *path);

/**
 * Print contents of 'path' to stdout as a comma separated list of
 * ports
 */
void print_path(struct path_t *path);

/**
 * Delete and free all nodes
 */
long empty_path(struct path_t **path,struct path_t **path_tail);

/**
 * Truncate the path at 'path_tail'
 */
long delete_tail(struct path_t **path,struct path_t *path_tail);

/**
 * Add a node at path_tail
 */
void path_enqueue(struct path_t **path, struct path_t **path_tail, struct path_t **path_penultimate, long port);


/**
 * Dequeue a node from head of 'path' and return value of its 'port'
 */
long path_dequeue(struct path_t **path, struct path_t **path_tail);
#endif
