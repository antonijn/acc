/*
 * Utilities for AST manipulation
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
#include <assert.h>

#include <acc/itm/ast.h>
#include <acc/parsing/ast.h>
#include <acc/target.h>
#include <acc/ext.h>
#include <acc/term.h>

struct ctype cint, cshort, clong, cuint, cushort, culong,
	clonglong, culonglong, cchar, cuchar,
	cfloat, cdouble, cvoid, clongdouble, cbool;

static struct list *alltypes;
static struct list *typescopes;
static struct list *allsyms;
static struct list *symscopes;

static void delete_symbol(void *sym);

static void registerty(struct ctype *t)
{
	list_push_back(alltypes, t);
	struct list *topscope = list_last(typescopes);
	list_push_back(topscope, t);
}

static void primitive_free(struct ctype *p)
{
}

static void primitive_to_string(FILE *f, struct ctype *p)
{
	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, ANSI_BLUE(ITM_COLORS));
	fprintf(f, "%s", p->name);
	fprintf(f, ANSI_RESET(ITM_COLORS));
}

static enum typecomp primitive_compare(struct ctype *l, struct ctype *r)
{
	if (l == r)
		return TC_EQUAL;
	if (hastc(r, TC_ARITHMETIC))
		return TC_IMPLICIT;
	if (hastc(r, TC_POINTER) && hastc(l, TC_INTEGRAL))
		return TC_EXPLICIT;
	return TC_INCOMPATIBLE;
}

static void initprimitive(struct ctype *p, const char *name)
{
	p->free = &primitive_free;
	p->type = PRIMITIVE;
	p->size = gettypesize(p);
	p->name = name;
	p->to_string = &primitive_to_string;
	p->compare = &primitive_compare;
	if ((p != &cbool || isext(EX_BOOL)) &&
	    (p != &clonglong || isext(EX_LONG_LONG)) &&
	    (p != &culonglong || isext(EX_LONG_LONG)) &&
	    (p != &clongdouble || isext(EX_LONG_DOUBLE)))
		registerty(p);
}

bool hastc(struct ctype *ty, enum typeclass tc)
{
	return gettc(ty) & tc;
}

enum typeclass gettc(struct ctype *ty)
{
	if (ty->type == QUALIFIED)
		return gettc(((struct cqualified *)ty)->type);

	if (ty == &cfloat ||
	    ty == &cdouble ||
	    ty == &clongdouble)
		return TC_ARITHMETIC | TC_FLOATING;
	if (ty == &cuchar ||
	    ty == &cushort ||
	    ty == &cuint ||
	    ty == &culong ||
	    ty == &culonglong)
		return TC_ARITHMETIC | TC_INTEGRAL | TC_UNSIGNED;
	if (ty->type == PRIMITIVE)
		return TC_ARITHMETIC | TC_INTEGRAL | TC_SIGNED;
	if (ty->type == POINTER)
		return TC_POINTER;
	return TC_COMPOSITE;
}

void ast_init(void)
{
	alltypes = new_list(NULL, 0);
	typescopes = new_list(NULL, 0);
	allsyms = new_list(NULL, 0);
	symscopes = new_list(NULL, 0);

	enter_scope();
	initprimitive(&cbool, "_Bool");
	initprimitive(&cint, "int");
	initprimitive(&cshort, "short");
	initprimitive(&cchar, "signed char");
	initprimitive(&clong, "long");
	initprimitive(&cuint, "unsigned");
	initprimitive(&cushort, "unsigned short");
	initprimitive(&cuchar, "unsigned char");
	initprimitive(&culong, "unsigned long");
	initprimitive(&cfloat, "float");
	initprimitive(&cdouble, "double");
	initprimitive(&cvoid, "void");
	initprimitive(&clonglong, "long long");
	initprimitive(&culonglong, "unsigned long long");
	initprimitive(&clongdouble, "long double");
	enter_scope();
}

static void destroy_type(void * ty)
{
	struct ctype *cty = ty;
	cty->free(cty);
}

void ast_destroy(void)
{
	leave_scope();
	leave_scope();
	delete_list(alltypes, &destroy_type);
	delete_list(typescopes, NULL);
	delete_list(symscopes, NULL);
	delete_list(allsyms, &delete_symbol);
}

void enter_scope(void)
{
	list_push_back(typescopes, new_list(NULL, 0));
	list_push_back(symscopes, new_list(NULL, 0));
}

void leave_scope(void)
{
	struct list * typescope = list_pop_back(typescopes);
	struct list * symscope = list_pop_back(symscopes);
	delete_list(typescope, NULL);
	delete_list(symscope, NULL);
}


static void pointer_to_string(FILE *f, struct ctype *p)
{
	struct cpointer *ptr = (struct cpointer *)p;
	ptr->pointsto->to_string(f, ptr->pointsto);
	fprintf(f, "*");
}

static enum typecomp pointer_compare(struct ctype *l, struct ctype *r)
{
	if (r->type == QUALIFIED) {
		struct cqualified *cq = (struct cqualified *)r;
		return l->compare(l, cq->type);
	}
	if (r->type == PRIMITIVE)
		return TC_EXPLICIT;
	if (r->type != POINTER)
		return TC_INCOMPATIBLE;

	struct cpointer *cpl = (struct cpointer *)l;
	struct cpointer *cpr = (struct cpointer *)r;

	if (cpl->pointsto->compare(cpl->pointsto, cpr->pointsto) == TC_EQUAL)
		return TC_EQUAL;
	if (cpl->pointsto == &cvoid || cpr->pointsto == &cvoid)
		return TC_IMPLICIT;
	return TC_EXPLICIT;
}

struct ctype *new_pointer(struct ctype *base)
{
	struct cpointer *ty = malloc(sizeof(struct cpointer));
	ty->base.free = (void (*)(struct ctype *))&free;
	ty->base.type = POINTER;
	ty->base.size = gettypesize((struct ctype *)ty);
	ty->base.name = NULL;
	ty->base.to_string = &pointer_to_string;
	ty->base.compare = &pointer_compare;
	ty->pointsto = base;
	registerty((struct ctype *)ty);
	return (struct ctype *)ty;
}

static void struct_to_string(FILE *f, struct ctype *p)
{
	struct cstruct *cs = (struct cstruct *)p;
	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, ANSI_BLUE(ITM_COLORS));
	fprintf(f, "struct");
	fprintf(f, ANSI_RESET(ITM_COLORS));
	fprintf(f, " %s", cs->base.name ? cs->base.name : "{ ... }");
}

static enum typecomp struct_compare(struct ctype *l, struct ctype *r)
{
	if (l == r)
		return TC_EQUAL;
	return TC_INCOMPATIBLE;
}

static void free_struct(struct ctype *t)
{
	struct cstruct *cs = (struct cstruct *)t;
	free((void *)t->name);
	delete_list(cs->fields, NULL);
	free(t);
}

struct ctype *new_struct(char *id)
{
	struct cstruct *ty = malloc(sizeof(struct cpointer));
	ty->base.free = &free_struct;
	ty->base.type = STRUCTURE;
	ty->base.size = gettypesize((struct ctype *)ty);
	if (id) {
		ty->base.name = calloc(strlen(id) + 1, sizeof(char));
		sprintf((char *)ty->base.name, "%s", id);
	} else
		ty->base.name = NULL;
	ty->base.to_string = &struct_to_string;
	ty->base.compare = &struct_compare;
	ty->fields = new_list(NULL, 0);
	registerty((struct ctype *)ty);
	return (struct ctype *)ty;
}

void struct_add_field(struct ctype *type, struct ctype *ty, char *id)
{
	struct cstruct *cs = (struct cstruct *)type;
	size_t strl = strlen(id) + 1;
	struct field * fi = malloc(sizeof(struct field));
	fi->id = calloc(strl, sizeof(char));
	sprintf(fi->id, "%s", id);
	fi->type = ty;
	list_push_back(cs->fields, fi);
}

struct field *struct_get_field(struct ctype *type, char *name)
{
	struct cstruct *cs = (struct cstruct *)type;
	struct field *fi;
	void *it = list_iterator(cs->fields);
	while (iterator_next(&it, (void **)&fi))
		if (!strcmp(name, fi->id))
			return fi;

	return NULL;
}

struct ctype *new_union(char *name)
{
	return new_struct(name);
}

struct ctype *new_array(struct ctype *etype, int length)
{
	// TODO: implement
	return NULL;
}

static void qualified_to_string(FILE *f, struct ctype *ty)
{
	struct cqualified *cq = (struct cqualified *)ty;
	cq->type->to_string(f, cq->type);
	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, ANSI_BLUE(ITM_COLORS));
	if (cq->qualifiers & Q_CONST)
		fprintf(f, "const ");
	if (cq->qualifiers & Q_VOLATILE)
		fprintf(f, "volatile ");
	fprintf(f, ANSI_RESET(ITM_COLORS));
}

static enum typecomp qualified_compare(struct ctype *l, struct ctype *r)
{
	// TODO: is this right?
	struct cqualified *cq = (struct cqualified *)l;
	return cq->type->compare(cq->type, r);
}

struct ctype * new_qualified(struct ctype *base, enum qualifier q)
{
	struct cqualified *ty = malloc(sizeof(struct cqualified));
	ty->base.free = (void (*)(struct ctype *))&free;
	ty->base.type = QUALIFIED;
	ty->base.size = base->size;
	ty->base.name = NULL;
	ty->base.to_string = &qualified_to_string;
	ty->base.compare = &qualified_compare;
	ty->type = base;
	ty->qualifiers = q;
	registerty((struct ctype *)ty);
	return (struct ctype *)ty;
}

static void free_function(struct ctype *ty)
{
	struct cfunction *f = (struct cfunction *)ty;
	delete_list(f->parameters, NULL);
	free(f);
}

static void function_to_string(FILE *f, struct ctype *ty)
{
	struct cfunction *cf = (struct cfunction *)ty;
	cf->ret->to_string(f, cf->ret);
	fprintf(f, " (");

	int i = 0;
	struct symbol *sym;
	void *it = list_iterator(cf->parameters);
	while (iterator_next(&it, (void **)&sym)) {
		sym->type->to_string(f, sym->type);

		if (i++ != list_length(cf->parameters) - 1)
			fprintf(f, ", ");
	}
	fprintf(f, ")");
}

static enum typecomp function_compare(struct ctype *l, struct ctype *r)
{
	// TODO: proper implementation
	return TC_INCOMPATIBLE;
}

struct ctype *new_function(struct ctype *ret, struct list *params)
{
	struct cfunction *ty = malloc(sizeof(struct cfunction));
	ty->base.free = &free_function;
	ty->base.type = FUNCTION;
	ty->base.size = -1;
	ty->base.name = NULL;
	ty->base.to_string = &function_to_string;
	ty->base.compare = &function_compare;
	ty->ret = ret;
	ty->parameters = clone_list(params);
	registerty((struct ctype *)ty);
	return (struct ctype *)ty;
}

struct symbol *get_symbol(char *id)
{
	struct list *l;
	void *revit = list_rev_iterator(symscopes);
	while (rev_iterator_next(&revit, (void **)&l)) {
		struct symbol *sym;
		void *it = list_iterator(l);
		while (iterator_next(&it, (void **)&sym)) {
			if (!sym->id)
				continue;
			if (!strcmp(sym->id, id) && sym->storage != SC_TYPEDEF)
				return sym;
		}
	}
	return NULL;
}

struct enumerator *get_enumerator(char *id)
{
	// TODO: implement
	return NULL;
}

struct cstruct *get_struct(char *name)
{
	struct list *l;
	void *revit = list_rev_iterator(typescopes);
	while (rev_iterator_next(&revit, (void **)&l)) {
		struct ctype *sym;
		void *it = list_iterator(l);
		while (iterator_next(&it, (void **)&sym)) {
			if (!sym->name)
				continue;
			if (sym->type == STRUCTURE && !strcmp(sym->name, name))
				return (struct cstruct *)sym;
		}
	}
	return NULL;
}

struct cstruct *get_union(char *name)
{
	struct list *l;
	void *revit = list_rev_iterator(typescopes);
	while (rev_iterator_next(&revit, (void **)&l)) {
		struct ctype *sym;
		void *it = list_iterator(l);
		while (iterator_next(&it, (void **)&sym)) {
			if (!sym->name)
				continue;
			if (sym->type == UNION && !strcmp(sym->name, name))
				return (struct cstruct *)sym;
		}
	}
	return NULL;
}

struct symbol *new_symbol(struct ctype *type, char *id,
	enum storageclass sc, bool reg)
{
	struct symbol *sym = malloc(sizeof(struct symbol));
	sym->value = NULL;
	sym->type = type;
	if (id) {
		sym->id = malloc((strlen(id) + 1) * sizeof(char));
		sprintf(sym->id, "%s", id);
	} else {
		sym->id = NULL;
	}
	sym->storage = sc;
	if (reg)
		registersym(sym);
	list_push_back(allsyms, sym);
	return sym;
}

void registersym(struct symbol *sym)
{
	struct list *syms = list_last(symscopes);
	list_push_back(syms, sym);
}

static void delete_symbol(void *ptr)
{
	struct symbol *sym = ptr;
	if (sym->id)
		free(sym->id);
	if (sym->value && sym->value->etype == ITME_CONTAINER)
		delete_itm_container((struct itm_container *)sym->value);
	free(sym);
}

struct ctype *get_typedef(char *id)
{
	struct list *l;
	void *revit = list_rev_iterator(symscopes);
	while (rev_iterator_next(&revit, (void **)&l)) {
		struct symbol *sym;
		void *it = list_iterator(l);
		while (iterator_next(&it, (void **)&sym))
			if (!strcmp(sym->id, id) && sym->storage == SC_TYPEDEF)
				return sym->type;
	}
	return NULL;
}

struct operator *binoperators[] = {
	&binop_plus, &binop_min,
	&binop_div, &binop_mul,
	&binop_mod, &binop_shl,
	&binop_shr, &binop_or,
	&binop_and, &binop_xor,
	&binop_shcor, &binop_shcand,
	&binop_lt, &binop_lte,
	&binop_gt, &binop_gte,
	&binop_eq, &binop_neq,
	&binop_assign, &binop_assign_plus,
	&binop_assign_min, &binop_assign_mul,
	&binop_assign_div, &binop_assign_mod,
	&binop_assign_shl, &binop_assign_shr,
	&binop_assign_and, &binop_assign_xor,
	&binop_assign_or
};

struct operator *unoperators[] = {
	&unop_preinc, &unop_predec,
	&unop_postinc, &unop_postdec, // order is important
	&unop_plus, &unop_min,
	&unop_deref, &unop_ref,
	&unop_not, &unop_sizeof
};

struct operator *getbop(const char *opname)
{
	for (int i = 0; i < sizeof(binoperators) / sizeof(struct operator *); ++i)
		if (!strcmp(binoperators[i]->rep, opname))
			return binoperators[i];

	return NULL;
}

struct operator *getuop(const char *opname)
{
	for (int i = 0; i < sizeof(unoperators) / sizeof(struct operator *); ++i)
		if (!strcmp(unoperators[i]->rep, opname))
			return unoperators[i];

	return NULL;
}

struct operator binop_plus = { 6, false, "+" };
struct operator binop_min = { 6, false, "-" };
struct operator binop_div = { 5, false, "/" };
struct operator binop_mul = { 5, false, "*" };
struct operator binop_mod = { 5, false, "%" };
struct operator binop_shl = { 7, false, "<<" };
struct operator binop_shr = { 7, false, ">>" };
struct operator binop_or = { 12, false, "|" };
struct operator binop_and = { 10, false, "&" };
struct operator binop_xor = { 11, false, "^" };
struct operator binop_shcor = { 14, false, "||" };
struct operator binop_shcand = { 14, false, "&&" };
struct operator binop_lt = { 8, false, "<" };
struct operator binop_lte = { 8, false, "<=" };
struct operator binop_gt = { 8, false, ">" };
struct operator binop_gte = { 8, false, ">=" };
struct operator binop_eq = { 9, false, "==" };
struct operator binop_neq = { 9, false, "!=" };
struct operator binop_assign = { 16, true, "=" };
struct operator binop_assign_plus = { 16, true, "+=" };
struct operator binop_assign_min = { 16, true, "-=" };
struct operator binop_assign_mul = { 16, true, "*=" };
struct operator binop_assign_div = { 16, true, "/=" };
struct operator binop_assign_mod = { 16, true, "%=" };
struct operator binop_assign_shl = { 16, true, "<<=" };
struct operator binop_assign_shr = { 16, true, ">>=" };
struct operator binop_assign_and = { 16, true, "&=" };
struct operator binop_assign_xor = { 16, true, "^=" };
struct operator binop_assign_or = { 16, true, "|=" };

struct operator unop_postinc = { 2, false, "++" };
struct operator unop_postdec = { 2, false, "--" };
struct operator unop_preinc = { 3, true, "++" };
struct operator unop_predec = { 3, true, "--" };
struct operator unop_plus = { 3, true, "+" };
struct operator unop_min = { 3, true, "-" };
struct operator unop_deref = { 3, true, "*" };
struct operator unop_ref = { 3, true, "&" };
struct operator unop_not = { 3, true, "~" };
struct operator unop_sizeof = { 3, true, "sizeof" };
