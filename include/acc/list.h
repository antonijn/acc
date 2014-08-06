#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>

struct list;

struct list * new_list(void * init[], int count);
struct list * clone_list(struct list * l);
void delete_list(struct list * l, void (*destr)(void *));

void * list_iterator(struct list * l);
void * iterator_next(void ** it);

void * get_list_item(struct list * l, int idx);
void set_list_item(struct list * l, int idx, void * data);
void list_push_back(struct list * l, void * data);
void * list_pop_back(struct list * l);
void list_push_front(struct list * l, void * data);
void * list_pop_front(struct list * l);
void * list_head(struct list * l);
void * list_last(struct list * l);
size_t list_length(struct list * l);

#endif
