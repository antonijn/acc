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
	struct list *result = new_list(NULL, 0);
	struct node *prev = NULL;
	struct node *head = NULL;
	void *item;
	void *it = list_iterator(l);
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

void *list_iterator(struct list *l)
{
	return l->head;
}

bool iterator_next(void **it, void **item)
{
	struct node *n = *it;
	if (!n)
		return false;
	*item = n->data;
	*it = n->next;
	return true;
}

void *list_rev_iterator(struct list *l)
{
	return l->last;
}

bool rev_iterator_next(void **it, void **item)
{
	struct node *n = *it;
	if (!n)
		return false;
	*item = n->data;
	*it = n->previous;
	return true;
}

void *get_list_item(struct list *l, int idx)
{
	struct node *n = l->head;
	while (idx-- > 0)
		n = n->next;
	return n->data;
}

void set_list_item(struct list *l, int idx, void *data)
{
	struct node *n = l->head;
	while (idx-- > 0)
		n = n->next;
	n->data = data;
}

void list_push_back(struct list *l, void *data)
{
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
	void *data = l->last->data;

	--l->length;
	struct node *nl = l->last->previous;
	if (!nl) {
		l->last = NULL;
		l->head = NULL;
		return data;
	}
	nl->next = NULL;
	free(l->last);
	l->last = nl;
	return data;
}

void list_push_front(struct list *l, void *data)
{
	struct node *n = malloc(sizeof(struct node));
	n->previous = NULL;
	n->next = l->head;
	n->data = data;
	l->head = n;
	++l->length;
}

void *list_pop_front(struct list *l)
{
	void *data = l->head->data;

	--l->length;
	struct node *nh = l->head->next;
	if (!nh) {
		l->last = NULL;
		l->head = NULL;
		return data;
	}
	nh->previous = NULL;
	free(l->head);
	l->head = nh;
	return data;
}

bool list_contains(struct list *l, void *data)
{
	void *dat;
	void *it = list_iterator(l);
	while (iterator_next(&it, &dat))
		if (dat == data)
			return true;

	return false;
}

void *list_head(struct list *l)
{
	return l->head->data;
}

void *list_last(struct list *l)
{
	return l->last->data;
}

size_t list_length(struct list *l)
{
	return l->length;
}
