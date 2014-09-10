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
#include <acc/list.h>

struct itm_tag {
	itm_tag_type_t type;
	const char *name;
	enum itm_tag_object object;
	void (*free)(void *data);

	union {
		int i;
		void *data;
	} value;
};

static void free_expr_list(void *data)
{
	delete_list(data, NULL);
}

struct itm_tag *new_itm_tag(itm_tag_type_t type, const char *name,
	enum itm_tag_object obj)
{
	struct itm_tag *tag = malloc(sizeof(struct itm_tag));

	assert(name != NULL);

	tag->type = type;
	tag->name = name;
	tag->object = obj;
	tag->free = (obj == TO_EXPR_LIST) ? &free_expr_list : NULL;
	tag->value.data = (obj == TO_EXPR_LIST) ? new_list(NULL, 0) : NULL;

	return tag;
}

void delete_itm_tag(struct itm_tag *tag)
{
	if (tag->free)
		tag->free(tag->value.data);
}

itm_tag_type_t itm_tag_type(struct itm_tag *tag)
{
	return tag->type;
}

const char *itm_tag_name(struct itm_tag *tag)
{
	return tag->name;
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
