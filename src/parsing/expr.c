/*
 * Expression parsing and generation of intermediate representation
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

#include <acc/parsing/expr.h>
#include <acc/parsing/tools.h>
#include <acc/list.h>
#include <acc/token.h>
#include <acc/ext.h>
#include <acc/error.h>

static struct itm_expr *pack(struct itm_block * b, struct itm_expr * e,
	enum exprflags flags);

static struct itm_expr * parseexpro(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * parsefcall(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * parseid(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * parsebop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * performaop(struct operator * op, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc, struct itm_expr * r);
static struct itm_expr * parseuop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * parseparents(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * parselit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * parseintlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);
static struct itm_expr * parsefloatlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc);

static struct itm_expr *pack(struct itm_block * b, struct itm_expr * e, enum exprflags flags)
{
	if ((flags & EF_EXPECT_RVALUE) && e->islvalue)
		return (struct itm_expr *)itm_load(b, e);
	if ((flags & EF_EXPECT_LVALUE) && !e->islvalue)
		report(E_ERROR | E_HIDE_TOKEN, NULL, "expected lvalue");
	return e;
}

static struct itm_expr * parseexpro(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	struct itm_expr * res;
	struct token * tok;
	
	if (((flags & EF_FINISH_BRACKET) && (tok = chktp(f, ")"))) ||
	    ((flags & EF_FINISH_SEMICOLON) && (tok = chktp(f, ";"))) ||
	    ((flags & EF_FINISH_SQUARE_BRACKET) && (tok = chktp(f, "]"))) ||
	    ((flags & EF_FINISH_COMMA) && (tok = chktp(f, ",")))) {
		ungettok(tok, f);
		freetp(tok);
		return pack(*block, acc, flags);
	}
	
	if ((res = parselit(f, flags, block, initty, operators, acc)) ||
	    (res = parseid(f, flags, block, initty, operators, acc)) ||
	    (res = parsebop(f, flags, block, initty, operators, acc)) ||
	    (res = parseuop(f, flags, block, initty, operators, acc)) ||
	    (res = parseparents(f, flags, block, initty, operators, acc)) ||
	    (res = parsefcall(f, flags, block, initty, operators, acc)))
		return pack(*block, res, flags);
	return NULL;
}

static struct itm_expr * parsefcall(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	/* TODO: implement */
	return NULL;
}

static struct itm_expr * parseid(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	/* TODO: implement */
	return NULL;
}

static struct itm_expr * parsebop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	struct operator * op;
	struct token * tok;
	struct itm_expr * e, * r;
	
	if (!acc)
		return NULL;
	
	if (!(tok = chkttp(f, T_OPERATOR)))
		return NULL;
	
	op = getbop(tok->lexeme);
	if (!op) {
		ungettok(tok, f);
		freetp(tok);
		return NULL;
	}
	
	if (op->rtol) {
		if (list_length(operators) > 0 &&
		   ((struct operator *)list_head(operators))->prec < op->prec)
			goto opbreak;
	} else {
		if (list_length(operators) > 0 &&
		   ((struct operator *)list_last(operators))->prec <= op->prec)
			goto opbreak;
	}
	
	list_push_back(operators, op);
	r = parseexpro(f, (flags & EF_CLEAR_MASK) | EF_EXPECT_RVALUE,
		block, initty, operators, NULL);
	list_pop_back(operators);
	
	if (e = performaop(op, flags, block, initty, operators, acc, r))
		goto ret;
	
ret:
	freetp(tok);
	return parseexpro(f, flags, block, initty, operators, e);
	
opbreak:
	ungettok(tok, f);
	freetp(tok);
	return acc;
}

static struct itm_expr * performaop(struct operator * op, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc, struct itm_expr * r)
{
	struct itm_instr * (*ifunc)(struct itm_block * b,
		struct itm_expr * l, struct itm_expr * r);
	struct ctype * lt = acc->type;
	struct ctype * rt = r->type;
	struct ctype * et;

	acc = pack(*block, acc, EF_EXPECT_RVALUE);
	
	if (op == &binop_plus) {
		if (hastc(acc->type, TC_ARITHMETIC))
			ifunc = &itm_add;
		else if (hastc(acc->type, TC_POINTER))
			/* TODO */;
		else
			goto invalid;
	} else if (op == &binop_min) {
		if (hastc(acc->type, TC_ARITHMETIC))
			ifunc = &itm_sub;
		else if (hastc(acc->type, TC_POINTER))
			/* TODO */;
		else
			goto invalid;
	} else if (op == &binop_div) {
		if (!hastc(acc->type, TC_ARITHMETIC))
			goto invalid;
		ifunc = &itm_div;
	} else if (op == &binop_mul) {
		if (!hastc(acc->type, TC_ARITHMETIC))
			goto invalid;
		ifunc = &itm_mul;
	} else if (op == &binop_mod) {
		if (!hastc(acc->type, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_rem;
	} else if (op == &binop_shl) {
		if (!hastc(acc->type, TC_INTEGRAL))
			goto invalid;
		if (hastc(acc->type, TC_SIGNED))
			ifunc = &itm_sal;
		else
			ifunc = &itm_shl;
	} else if (op == &binop_shr) {
		if (!hastc(acc->type, TC_INTEGRAL))
			goto invalid;
		if (hastc(acc->type, TC_SIGNED))
			ifunc = &itm_sar;
		else
			ifunc = &itm_shr;
	} else if (op == &binop_or) {
		if (!hastc(acc->type, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_or;
	} else if (op == &binop_and) {
		if (!hastc(acc->type, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_and;
	} else if (op == &binop_xor) {
		if (!hastc(acc->type, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_xor;
	} else
		return NULL; /* this shouldn't happen. ever. */
	
	if (lt == &cdouble || rt == &cdouble)
		et = &cdouble;
	else if (lt == &cfloat || rt == &cfloat)
		et = &cfloat;
	else if (lt == &culong || rt == &culong)
		et = &culong;
	else if (lt == &clong || rt == &clong)
		et = &clong;
	else if (lt == &cuint || rt == &cuint)
		et = &cuint;
	else
		et = &cint;
	
	acc = cast(acc, et, *block);
	r = cast(r, et, *block);
	
	return (struct itm_expr *)ifunc(*block, acc, r);

invalid:
	report(E_ERROR | E_HIDE_TOKEN, NULL, "invalid type for left expression");
	return NULL;
}

static struct itm_expr * parseuop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	/* TODO: implement */
	return NULL;
}

static struct itm_expr * parseparents(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	struct token close;
	struct list * noperators;
	if (acc)
		return NULL;
	
	if (!chkt(f, "("))
		return NULL;
	
	/* TODO: check for type cast */
	
	noperators = new_list(NULL, 0);
	acc = parseexpro(f, EF_NORMAL | EF_FINISH_BRACKET,
		block, initty, noperators, NULL);
	delete_list(noperators, NULL);
	
	/* remove closing ) from the stream */
	close = gettok(f);
	freetok(&close);
	
	return parseexpro(f, flags, block, initty, operators, acc);
}

static struct itm_expr * parselit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	struct itm_expr * res;
	if (res = parseintlit(f, flags, block, initty, operators, acc))
		return res;
	if (res = parsefloatlit(f, flags, block, initty, operators, acc))
		return res;
	return NULL;
}

static struct itm_expr * parseintlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	unsigned long ul;	
	struct token tok = gettok(f);
	struct itm_literal * lit;
	struct ctype * itypes[5] = { 0 };
	struct ctype * type;
	int i;
	
	switch (tok.type) {
	case T_DEC:
		sscanf(tok.lexeme, "%lu", &ul);
		itypes[0] = &cint;
		itypes[1] = &clong;
		break;
	case T_OCT:
		sscanf(tok.lexeme, "0%lo", &ul);
		itypes[0] = &cint;
		itypes[1] = &cuint;
		itypes[2] = &clong;
		itypes[3] = &culong;
		break;
	case T_HEX:
		if (!sscanf(tok.lexeme, "0x%lx", &ul))
			sscanf(tok.lexeme, "0X%lx", &ul);
		itypes[0] = &cint;
		itypes[1] = &cuint;
		itypes[2] = &clong;
		itypes[3] = &culong;
		break;
	default:
		ungettok(&tok, f);
		freetok(&tok);
		return NULL;
	}
	
	freetok(&tok);
	
	for (i = 0; itypes[i]; ++i) {
		unsigned long top;
		type = itypes[i];
		if (type == &cuint ||
		    type == &culong) {
			top = (1ul << (type->size * 8 - 1)) - 1;
			top <<= 1;
			top |= 1;
		} else {
			top = (1ul << (type->size * 8 - 2)) - 1;
			top <<= 1;
			top |= 1;
		}
		if (ul <= top)
			break;
	}

	lit = new_itm_literal(type);
	lit->value.i = ul;

	return parseexpro(f, flags, block, initty, operators,
		(struct itm_expr *)lit);
}

static struct itm_expr * parsefloatlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct itm_expr * acc)
{
	struct token * tok;
	struct itm_literal * lit;
	if (tok = chkttp(f, T_FLOAT)) {
		float f;
		sscanf(tok->lexeme, "%ff", &f);
		lit = new_itm_literal(&cfloat);
		lit->value.f = f;
	} else if (tok = chkttp(f, T_DOUBLE)) {
		double d;
		sscanf(tok->lexeme, "%lf", &d);
		lit = new_itm_literal(&cdouble);
		lit->value.d = d;
	} else
		return NULL;
	
	freetp(tok);
	
	return parseexpro(f, flags, block, initty, operators,
		(struct itm_expr *)lit);
}

struct itm_expr * parseexpr(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty)
{
	struct list * operators = new_list(NULL, 0);
	struct itm_expr * e;
	e = parseexpro(f, flags, block, initty, operators, NULL);
	delete_list(operators, NULL);
	return e;
}

struct itm_expr * cast(struct itm_expr * e, struct ctype * ty,
	struct itm_block * b)
{
	assert(e != NULL);
	assert(ty != NULL);
	assert(e->type != NULL);

	if (e->type == ty)
		return e;

	if (hastc(e->type, TC_FLOATING) && hastc(ty, TC_INTEGRAL))
		return (struct itm_expr *)itm_ftoi(b, e, ty);

	if (hastc(e->type, TC_INTEGRAL) && hastc(ty, TC_FLOATING))
		return (struct itm_expr *)itm_itof(b, e, ty);

	if (hastc(e->type, TC_POINTER) && hastc(ty, TC_INTEGRAL)) {
		if (e->type->size > ty->size)
			return (struct itm_expr *)itm_trunc(b, e, ty);
		if (e->type->size < ty->size)
			return (struct itm_expr *)itm_zext(b, e, ty);
		return (struct itm_expr *)itm_bitcast(b, e, ty);
	}

	/* TODO: other casts */

	return e;
}
