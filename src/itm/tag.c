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

void new_itm_tag(struct itm_tag *tag, itm_tag_type_t *type, const char *name)
{
	assert(tag != NULL);
	assert(name != NULL);

	tag->type = type;
	tag->name = name;

	tag->ival = 0;
	tag->bval = false;
	tag->strval = NULL;
}

void delete_itm_tag(struct itm_tag *tag)
{
	if (tag->strval)
		free(tag->strval);
}

itm_tag_type_t *itm_tag_type(struct itm_tag *tag)
{
	return tag->type;
}

const char *itm_tag_name(struct itm_tag *tag)
{
	return tag->name;
}

void itm_tag_setb(struct itm_tag *tag, bool b)
{
	tag->bval = b;
}

bool itm_tag_getb(struct itm_tag *tag)
{
	return tag->bval;
}

void itm_tag_seti(struct itm_tag *tag, int i)
{
	tag->ival = i;
}

int itm_tag_geti(struct itm_tag *tag)
{
	return tag->ival;
}

void itm_tag_sets(struct itm_tag *tag, char *str)
{
	if (tag->strval)
		free(tag->strval);

	if (!str) {
		tag->strval = NULL;
		return;
	}

	tag->strval = malloc((strlen(str) + 1) * sizeof(char));
	sprintf(tag->strval, "%s", str);
}

char *itm_tag_gets(struct itm_tag *tag)
{
	return tag->strval;
}
