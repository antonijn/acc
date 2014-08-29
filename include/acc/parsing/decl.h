/*
 * Declaration parsing and generation of intermediate representation
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

#ifndef PARSING_DECL_H
#define PARSING_DECL_H

#include <stdio.h>
#include <stdbool.h>

#include <acc/ast.h>
#include <acc/list.h>

/* flags for what is and what isn't allowed when parsing a declaration */
enum declflags {
	DF_INIT = 0x01,
	DF_BITFIELD = 0x02,
	DF_FUNCTION = 0x04,
	DF_MULTIPLE = 0x08,
	DF_NO_ID = 0x10,
	DF_FINISH_COMMA = 0x40,
	DF_REGISTER = 0x80,
	DF_EXTERN = 0x100,
	DF_REGISTER_SYMBOL = 0x200,
	DF_FINISH_SEMICOLON = 0x400,
	DF_FINISH_PARENT = 0x800, /* doesn't remove ) from file stream */
	DF_FINISH_BRACE = 0x1000, /* doesn't remove { from file stream */
	DF_ARRAY_POINTER = 0x2000,
	DF_ALLOCA = 0x4000,

	DF_BASIC = DF_INIT | DF_MULTIPLE | DF_REGISTER |
		DF_REGISTER_SYMBOL | DF_FINISH_SEMICOLON,

	DF_GLOBAL = DF_BASIC | DF_FUNCTION | DF_FINISH_BRACE,
	DF_LOCAL = DF_BASIC | DF_ALLOCA,
	DF_FIELD = DF_BITFIELD | DF_MULTIPLE | DF_FINISH_SEMICOLON,
	DF_CAST = DF_NO_ID | DF_FINISH_PARENT,
	DF_PARAM = DF_FINISH_COMMA | DF_FINISH_PARENT | DF_ARRAY_POINTER
};

bool parsedecl(FILE *f, enum declflags flags, struct list *syms, struct itm_block **b);

#endif
 
