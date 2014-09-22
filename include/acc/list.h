/*
 * Linked list utility
 * Copyright (C) 2014  Antonie Blom
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

typedef struct node *it_t;

struct list;

struct list *new_list(void *init[], int count);
struct list *clone_list(struct list *l);
void delete_list(struct list *l, void (*destr)(void *));

it_t list_iterator(struct list *l);
bool iterator_next(it_t *restrict it, void **restrict item);
it_t list_rev_iterator(struct list *l);
bool rev_iterator_next(it_t *restrict it, void **restrict item);

void *get_list_item(struct list *l, int idx);
void set_list_item(struct list *l, int idx, void *restrict data);
void list_push_back(struct list *l, void *restrict data);
void *list_pop_back(struct list *l);
void list_push_front(struct list *l, void *restrict data);
void *list_pop_front(struct list *l);
bool list_contains(struct list *l, void *restrict data);
void list_remove(struct list *l, void *restrict data);
void *list_head(struct list *l);
void *list_last(struct list *l);
size_t list_length(struct list *l);

void dict_push_back(struct list *restrict d, void *key, void *value);
bool dict_get(struct list *restrict d, void *key, void **value);

#endif
