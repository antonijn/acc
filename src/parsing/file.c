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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <acc/parsing/file.h>
#include <acc/parsing/decl.h>
#include <acc/parsing/stat.h>
#include <acc/parsing/tools.h>
#include <acc/ast.h>
#include <acc/ext.h>
#include <acc/error.h>
#include <acc/token.h>

static void addparams(void * fsym)
{
	struct symbol * sym = fsym;
	struct cfunction * fun = (struct cfunction *)sym->type;
	void * it;

	it = list_iterator(fun->parameters);
	while (iterator_next(&it, (void **)&sym))
		registersym(sym);
}

static void processdecls(FILE * f, struct list * decls, struct list * syms)
{
	void * it;
	struct symbol * sym;
	struct token * tok;

	it = list_iterator(decls);
	while (iterator_next(&it, (void **)&sym))
		list_push_back(syms, sym);
	
	if (list_length(decls) == 1 &&
	   ((struct symbol *)list_head(decls))->type->type == FUNCTION &&
	   (tok = chktp(f, "{"))) {
		struct itm_block * block = new_itm_block(NULL, NULL);
		struct itm_block * bb = block;

		ungettok(tok, f);
		freetp(tok);

		enter_scope();
		addparams(list_head(decls));
		assert(parseblock(f, SF_NORMAL, &block));
		leave_scope();
#ifndef NDEBUG
		itm_block_to_string(stdout, bb);
#endif
	}
}

void parsefile(FILE * f, struct list * syms)
{
	struct list * declsyms;

	while (!chktt(f, T_EOF) &&
	      parsedecl(f, DF_GLOBAL, (declsyms = new_list(NULL, 0)))) {
		processdecls(f, declsyms, syms);
		delete_list(declsyms, NULL);
	}
}
