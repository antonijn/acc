/*
 * Intermediate tagging tools
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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <acc/itm/tag.h>
#include <acc/itm/ast.h>
#include <acc/list.h>

struct itm_tag {
	const char *const *type;
	enum itm_tag_object object;
	void (*free)(void *data);
	void (*print)(FILE *f, void *data);

	union {
		int i;
		void *data;
	} value;
};

static void free_expr_list(void *data)
{
	delete_list(data, NULL);
}

struct itm_tag *new_itm_tag(const char *const *type, enum itm_tag_object obj)
{
	struct itm_tag *tag = malloc(sizeof(struct itm_tag));

	assert(type != NULL);

	tag->type = type;
	tag->object = obj;
	tag->free = (obj == TO_EXPR_LIST) ? &free_expr_list : NULL;
	tag->value.data = (obj == TO_EXPR_LIST) ? new_list(NULL, 0) : NULL;
	tag->print = NULL;

	return tag;
}

void delete_itm_tag(struct itm_tag *tag)
{
	if (tag->free)
		tag->free(tag->value.data);
}

static void print_expr_list(FILE *f, it_t it)
{
	struct itm_expr *e;
	bool first = true;
	while (iterator_next(&it, (void **)&e)) {
		if (!first)
			fprintf(f, ", ");
		else
			first = false;
		e->to_string(f, e);
	}
}

void itm_tag_to_string(FILE *f, struct itm_tag *tag)
{
	if (tag->print) {
		tag->print(f, tag->value.data);
		return;
	}

	fprintf(f, "%s(", *tag->type);
	switch (tag->object) {
	case TO_INT:
		fprintf(f, "%d", tag->value.i);
		break;
	case TO_EXPR_LIST:
		print_expr_list(f, list_iterator(itm_tag_get_list(tag)));
		break;
	}
	fprintf(f, ")");
}

const char *const *itm_tag_type(struct itm_tag *tag)
{
	return tag->type;
}

enum itm_tag_object itm_tag_object(struct itm_tag *tag)
{
	return tag->object;
}

void itm_tag_seti(struct itm_tag *tag, int i)
{
	assert(tag->object == TO_INT);

	tag->value.i = i;
}

int itm_tag_geti(struct itm_tag *tag)
{
	assert(tag->object == TO_INT);

	return tag->value.i;
}

void itm_tag_add_item(struct itm_tag *tag, void *expr)
{
	assert(tag->object == TO_EXPR_LIST);

	list_push_back(tag->value.data, expr);
}

struct list *itm_tag_get_list(struct itm_tag *tag)
{
	assert(tag->object == TO_EXPR_LIST);

	return tag->value.data;
}

void itm_tag_set_user_ptr(struct itm_tag *tag, void *ptr,
	void (*print)(FILE *, void *))
{
	assert(tag->object == TO_USER_PTR);

	tag->print = print;
	tag->value.data = ptr;
}

void *itm_tag_get_user_ptr(struct itm_tag *tag)
{
	assert(tag->object == TO_USER_PTR);

	return tag->value.data;
}
