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
#include <string.h>

#include <acc/ast.h>

struct ctype cint;
struct ctype cshort;
struct ctype cchar;
struct ctype clong;
struct ctype cuint;
struct ctype cushort;
struct ctype cuchar;
struct ctype culong;
struct ctype cfloat;
struct ctype cdouble;
struct ctype cvoid;

static struct list * alltypes;
static struct list * typescopes;
static struct list * symscopes;

static void registerty(struct ctype * t)
{
	struct list * topscope;
	list_push_back(alltypes, t);
	topscope = list_last(typescopes);
	list_push_back(topscope, t);
}

static void primitive_free(struct ctype * p)
{
}

static void primitive_to_string(FILE * f, struct ctype * p)
{
	fprintf(f, "%s", p->name);
}

static enum typecomp primitive_compare(struct ctype * l, struct ctype * r)
{
	/* TODO: proper implementation */
	return EXPLICIT;
}

static void initprimitive(struct ctype * p, const char * name)
{
	p->free = &primitive_free;
	p->type = PRIMITIVE;
	p->size = -1; /* TODO: get platform-specific size */
	p->name = name;
	p->to_string = &primitive_to_string;
	p->compare = &primitive_compare;
	registerty(p);
}

void ast_init(void)
{
	alltypes = new_list(NULL, 0);
	typescopes = new_list(NULL, 0);
	symscopes = new_list(NULL, 0);

	enter_scope();
	initprimitive(&cint, "signed int");
	initprimitive(&cshort, "signed short int");
	initprimitive(&cchar, "signed char");
	initprimitive(&clong, "signed long int");
	initprimitive(&cuint, "unsigned int");
	initprimitive(&cushort, "unsigned short int");
	initprimitive(&cuchar, "unsigned char");
	initprimitive(&culong, "unsigned long int");
	initprimitive(&cfloat, "float");
	initprimitive(&cdouble, "double");
	initprimitive(&cvoid, "void");
	enter_scope();
}

void ast_destroy(void)
{
	leave_scope();
	leave_scope();
	delete_list(alltypes, NULL);
	delete_list(typescopes, NULL);
	delete_list(symscopes, NULL);
}

void enter_scope(void)
{
	struct list * typescope = new_list(NULL, 0);
	struct list * symscope = new_list(NULL, 0);
	list_push_back(typescopes, typescope);
	list_push_back(symscopes, symscope);
}

void leave_scope(void)
{
	struct list * typescope = list_pop_back(typescopes);
	struct list * symscope = list_pop_back(symscopes);
	delete_list(typescope, NULL);
	delete_list(symscope, NULL);
}


static void pointer_to_string(FILE * f, struct ctype * p)
{
	struct cpointer * ptr = (struct cpointer *)p;
	fprintf(f, "ptr(");
	ptr->pointsto->to_string(f, ptr->pointsto);
	fprintf(f, ")");
}

static enum typecomp pointer_compare(struct ctype * l, struct ctype * r)
{
	/* TODO: proper implementation */
	return IMPLICIT;
}

struct ctype * new_pointer(struct ctype * base)
{
	struct cpointer * ty = malloc(sizeof(struct cpointer));
	ty->base.free = (void (*)(struct ctype *))&free;
	ty->base.type = POINTER;
	ty->base.size = -1; /* TODO: get target-specific value */
	ty->base.name = NULL;
	ty->base.to_string = &pointer_to_string;
	ty->base.compare = &pointer_compare;
	ty->pointsto = base;
	registerty((struct ctype *)ty);
	return (struct ctype *)ty;
}

static void struct_to_string(FILE * f, struct ctype * p)
{
	struct cstruct * cs = (struct cstruct *)p;
	fprintf(f, "struct %s", cs->base.name ? cs->base.name : "{ ... }");
}

static enum typecomp struct_compare(struct ctype * l, struct ctype * r)
{
	return INCOMPATIBLE;
}

static void free_struct(struct ctype * t)
{
	struct cstruct * cs = (struct cstruct *)t;
	free((void *)t->name);
	delete_list(cs->fields, NULL);
	free(t);
}

struct ctype * new_struct(char * id)
{
	struct cstruct * ty = malloc(sizeof(struct cpointer));
	ty->base.free = &free_struct;
	ty->base.type = STRUCTURE;
	ty->base.size = -1; /* TODO: get target-specific value */
	if (id) {
		ty->base.name = calloc(sizeof(char), strlen(id) + 1);
		sprintf((char *)ty->base.name, "%s", id);
	} else
		ty->base.name = NULL;
	ty->base.to_string = &struct_to_string;
	ty->base.compare = &struct_compare;
	ty->fields = new_list(NULL, 0);
	registerty((struct ctype *)ty);
	return (struct ctype *)ty;
}

void struct_add_field(struct ctype * type, struct ctype * ty, char * id)
{
	struct cstruct * cs = (struct cstruct *)type;
	size_t strl = strlen(id) + 1;
	struct field * fi = malloc(sizeof(struct field));
	fi->id = calloc(sizeof(char), strl);
	sprintf(fi->id, "%s", id);
	fi->type = ty;
	list_push_back(cs->fields, fi);
}

struct field * struct_get_field(struct ctype * type, char * name)
{
	struct cstruct * cs = (struct cstruct *)type;
	void * it;
	struct field * fi;
	it = list_iterator(cs->fields);
	while (iterator_next(&it, (void **)&fi))
		if (!strcmp(name, fi->id))
			return fi;

	return NULL;
}

struct ctype * new_union(char * name)
{
	return new_struct(name);
}

struct ctype * new_array(struct ctype * etype, int length)
{
	/* TODO: implement */
	return NULL;
}

static void qualified_to_string(FILE * f, struct ctype * ty)
{
	struct cqualified * cq = (struct cqualified *)ty;
	if (cq->qualifiers & Q_CONST)
		fprintf(f, "const(");
	if (cq->qualifiers & Q_VOLATILE)
		fprintf(f, "volatile(");
	cq->type->to_string(f, cq->type);
	if (cq->qualifiers & Q_CONST)
		fprintf(f, ")");
	if (cq->qualifiers & Q_VOLATILE)
		fprintf(f, ")");
}

static enum typecomp qualified_compare(struct ctype * l, struct ctype * r)
{
	/* TODO: proper implementation */
	return EXPLICIT;
}

struct ctype * new_qualified(struct ctype * base, enum qualifier q)
{
	struct cqualified * ty = malloc(sizeof(struct cqualified));
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

static void free_function(struct ctype * ty)
{
	struct cfunction * f = (struct cfunction *)ty;
	delete_list(f->parameters, NULL);
	free(f);
}

static void function_to_string(FILE * f, struct ctype * ty)
{
	int i;
	void * it;
	struct symbol * sym;
	struct cfunction * cf = (struct cfunction *)ty;
	fprintf(f, "ret(");
	cf->ret->to_string(f, cf->ret);
	fprintf(f, ") wparams (");

	i = 0;
	it = list_iterator(cf->parameters);
	while (iterator_next(&it, (void **)&sym)) {
		sym->type->to_string(f, sym->type);

		if (i++ != list_length(cf->parameters) - 1)
			fprintf(f, ", ");
	}
	fprintf(f, ")");
}

static enum typecomp function_compare(struct ctype * l, struct ctype * r)
{
	/* TODO: proper implementation */
	return INCOMPATIBLE;
}

struct ctype * new_function(struct ctype * ret, struct list * params)
{
	struct cfunction * ty = malloc(sizeof(struct cfunction));
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

struct symbol * get_symbol(char * id)
{
	int i;
	for (i = list_length(symscopes); i >= 0; --i) {
		struct list * l = get_list_item(symscopes, i);
		void * it;
		struct symbol * sym;
		it = list_iterator(l);
		while (iterator_next(&it, (void **)&sym))
			if (!strcmp(sym->id, id))
				return sym;
	}
	return NULL;
}

struct enumerator * get_enumerator(char * id)
{
	/* TODO: implement */
	return NULL;
}

struct cstruct * get_struct(char * name)
{
	int i;
	for (i = list_length(typescopes); i >= 0; --i) {
		struct list * l = get_list_item(typescopes, i);
		void * it;
		struct ctype * sym;
		it = list_iterator(l);
		while (iterator_next(&it, (void **)&sym))
			if (sym->type == STRUCTURE && !strcmp(sym->name, name))
				return (struct cstruct *)sym;
	}
	return NULL;
}

struct cunion * get_union(char * name)
{
	int i;
	for (i = list_length(typescopes); i >= 0; --i) {
		struct list * l = get_list_item(typescopes, i);
		void * it;
		struct ctype * sym;
		it = list_iterator(l);
		while (iterator_next(&it, (void **)&sym))
			if (sym->type == UNION && !strcmp(sym->name, name))
				return (struct cunion *)sym;
	}
	return NULL;
}

struct symbol * new_symbol(struct ctype * type, char * id,
	enum storageclass sc, int reg)
{
	struct symbol * sym = malloc(sizeof(struct symbol));
	sym->type = type;
	sym->id = calloc(sizeof(char), strlen(id));
	sprintf(sym->id, "%s", id);
	sym->storage = sc;
	return sym;
}

struct operator binop_plus = { 6, 0, "+" };
struct operator binop_min = { 6, 0, "-" };
struct operator binop_div = { 5, 0, "/" };
struct operator binop_mul = { 5, 0, "*" };
struct operator binop_mod = { 5, 0, "%" };
struct operator binop_shl = { 7, 0, "<<" };
struct operator binop_shr = { 7, 0, ">>" };
struct operator binop_or = { 12, 0, "|" };
struct operator binop_and = { 10, 0, "&" };
struct operator binop_xor = { 11, 0, "^" };
struct operator binop_shcor = { 14, 0, "||" };
struct operator binop_shcand = { 14, 0, "&&" };
struct operator binop_lt = { 8, 0, "<" };
struct operator binop_lte = { 8, 0, "<=" };
struct operator binop_gt = { 8, 0, ">" };
struct operator binop_gte = { 8, 0, ">=" };
struct operator binop_eq = { 9, 0, "==" };
struct operator binop_neq = { 9, 0, "!=" };
struct operator binop_assign = { 16, 1, "=" };
struct operator binop_assign_plus = { 16, 1, "+=" };
struct operator binop_assign_min = { 16, 1, "-=" };
struct operator binop_assign_mul = { 16, 1, "*=" };
struct operator binop_assign_div = { 16, 1, "/=" };
struct operator binop_assign_mod = { 16, 1, "%=" };
struct operator binop_assign_shl = { 16, 1, "<<=" };
struct operator binop_assign_shr = { 16, 1, ">>=" };
struct operator binop_assign_and = { 16, 1, "&=" };
struct operator binop_assign_xor = { 16, 1, "^=" };
struct operator binop_assign_or = { 16, 1, "|=" };

struct operator unop_suffinc = { 2, 0, "++" };
struct operator unop_suffdef = { 2, 0, "--" };
struct operator unop_preinc = { 3, 1, "++" };
struct operator unop_predec = { 3, 1, "--" };
struct operator unop_plus = { 3, 1, "+" };
struct operator unop_min = { 3, 1, "-" };
struct operator unop_deref = { 3, 1, "-" };
struct operator unop_ref = { 3, 1, "-" };
struct operator unop_sizeof = { 3, 1, "-" };
