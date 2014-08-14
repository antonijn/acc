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

#include <acc/intermediate.h>
#include <acc/error.h>

#ifndef NDEBUG
int itm_instr_number(struct itm_instr * i)
{
	if (!i->previous)
		return 0;
	if (i->base.type == &cvoid)
		return itm_instr_number(i->previous);
	return itm_instr_number(i->previous) + 1;
}

void itm_instr_to_string(FILE * f, struct itm_instr * i)
{
	void * it;
	int j;
	struct itm_expr * ex;

	fprintf(f, "\t");
	if (i->base.type != &cvoid)
		fprintf(f, "%%%d = ", itm_instr_number(i));
	i->base.type->to_string(f, i->base.type);
	fprintf(f, " %s", i->operation);
	if (list_length(i->operands) > 0 || i->typeoperand)
		fprintf(f, " ");

	it = list_iterator(i->operands);
	for (j = 0; iterator_next(&it, (void **)&ex); ++j) {
		ex->to_string(f, ex);
		if (j != list_length(i->operands) - 1 || i->typeoperand)
			fprintf(f, ", ");
	}

	if (i->typeoperand)
		i->typeoperand->to_string(f, i->typeoperand);
	
	fprintf(f, "\n");
	if (i->next)
		itm_instr_to_string(f, i->next);
}

static void itm_instr_expr_to_string(FILE * f, struct itm_expr * e)
{
	int inum = itm_instr_number((struct itm_instr *)e);
	char buf[32] = { 0 };
	sprintf(buf, "%%%d", inum);
	e->type->to_string(f, e->type);
	fprintf(f, " %s", buf);
}

static void itm_literal_to_string(FILE * f, struct itm_expr * e)
{
	struct itm_literal * li = (struct itm_literal *)e;
	e->type->to_string(f, e->type);
	fprintf(f, " %lu", li->value.i);
}

void itm_block_to_string(FILE * f, struct itm_block * block)
{
	/* TODO: implement and print number */
	itm_instr_to_string(f, block->first);
}

#endif

static void free_dummy(struct itm_expr * e)
{
}

static struct itm_instr * impl_op(struct itm_block * b, struct ctype * type, void (*id)(void),
	const char * operation, int isterminal)
{
	struct itm_instr * res = malloc(sizeof(struct itm_instr));
	
	res->base.etype = ITME_INSTRUCTION;
	res->base.type = type;
	res->base.free = &free_dummy;
#ifndef NDEBUG
	res->base.to_string = &itm_instr_expr_to_string;
#endif

	res->id = id;
	res->operation = operation;
	res->isterminal = 0;

	res->operands = new_list(NULL, 0);
	res->typeoperand = NULL;
	res->next = NULL;
	res->previous = b->last;
	if (b->last)
		b->last->next = res;
	if (!b->first)
		b->first = res;
	b->last = res;
	res->block = b;
	
	return res;
}

static struct itm_instr * impl_aop(struct itm_block * b, struct itm_expr * l, struct itm_expr * r,
	void (*id)(void), const char * operation)
{
	struct itm_instr * res = impl_op(b, l->type, id, operation, 0);
	
	list_push_back(res->operands, l);
	list_push_back(res->operands, r);

	return res;
}

static struct itm_instr * impl_cast(struct itm_block * b, struct itm_expr * l, struct ctype * type,
	void (*id)(void), const char * operation)
{
	struct itm_instr * res = impl_op(b, type, id, operation, 0);
	list_push_back(res->operands, l);
	res->typeoperand = type;
	return res;
}

struct itm_instr *itm_add(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_add, "add");
}

struct itm_instr *itm_sub(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_sub, "sub");
}

struct itm_instr *itm_mul(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_mul, "mul");
}

struct itm_instr *itm_imul(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_imul, "imul");
}

struct itm_instr *itm_div(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_div, "div");
}

struct itm_instr *itm_idiv(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_idiv, "idiv");
}

struct itm_instr *itm_rem(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_rem, "rem");
}

struct itm_instr *itm_shl(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_shl, "shl");
}

struct itm_instr *itm_shr(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_shr, "shr");
}

struct itm_instr *itm_sal(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_sal, "sal");
}

struct itm_instr *itm_sar(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_sar, "sar");
}

struct itm_instr *itm_or(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_or, "or");
}

struct itm_instr *itm_and(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_and, "and");
}

struct itm_instr *itm_xor(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_xor, "xor");
}


struct itm_instr *itm_bitcast(struct itm_block * b, struct itm_expr * l, struct ctype * to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_bitcast, "bitcast");
}

struct itm_instr *itm_trunc(struct itm_block * b, struct itm_expr * l, struct ctype * to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_trunc, "trunc");
}

struct itm_instr *itm_zext(struct itm_block * b, struct itm_expr * l, struct ctype * to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_zext, "zext");
}

struct itm_instr *itm_sext(struct itm_block * b, struct itm_expr * l, struct ctype * to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_sext, "sext");
}

struct itm_instr *itm_itof(struct itm_block * b, struct itm_expr * l, struct ctype * to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_itof, "itof");
}

struct itm_instr *itm_ftoi(struct itm_block * b, struct itm_expr * l, struct ctype * to)
{
	return impl_cast(b, l, to, (void (*)(void))&itm_ftoi, "ftoi");
}


struct itm_instr *itm_getptr(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	return impl_aop(b, l, r, (void (*)(void))&itm_getptr, "getptr");
}

struct itm_instr *itm_deepptr(struct itm_block * b, struct itm_expr * l, struct itm_expr * r)
{
	struct itm_instr * res;
	int j;
	struct ctype * deeptype = ((struct cpointer *)l->type)->pointsto;
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
	res = impl_op(b, deeptype, (void (*)(void))&itm_deepptr, "deepptr", 0);
	list_push_back(res->operands, l);
	list_push_back(res->operands, r);
	return res;
}

struct itm_instr *itm_alloca(struct itm_block * b, struct ctype * ty);
struct itm_instr *itm_load(struct itm_block * b, struct itm_expr * l);
struct itm_instr *itm_store(struct itm_block * b, struct itm_expr * l, struct itm_expr * r);

struct itm_instr *itm_jmp(struct itm_block * b, struct itm_block * to);
struct itm_instr *itm_split(struct itm_block * b, struct itm_expr * c, struct itm_block * t, struct itm_block * e);
struct itm_instr *itm_ret(struct itm_block * b, struct itm_expr * l);
struct itm_instr *itm_leave(struct itm_block * b);

struct itm_literal *new_itm_literal(struct ctype * ty)
{
	struct itm_literal * lit = malloc(sizeof(struct itm_literal));
	lit->base.etype = ITME_LITERAL;
	lit->base.type = ty;
	lit->base.free = (void (*)(struct itm_expr *))&free;
#ifndef NDEBUG
	lit->base.to_string = &itm_literal_to_string;
#endif
	return lit;
}

struct itm_block * new_itm_block(struct itm_block * previous)
{
	struct itm_block * res = malloc(sizeof(struct itm_block));
	res->previous = previous;
	res->next = new_list(NULL, 0);
	res->first = NULL;
	res->last = NULL;
	return res;
}

static void cleanup_instr(struct itm_instr * i)
{
	void * it;
	struct itm_expr * op;
	
	it = list_iterator(i->operands);
	while (iterator_next(&it, (void **)&op))
		op->free(op);
	
	if (i->next)
		cleanup_instr(i->next);
}

void delete_itm_block(struct itm_block * block)
{
	cleanup_instr(block->first);
	free(block);
}
