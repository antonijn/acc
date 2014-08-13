/*
 * File parsing and generation of intermediate representation
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
 *
 *
 * Let's establish some rules, shall we?
 *
 * When a parser function is entered, the token at which it shall begin to
 * parse will have yet to be retrieved from the file stream.
 * When a parser function returns, it shall leave the next logical token
 * unread.
 */

#include <stdlib.h>
#include <string.h>

#include <acc/parsing/file.h>
#include <acc/parsing/decl.h>
#include <acc/ast.h>
#include <acc/ext.h>
#include <acc/error.h>
#include <acc/token.h>

struct itm_module parsefile(FILE * f)
{
	struct itm_module res;
	struct list * syms = new_list(NULL, 0);
	void * it;
	struct symbol * sym;
	parsedecl(f, DF_GLOBAL, syms);
	parsedecl(f, DF_GLOBAL, syms);

	it = list_iterator(syms);
	while (iterator_next(&it, (void **)&sym)) {
		sym->type->to_string(stdout, sym->type);
		printf(" %s\n", sym->id);
	}
	delete_list(syms, NULL);

	return res;
}
