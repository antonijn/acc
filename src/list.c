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

#include <stdlib.h>
#include <assert.h>

#include <acc/list.h>

struct node {
	struct node *previous;
	struct node *next;
	void *data;
};

struct list {
	struct node *head;
	struct node *last;
	size_t length;
};

static struct node *getnode(struct list *restrict l, void *data)
{
	it_t it = list_iterator(l);
	void *d;
	while (true) {
		it_t new = it;
		if (!iterator_next(&new, &d))
			break;

		if (d == data)
			return it;

		it = new;
	}

	return NULL;
}

static void rmnode(struct list *restrict l, struct node *node)
{
	assert(l != NULL);
	assert(node != NULL);
	
	if (node == l->head)
		l->head = node->next;
	if (node == l->last)
		l->last = node->previous;
	if (node->previous)
		node->previous->next = node->next;
	if (node->next)
		node->next->previous = node->previous;
	free(node);
	--l->length;
}

struct list *new_list(void *init[], int count)
{
	struct list *result = malloc(sizeof(struct list));

	if (!init) {
		result->head = NULL;
		result->last = NULL;
		result->length = 0;
		return result;
	}

	struct node *prev = NULL;
	struct node *head = NULL;
	for (int i = 0; i < count; ++i) {
		struct node *n = malloc(sizeof(struct node));
		n->data = init[i];
		n->next = NULL;
		n->previous = prev;
		if (prev)
			prev->next = n;
		if (!head)
			head = n;
		prev = n;
	}

	result->head = head;
	result->last = prev;
	result->length = count;
	return result;
}

struct list *clone_list(struct list *l)
{
	assert(l != NULL);

	struct list *result = new_list(NULL, 0);
	struct node *prev = NULL;
	struct node *head = NULL;
	void *item;
	it_t it = list_iterator(l);
	while (iterator_next(&it, &item)) {
		struct node *n = malloc(sizeof(struct node));
		n->data = item;
		n->next = NULL;
		n->previous = prev;
		if (prev)
			prev->next = n;
		if (!head)
			head = n;
		prev = n;
	}

	result->head = head;
	result->last = prev;
	result->length = list_length(l);
	return result;
}

void delete_list(struct list *l, void (*destr)(void *))
{
	assert(l != NULL);

	struct node *next = l->head;
	while (next) {
		struct node *n = next;
		if (destr)
			destr(n->data);
		next = n->next;
		free(n);
	}
	free(l);
}

it_t list_iterator(struct list *l)
{
	assert(l != NULL);

	return l->head;
}

bool iterator_next(it_t *restrict it, void **restrict item)
{
	assert(it != NULL);
	assert(item != NULL);

	struct node *n = *it;
	if (!n)
		return false;
	*item = n->data;
	*it = n->next;
	return true;
}

it_t list_rev_iterator(struct list *l)
{
	assert(l != NULL);

	return l->last;
}

bool rev_iterator_next(it_t *restrict it, void **restrict item)
{
	assert(it != NULL);
	assert(item != NULL);

	struct node *n = *it;
	if (!n)
		return false;
	*item = n->data;
	*it = n->previous;
	return true;
}

void *get_list_item(struct list *l, int idx)
{
	assert(l != NULL);

	if (idx < 0)
		idx = l->length + idx;

	assert(idx >= 0);

	struct node *n = l->head;
	while (idx-- > 0)
		n = n->next;
	return n->data;
}

void set_list_item(struct list *l, int idx, void *restrict data)
{
	assert(l != NULL);

	if (idx < 0)
		idx = l->length + idx;

	assert(idx >= 0);

	struct node *n = l->head;
	while (idx-- > 0)
		n = n->next;
	n->data = data;
}

void list_push_back(struct list *l, void *restrict data)
{
	assert(l != NULL);

	struct node *n = malloc(sizeof(struct node));
	n->next = NULL;
	n->previous = l->last;
	n->data = data;
	if (l->last)
		l->last->next = n;
	else
		l->head = n;
	l->last = n;
	++l->length;
}

void *list_pop_back(struct list *l)
{
	assert(l != NULL);

	void *data = l->last->data;

	rmnode(l, l->last);
	return data;
}

void list_push_front(struct list *l, void *restrict data)
{
	assert(l != NULL);

	struct node *n = malloc(sizeof(struct node));
	n->previous = NULL;
	n->next = l->head;
	n->data = data;
	if (l->head)
		l->head->previous = n;
	else
		l->last = n;
	l->head = n;
	++l->length;
}

void *list_pop_front(struct list *l)
{
	assert(l != NULL);

	void *data = l->head->data;

	rmnode(l, l->head);
	return data;
}

bool list_contains(struct list *l, void *restrict data)
{
	return getnode(l, data) != NULL;
}

void list_remove(struct list *l, void *restrict data)
{
	struct node *node = getnode(l, data);
	rmnode(l, node);
}

void list_remove_at(struct list *l, int idx)
{
	struct node *n = l->head;
	while (idx-- > 0)
		n = n->next;
	rmnode(l, n);
}

void *list_head(struct list *l)
{
	assert(l != NULL);
	assert(l->head != NULL);

	return l->head->data;
}

void *list_last(struct list *l)
{
	assert(l != NULL);
	assert(l->last != NULL);

	return l->last->data;
}

size_t list_length(struct list *l)
{
	assert(l != NULL);

	return l->length;
}

void dict_push_back(struct list *restrict d, void *key, void *value)
{
	list_push_back(d, key);
	list_push_back(d, value);
}

bool dict_get(struct list *restrict d, void *key, void **val)
{
	void *kact;
	it_t it = list_iterator(d);
	while (iterator_next(&it, &kact)) {
		bool suc = iterator_next(&it, val);
		assert(suc);
		if (kact == key)
			return true;
	}
	return false;
}

