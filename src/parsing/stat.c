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
#include <acc/parsing/decl.h>
#include <acc/parsing/token.h>
#include <acc/list.h>
#include <acc/ext.h>
#include <acc/error.h>

static bool parsestatx(FILE *f, enum statflags flags, struct itm_block **block,
	struct itm_block *tobreak, struct itm_block *tocont);
static bool parseblockx(FILE *f, enum statflags flags, struct itm_block **block,
	struct itm_block *tobreak, struct itm_block *tocont);
static bool parseif(FILE *f, enum statflags flags, struct itm_block **block,
	struct itm_block *tobreak, struct itm_block *tocont);
static bool parsedo(FILE *f, enum statflags flags, struct itm_block **block);
static bool parsefor(FILE *f, enum statflags flags, struct itm_block **block);
static bool parsewhile(FILE *f, enum statflags flags, struct itm_block **block);
static bool parseestat(FILE *f, enum statflags flags, struct itm_block **block);

static bool parseif(FILE *f, enum statflags flags, struct itm_block **block,
	struct itm_block *tobreak, struct itm_block *tocont)
{
	/*
	 * if (cond) ontrue; else onfalse;
	 * becomes:
	 *   ...
	 *     cond
	 *     split ontrue, onfalse
	 *   ontrue
	 *     jmp quit
	 *   onfalse
	 *     jmp quit
	 *   quit
	 *
	 * if (cond) ontrue;
	 * becomes:
	 *   ...
	 *     cond
	 *     split ontrue, onfalse
	 *   ontrue
	 *     jmp onfalse
	 *   onfalse
	 */

	struct token tok;
	
	if (!chkt(f, "if"))
		return false;
	
	if (!chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected '('");
		ungettok(&tok, f);
		freetok(&tok);
	}

	struct itm_block *ontrue = new_itm_block();
	struct itm_block *ontruelbl = ontrue;
	struct itm_block *onfalse = new_itm_block();
	struct itm_block *onfalselbl = onfalse;
	
	struct expr cond = parseexpr(f, EF_FINISH_BRACKET | EF_EXPECT_RVALUE, block, NULL);
	cond = cast(cond, &cbool, *block);

	// remove ) from stream
	tok = gettok(f);
	freetok(&tok);

	itm_split(*block, cond.itm, ontruelbl, onfalselbl);

	parsestatx(f, SF_NORMAL, &ontrue, tobreak, tocont);

	if (chkt(f, "else")) {
		struct itm_block *onquit = new_itm_block();

		itm_jmp(ontrue, onquit);

		parsestatx(f, SF_NORMAL, &onfalse, tobreak, tocont);
		itm_jmp(onfalse, onquit);

		// construct if-else graph
		itm_progress(*block, ontruelbl);
		itm_progress(*block, onfalselbl);
		itm_progress(ontrue, onquit);
		itm_progress(onfalse, onquit);
		itm_lex_progress(*block, ontruelbl);
		itm_lex_progress(ontrue, onfalselbl);
		itm_lex_progress(onfalse, onquit);

		*block = onquit;
	} else {
		itm_jmp(ontrue, onfalselbl);

		// construct if graph
		itm_progress(*block, ontruelbl);
		itm_progress(*block, onfalselbl);
		itm_progress(ontrue, onfalselbl);
		itm_lex_progress(*block, ontruelbl);
		itm_lex_progress(ontrue, onfalselbl);

		*block = onfalse;
	}
	return true;
}

static bool parsefor(FILE *f, enum statflags flags, struct itm_block **block)
{
	/*
	 * for (init; cond; final) body;
	 * becomes:
	 *   ...
	 *     init
	 *     jmp condb
	 *   condb
	 *     cond
	 *     split body, quit
	 *   body
	 *     jmp finalb
	 *   finalb
	 *     final
	 *     jmp condb
	 * 
	 * if cond is omitted:
	 *   condb is aliased to body
	 * if final is omitted:
	 *   final is aliased to condb
	 * 
	 * continue jumps to final
	 * break jumps to quit
	 * 
	 * As opposed to the other control flow statements, the graph is
	 * generated on the fly instead of at the end in this one.
	 */
	
	struct token tok;
	
	if (!chkt(f, "for"))
		return false;
	
	if (!chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected '('");
		ungettok(&tok, f);
		freetok(&tok);
	}

	struct itm_block *condb;
	struct itm_block *condblbl;
	struct itm_block *body = new_itm_block();
	struct itm_block *bodylbl = body;
	struct itm_block *finalb;
	struct itm_block *finalblbl;
	struct itm_block *quit = new_itm_block();
	struct itm_block *quitlbl = quit;
	
	if (!chkt(f, ";")) {
		parseexpr(f, EF_FINISH_SEMICOLON, block, NULL);

		// remove ; from stream
		tok = gettok(f);
		freetok(&tok);
	}
	
	if (!chkt(f, ";")) {
		condb = new_itm_block();
		condblbl = condb;

		itm_jmp(*block, condblbl);
		itm_progress(*block, condblbl);

		struct expr cond = parseexpr(f,
			EF_FINISH_SEMICOLON | EF_EXPECT_RVALUE, &condb, NULL);
		cond = cast(cond, &cbool, condb);

		// remove ; from stream
		tok = gettok(f);
		freetok(&tok);
		
		itm_split(condb, cond.itm, bodylbl, quitlbl);
		itm_progress(condb, bodylbl);
		itm_progress(condb, quitlbl);
	} else {
		condb = body;
		condblbl = bodylbl;
		
		itm_jmp(*block, bodylbl);
		itm_progress(*block, bodylbl);
	}
	
	if (!chkt(f, ")")) {
		finalb = new_itm_block();
		finalblbl = finalb;
		
		parseexpr(f, EF_FINISH_BRACKET, &finalb, NULL);

		// remove ) from stream
		tok = gettok(f);
		freetok(&tok);
		
		itm_jmp(finalb, condblbl);
		itm_progress(finalb, condblbl);
	} else {
		finalb = condb;
		finalblbl = condblbl;
	}
	
	parsestatx(f, SF_NORMAL, &body, quitlbl, finalblbl);
	itm_jmp(body, finalblbl);
	itm_progress(body, finalblbl);
	
	itm_lex_progress(*block, condblbl);
	if (condb != body)
		itm_lex_progress(condb, bodylbl);
	itm_lex_progress(body, finalblbl);
	if (finalb != condb)
		itm_lex_progress(finalb, quitlbl);
	
	*block = quit;
	return true;
}

static bool parsedo(FILE *f, enum statflags flags, struct itm_block **block)
{
	/*
	 * do body; while(cond);
	 * becomes:
	 *   ...
	 *     jmp body
	 *   body
	 *     jmp condb
	 *   condb
	 *     cond
	 *     split body, quit
	 *   quit
	 *
	 * continue jumps to condb
	 * break jumps to quit
	 */

	struct token tok;

	if (!chkt(f, "do"))
		return false;

	struct itm_block *body = new_itm_block();
	struct itm_block *bodylbl = body;
	struct itm_block *condb = new_itm_block();
	struct itm_block *condblbl = condb;
	struct itm_block *quit = new_itm_block();
	struct itm_block *quitlbl = quit;

	itm_jmp(*block, bodylbl);

	parsestatx(f, SF_NORMAL, &body, quitlbl, condblbl);
	itm_jmp(body, condblbl);

	if (!chkt(f, "while") || !chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected 'while' and '(' pair");
		ungettok(&tok, f);
		freetok(&tok);
	}

	struct expr cond = parseexpr(f, EF_FINISH_BRACKET | EF_EXPECT_RVALUE,
		&condb, NULL);
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

	itm_split(condb, cond.itm, bodylbl, quitlbl);

	// construct do-while graph
	itm_progress(*block, bodylbl);
	itm_progress(body, condblbl);
	itm_progress(condb, bodylbl);
	itm_progress(condb, quitlbl);
	itm_lex_progress(*block, bodylbl);
	itm_lex_progress(body, condblbl);
	itm_lex_progress(condb, quitlbl);

	*block = quit;
	return true;
}

static bool parsewhile(FILE *f, enum statflags flags, struct itm_block **block)
{
	/*
	 * while (cond) body;
	 * becomes:
	 *   ...
	 *     jmp condb
	 *   condb
	 *     cond
	 *     split body, quit
	 *   body
	 *     jmp condb
	 *   quit
	 */

	struct token tok;

	if (!chkt(f, "while"))
		return false;
	
	if (!chkt(f, "(")) {
		tok = gettok(f);
		report(E_PARSER, &tok, "expected '('");
		ungettok(&tok, f);
		freetok(&tok);
	}

	struct itm_block *body = new_itm_block();
	struct itm_block *bodylbl = body;
	struct itm_block *condb = new_itm_block();
	struct itm_block *condblbl = condb;
	struct itm_block *quit = new_itm_block();
	struct itm_block *quitlbl = quit;

	itm_jmp(*block, condblbl);
	
	struct expr cond = parseexpr(f, EF_FINISH_BRACKET | EF_EXPECT_RVALUE,
		&condb, NULL);
	cond = cast(cond, &cbool, condb);

	// remove ) from stream
	tok = gettok(f);
	freetok(&tok);

	itm_split(condb, cond.itm, bodylbl, quitlbl);

	parsestatx(f, SF_NORMAL, &body, quitlbl, condblbl);
	itm_jmp(body, condb);

	// construct while graph
	itm_progress(*block, condblbl);
	itm_progress(condb, bodylbl);
	itm_progress(condb, quitlbl);
	itm_progress(body, condblbl);
	itm_lex_progress(*block, condblbl);
	itm_lex_progress(condb, bodylbl);
	itm_lex_progress(body, quitlbl);

	*block = quit;
	return true;
}

static bool parseestat(FILE *f, enum statflags flags, struct itm_block **block)
{
	struct expr e = parseexpr(f, EF_FINISH_SEMICOLON, block, NULL);
	if (!e.itm)
		return false;
	// remove ; from stream
	struct token tok = gettok(f);
	freetok(&tok);
	return true;
}

static bool parsestatx(FILE *f, enum statflags flags, struct itm_block **block,
	struct itm_block *tobreak, struct itm_block *tocont)
{
	if (chkt(f, ";"))
		return true;

	struct token btok;
	if (chktp(f, "break", &btok)) {
		if (!tobreak) {
			report(E_PARSER, &btok, "no loop to break from");
		} else {
			itm_jmp(*block, tobreak);
			struct itm_block *newblock = new_itm_block();
			itm_lex_progress(*block, newblock);
			itm_progress(*block, tobreak);
			*block = newblock;
		}

		if (!chkt(f, ";"))
			report(E_PARSER, &btok, "expected ';' after statement");

		return true;
	}

	struct token ctok;
	if (chktp(f, "continue", &btok)) {
		if (!tocont) {
			report(E_PARSER, &btok, "no loop to continue from");
		} else {
			itm_jmp(*block, tocont);
			struct itm_block *newblock = new_itm_block();
			itm_lex_progress(*block, newblock);
			itm_progress(*block, tocont);
			*block = newblock;
		}

		if (!chkt(f, ";"))
			report(E_PARSER, &btok, "expected ';' after statement");

		return true;
	}

	return parseif(f, flags, block, tobreak, tocont) ||
	       parsedo(f, flags, block) ||
	       parsefor(f, flags, block) ||
	       parsewhile(f, flags, block) ||
	       parseblockx(f, flags, block, tobreak, tocont) ||
	       parseestat(f, flags, block);
}


bool parsestat(FILE *f, enum statflags flags, struct itm_block **block)
{
	return parsestatx(f, flags, block, NULL, NULL);
}

static bool parseblockx(FILE *f, enum statflags flags, struct itm_block **block,
	struct itm_block *tobreak, struct itm_block *tocont)
{
	if (!chkt(f, "{"))
		return false;

	while (parsedecl(f, DF_LOCAL, NULL, block))
		if (chkt(f, "}"))
			return true;

	while (!chkt(f, "}"))
		parsestatx(f, flags, block, tobreak, tocont);

	return true;
}

bool parseblock(FILE *f, enum statflags flags, struct itm_block **block)
{
	return parseblockx(f, flags, block, NULL, NULL);
}
