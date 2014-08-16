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

static struct expr pack(struct itm_block * b, struct expr e,
	enum exprflags flags);

static struct expr parseexpro(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parsefcall(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parseid(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parsebop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr performaop(FILE * f, struct operator * op, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr performasnop(FILE * f, struct operator * op, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parseuop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parseparents(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parselit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parseintlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);
static struct expr parsefloatlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc);

static struct expr pack(struct itm_block * b, struct expr e, enum exprflags flags)
{
	assert(e.itm != NULL);

	if ((flags & EF_EXPECT_RVALUE) && e.islvalue) {
		struct expr res;
		res.islvalue = 0;
		res.itm = (struct itm_expr *)itm_load(b, e.itm);
		return res;
	}
	if ((flags & EF_EXPECT_LVALUE) && !e.islvalue)
		report(E_ERROR | E_HIDE_TOKEN, NULL, "expected lvalue");
	return e;
}

static struct expr parseexpro(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct expr res = { 0 };
	struct token * tok;
	
	if (((flags & EF_FINISH_BRACKET) && (tok = chktp(f, ")"))) ||
	    ((flags & EF_FINISH_SEMICOLON) && (tok = chktp(f, ";"))) ||
	    ((flags & EF_FINISH_SQUARE_BRACKET) && (tok = chktp(f, "]"))) ||
	    ((flags & EF_FINISH_COMMA) && (tok = chktp(f, ",")))) {
		ungettok(tok, f);
		freetp(tok);
		return pack(*block, acc, flags);
	}
	
	if ((res = parselit(f, flags, block, initty, operators, acc), res.itm) ||
	    (res = parseid(f, flags, block, initty, operators, acc), res.itm) ||
	    (res = parsebop(f, flags, block, initty, operators, acc), res.itm) ||
	    (res = parseuop(f, flags, block, initty, operators, acc), res.itm) ||
	    (res = parseparents(f, flags, block, initty, operators, acc), res.itm) ||
	    (res = parsefcall(f, flags, block, initty, operators, acc), res.itm))
		return pack(*block, res, flags);
	return res;
}

static struct expr parsefcall(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	/* TODO: implement */
	struct expr res = { 0 };
	return res;
}

static struct expr parseid(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct token * id;
	struct symbol * sym;
	struct expr nil = { 0 };
	if (acc.itm)
		return nil;
	if (!(id = chkttp(f, T_IDENTIFIER)))
		return nil;

	sym = get_symbol(id->lexeme);
	freetp(id);

	acc.itm = sym->value;
	acc.islvalue = 1;

	return parseexpro(f, flags, block, initty, operators, acc);
}

static struct expr parsebop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct operator * op;
	struct token * tok;
	struct expr e, r;
	struct expr nil = { 0 };
	
	if (!acc.itm)
		return nil;
	
	if (!(tok = chkttp(f, T_OPERATOR)))
		return nil;
	
	op = getbop(tok->lexeme);
	if (!op) {
		ungettok(tok, f);
		freetp(tok);
		return nil;
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
	
	e = performasnop(f, op, flags, block, initty, operators, acc);
	if (e.itm)
		goto ret;
	e = performaop(f, op, flags, block, initty, operators, acc);
	if (e.itm)
		goto ret;
	
ret:
	list_pop_back(operators);

	freetp(tok);
	return parseexpro(f, flags, block, initty, operators, e);
	
opbreak:
	ungettok(tok, f);
	freetp(tok);
	return acc;
}

static struct expr performasnop(FILE * f, struct operator * op, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct expr nil = { 0 };
	struct expr r;
	if (op != &binop_assign)
		return nil;

	acc = pack(*block, acc, EF_EXPECT_LVALUE);

	r = parseexpro(f, (flags & EF_CLEAR_MASK) | EF_EXPECT_RVALUE,
		block, initty, operators, nil);
	r = cast(r, ((struct cpointer *)acc.itm->type)->pointsto, *block);

	itm_store(*block, r.itm, acc.itm);
	return r;
}

static struct expr performaop(FILE * f, struct operator * op, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct itm_instr * (*ifunc)(struct itm_block * b,
		struct itm_expr * l, struct itm_expr * r);
	struct ctype * lt;
	struct ctype * rt;
	struct ctype * et;
	struct expr nil = { 0 };
	struct expr r;

	acc = pack(*block, acc, EF_EXPECT_RVALUE);

	r = parseexpro(f, (flags & EF_CLEAR_MASK) | EF_EXPECT_RVALUE,
		block, initty, operators, nil);

	lt = acc.itm->type;
	rt = r.itm->type;

	if (op == &binop_plus) {
		if (hastc(lt, TC_ARITHMETIC))
			ifunc = &itm_add;
		else if (hastc(lt, TC_POINTER))
			/* TODO */ assert(0);
		else
			goto invalid;
	} else if (op == &binop_min) {
		if (hastc(lt, TC_ARITHMETIC))
			ifunc = &itm_sub;
		else if (hastc(lt, TC_POINTER))
			/* TODO */ assert(0);
		else
			goto invalid;
	} else if (op == &binop_div) {
		if (!hastc(lt, TC_ARITHMETIC))
			goto invalid;
		ifunc = &itm_div;
	} else if (op == &binop_mul) {
		if (!hastc(lt, TC_ARITHMETIC))
			goto invalid;
		ifunc = &itm_mul;
	} else if (op == &binop_mod) {
		if (!hastc(lt, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_rem;
	} else if (op == &binop_shl) {
		if (!hastc(lt, TC_INTEGRAL))
			goto invalid;
		if (hastc(lt, TC_SIGNED))
			ifunc = &itm_sal;
		else
			ifunc = &itm_shl;
	} else if (op == &binop_shr) {
		if (!hastc(lt, TC_INTEGRAL))
			goto invalid;
		if (hastc(lt, TC_SIGNED))
			ifunc = &itm_sar;
		else
			ifunc = &itm_shr;
	} else if (op == &binop_or) {
		if (!hastc(lt, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_or;
	} else if (op == &binop_and) {
		if (!hastc(lt, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_and;
	} else if (op == &binop_xor) {
		if (!hastc(lt, TC_INTEGRAL))
			goto invalid;
		ifunc = &itm_xor;
	} else if (op == &binop_eq)
		ifunc = &itm_cmpeq;
	else if (op == &binop_neq)
		ifunc = &itm_cmpneq;
	else if (op == &binop_gt)
		ifunc = &itm_cmpgt;
	else if (op == &binop_gte)
		ifunc = &itm_cmpgte;
	else if (op == &binop_lt)
		ifunc = &itm_cmplt;
	else if (op == &binop_lte)
		ifunc = &itm_cmplte;
	else
		return nil; /* this shouldn't happen. ever. */
	
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
	
	acc.itm = (struct itm_expr *)ifunc(*block, acc.itm, r.itm);
	acc.islvalue = 0;
	return acc;

invalid:
	report(E_ERROR | E_HIDE_TOKEN, NULL, "invalid type for left expression");
	return nil;
}

static struct expr parseuop(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	/* TODO: implement */
	struct expr nil = { 0 };
	return nil;
}

static struct expr parseparents(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct token close;
	struct list * noperators;
	struct expr nil = { 0 };
	if (acc.itm)
		return nil;
	
	if (!chkt(f, "("))
		return nil;
	
	/* TODO: check for type cast */
	
	noperators = new_list(NULL, 0);
	acc = parseexpro(f, EF_NORMAL | EF_FINISH_BRACKET,
		block, initty, noperators, nil);
	delete_list(noperators, NULL);
	
	/* remove closing ) from the stream */
	close = gettok(f);
	freetok(&close);
	
	return parseexpro(f, flags, block, initty, operators, acc);
}

static struct expr parselit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct expr res = { 0 };
	res = parseintlit(f, flags, block, initty, operators, acc);
	if (res.itm)
		return res;
	res = parsefloatlit(f, flags, block, initty, operators, acc);
	if (res.itm)
		return res;
	return res;
}

static struct expr parseintlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	unsigned long ul;	
	struct token tok = gettok(f);
	struct itm_literal * lit;
	struct ctype * itypes[5] = { 0 };
	struct ctype * type;
	struct expr nil = { 0 };
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
		return nil;
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

	acc.itm = (struct itm_expr *)lit;
	acc.islvalue = 0;
	return parseexpro(f, flags, block, initty, operators, acc);
}

static struct expr parsefloatlit(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty, struct list * operators,
	struct expr acc)
{
	struct token * tok;
	struct itm_literal * lit;
	struct expr nil = { 0 };
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
		return nil;
	
	freetp(tok);
	
	acc.itm = (struct itm_expr *)lit;
	acc.islvalue = 0;
	return parseexpro(f, flags, block, initty, operators, acc);
}

struct expr parseexpr(FILE * f, enum exprflags flags,
	struct itm_block ** block, struct ctype * initty)
{
	struct list * operators = new_list(NULL, 0);
	struct expr e = { 0 };

	assert(block != NULL);
	assert(*block != NULL);

	e = parseexpro(f, flags, block, initty, operators, e);
	delete_list(operators, NULL);
	return e;
}

struct expr cast(struct expr e, struct ctype * ty,
	struct itm_block * b)
{
	struct expr res = e;

	assert(e.itm != NULL);
	assert(ty != NULL);
	assert(e.itm->type != NULL);

	if (e.itm->type == ty)
		return e;

	if (ty == &cbool) {
		struct itm_literal * lit;
		if (hastc(e.itm->type, TC_INTEGRAL) || hastc(e.itm->type, TC_POINTER)) {
			lit = new_itm_literal(e.itm->type);
			lit->value.i = 0ul;
		} else if (e.itm->type == &cfloat) {
			lit = new_itm_literal(&cfloat);
			lit->value.f = 0.0f;
		} else if (e.itm->type == &cdouble) {
			lit = new_itm_literal(&cdouble);
			lit->value.d = 0.0;
		} else
			report(E_ERROR | E_HIDE_TOKEN, NULL, "cannot convert to boolean value");

		res.itm = (struct itm_expr *)itm_cmpneq(b, e.itm, (struct itm_expr *)lit);
	} else if (hastc(e.itm->type, TC_FLOATING) && hastc(ty, TC_INTEGRAL))
		res.itm = (struct itm_expr *)itm_ftoi(b, e.itm, ty);
	else if (hastc(e.itm->type, TC_INTEGRAL) && hastc(ty, TC_FLOATING))
		res.itm = (struct itm_expr *)itm_itof(b, e.itm, ty);
	else if (hastc(e.itm->type, TC_POINTER) && hastc(ty, TC_INTEGRAL)) {
		if (e.itm->type->size > ty->size)
			res.itm = (struct itm_expr *)itm_trunc(b, e.itm, ty);
		else if (e.itm->type->size < ty->size)
			res.itm = (struct itm_expr *)itm_zext(b, e.itm, ty);
		else
			res.itm = (struct itm_expr *)itm_bitcast(b, e.itm, ty);
	} else if (hastc(e.itm->type, TC_INTEGRAL) && hastc(ty, TC_INTEGRAL)) {
		if (e.itm->type->size > ty->size)
			res.itm = (struct itm_expr *)itm_trunc(b, e.itm, ty);
		else if (e.itm->type->size < ty->size) {
			if (hastc(e.itm->type, TC_UNSIGNED))
				res.itm = (struct itm_expr *)itm_zext(b, e.itm, ty);
			else
				res.itm = (struct itm_expr *)itm_sext(b, e.itm, ty);
		}
	}

	/* TODO: other casts */

	return res;
}
