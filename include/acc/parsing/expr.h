/*
 * Expression parsing and generation of intermediate representation
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

#ifndef PARSING_EXPR_H
#define PARSING_EXPR_H

#include <stdio.h>

#include <acc/itm.h>

enum exprflags {
	EF_NORMAL = 0x00,
	EF_INIT = 0x01,
	EF_FINISH_BRACKET = 0x02,
	EF_FINISH_SEMICOLON = 0x04,
	EF_FINISH_SQUARE_BRACKET = 0x08,
	EF_FINISH_COMMA = 0x10
};

struct itm_expr * parseexpr(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty);
struct itm_expr * cast(struct itm_expr * e, struct ctype * ty,
	struct itm_block * block);

#endif