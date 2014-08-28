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
#include <stdbool.h>
#include <string.h>

#include <acc/parsing/stat.h>
#include <acc/parsing/expr.h>
#include <acc/parsing/tools.h>
#include <acc/parsing/decl.h>
#include <acc/list.h>
#include <acc/token.h>
#include <acc/ext.h>
#include <acc/error.h>

static bool parseif(FILE *f, enum statflags flags, struct itm_block **block);
static bool parsedo(FILE *f, enum statflags flags, struct itm_block **block);
static bool parsefor(FILE *f, enum statflags flags, struct itm_block **block);
static bool parsewhile(FILE *f, enum statflags flags,
	struct itm_block **block);
static bool parseestat(FILE *f, enum statflags flags,
	struct itm_block **block);

static bool parseif(FILE *f, enum statflags flags, struct itm_block **block)
{
	struct token tok;
	
	if (!chkt(f, "if"))
		return false;
	
	if (!chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected '('");
		ungettok(&tok, f);
		freetok(&tok);
	}
	
	struct expr cond = parseexpr(f, EF_FINISH_BRACKET | EF_EXPECT_RVALUE, block, NULL);
	cond = cast(cond, &cbool, *block);

	// remove ) from stream
	tok = gettok(f);
	freetok(&tok);

	struct itm_label *tlabel = new_itm_label();
	struct itm_label *elabel = new_itm_label();
	itm_split(*block, cond.itm, tlabel, elabel);
	
	struct list *prev = new_list(NULL, 0);
	list_push_back(prev, *block);
	struct itm_block *tblock = new_itm_block(*block, prev);
	set_itm_label_block(tlabel, tblock);

	parsestat(f, SF_NORMAL, &tblock);

	if (chkt(f, "else")) {
		struct itm_label *common = new_itm_label();

		itm_jmp(tblock, common);

		prev = new_list(NULL, 0);
		list_push_back(prev, *block);
		*block = new_itm_block(tblock, prev);
		set_itm_label_block(elabel, *block);

		parsestat(f, SF_NORMAL, block);

		itm_jmp(*block, common);

		prev = new_list(NULL, 0);
		list_push_back(prev, *block);
		*block = new_itm_block(*block, prev);
		set_itm_label_block(common, *block);
	} else {
		itm_jmp(tblock, elabel);

		prev = new_list(NULL, 0);
		list_push_back(prev, tblock);
		list_push_back(prev, *block);
		*block = new_itm_block(tblock, prev);
		set_itm_label_block(elabel, *block);
	}
	
	return true;
}

static bool parsedo(FILE *f, enum statflags flags, struct itm_block **block)
{
	// TODO: implement
	return false;
}

static bool parsefor(FILE *f, enum statflags flags, struct itm_block **block)
{
	struct list *prev;
	struct token tok;

	if (!chkt(f, "do"))
		return false;

	struct itm_label *condl = new_itm_label();
	struct itm_label *bodyl = new_itm_label();
	struct itm_label *quitl = new_itm_label();

	itm_jmp(*block, bodyl);

	prev = new_list(NULL, 0);
	list_push_back(prev, *block);
	struct itm_block *bodyb = new_itm_block(*block, prev);
	set_itm_label_block(bodyl, bodyb);

	parsestat(f, SF_NORMAL, &bodyb);

	if (!chkt(f, "while") || !chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected 'while' and '(' pair");
		ungettok(&tok, f);
		freetok(&tok);
	}

	prev = new_list(NULL, 0);
	list_push_back(prev, bodyb);
	struct itm_block *condb = new_itm_block(bodyb, prev);
	set_itm_label_block(condl, condb);

	list_push_back(bodyb->previous, condb);
	list_push_back(condb->next, bodyb);

	struct expr cond = parseexpr(f, EF_FINISH_BRACKET | EF_EXPECT_RVALUE, &condb, NULL);
	cond = cast(cond, &cbool, condb);

	// remove ) from stream
	tok = gettok(f);
	freetok(&tok);

	if (!chkt(f, ";")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected ';'");
		ungettok(&tok, f);
		freetok(&tok);
	}

	itm_split(condb, cond.itm, bodyl, quitl);

	prev = new_list(NULL, 0);
	list_push_back(prev, condb);
	struct itm_block *quitb = new_itm_block(condb, prev);
	set_itm_label_block(quitl, quitb);

	*block = quitb;

	return true;
}

static bool parsewhile(FILE *f, enum statflags flags,
	struct itm_block **block)
{
	struct list *prev;
	struct token tok;

	if (!chkt(f, "while"))
		return false;
	
	if (!chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected '('");
		ungettok(&tok, f);
		freetok(&tok);
	}

	struct itm_label *condl = new_itm_label();
	struct itm_label *bodyl = new_itm_label();
	struct itm_label *quitl = new_itm_label();

	itm_jmp(*block, condl);

	prev = new_list(NULL, 0);
	list_push_back(prev, *block);
	struct itm_block *condb = new_itm_block(*block, prev);
	set_itm_label_block(condl, condb);
	
	struct expr cond = parseexpr(f, EF_FINISH_BRACKET | EF_EXPECT_RVALUE, &condb, NULL);
	cond = cast(cond, &cbool, condb);

	// remove ) from stream
	tok = gettok(f);
	freetok(&tok);

	itm_split(condb, cond.itm, bodyl, quitl);

	prev = new_list(NULL, 0);
	list_push_back(prev, condb);
	struct itm_block *bodyb = new_itm_block(condb, prev);
	set_itm_label_block(bodyl, bodyb);

	parsestat(f, SF_NORMAL, &bodyb);

	itm_jmp(bodyb, condl);

	prev = new_list(NULL, 0);
	list_push_back(prev, condb);
	list_push_back(prev, bodyb);
	struct itm_block *quitb = new_itm_block(bodyb, prev);
	set_itm_label_block(quitl, quitb);

	*block = quitb;

	return true;
}

static bool parseestat(FILE *f, enum statflags flags,
	struct itm_block **block)
{
	struct expr e = parseexpr(f, EF_FINISH_SEMICOLON, block, NULL);
	if (!e.itm)
		return false;
	// remove ; from stream
	struct token tok = gettok(f);
	freetok(&tok);
	return true;
}

bool parsestat(FILE *f, enum statflags flags, struct itm_block **block)
{
	return parseif(f, flags, block) ||
	       parsedo(f, flags, block) ||
	       parsefor(f, flags, block) ||
	       parsewhile(f, flags, block) ||
	       parseblock(f, flags, block) ||
	       parseestat(f, flags, block);
}

bool parseblock(FILE *f, enum statflags flags, struct itm_block **block)
{
	if (!chkt(f, "{"))
		return false;

	while (parsedecl(f, DF_LOCAL, NULL, block))
		if (chkt(f, "}"))
			return true;
	
	while (!chkt(f, "}"))
		parsestat(f, flags, block);

	return true;
}
