#ifndef _list
#define _list

typedef struct node_list {
  void *data;
  struct node_list *next;
  struct node_list *prev;
} node_list;
 
typedef struct list_t {
  long length;
  int t_size;
  struct node_list *head;
  struct node_list *tail;
  struct node_list *bookmark;
  struct node_list *current;
  struct node_list *rem_current;
} list_t;
 
void list_initialize(list_t *list, int t_size);

void list_destroy(list_t *list);
 
node_list *list_insert(list_t *list, void *elem);

void list_append(list_t *list, void *elem);

void list_bookmark(list_t *list);

long list_length(list_t *list);

long list_empty(list_t *l);

void list_head(list_t *list, void **elem);

void list_head_node(list_t *list, void **elem);

void list_tail(list_t *list, void **elem);

void list_tail_node(list_t *l, void **elem);

void list_reset(list_t *l);

long list_next(list_t *l, void **elem);

long list_prev(list_t *l, void **elem);

void list_rem_head(list_t *l);

void list_rem_current(list_t *l);

void list_rem_tail(list_t *l);

void list_trunc(list_t *l);

void list_concat(list_t *l1, list_t *l2);

void list_rem_elem(list_t *l);

void list_rem_head_append(list_t *sl, list_t *dl);

void list_rem_node(list_t *li, node_list *node);

void list_get_i(list_t *l, long num, void **elem);

void list_copy(list_t *l_src, list_t *l_dst, long i, long j);

long list_eq(list_t *l1, list_t *l2, long i);

void list_concat2(list_t *l_dst, list_t *l1, list_t *l2);

#endif
