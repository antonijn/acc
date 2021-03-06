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

#ifndef PARSING_AST_H
#define PARSING_AST_H

#include <stdio.h>
#include <stdbool.h>

#include <acc/list.h>

/*
 * Indicates type compatibility
 */
enum typecomp {
	TC_EQUAL,
	TC_IMPLICIT,
	TC_EXPLICIT,
	TC_INCOMPATIBLE
};

enum ctypeid {
	ENUMERATION,
	POINTER,
	ARRAY,
	PRIMITIVE,
	FUNCTION,
	STRUCTURE,
	UNION,
	QUALIFIED /* e.g. const, volatile */
};

enum storageclass {
	SC_DEFAULT,
	SC_AUTO,
	SC_STATIC,
	SC_REGISTER,
	SC_EXTERN,
	SC_TYPEDEF
};

enum qualifier {
	Q_NONE = 0x0,
	Q_CONST = 0x1,
	Q_VOLATILE = 0x2
};

struct ctype {
	void (*free)(struct ctype *t);
	enum ctypeid type;
	size_t size;
	const char * name;
	void (*to_string)(FILE *f, struct ctype *t);
	enum typecomp (*compare)(struct ctype *t, struct ctype *r);
};

/*
 * The primitive types in C
 */
extern struct ctype cint, cshort, clong, cuint, cushort, culong,
	clonglong, culonglong, cchar, cuchar,
	cfloat, cdouble, cvoid, clongdouble, cbool;

/*
 * struct/union fields
 */
struct field {
	struct ctype *type;
	char *id;
};

/*
 * Also used to represent union types (in which case the offset is 0 for all elements)
 */
struct cstruct {
	struct ctype base;
	struct list *fields;
	int (*field_offset)(struct field *f);
};

struct cpointer {
	struct ctype base;
	struct ctype *pointsto;
};

struct carray {
	struct ctype base;
	int length;
	struct ctype *elementtype;
};

/*
 * 'type' represents the unqualified type being qualified
 */
struct cqualified {
	struct ctype base;
	struct ctype *type;
	enum qualifier qualifiers;
};

struct cfunction {
	struct ctype base;
	struct ctype *ret;
	struct list *parameters; /* symbol list */
};

struct ctype *new_pointer(struct ctype *base);
struct ctype *new_struct(char *id);
void struct_add_field(struct ctype *type, struct ctype *ty, char *id);
struct field *struct_get_field(struct ctype *type, char *name);
struct ctype *new_union(char *name);
struct ctype *new_array(struct ctype *etype, int length);
struct ctype *new_qualified(struct ctype *base, enum qualifier q);
struct ctype *new_function(struct ctype *ret, struct list *params);

enum typeclass {
	TC_ARITHMETIC = 0x1,
	TC_FLOATING = 0x2,
	TC_INTEGRAL = 0x4,
	TC_POINTER = 0x8,
	TC_COMPOSITE = 0x10,
	TC_SIGNED = 0x20,
	TC_UNSIGNED = 0x40
};

bool hastc(struct ctype *ty, enum typeclass tc);
enum typeclass gettc(struct ctype *ty);

struct symbol {
	struct ctype *type;
	char *id;
	enum storageclass storage;
	struct itm_expr *value;
};

struct symbol *new_symbol(struct ctype *type, char *id,
	enum storageclass sc, bool reg);
/*
 * Register a symbol in the AST
 */
void registersym(struct symbol *sym);

/*
 * C enumerator
 */
struct enumerator {
	struct ctype *type;
	char *id;
	long i;
};

/*
 * C operator
 */
struct operator {
	int prec;
	bool rtol;
	const char *rep;
};

void ast_init(void);
void ast_destroy(void);

void enter_scope(void);
void leave_scope(void);

/*
 * Extract data from the AST
 */
struct symbol *get_symbol(char *id);
struct enumerator *get_enumerator(char *id);
struct cstruct *get_struct(char *name);
struct cstruct *get_union(char *name);
struct ctype *get_typedef(char *id);

/*
 * Get binary operator by operator string
 */
struct operator *getbop(const char *opname);
/*
 * Get unary operator by operator string
 * Always returns preinc and predec operators for "++" and "--" respectively
 */
struct operator *getuop(const char *opname);

extern struct operator binop_plus;
extern struct operator binop_min;
extern struct operator binop_div;
extern struct operator binop_mul;
extern struct operator binop_mod;
extern struct operator binop_shl;
extern struct operator binop_shr;
extern struct operator binop_or;
extern struct operator binop_and;
extern struct operator binop_xor;
extern struct operator binop_shcor;
extern struct operator binop_shcand;
extern struct operator binop_lt;
extern struct operator binop_lte;
extern struct operator binop_gt;
extern struct operator binop_gte;
extern struct operator binop_eq;
extern struct operator binop_neq;
extern struct operator binop_assign;
extern struct operator binop_assign_plus;
extern struct operator binop_assign_min;
extern struct operator binop_assign_mul;
extern struct operator binop_assign_div;
extern struct operator binop_assign_mod;
extern struct operator binop_assign_shl;
extern struct operator binop_assign_shr;
extern struct operator binop_assign_and;
extern struct operator binop_assign_xor;
extern struct operator binop_assign_or;

extern struct operator unop_postinc;
extern struct operator unop_postdec;
extern struct operator unop_preinc;
extern struct operator unop_predec;
extern struct operator unop_plus;
extern struct operator unop_min;
extern struct operator unop_deref;
extern struct operator unop_ref;
extern struct operator unop_not;
extern struct operator unop_sizeof;

#endif
