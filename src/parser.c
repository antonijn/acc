/*
 * Parsing and generation of intermediate representation
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

#include <acc/parser.h>
#include <acc/ast.h>
#include <acc/ext.h>
#include <acc/error.h>
#include <acc/token.h>

static int chkt(FILE * f, const char * t);
static int chktt(FILE * f, enum tokenty tt);
static struct token * chktp(FILE * f, const char * t);
static struct token * chkttp(FILE * f, enum tokenty tt);
static void freetp(struct token * t);

/* declaration parsing */

/* flags for what is and what isn't allowed when parsing a declaration */
enum declflags {
	DF_INIT = 0x01,
	DF_BITFIELD = 0x02,
	DF_FUNCTION = 0x04,
	DF_MULTIPLE = 0x08,
	DF_NO_ID = 0x10,
	DF_OPTIONAL_ID = 0x20,
	DF_FINISH_COMMA = 0x40,
	DF_REGISTER = 0x80,
	DF_EXTERN = 0x100,
	DF_REGISTER_SYMBOL = 0x200,
	DF_FINISH_SEMICOLON = 0x400,

	DF_GLOBAL = DF_INIT | DF_FUNCTION | DF_MULTIPLE | DF_EXTERN | DF_REGISTER_SYMBOL | DF_FINISH_SEMICOLON,
	DF_LOCAL = DF_INIT | DF_MULTIPLE | DF_REGISTER | DF_REGISTER_SYMBOL | DF_FINISH_SEMICOLON,
	DF_FIELD = DF_BITFIELD | DF_MULTIPLE | DF_FINISH_SEMICOLON,
	DF_TYPEDEF = DF_FUNCTION | DF_REGISTER_SYMBOL | DF_FINISH_SEMICOLON,
	DF_CAST = DF_NO_ID,
	DF_PARAM = DF_OPTIONAL_ID | DF_FINISH_COMMA | DF_REGISTER_SYMBOL
};

/* primitive modifiers */
enum primmod {
	PM_NONE = 0x00,
	PM_UNSIGNED = 0x01,
	PM_SIGNED = 0x02,
	PM_LONG = 0x04,
	PM_LONG_LONG = 0x08,
	PM_SHORT = 0x10
};

static void parsedecl(FILE * f, enum declflags flags, struct list * syms);
static int parsemod(FILE * f, enum declflags flags, enum qualifier * quals,
	enum primmod * pm, enum storageclass * sc);
static struct ctype * parsebasety(FILE * f, enum declflags flags,
	enum storageclass * sc);
static struct symbol * parsedeclarator(FILE * f, enum declflags flags,
	struct ctype * ty, enum storageclass sc);
static struct symbol * parseddeclarator(FILE * f, enum declflags flags,
	struct ctype * ty, enum storageclass sc);

static struct ctype * parseprimitive(FILE * f, enum primmod mods);
static struct ctype * parsestructure(FILE * f);
static struct ctype * parsetypedef(FILE * f);

/* implementations */
/* token checking */
static int chkt(FILE * f, const char * t)
{
	struct token * tok = chktp(f, t);
	if (!tok)
		return 0;
	freetp(tok);
	return 1;
}

static int chktt(FILE * f, enum tokenty tt)
{
	struct token * t = chkttp(f, tt);
	if (!t)
		return 0;
	freetp(t);
	return 1;
}

static struct token * chktp(FILE * f, const char * t)
{
	struct token * nxt = malloc(sizeof(struct token));
	*nxt = gettok(f);
	if (!strcmp(t, nxt->lexeme))
		return nxt;
	ungettok(nxt, f);
	freetp(nxt);
	return NULL;
}

static struct token * chkttp(FILE * f, enum tokenty tt)
{
	struct token * nxt = malloc(sizeof(struct token));
	*nxt = gettok(f);
	if (nxt->type == tt)
		return nxt;
	ungettok(nxt, f);
	freetp(nxt);
	return NULL;
}

static void freetp(struct token * t)
{
	freetok(t);
	free(t);
}

/* declarations */
static void parsedecl(FILE * f, enum declflags flags, struct list * syms)
{
	enum storageclass sc;
	struct ctype * basety = parsebasety(f, flags, &sc);

	while (1) {
		struct token tok;
		struct symbol * sym;

		sym = parsedeclarator(f, flags, basety, sc);
		list_push_back(syms, sym);
		if ((flags & DF_INIT) && chkt(f, "=")) {
			/* TODO: parse initial expression */
		}
		if ((flags & DF_BITFIELD) && chkt(f, ":")) {
			/* TODO: parse bitfield */
		}
		if (((flags & DF_FINISH_SEMICOLON) && chkt(f, ";")) ||
		    ((flags & DF_FINISH_COMMA) && chkt(f, ",")))
			break;
		if ((flags & DF_MULTIPLE) && chkt(f, ","))
			continue;

		tok = gettok(f);
		report(E_PARSER, &tok, "unexpected token in declaration");
		break;
	}
}

static int parsestorage(FILE * f, enum storageclass * sc,
	const char * mod, enum storageclass set)
{
	struct token * nxt;
	if (nxt = chktp(f, mod)) {
		if (*sc == set)
			report(E_PARSER, nxt, "duplicate \"%s\" modifier", mod);
		else if (*sc != SC_DEFAULT)
			report(E_PARSER, nxt, "multiple storage class specifiers");
		*sc = set;
		freetp(nxt);
		return 1;
	}
	return 0;
}

static int parsemod(FILE * f, enum declflags flags, enum qualifier * quals,
	enum primmod * pm, enum storageclass * sc)
{
	struct token * nxt;
	if (nxt = chktp(f, "const")) {
		if (*quals & Q_CONST)
			report(E_PARSER, nxt, "duplicate \"const\" modifier");
		*quals |= Q_CONST;
		freetp(nxt);
		return 1;
	} else if (nxt = chktp(f, "volatile")) {
		if (*quals & Q_VOLATILE)
			report(E_PARSER, nxt, "duplicate \"volatile\" modifier");
		*quals |= Q_VOLATILE;
		freetp(nxt);
		return 1;
	} else if (parsestorage(f, sc, "auto", SC_AUTO))
		return 1;
	else if (parsestorage(f, sc, "static", SC_STATIC))
		return 1;
	else if ((flags & DF_EXTERN) && parsestorage(f, sc, "extern", SC_EXTERN))
		return 1;
	else if ((flags & DF_REGISTER) && parsestorage(f, sc, "register", SC_REGISTER))
		return 1;
	else if (nxt = chktp(f, "unsigned")) {
		if (*pm & PM_UNSIGNED)
			report(E_PARSER, nxt, "duplicate \"unsigned\" modifier");
		if (*pm & PM_SIGNED)
			report(E_PARSER, nxt, "\"signed\" and \"unsigned\" are mutually exclusive");
		else
			*pm |= PM_UNSIGNED;
		freetp(nxt);
		return 1;
	} else if (nxt = chktp(f, "signed")) {
		if (*pm & PM_SIGNED)
			report(E_PARSER, nxt, "duplicate \"signed\" modifier");
		if (*pm & PM_UNSIGNED)
			report(E_PARSER, nxt, "\"signed\" and \"unsigned\" are mutually exclusive");
		else
			*pm |= PM_SIGNED;
		freetp(nxt);
		return 1;
	} else if (nxt = chktp(f, "short")) {
		if (*pm & PM_SHORT)
			report(E_PARSER, nxt, "duplicate \"short\" modifier");
		if (*pm & PM_LONG)
			report(E_PARSER, nxt, "\"short\" and \"long\" are mutually exclusive");
		else
			*pm |= PM_SHORT;
		freetp(nxt);
		return 1;
	} else if (nxt = chktp(f, "long")) {
		if (*pm & PM_LONG)
			report(E_PARSER, nxt, "duplicate \"long\" modifier");
		if (*pm & PM_SHORT)
			report(E_PARSER, nxt, "\"short\" and \"long\" are mutually exclusive");
		else
			*pm |= PM_LONG;
		freetp(nxt);
		return 1;
	}

	return 0;
}

static struct ctype * parsebasety(FILE * f, enum declflags flags,
	enum storageclass * sc)
{
	struct ctype * res;
	enum primmod pm = PM_NONE;
	enum qualifier quals = Q_NONE;

	while (parsemod(f, flags, &quals, &pm, sc))
		;

	if (*sc == SC_DEFAULT)
		*sc = SC_AUTO;

	if (res = parsestructure(f))
		goto ret;
	if (res = parsetypedef(f))
		goto ret;
	if (res = parseprimitive(f, pm))
		goto ret;

ret:
	if (quals != Q_NONE)
		res = new_qualified(res, quals);
	return res;
}

static struct symbol * parsedeclarator(FILE * f, enum declflags flags,
	struct ctype * ty, enum storageclass sc)
{
	while (chkt(f, "*")) {
		enum qualifier quals = Q_NONE;

		ty = new_pointer(ty);
		while (parsemod(f, flags, &quals, NULL, NULL))
			;
		if (quals != Q_NONE)
			ty = new_qualified(ty, quals);
	}

	return parseddeclarator(f, flags, ty, sc);
}

static struct symbol * parseddeclarator(FILE * f, enum declflags flags,
	struct ctype * ty, enum storageclass sc)
{
	struct symbol * res;
	struct token * tok;
	struct token t;
	if (tok = chkttp(f, T_IDENTIFIER)) {
		res = new_symbol(ty, tok->lexeme, sc, flags & DF_REGISTER_SYMBOL);
		freetp(tok);
	} else if (chkt(f, "(")) {
		res = parsedeclarator(f, flags, ty, sc);
		t = gettok(f);
		if (strcmp(t.lexeme, ")")) {
			report(E_PARSER, &t, "expected ')' to finish declarator");
			ungettok(&t, f);
		}
		freetok(&t);
	}
	/* TODO: parameter list */
	/* TODO: array */
	return res;
}

static struct ctype * parseprimitive(FILE * f, enum primmod mods)
{
	struct token * tok;
	if (mods == PM_NONE) {
		if (chkt(f, "float"))
			return &cfloat;
		if (chkt(f, "double"))
			return &cdouble;
		if (chkt(f, "int"))
			return &cint;
		if (chkt(f, "char"))
			return isext(EX_UNSIGNED_CHAR) ? &cuchar : &cchar;
		return NULL;
	}

	if (tok = chktp(f, "char")) {
		if (mods == PM_SIGNED)
			return &cchar;
		if (mods == PM_UNSIGNED)
			return &cuchar;
		report(E_PARSER, tok, "invalid modifiers for type \"char\"");
		freetp(tok);
		return &cchar;
	}

	chkt(f, "int");

	switch (mods) {
	case (PM_SIGNED | PM_SHORT):
	case PM_SHORT:
		return &cshort;
	case (PM_UNSIGNED | PM_SHORT):
		return &cushort;
	case (PM_SIGNED | PM_LONG):
	case PM_LONG:
		return &clong;
	case (PM_UNSIGNED | PM_LONG):
		return &culong;
	case PM_SIGNED:
		return &cint;
	case PM_UNSIGNED:
		return &cuint;
	}

	/* unreachable */
	return NULL;
}

static struct ctype * parsestructure(FILE * f)
{
	struct token * idtok = NULL, * tok = NULL;
	struct cstruct * str;
	if (!chkt(f, "struct"))
		return NULL;

	idtok = chkttp(f, T_IDENTIFIER);

	if (!(tok = chktp(f, "{"))) {
		struct cstruct * str;
		if (!idtok) {
			report(E_PARSER, tok, "expected '{' or ';'");
		} else if (str = get_struct(idtok->lexeme)) {
			freetp(idtok);
			freetp(tok);
			return (struct ctype *)str;
		} else {
			str = (struct cstruct *)new_struct(idtok->lexeme);
			freetp(idtok);
			freetp(tok);
			return (struct ctype *)str;
		}

		freetp(tok);
		return NULL;
	}

	str = (struct cstruct *)new_struct(idtok->lexeme);
	freetp(idtok);
	while (!chkt(f, "}")) {
		struct list * syms = new_list(NULL, 0);
		struct symbol * sym;
		void * it;
		parsedecl(f, DF_FIELD, syms);

		it = list_iterator(syms);
		while (iterator_next(&it, (void **)&sym))
			struct_add_field((struct ctype *)str, sym->type, sym->id);
		delete_list(syms, NULL);
	}

	return (struct ctype *)str;
}

static struct ctype * parsetypedef(FILE * f)
{
	/* TODO: placeholder */
	return NULL;
}

struct itm_module parsefile(FILE * f)
{
	struct itm_module res;
	struct list * syms = new_list(NULL, 0);
	void * it;
	struct symbol * sym;
	parsedecl(f, DF_GLOBAL, syms);

	it = list_iterator(syms);
	while (iterator_next(&it, (void **)&sym)) {
		sym->type->to_string(stdout, sym->type, sym->id);
		printf("\n");
	}
	delete_list(syms, NULL);

	return res;
}
