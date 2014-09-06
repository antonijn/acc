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

#ifndef ITM_TAG_H
#define ITM_TAG_H

#include <stdbool.h>

enum itm_tag_type {
	TT_ACC,
	TT_USED,
	TT_VALUE
};

struct itm_tag {
	enum itm_tag_type type;
	const char *name;

	int ival;
	bool bval;
	char *strval;
};

void new_itm_tag(struct itm_tag *tag, enum itm_tag_type type, const char *name);
void delete_itm_tag(struct itm_tag *tag);

enum itm_tag_type itm_tag_type(struct itm_tag *tag);
const char *itm_tag_name(struct itm_tag *tag);
void itm_tag_setb(struct itm_tag *tag, bool b);
bool itm_tag_getb(struct itm_tag *tag);
void itm_tag_seti(struct itm_tag *tag, int i);
int itm_tag_geti(struct itm_tag *tag);
void itm_tag_sets(struct itm_tag *tag, char *str);
char *itm_tag_gets(struct itm_tag *tag);

#endif
