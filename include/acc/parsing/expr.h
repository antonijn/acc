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
#include <acc/ast.h>

struct expr {
	struct itm_expr *itm;
	int islvalue;
};

enum exprflags {
	EF_NORMAL = 0x00,
	EF_INIT = 0x01,
	EF_FINISH_BRACKET = 0x02,
	EF_FINISH_SEMICOLON = 0x04,
	EF_FINISH_SQUARE_BRACKET = 0x08,
	EF_FINISH_COMMA = 0x10,
	EF_EXPECT_RVALUE = 0x20,
	EF_EXPECT_LVALUE = 0x40,
	EF_CLEAR_MASK = ~(EF_EXPECT_LVALUE | EF_EXPECT_RVALUE)
};

struct expr parseexpr(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty);
struct expr cast(struct expr e, struct ctype *ty,
	struct itm_block *block);

#endif
