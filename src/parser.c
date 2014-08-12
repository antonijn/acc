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

	DF_GLOBAL = DF_INIT | DF_FUNCTION | DF_MULTIPLE |
		DF_EXTERN | DF_REGISTER_SYMBOL | DF_FINISH_SEMICOLON,
	DF_LOCAL = DF_INIT | DF_MULTIPLE | DF_REGISTER |
		DF_REGISTER_SYMBOL | DF_FINISH_SEMICOLON,
	DF_FIELD = DF_BITFIELD | DF_MULTIPLE | DF_FINISH_SEMICOLON,
	DF_TYPEDEF = DF_FUNCTION | DF_REGISTER_SYMBOL | DF_FINISH_SEMICOLON,
	DF_CAST = DF_NO_ID,
	DF_PARAM = DF_OPTIONAL_ID | DF_FINISH_COMMA | DF_REGISTER_SYMBOL
};

/* primitive modifiers */
enum primmod {
	PM_NONE = 0x00,
	PM_IMOD = 0x200,
	PM_UNSIGNED = 0x01 | PM_IMOD,
	PM_SIGNED = 0x02 | PM_IMOD,
	PM_LONG = 0x04 | PM_IMOD,
	PM_LONG_LONG = 0x08 | PM_IMOD,
	PM_SHORT = 0x10 | PM_IMOD,
	PM_INT = 0x20 | PM_IMOD,
	PM_CHAR = 0x40 | PM_IMOD,
	PM_FLOAT = 0x80,
	PM_DOUBLE = 0x100
};

static int parsedecl(FILE * f, enum declflags flags, struct list * syms);
static int parsemod(FILE * f, enum declflags flags, enum qualifier * quals,
	enum primmod * pm, enum storageclass * sc);
static struct ctype * parsebasety(FILE * f, enum declflags flags,
	enum storageclass * sc);
static struct symbol * parsedeclarator(FILE * f, enum declflags flags,
	struct ctype * ty, enum storageclass sc);
static struct symbol * parseddeclarator(FILE * f, enum declflags flags,
	struct ctype * ty, enum storageclass sc);

static int aremods(enum primmod new, enum primmod prev,
	enum primmod l, enum primmod r);
static void checkmods(struct token * tok, enum primmod new, enum primmod prev);

static struct ctype * getprimitive(enum primmod mods);

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
static int parsedecl(FILE * f, enum declflags flags, struct list * syms)
{
	enum storageclass sc;
	struct ctype * basety = parsebasety(f, flags, &sc);
	if (!basety)
		return 0;

	while (1) {
		struct token tok;
		struct symbol * sym;

		sym = parsedeclarator(f, flags, basety, sc);
		list_push_back(syms, sym);
		if ((flags & DF_INIT) && chkt(f, "=") &&
		    sc != SC_EXTERN && sc != SC_TYPEDEF) {
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

	return 1;
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

static int aremods(enum primmod new, enum primmod prev,
	enum primmod l, enum primmod r)
{
	return ((prev & l) == l && (new & r) == r) ||
	       ((new & l) == l && (prev & r) == r);
}

static void checkmods(struct token * tok, enum primmod new, enum primmod prev)
{
	if (prev == PM_NONE)
		return;

	if ((prev & new) == new)
		report(E_PARSER, tok, "duplicate modifier");

	if (((prev & PM_IMOD) && !(new & PM_IMOD)) ||
	    ((new & PM_IMOD) && !(prev & PM_IMOD)))
		report(E_PARSER, tok, "invalid modifier for type");
	if (aremods(prev, new, PM_LONG, PM_CHAR))
		report(E_PARSER, tok, "invalid modifier for type");
	if (aremods(prev, new, PM_SHORT, PM_CHAR))
		report(E_PARSER, tok, "invalid modifier for type");
	if (aremods(prev, new, PM_SIGNED, PM_UNSIGNED))
		report(E_PARSER, tok, "\"signed\" and \"unsigned\" are mutually exclusive");
	if (aremods(prev, new, PM_LONG, PM_SHORT))
		report(E_PARSER, tok, "\"short\" and \"long\" are mutually exclusive");
	if (aremods(prev, new, PM_FLOAT, PM_DOUBLE))
		report(E_PARSER, tok, "\"float\" and \"double\" are mutually exclusive");
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
	} else if (sc && parsestorage(f, sc, "auto", SC_AUTO))
		return 1;
	else if (sc && parsestorage(f, sc, "static", SC_STATIC))
		return 1;
	else if (sc && parsestorage(f, sc, "typedef", SC_TYPEDEF))
		return 1;
	else if (sc && (flags & DF_EXTERN) && parsestorage(f, sc, "extern", SC_EXTERN))
		return 1;
	else if (sc && (flags & DF_REGISTER) && parsestorage(f, sc, "register", SC_REGISTER))
		return 1;
	else if (pm && (nxt = chktp(f, "unsigned"))) {
		checkmods(nxt, PM_UNSIGNED, *pm);
		*pm |= PM_UNSIGNED;
		freetp(nxt);
		return 1;
	} else if (pm && (nxt = chktp(f, "signed"))) {
		checkmods(nxt, PM_SIGNED, *pm);
		*pm |= PM_SIGNED;
		freetp(nxt);
		return 1;
	} else if (pm && (nxt = chktp(f, "short"))) {
		checkmods(nxt, PM_SHORT, *pm);
		*pm |= PM_SHORT;
		freetp(nxt);
		return 1;
	} else if (pm && (nxt = chktp(f, "long"))) {
		checkmods(nxt, PM_LONG, *pm);
		*pm |= PM_LONG;
		freetp(nxt);
		return 1;
	} else if (pm && (nxt = chktp(f, "int"))) {
		checkmods(nxt, PM_INT, *pm);
		*pm |= PM_INT;
		freetp(nxt);
		return 1;
	} else if (pm && (nxt = chktp(f, "char"))) {
		checkmods(nxt, PM_CHAR, *pm);
		*pm |= PM_CHAR;
		freetp(nxt);
		return 1;
	} else if (pm && (nxt = chktp(f, "float"))) {
		checkmods(nxt, PM_FLOAT, *pm);
		*pm |= PM_FLOAT;
		freetp(nxt);
		return 1;
	} else if (pm && (nxt = chktp(f, "double"))) {
		checkmods(nxt, PM_DOUBLE, *pm);
		*pm |= PM_DOUBLE;
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

	if (pm != PM_NONE) {
		res = getprimitive(pm);
		goto ret;
	}

	if (res = parsestructure(f))
		goto ret;
	if (res = parsetypedef(f))
		goto ret;
	if (quals != Q_NONE || *sc != SC_DEFAULT) {
		res = getprimitive(PM_INT);
		goto ret;
	}

	return NULL;
ret:
	while (parsemod(f, flags, &quals, NULL, sc))
		;
	if (*sc == SC_DEFAULT)
		*sc = SC_AUTO;
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

static struct ctype * getprimitive(enum primmod mods)
{
	if ((mods & PM_LONG) == PM_LONG) {
		if ((mods & PM_UNSIGNED) == PM_UNSIGNED)
			return &culong;
		return &clong;
	} else if ((mods & PM_SHORT) == PM_SHORT) {
		if ((mods & PM_UNSIGNED) == PM_UNSIGNED)
			return &cushort;
		return &cshort;
	} else if ((mods & PM_CHAR) == PM_CHAR) {
		if ((mods & PM_UNSIGNED) == PM_UNSIGNED)
			return &cuchar;
		else if ((mods & PM_SIGNED) == PM_SIGNED)
			return &cchar;
		return isext(EX_UNSIGNED_CHAR) ? &cuchar : &cchar;
	} else if (mods & PM_IMOD) {
		if ((mods & PM_UNSIGNED) == PM_UNSIGNED)
			return &cuint;
		return &cint;
	} else if (mods & PM_FLOAT)
		return &cfloat;
	else if (mods & PM_DOUBLE)
		return &cdouble;

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
