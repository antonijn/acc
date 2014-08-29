/*
 * Declaration parsing and generation of intermediate representation
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
#include <assert.h>

#include <acc/parsing/decl.h>
#include <acc/parsing/expr.h>
#include <acc/error.h>
#include <acc/ext.h>
#include <acc/itm.h>
#include <acc/ast.h>

// primitive modifiers
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
	PM_DOUBLE = 0x100,
	PM_VOID = 0x400
};

static bool parsemod(FILE *f, enum declflags flags, enum qualifier *quals,
	enum primmod *pm, enum storageclass *sc);
static struct ctype *parsebasety(FILE *f, enum declflags flags,
	enum storageclass *sc);
static struct symbol *parsedeclarator(FILE *f, enum declflags flags,
	struct ctype *ty, enum storageclass sc);
static struct symbol *parseddeclarator(FILE *f, enum declflags flags,
	struct ctype *ty, enum storageclass sc);
static struct ctype *parseddend(FILE *f, struct ctype *ty);
static struct ctype *parseparamlist(FILE *f, struct ctype *ty);
static struct ctype *parsearray(FILE *f, struct ctype *ty);

static struct ctype *getfullty(struct ctype *incomp, struct ctype *ty);

static bool aremods(enum primmod new, enum primmod prev,
	enum primmod l, enum primmod r);
static void checkmods(struct token *tok, enum primmod new, enum primmod prev);

static struct ctype *getprimitive(enum primmod mods);

static struct ctype *parsestructure(FILE *f);
static struct ctype *parsetypedef(FILE *f);

bool parsedecl(FILE *f, enum declflags flags, struct list *syms, struct itm_block **b)
{
	enum storageclass sc = SC_DEFAULT;
	struct ctype *basety = parsebasety(f, flags, &sc);
	// number of symbols in syms before syms was appended to
	int numsymsb4 = syms ? list_length(syms) : -1;
	if (!basety)
		return false;

	while (true) {
		struct symbol *sym = parsedeclarator(f, flags, basety, sc);
		if (syms)
			list_push_back(syms, sym);

		if (flags & DF_ALLOCA) {
			assert(b != NULL);
			assert(sym != NULL);
			sym->value = (struct itm_expr *)itm_alloca(*b, sym->type);
		}
		
		if ((flags & DF_INIT) && chkt(f, "=") &&
		    sc != SC_EXTERN && sc != SC_TYPEDEF) {
			// TODO: this only works for locals
			struct expr e = parseexpr(f, EF_INIT | EF_FINISH_SEMICOLON |
				EF_EXPECT_RVALUE, b, sym->type);
			itm_store(*b, e.itm, sym->value);
		}
		if ((flags & DF_BITFIELD) && chkt(f, ":")) {
			// TODO: parse bitfield
			assert(false);
		}
		if (((flags & DF_FINISH_SEMICOLON) && chkt(f, ";")) ||
		    ((flags & DF_FINISH_COMMA) && chkt(f, ",")))
			break;

		struct token closep;
		if (((flags & DF_FINISH_PARENT) && chktp(f, ")", &closep)) ||
		    ((flags & DF_FINISH_BRACE) && sym->type->type == FUNCTION &&
		     syms && (list_length(syms) - numsymsb4) == 1 &&
		    chktp(f, "{", &closep))) {
			ungettok(&closep, f);
			freetok(&closep);
			break;
		}
		if ((flags & DF_MULTIPLE) && chkt(f, ","))
			continue;

		struct token tok = gettok(f);
		report(E_PARSER, &tok, "unexpected token in declaration");
		break;
	}

	return true;
}

static bool parsestorage(FILE *f, enum storageclass *sc,
	const char *mod, enum storageclass set)
{
	struct token nxt;
	if (chktp(f, mod, &nxt)) {
		if (*sc == set)
			report(E_PARSER, &nxt, "duplicate \"%s\" modifier", mod);
		else if (*sc != SC_DEFAULT)
			report(E_PARSER, &nxt, "multiple storage class specifiers");
		*sc = set;
		freetok(&nxt);
		return true;
	}
	return false;
}

static bool aremods(enum primmod new, enum primmod prev,
	enum primmod l, enum primmod r)
{
	return ((prev & l) == l && (new & r) == r) ||
	       ((new & l) == l && (prev & r) == r);
}

static void checkmods(struct token *tok, enum primmod new, enum primmod prev)
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
	if (aremods(prev, new, PM_FLOAT, PM_VOID))
		report(E_PARSER, tok, "\"float\" and \"void\" are mutually exclusive");
	if (aremods(prev, new, PM_VOID, PM_DOUBLE))
		report(E_PARSER, tok, "\"void\" and \"double\" are mutually exclusive");
}

static bool parsemod(FILE *f, enum declflags flags, enum qualifier *quals,
	enum primmod *pm, enum storageclass *sc)
{
	struct token nxt;
	if (chktp(f, "const", &nxt)) {
		if (*quals & Q_CONST)
			report(E_PARSER, &nxt, "duplicate \"const\" modifier");
		*quals |= Q_CONST;
		freetok(&nxt);
		return true;
	} else if (chktp(f, "volatile", &nxt)) {
		if (*quals & Q_VOLATILE)
			report(E_PARSER, &nxt, "duplicate \"volatile\" modifier");
		*quals |= Q_VOLATILE;
		freetok(&nxt);
		return true;
	} else if (sc && parsestorage(f, sc, "auto", SC_AUTO))
		return true;
	else if (sc && parsestorage(f, sc, "static", SC_STATIC))
		return true;
	else if (sc && parsestorage(f, sc, "typedef", SC_TYPEDEF))
		return true;
	else if (sc && (flags & DF_EXTERN) && parsestorage(f, sc, "extern", SC_EXTERN))
		return true;
	else if (sc && (flags & DF_REGISTER) && parsestorage(f, sc, "register", SC_REGISTER))
		return true;
	else if (pm && chktp(f, "unsigned", &nxt)) {
		checkmods(&nxt, PM_UNSIGNED, *pm);
		*pm |= PM_UNSIGNED;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "signed", &nxt)) {
		checkmods(&nxt, PM_SIGNED, *pm);
		*pm |= PM_SIGNED;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "short", &nxt)) {
		checkmods(&nxt, PM_SHORT, *pm);
		*pm |= PM_SHORT;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "long", &nxt)) {
		checkmods(&nxt, PM_LONG, *pm);
		*pm |= PM_LONG;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "int", &nxt)) {
		checkmods(&nxt, PM_INT, *pm);
		*pm |= PM_INT;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "char", &nxt)) {
		checkmods(&nxt, PM_CHAR, *pm);
		*pm |= PM_CHAR;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "float", &nxt)) {
		checkmods(&nxt, PM_FLOAT, *pm);
		*pm |= PM_FLOAT;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "double", &nxt)) {
		checkmods(&nxt, PM_DOUBLE, *pm);
		*pm |= PM_DOUBLE;
		freetok(&nxt);
		return true;
	} else if (pm && chktp(f, "void", &nxt)) {
		checkmods(&nxt, PM_VOID, *pm);
		*pm |= PM_VOID;
		freetok(&nxt);
		return true;
	}

	return false;
}

static struct ctype *parsebasety(FILE *f, enum declflags flags,
	enum storageclass *sc)
{
	struct ctype *res;
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

static struct symbol *parsedeclarator(FILE *f, enum declflags flags,
	struct ctype *ty, enum storageclass sc)
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

static struct symbol *parseddeclarator(FILE *f, enum declflags flags,
	struct ctype *ty, enum storageclass sc)
{
	struct symbol *res;
	struct token tok;
	if (chkttp(f, T_IDENTIFIER, &tok)) {
		res = new_symbol(parseddend(f, ty), tok.lexeme, sc,
			flags & DF_REGISTER_SYMBOL);
		freetok(&tok);
	} else if (chkt(f, "(")) {
		res = parsedeclarator(f, flags, NULL, sc);
		struct token t = gettok(f);
		if (strcmp(t.lexeme, ")")) {
			report(E_PARSER, &t, "expected ')' to finish declarator");
			ungettok(&t, f);
		}
		freetok(&t);
		res->type = getfullty(res->type, parseddend(f, ty));
	} else
		return NULL;

	return res;
}

static struct ctype *parseddend(FILE *f, struct ctype *ty)
{
	struct ctype *backup = ty;
	ty = parseparamlist(f, ty);
	ty = parsearray(f, ty);
	if (backup != ty)
		return parseddend(f, ty);
	return ty;
}

static struct ctype *parseparamlist(FILE *f, struct ctype *ty)
{
	if (!chkt(f, "("))
		return ty;

	struct list *paramlist = new_list(NULL, 0);

	struct token tok1;
	if (chktp(f, "void", &tok1)) {
		struct token tok2;
		if (chktp(f, ")", &tok2)) {
			freetok(&tok1);
			freetok(&tok2);
			goto ret;
		} else {
			ungettok(&tok1, f);
			freetok(&tok1);
		}
	}

	while (!chkt(f, ")") && parsedecl(f, DF_PARAM, paramlist, NULL))
		;

ret:
	ty = new_function(ty, paramlist);
	delete_list(paramlist, NULL);
	return ty;
}

static struct ctype *parsearray(FILE *f, struct ctype *ty)
{
	// TODO: placeholder implementation
	return ty;
}

static struct ctype *getfullty(struct ctype *incomp, struct ctype *ty)
{
	struct cpointer *cp;
	struct cfunction *cf;

	if (!incomp)
		return ty;

	switch (incomp->type) {
	case POINTER:
		cp = (struct cpointer *)incomp;
		cp->pointsto = getfullty(cp->pointsto, ty);
		return incomp;
	case FUNCTION:
		cf = (struct cfunction *)incomp;
		cf->ret = getfullty(cf->ret, ty);
		return incomp;
	}

	return incomp;
}

static struct ctype *getprimitive(enum primmod mods)
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
	else if (mods & PM_VOID)
		return &cvoid;

	// unreachable
	return NULL;
}

static struct ctype *parsestructure(FILE *f)
{
	struct cstruct *str;
	if (!chkt(f, "struct"))
		return NULL;

	struct token idtok;
	bool hasid = chkttp(f, T_IDENTIFIER, &idtok);

	struct token tok;
	if (!chktp(f, "{", &tok)) {
		if (!hasid) {
			report(E_PARSER, &tok, "expected '{' or ';'");
		} else if (str = get_struct(idtok.lexeme)) {
			freetok(&idtok);
			freetok(&tok);
			return (struct ctype *)str;
		} else {
			str = (struct cstruct *)new_struct(idtok.lexeme);
			freetok(&idtok);
			freetok(&tok);
			return (struct ctype *)str;
		}

		freetok(&tok);
		return NULL;
	}

	str = (struct cstruct *)new_struct(hasid ? idtok.lexeme : NULL);
	if (hasid)
		freetok(&idtok);
	
	while (!chkt(f, "}")) {
		struct list *syms = new_list(NULL, 0);
		parsedecl(f, DF_FIELD, syms, NULL);

		struct symbol *sym;
		void *it = list_iterator(syms);
		while (iterator_next(&it, (void **)&sym))
			struct_add_field((struct ctype *)str, sym->type, sym->id);
		delete_list(syms, NULL);
	}

	return (struct ctype *)str;
}

static struct ctype *parsetypedef(FILE *f)
{
	struct token tok;
	if (!chkttp(f, T_IDENTIFIER, &tok))
		return NULL;
	struct ctype *ty = get_typedef(tok.lexeme);
	if (!ty)
		ungettok(&tok, f);
	freetok(&tok);
	return ty;
}
