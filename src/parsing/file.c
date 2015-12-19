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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <acc/parsing/file.h>
#include <acc/parsing/decl.h>
#include <acc/parsing/stat.h>
#include <acc/parsing/ast.h>
#include <acc/parsing/token.h>
#include <acc/ext.h>
#include <acc/error.h>

static void addparams(struct cfunction *fun)
{
	struct symbol *sym;
	it_t it = list_iterator(fun->parameters);
	while (iterator_next(&it, (void **)&sym))
		registersym(sym);
}

static void processdecls(FILE *f, struct list *decls, struct list *syms)
{
	struct symbol *sym;
	it_t it = list_iterator(decls);
	while (iterator_next(&it, (void **)&sym))
		if (sym->value)
			list_push_back(syms, sym->value);

	struct token tok;
	if (list_length(decls) != 1 ||
	   ((struct symbol *)list_head(decls))->type->type != FUNCTION ||
	   !chktp(f, "{", &tok))
		return;


	ungettok(&tok, f);
	freetok(&tok);

	struct symbol *sf = list_head(decls);
	struct cfunction *cf = (struct cfunction *)sf->type;
	// TODO: retrieve correct linkage, not just IL_GLOBAL
	struct symbol *last = list_last(decls);
	struct itm_container *cont = (struct itm_container *)last->value;
	struct itm_block *block = new_itm_block(cont);
	cont->block = block;

	enter_scope();
	addparams(cf);
	bool success = parseblock(f, SF_NORMAL, &block, cf->ret);
	leave_scope();

	if (!block->last || !block->last->isterminal)
		itm_leave(block);
}

void parsefile(FILE *f, struct list *syms)
{
	struct list * declsyms;
	while (!chktt(f, T_EOF) &&
	      parsedecl(f, DF_GLOBAL, (declsyms = new_list(NULL, 0)), NULL)) {
		processdecls(f, declsyms, syms);
		delete_list(declsyms, NULL);
	}
}
