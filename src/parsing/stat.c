/*
 * Statement parsing and generation of intermediate representation
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
#include <string.h>

#include <acc/parsing/stat.h>
#include <acc/parsing/expr.h>
#include <acc/parsing/tools.h>
#include <acc/list.h>
#include <acc/token.h>
#include <acc/ext.h>
#include <acc/error.h>

static int parseif(FILE * f, enum statflags flags, struct itm_block ** block);
static int parsedo(FILE * f, enum statflags flags, struct itm_block ** block);
static int parsefor(FILE * f, enum statflags flags, struct itm_block ** block);
static int parsewhile(FILE * f, enum statflags flags,
	struct itm_block ** block);
static int parseestat(FILE * f, enum statflags flags,
	struct itm_block ** block);

static int parseif(FILE * f, enum statflags flags, struct itm_block ** block)
{
	struct itm_expr * cond;
	struct itm_block * tblock, * eblock;
	struct itm_label * tlabel, * elabel;
	struct list * prev;
	struct token tok;
	
	if (!chkt(f, "if"))
		return 0;
	
	if (!chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected '('");
		ungettok(&tok, f);
		freetok(&tok);
	}
	
	cond = parseexpr(f, EF_FINISH_BRACKET | EF_EXPECT_RVALUE, block, NULL);
	
	/* remove ) from stream */
	tok = gettok(f);
	freetok(&tok);
	
	cond = cast(cond, &cbool, *block);
	
	prev = new_list(NULL, 0);
	list_push_back(prev, *block);
	tblock = new_itm_block(prev);
}

static int parsedo(FILE * f, enum statflags flags, struct itm_block ** block)
{
	
}

static int parsefor(FILE * f, enum statflags flags, struct itm_block ** block)
{
	
}

static int parsewhile(FILE * f, enum statflags flags,
	struct itm_block ** block)
{
	
}

static int parseestat(FILE * f, enum statflags flags,
	struct itm_block ** block)
{
	
}

int parsestat(FILE * f, enum statflags flags, struct itm_block ** block)
{
	return parseif(f, flags, block) ||
	       parsedo(f, flags, block) ||
	       parsefor(f, flags, block) ||
	       parsewhile(f, flags, block) ||
	       parseestat(f, flags, block);
}

int parseblock(FILE * f, enum statflags flags, struct itm_block ** block)
{
	
}
