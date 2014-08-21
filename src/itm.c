/*
 * Utilities for intermediate representation
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
#include <assert.h>

#include <acc/itm.h>
#include <acc/error.h>

/* to_string functions */
#ifndef NDEBUG
static int itm_instr_number(struct itm_instr *i)
{
	assert(i != NULL);

	if (!i->previous)
		return i->block->number + 1;
	if (i->base.type == &cvoid)
		return itm_instr_number(i->previous);
	return itm_instr_number(i->previous) + 1;
}

static void itm_instr_to_string(FILE *f, struct itm_instr *i)
{
	void *it;
	int j;
	struct itm_expr *ex;
	struct itm_label *lbl;

	assert(i != NULL);

	fprintf(f, "\t");
	if (i->base.type != &cvoid) {
		fprintf(f, "%%%d = ", itm_instr_number(i));
		i->base.type->to_string(f, i->base.type);
		fprintf(f, " ");
	}
	fprintf(f, "%s ", i->operation);

	it = list_iterator(i->operands);
	for (j = 0; iterator_next(&it, (void **)&ex); ++j) {
		ex->to_string(f, ex);
		if (j != list_length(i->operands) - 1 ||
		   (i->labeloperands && list_length(i->labeloperands) > 0) ||
		    i->typeoperand)
			fprintf(f, ", ");
	}
	
	if (i->labeloperands) {
		it = list_iterator(i->labeloperands);
		for (j = 0; iterator_next(&it, (void **)&lbl); ++j) {
			fprintf(f, "block %%%d", lbl->block->number);
			if (j != list_length(i->labeloperands) - 1 || i->typeoperand)
				fprintf(f, ", ");
		}
	}

	if (i->typeoperand)
		i->typeoperand->to_string(f, i->typeoperand);
	
	fprintf(f, "\n");
	if (i->next)
		itm_instr_to_string(f, i->next);
}

static void itm_instr_expr_to_string(FILE *f, struct itm_expr *e)
{
	int inum = itm_instr_number((struct itm_instr *)e);
	char buf[32] = { 0 };

	assert(e != NULL);

	sprintf(buf, "%%%d", inum);
	e->type->to_string(f, e->type);
	fprintf(f, " %s", buf);
}

static void itm_literal_to_string(FILE *f, struct itm_expr *e)
{
	struct itm_literal *li = (struct itm_literal *)e;

	assert(li != NULL);

	e->type->to_string(f, e->type);
	if (e->type == &cdouble)
		fprintf(f, " %f", li->value.d);
	else if (e->type == &cfloat)
		fprintf(f, " %f", (double)li->value.f);
	else
		fprintf(f, " %lu", li->value.i);
}

void itm_block_to_string(FILE *f, struct itm_block *block)
{
	void *it;
	struct itm_block *nxt;
	
	fprintf(f, "\n%%%d:\n", block->number);
	if (block->first)
		itm_instr_to_string(f, block->first);
	
	if (block->lexnext)
		itm_block_to_string(f, block->lexnext);
}

#endif

/* label, literal and block initializers/destructors */
struct itm_label *new_itm_label(void)
{
	struct itm_label *lab = malloc(sizeof(struct itm_label));
	lab->block = NULL;
	return lab;
}

void set_itm_label_block(struct itm_label *l, struct itm_block *b)
{
	list_push_back(b->labels, l);
	l->block = b;
}

struct itm_literal *new_itm_literal(struct ctype *ty)
{
	struct itm_literal *lit = malloc(sizeof(struct itm_literal));
	assert(ty != NULL);
	lit->base.etype = ITME_LITERAL;
	lit->base.type = ty;
	lit->base.free = (void (*)(struct itm_expr *))&free;
#ifndef NDEBUG
	lit->base.to_string = &itm_literal_to_string;
#endif
	return lit;
}

struct itm_block *new_itm_block(struct itm_block *before, struct list *previous)
{
	struct itm_block *res = malloc(sizeof(struct itm_block));
	res->previous = previous;
	res->lexnext = NULL;
	res->lexprev = before;
	if (before) {
#ifndef NDEBUG
		res->number = itm_instr_number(before->last) + 1;
#endif
		before->lexnext = res;
	} else {
#ifndef NDEBUG
		res->number = 0;
#endif
	}
	
	if (previous) {
		struct itm_block *fprev;
		void *it;
		
		it = list_iterator(previous);
		while (iterator_next(&it, (void **)&fprev))
			list_push_back(fprev->next, res);
	}
	res->next = new_list(NULL, 0);
	res->first = NULL;
	res->last = NULL;
	res->labels = new_list(NULL, 0);
	return res;
}

static void cleanup_instr(struct itm_instr *i)
{
	void *it;
	struct itm_expr *op;
	
	it = list_iterator(i->operands);
	while (iterator_next(&it, (void **)&op))
		op->free(op);
	
	if (i->labeloperands)
		delete_list(i->labeloperands, NULL);
	
	if (i->next)
		cleanup_instr(i->next);
}

void delete_itm_block(struct itm_block *block)
{
	struct list *blocks = new_list(NULL, 0);
	if (block->lexnext)
		delete_itm_block(block->lexnext);

	if (block->first)
		cleanup_instr(block->first);
	delete_list(block->labels, &free);
	if (block->previous)
		delete_list(block->previous, NULL);
	free(block);
}

static void free_dummy(struct itm_expr *e)
{
}

/* instruction initializers and instructions */
enum opflags {
	OF_NONE = 0x0,
	OF_TERMINAL = 0x1,
	OF_START = 0x2 /* only really used for alloca */
};

static struct itm_instr *impl_op(struct itm_block *b, struct ctype *type, void (*id)(void),
	const char *operation, enum opflags opflags)
{
	struct itm_instr *res = malloc(sizeof(struct itm_instr));

	assert(type != NULL);

	res->base.etype = ITME_INSTRUCTION;
	res->base.type = type;
	res->base.free = &free_dummy;
#ifndef NDEBUG
	res->base.to_string = &itm_instr_expr_to_string;
#endif

	res->id = id;
	res->operation = operation;
	res->isterminal = opflags & OF_TERMINAL;

	res->operands = new_list(NULL, 0);
	res->typeoperand = NULL;
	res->labeloperands = NULL;
	res->next = NULL;
	if (opflags & OF_START) {
		struct itm_instr *before = NULL;
		struct itm_instr *after;
		while (b->lexprev)
			b = b->lexprev;
		
		/* find first non-id instruction */
		after = b->first;
		while (after && after->id == id) {
			before = after;
			after = after->next;
		}
		
		if (after)
			after->previous = res;
		else
			b->last = res;
		
		if (before)
			before->next = res;
		else
			b->first = res;
		
		res->previous = before;
		res->next = after;
	} else {
		res->previous = b->last;
		if (b->last)
			b->last->next = res;
		if (!b->first)
			b->first = res;
		b->last = res;
	}
	res->block = b;
	
	return res;
}

static struct itm_instr *impl_aop(struct itm_block *b, struct itm_expr *l, struct itm_expr *r,
	void (*id)(void), const char *operation)
{
	struct itm_instr *res = impl_op(b, l->type, id, operation, OF_NONE);
	
	list_push_back(res->operands, l);
	list_push_back(res->operands, r);

	return res;
}

static struct itm_instr *impl_cast(struct itm_block *b, struct itm_expr *l, struct ctype *type,
	void (*id)(void), const char *operation)
{
	struct itm_instr *res = impl_op(b, type, id, operation, OF_NONE);
	list_push_back(res->operands, l);
	res->typeoperand = type;
	return res;
}

struct itm_instr *itm_add(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_add, "add");
}

struct itm_instr *itm_sub(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_sub, "sub");
}

struct itm_instr *itm_mul(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_mul, "mul");
}

struct itm_instr *itm_imul(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_imul, "imul");
}

struct itm_instr *itm_div(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_div, "div");
}

struct itm_instr *itm_idiv(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_idiv, "idiv");
}

struct itm_instr *itm_rem(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_rem, "rem");
}

struct itm_instr *itm_shl(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_shl, "shl");
}

struct itm_instr *itm_shr(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_shr, "shr");
}

struct itm_instr *itm_sal(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_sal, "sal");
}

struct itm_instr *itm_sar(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_sar, "sar");
}

struct itm_instr *itm_or(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_or, "or");
}

struct itm_instr *itm_and(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_and, "and");
}

struct itm_instr *itm_xor(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_xor, "xor");
}


struct itm_instr *itm_cmpeq(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, (void (*)(void))&itm_cmpeq, "cmp eq");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmpneq(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, (void (*)(void))&itm_cmpneq, "cmp neq");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmpgt(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, (void (*)(void))&itm_cmpgt, "cmp gt");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmpgte(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, (void (*)(void))&itm_cmpgte, "cmp gte");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmplt(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, (void (*)(void))&itm_cmplt, "cmp lt");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmplte(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, (void (*)(void))&itm_cmplte, "cmp lte");
	i->base.type = &cbool;
	return i;
}


struct itm_instr *itm_bitcast(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_bitcast, "bitcast");
}

struct itm_instr *itm_trunc(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_trunc, "trunc");
}

struct itm_instr *itm_zext(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_zext, "zext");
}

struct itm_instr *itm_sext(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_sext, "sext");
}

struct itm_instr *itm_itof(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_itof, "itof");
}

struct itm_instr *itm_ftoi(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_ftoi, "ftoi");
}


struct itm_instr *itm_getptr(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_getptr, "getptr");
}

struct itm_instr *itm_deepptr(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr *res;
	int j;
	struct ctype *deeptype = ((struct cpointer *)l->type)->pointsto;
	switch (deeptype->type) {
	case POINTER:
		deeptype = ((struct cpointer *)deeptype)->pointsto;
		break;
	case STRUCTURE:
		j = ((struct itm_literal *)r)->value.i;
		deeptype = get_list_item(((struct cstruct *)deeptype)->fields, j);
		break;
	case ARRAY:
		deeptype = ((struct carray *)deeptype)->elementtype;
		break;
	default:
		report(E_INTERNAL, NULL, "ITM: trying to access member of non-composite time");
		break;
	}
	deeptype = new_pointer(deeptype);
	res = impl_op(b, deeptype, (void (*)(void))&itm_deepptr, "deepptr", OF_NONE);
	list_push_back(res->operands, l);
	list_push_back(res->operands, r);
	return res;
}

struct itm_instr *itm_alloca(struct itm_block *b, struct ctype *ty)
{
	struct itm_instr *res;
	res = impl_op(b, new_pointer(ty), (void (*)(void))&itm_alloca, "alloca", OF_START);
	res->typeoperand = ty;
	return res;
}

struct itm_instr *itm_load(struct itm_block *b, struct itm_expr *l)
{
	struct itm_instr *res;
	assert(l->type->type == POINTER);
	res = impl_op(b, ((struct cpointer *)l->type)->pointsto,
		(void (*)(void))&itm_load, "load", OF_NONE);

	list_push_back(res->operands, l);

	return res;
}

struct itm_instr *itm_store(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, (void (*)(void))&itm_store, "store", OF_NONE);

	list_push_back(res->operands, l);
	list_push_back(res->operands, r);

	return res;
}

struct itm_instr *itm_jmp(struct itm_block *b, struct itm_label *to)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, (void (*)(void))&itm_jmp, "jmp", OF_TERMINAL);

	res->labeloperands = new_list(NULL, 0);
	list_push_back(res->labeloperands, to);

	return res;
}

struct itm_instr *itm_split(struct itm_block *b, struct itm_expr *c, struct itm_label *t, struct itm_label *e)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, (void (*)(void))&itm_split, "split", OF_TERMINAL);

	list_push_back(res->operands, c);

	res->labeloperands = new_list(NULL, 0);
	list_push_back(res->labeloperands, t);
	list_push_back(res->labeloperands, e);

	return res;
}

struct itm_instr *itm_ret(struct itm_block *b, struct itm_expr *l)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, (void (*)(void))&itm_ret, "ret", OF_TERMINAL);
	list_push_back(res->operands, l);
	return res;
}

struct itm_instr *itm_leave(struct itm_block *b)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, (void (*)(void))&itm_ret, "leave", OF_TERMINAL);
	return res;
}
