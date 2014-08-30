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
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <acc/parsing/expr.h>
#include <acc/parsing/token.h>
#include <acc/list.h>
#include <acc/ext.h>
#include <acc/error.h>

static struct expr pack(struct itm_block *b, struct expr e,
	enum exprflags flags);

static struct expr parseexpro(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr parsefcall(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr parseid(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);

static struct expr parsebop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr doaop(struct operator *op, enum exprflags flags,
	struct itm_block **block, struct ctype *initty,
	struct expr l, struct expr r);
static struct expr performaop(FILE *f, struct operator *op, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr performasnop(FILE *f, struct operator *op, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);

static struct expr parseuop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr performpreuop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct operator *op, struct token *opt);
static struct expr performpostuop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc, struct operator *op, struct token *opt);

static struct expr parseparents(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr parselit(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr parseintlit(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);
static struct expr parsefloatlit(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc);

static struct expr pack(struct itm_block *b, struct expr e, enum exprflags flags)
{
	assert(e.itm != NULL);

	if ((flags & EF_EXPECT_RVALUE) && e.islvalue) {
		struct expr res;
		res.islvalue = false;
		res.itm = (struct itm_expr *)itm_load(b, e.itm);
		return res;
	}
	if ((flags & EF_EXPECT_LVALUE) && !e.islvalue)
		report(E_ERROR | E_HIDE_TOKEN, NULL, "expected lvalue");
	return e;
}

static struct expr parseexpro(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct expr res = { 0 };
	struct token tok;

	if (((flags & EF_FINISH_BRACKET) && chktp(f, ")", &tok)) ||
	    ((flags & EF_FINISH_SEMICOLON) && chktp(f, ";", &tok)) ||
	    ((flags & EF_FINISH_SQUARE_BRACKET) && chktp(f, "]", &tok)) ||
	    ((flags & EF_FINISH_COMMA) && chktp(f, ",", &tok))) {
		ungettok(&tok, f);
		freetok(&tok);
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

static struct expr parsefcall(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	// TODO: implement
	struct expr res = { 0 };
	return res;
}

static struct expr parseid(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct token id;
	struct symbol *sym;
	struct expr nil = { 0 };
	if (acc.itm)
		return nil;
	if (!chkttp(f, T_IDENTIFIER, &id))
		return nil;

	sym = get_symbol(id.lexeme);
	// TODO: C90 implicit function declarations
	if (!sym)
		report(E_PARSER, &id, "undeclared identifier");
	freetok(&id);

	acc.itm = sym->value;
	acc.islvalue = true;

	return parseexpro(f, flags, block, initty, operators, acc);
}

static struct expr parsebop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct expr nil = { 0 };
	
	if (!acc.itm)
		return nil;
	
	struct token tok;
	if (!chkttp(f, T_OPERATOR, &tok))
		return nil;
	
	struct operator *op = getbop(tok.lexeme);
	if (!op) {
		ungettok(&tok, f);
		freetok(&tok);
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
	
	struct expr e = performasnop(f, op, flags, block, initty, operators, acc);
	if (e.itm)
		goto ret;
	e = performaop(f, op, flags, block, initty, operators, acc);
	if (e.itm)
		goto ret;
	
ret:
	list_pop_back(operators);

	freetok(&tok);
	return parseexpro(f, flags, block, initty, operators, e);
	
opbreak:
	ungettok(&tok, f);
	freetok(&tok);
	return acc;
}

static struct expr performasnop(FILE *f, struct operator *op, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	// TODO: compound assignment operators
	struct expr nil = { 0 };
	if (op != &binop_assign)
		return nil;

	acc = pack(*block, acc, EF_EXPECT_LVALUE);

	struct expr r = parseexpro(f, (flags & EF_CLEAR_MASK) | EF_EXPECT_RVALUE,
		block, initty, operators, nil);
	r = cast(r, ((struct cpointer *)acc.itm->type)->pointsto, *block);

	itm_store(*block, r.itm, acc.itm);
	return r;
}

static struct expr doaop(struct operator *op, enum exprflags flags,
	struct itm_block **block, struct ctype *initty,
	struct expr l, struct expr r)
{
	assert(l.islvalue == 0);
	assert(r.islvalue == 0);

	struct expr nil = { 0 };

	struct ctype *lt = l.itm->type;
	struct ctype *rt = r.itm->type;

	struct itm_instr *(*ifunc)(struct itm_block *b,
		struct itm_expr *l, struct itm_expr *r);

	if (op == &binop_plus) {
		if (hastc(lt, TC_ARITHMETIC))
			ifunc = &itm_add;
		else if (hastc(lt, TC_POINTER))
			/* TODO */ assert(false);
		else
			goto invalid;
	} else if (op == &binop_min) {
		if (hastc(lt, TC_ARITHMETIC))
			ifunc = &itm_sub;
		else if (hastc(lt, TC_POINTER))
			/* TODO */ assert(false);
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
		assert(false); // operator not yet implemented

	struct ctype *et = NULL;
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

	assert(et != NULL);

	l = cast(l, et, *block);
	r = cast(r, et, *block);

	struct expr res;
	res.itm = (struct itm_expr *)ifunc(*block, l.itm, r.itm);
	res.islvalue = false;
	return res;

invalid:
	report(E_ERROR | E_HIDE_TOKEN, NULL, "invalid type for left expression");
	return nil;
}

static struct expr performaop(FILE *f, struct operator *op, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct expr nil = { 0 };

	acc = pack(*block, acc, EF_EXPECT_RVALUE);

	struct expr r = parseexpro(f, (flags & EF_CLEAR_MASK) | EF_EXPECT_RVALUE,
		block, initty, operators, nil);

	return doaop(op, flags, block, initty, acc, r);
}

static struct expr parseuop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct expr nil = { 0 };
	struct operator *op;
	struct token opt;

	if (acc.itm) {
		if (chktp(f, "++", &opt))
			op = &unop_postinc;
		else if (chktp(f, "--", &opt))
			op = &unop_postdec;
		else
			return nil;
	} else {
		if (!chkttp(f, T_OPERATOR, &opt))
			return nil;
		op = getuop(opt.lexeme);
		if (!op)
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

	struct expr res;

	if (op == &unop_postdec || op == &unop_postinc) {
		res = performpostuop(f, flags, block, initty, operators, acc,
			op, &opt);
	} else {
		list_push_back(operators, op);
		res = performpreuop(f, flags, block, initty, operators, op, &opt);
		list_pop_back(operators);
	}

	freetok(&opt);
	return parseexpro(f, flags, block, initty, operators, res);

opbreak:
	ungettok(&opt, f);
	freetok(&opt);
	return acc;
}

static struct expr performpreuop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct operator *op, struct token *opt)
{
	struct expr nil = { 0 };

	// first we deal with the special cases: ++, --, * and &

	if (op == &unop_preinc || op == &unop_predec) {

		op = (op == &unop_preinc) ? &binop_plus : &binop_min;

		struct expr r = parseexpro(f, flags & EF_CLEAR_MASK,
			block, initty, operators, nil);
		if (!r.islvalue)
			report(E_PARSER, opt, "right-hand side of operator must be an lvalue");

		struct expr rvalue = pack(*block, r, EF_EXPECT_RVALUE);

		struct itm_literal *one = new_itm_literal(rvalue.itm->type);
		one->value.i = 1;

		struct expr addition;
		addition.itm = (struct itm_expr *)one;
		addition.islvalue = false;

		struct expr newval = doaop(op, flags, block, initty,
			rvalue, addition);
		itm_store(*block, newval.itm, r.itm);

		return newval;
	}

	if (op == &unop_deref) {
		struct expr r = parseexpro(f, flags & EF_CLEAR_MASK,
			block, initty, operators, nil);
		r = pack(*block, r, EF_EXPECT_RVALUE);
		if (!hastc(r.itm->type, TC_POINTER))
			report(E_PARSER, opt, "right-hand side of operator must be a pointer");
		r.islvalue = true;
		return r;
	}

	if (op == &unop_ref) {
		struct expr r = parseexpro(f, flags & EF_CLEAR_MASK,
			block, initty, operators, nil);
		if (!r.islvalue)
			report(E_PARSER, opt, "right-hand side of operator must be an lvalue");
		r.islvalue = false;
		return r;
	}

	// now we handle the other unary operators
	// the all handle rvalues

	struct expr right = parseexpro(f, flags & EF_CLEAR_MASK,
		block, initty, operators, nil);
	right = pack(*block, right, EF_EXPECT_RVALUE);

	struct expr res;
	res.islvalue = false;

	if (op == &unop_plus) {
		// TODO: promotion for bitfields and enums
		if (!hastc(right.itm->type, TC_ARITHMETIC))
			report(E_PARSER, opt, "right-hand side of operator must have arithmetic type");
		if (hastc(right.itm->type, TC_INTEGRAL) &&
		    right.itm->type->size < cint.size)
			right = cast(right, &cint, *block);
		res = right;

	} else if (op == &unop_min) {
		struct itm_literal *zero = new_itm_literal(right.itm->type);
		if (!hastc(right.itm->type, TC_ARITHMETIC))
			report(E_PARSER, opt, "right-hand side of operator must have arithmetic type");
		if (right.itm->type == &cfloat)
			zero->value.f = 0.0f;
		else if (right.itm->type == &cdouble)
			zero->value.d = 0.0;
		else
			zero->value.i = 0ul;

		struct expr zeroe;
		zeroe.itm = (struct itm_expr *)zero;
		zeroe.islvalue = false;
		res = doaop(&binop_min, flags, block, initty, zeroe, right);

	} else if (op == &unop_not) {
		struct itm_literal *allbits = new_itm_literal(right.itm->type);
		if (!hastc(right.itm->type, TC_INTEGRAL))
			report(E_PARSER, opt, "right-hand side of operator must have integral type");
		allbits->value.i = 0;
		for (int i = 0; i < right.itm->type->size; ++i) {
			allbits->value.i <<= 8;
			allbits->value.i |= 0xff;
		}
		struct expr allbitse;
		allbitse.itm = (struct itm_expr *)allbits;
		allbitse.islvalue = false;
		res = doaop(&binop_xor, flags, block, initty, right, allbitse);
	} else {
		// TODO: sizeof
		assert(false);
	}

	return res;
}

static struct expr performpostuop(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc, struct operator *op, struct token *opt)
{
	if (!acc.islvalue)
		report(E_PARSER, opt, "left-hand side of operator must be an lvalue");

	op = (op == &unop_postinc) ? &binop_plus : &binop_min;

	// old (r)value of acc
	struct expr left = pack(*block, acc, EF_EXPECT_RVALUE);

	struct itm_literal *one = new_itm_literal(left.itm->type);
	one->value.i = 1;

	struct expr right;
	right.itm = (struct itm_expr *)one;
	right.islvalue = false;

	struct expr newval = doaop(op, flags, block, initty, left, right);
	itm_store(*block, newval.itm, acc.itm);

	return left;
}

static struct expr parseparents(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct expr nil = { 0 };
	if (acc.itm)
		return nil;
	
	if (!chkt(f, "("))
		return nil;
	
	/* TODO: check for type cast */
	
	struct list *noperators = new_list(NULL, 0);
	acc = parseexpro(f, EF_NORMAL | EF_FINISH_BRACKET,
		block, initty, noperators, nil);
	delete_list(noperators, NULL);
	
	/* remove closing ) from the stream */
	struct token close = gettok(f);
	freetok(&close);
	
	return parseexpro(f, flags, block, initty, operators, acc);
}

static struct expr parselit(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct expr res = parseintlit(f, flags, block, initty, operators, acc);
	if (res.itm)
		return res;
	res = parsefloatlit(f, flags, block, initty, operators, acc);
	if (res.itm)
		return res;
	return res;
}

static struct expr parseintlit(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	uint64_t ul;
	struct token tok = gettok(f);
	struct ctype *itypes[5] = { 0 };
	struct expr nil = { 0 };
	
	switch (tok.type) {
	case T_DEC:
		sscanf(tok.lexeme, "%" SCNu64, &ul);
		itypes[0] = &cint;
		itypes[1] = &clong;
		break;
	case T_OCT:
		sscanf(tok.lexeme, "0%" SCNo64, &ul);
		itypes[0] = &cint;
		itypes[1] = &cuint;
		itypes[2] = &clong;
		itypes[3] = &culong;
		break;
	case T_HEX:
		if (!sscanf(tok.lexeme, "0x%" SCNx64, &ul))
			sscanf(tok.lexeme, "0X%" SCNx64, &ul);
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

	struct ctype *type;
	for (int i = 0; itypes[i]; ++i) {
		uint64_t mask;
		type = itypes[i];
		if (type == &cuint ||
		    type == &culong) {
			mask = (1ul << (type->size * 8 - 1)) - 1;
			mask <<= 1;
			mask |= 1;
		} else {
			mask = (1ul << (type->size * 8 - 2)) - 1;
			mask <<= 1;
			mask |= 1;
		}
		if ((ul & ~mask) == 0)
			break;
	}

	struct itm_literal *lit = new_itm_literal(type);
	lit->value.i = ul;

	acc.itm = (struct itm_expr *)lit;
	acc.islvalue = false;
	return parseexpro(f, flags, block, initty, operators, acc);
}

static struct expr parsefloatlit(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty, struct list *operators,
	struct expr acc)
{
	struct expr nil = { 0 };
	struct token tok;
	struct itm_literal *lit;
	if (chkttp(f, T_FLOAT, &tok)) {
		float f;
		sscanf(tok.lexeme, "%ff", &f);
		lit = new_itm_literal(&cfloat);
		lit->value.f = f;
	} else if (chkttp(f, T_DOUBLE, &tok)) {
		double d;
		sscanf(tok.lexeme, "%lf", &d);
		lit = new_itm_literal(&cdouble);
		lit->value.d = d;
	} else {
		return nil;
	}
	
	freetok(&tok);
	
	acc.itm = (struct itm_expr *)lit;
	acc.islvalue = false;
	return parseexpro(f, flags, block, initty, operators, acc);
}

struct expr parseexpr(FILE *f, enum exprflags flags,
	struct itm_block **block, struct ctype *initty)
{
	assert(block != NULL);
	assert(*block != NULL);

	struct list *operators = new_list(NULL, 0);
	struct expr e = { 0 };

	e = parseexpro(f, flags, block, initty, operators, e);
	delete_list(operators, NULL);
	return e;
}

struct expr cast(struct expr e, struct ctype *ty,
	struct itm_block *b)
{
	assert(e.itm != NULL);
	assert(ty != NULL);
	assert(e.itm->type != NULL);

	enum typecomp tc = e.itm->type->compare(e.itm->type, ty);
	enum typeclass tcl = gettc(e.itm->type), tcr = gettc(ty);

	if (tc == TC_EXPLICIT) {
		report(E_PARSER | E_HIDE_TOKEN, NULL, "no implicit conversion");
	} else if (tc == TC_INCOMPATIBLE) {
		report(E_PARSER | E_HIDE_TOKEN, NULL, "no conversion exists");
		return e;
	}

	struct expr res = e;

	if (e.itm->type == ty)
		return e;

	if (ty == &cbool) {
		struct itm_literal *lit;
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
	} else if ((tcl & TC_FLOATING) && (tcr & TC_INTEGRAL)) {
		res.itm = (struct itm_expr *)itm_ftoi(b, e.itm, ty);
	} else if ((tcl & TC_INTEGRAL) && (tcr & TC_FLOATING)) {
		res.itm = (struct itm_expr *)itm_itof(b, e.itm, ty);
	} else if ((tcl & TC_FLOATING) && (tcr & TC_FLOATING)) {
		if (e.itm->type->size > ty->size)
			res.itm = (struct itm_expr *)itm_ftrunc(b, e.itm, ty);
		else
			res.itm = (struct itm_expr *)itm_fext(b, e.itm, ty);
	} else if (((tcl & TC_POINTER) && (tcr & TC_INTEGRAL)) ||
	           (tcl & TC_INTEGRAL) && (tcr & TC_POINTER)) {
		if (e.itm->type->size > ty->size)
			res.itm = (struct itm_expr *)itm_trunc(b, e.itm, ty);
		else if (e.itm->type->size < ty->size)
			res.itm = (struct itm_expr *)itm_zext(b, e.itm, ty);
		else
			res.itm = (struct itm_expr *)itm_bitcast(b, e.itm, ty);
	} else if ((tcl & TC_INTEGRAL) && (tcr & TC_INTEGRAL)) {
		if (e.itm->type->size > ty->size)
			res.itm = (struct itm_expr *)itm_trunc(b, e.itm, ty);
		else if (e.itm->type->size < ty->size) {
			if (hastc(e.itm->type, TC_UNSIGNED))
				res.itm = (struct itm_expr *)itm_zext(b, e.itm, ty);
			else
				res.itm = (struct itm_expr *)itm_sext(b, e.itm, ty);
		}
	} else {
		// TODO: other casts
		assert(false);
	}


	return res;
}
