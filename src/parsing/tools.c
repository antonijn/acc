/*
 * Parsing tools
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

#include <acc/parsing/tools.h>

int chkt(FILE *f, const char *t)
{
	struct token *tok = chktp(f, t);
	if (!tok)
		return 0;
	freetp(tok);
	return 1;
}

int chktt(FILE *f, enum tokenty tt)
{
	struct token *t = chkttp(f, tt);
	if (!t)
		return 0;
	freetp(t);
	return 1;
}

struct token *chktp(FILE *f, const char *t)
{
	struct token *nxt = malloc(sizeof(struct token));
	*nxt = gettok(f);
	if (!strcmp(t, nxt->lexeme))
		return nxt;
	ungettok(nxt, f);
	freetp(nxt);
	return NULL;
}

struct token *chkttp(FILE *f, enum tokenty tt)
{
	struct token *nxt = malloc(sizeof(struct token));
	*nxt = gettok(f);
	if (nxt->type == tt)
		return nxt;
	ungettok(nxt, f);
	freetp(nxt);
	return NULL;
}

void freetp(struct token *t)
{
	freetok(t);
	free(t);
}
