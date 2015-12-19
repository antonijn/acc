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
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <acc/itm/ast.h>
#include <acc/error.h>
#include <acc/term.h>

static void free_dummy(struct itm_expr *e);

// to_string functions
int itm_instr_number(struct itm_instr *i)
{
	assert(i != NULL);

	if (!i->previous)
		return itm_block_number(i->block) + (i->base.type == &cvoid ? 0 : 1);
	if (i->base.type == &cvoid)
		return itm_instr_number(i->previous);
	return itm_instr_number(i->previous) + 1;
}

int itm_block_number(struct itm_block *block)
{
	if (!block->lexprev)
		return 0;
	return itm_instr_number(block->lexprev->last) + 1;
}

static void print_tags(FILE *f, struct itm_expr *expr)
{
	if (!expr->tags)
		return;

	struct itm_tag *tag;
	it_t it = list_iterator(expr->tags);
	while (iterator_next(&it, (void **)&tag)) {
		fprintf(f, ANSI_GREEN(ITM_COLORS));
		fprintf(f, " /* ");
		itm_tag_to_string(f, tag);
		fprintf(f, " */");
		fprintf(f, ANSI_RESET(ITM_COLORS));
	}
}

static void itm_instr_to_string(FILE *f, struct itm_instr *i)
{

	assert(i != NULL);

	fprintf(f, "\t");
	if (i->base.type != &cvoid) {
		i->base.type->to_string(f, i->base.type);
		fprintf(f, " t%d = ", itm_instr_number(i));
	}
	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, ANSI_BLUE(ITM_COLORS));
	fprintf(f, "%s", i->operation);
	fprintf(f, ANSI_RESET(ITM_COLORS));
	fprintf(f, "(");

	struct itm_expr *ex;
	it_t it = list_iterator(i->operands);
	for (int j = 0; iterator_next(&it, (void **)&ex); ++j) {
		ex->to_string(f, ex);
		if (j != list_length(i->operands) - 1 ||
		    i->typeoperand)
			fprintf(f, ", ");
	}

	if (i->typeoperand)
		i->typeoperand->to_string(f, i->typeoperand);

	fprintf(f, ");");

	if (i->base.tags)
		print_tags(f, &i->base);

	fprintf(f, "\n");
	if (i->next)
		itm_instr_to_string(f, i->next);
}

static void itm_instr_expr_to_string(FILE *f, struct itm_expr *e)
{
	assert(e != NULL);

	int inum = itm_instr_number((struct itm_instr *)e);
	char buf[32] = { 0 };

	/*e->type->to_string(f, e->type);*/
	fprintf(f, "t%d", inum);
}

static void itm_literal_to_string(FILE *f, struct itm_expr *e)
{
	struct itm_literal *li = (struct itm_literal *)e;

	assert(li != NULL);

	fprintf(f, "(");
	e->type->to_string(f, e->type);
	fprintf(f, ")");
	fprintf(f, ANSI_MAGENTA(ITM_COLORS));
	if (e->type == &cdouble)
		fprintf(f, "%f", li->value.d);
	else if (e->type == &cfloat)
		fprintf(f, "%f", (double)li->value.f);
	else
		fprintf(f, "%lu", li->value.i);
	fprintf(f, ANSI_RESET(ITM_COLORS));
}

static void itm_undef_to_string(FILE *f, struct itm_expr *e)
{
	assert(e != NULL);

	fprintf(f, "(");
	e->type->to_string(f, e->type);
	fprintf(f, ")");
	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, ANSI_BLUE(ITM_COLORS));
	fprintf(f, "undef");
	fprintf(f, ANSI_RESET(ITM_COLORS));
}

static void itm_blocke_to_string(FILE *f, struct itm_expr *e)
{
	assert(e != NULL);

	struct itm_block *b = (struct itm_block *)e;
	fprintf(f, "L%d", itm_block_number(b));
}

static void itm_block_to_string(FILE *f, struct itm_block *block)
{
	fprintf(f, "\nL%d:", itm_block_number(block));
	print_tags(f, &block->base);
	fprintf(f, "\n");

	if (block->first)
		itm_instr_to_string(f, block->first);

	if (block->lexnext)
		itm_block_to_string(f, block->lexnext);
}

void itm_containere_to_string(FILE *f, struct itm_expr *e)
{
	assert(e != NULL);

	struct itm_container *b = (struct itm_container *)e;
	e->to_string(f, e);
	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, " @%s", b->id);
	fprintf(f, ANSI_RESET(ITM_COLORS));
}

void itm_container_to_string(FILE *f, struct itm_container *c)
{
	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, ANSI_BLUE(ITM_COLORS));
	switch (c->linkage) {
	case IL_GLOBAL:
		fprintf(f, "global ");
		break;
	case IL_STATIC:
		fprintf(f, "static ");
		break;
	case IL_EXTERN:
		fprintf(f, "extern ");
		break;
	}
	fprintf(f, ANSI_RESET(ITM_COLORS));

	c->base.type->to_string(f, c->base.type);

	fprintf(f, ANSI_BOLD(ITM_COLORS));
	fprintf(f, " @%s", c->id);
	fprintf(f, ANSI_RESET(ITM_COLORS));
	if (c->block) {
		fprintf(f, " {");
		itm_block_to_string(f, c->block);
		fprintf(f, "}\n");
	}
	fprintf(f, "\n");
}


struct itm_container *new_itm_container(enum itm_linkage linkage, char *id,
	struct ctype *ty)
{
	assert(id != NULL);
	assert(ty != NULL);

	struct itm_container *c = malloc(sizeof(struct itm_container));
	c->base.tags = NULL;
	c->base.etype = ITME_CONTAINER;
	c->base.type = new_pointer(ty);
	c->base.free = (void (*)(struct itm_expr *))&delete_itm_container;
	c->base.to_string = &itm_containere_to_string;
	c->block = NULL;
	c->id = malloc((strlen(id) + 1) * sizeof(char));
	sprintf(c->id, "%s", id);
	c->linkage = linkage;
	c->literals = new_list(NULL, 0);
	return c;
}

void delete_itm_container(struct itm_container *c)
{
	free(c->id);

	struct itm_expr *lit;
	it_t it = list_iterator(c->literals);
	while (iterator_next(&it, (void **)&lit))
		lit->free(lit);
	delete_list(c->literals, NULL);

	if (c->block)
		delete_itm_block(c->block);
	free(c);
}

// literal and block initializers/destructors
struct itm_literal *new_itm_literal(struct itm_container *c, struct ctype *ty)
{
	assert(ty != NULL);
	struct itm_literal *lit = malloc(sizeof(struct itm_literal));
	lit->base.tags = NULL;
	lit->base.etype = ITME_LITERAL;
	lit->base.type = ty;
	lit->base.free = (void (*)(struct itm_expr *))&free;
	lit->base.to_string = &itm_literal_to_string;
	list_push_back(c->literals, lit);
	return lit;
}

struct itm_expr *new_itm_undef(struct itm_container *c, struct ctype *ty)
{
	assert(ty != NULL);
	struct itm_expr *lit = malloc(sizeof(struct itm_expr));
	lit->tags = NULL;
	lit->etype = ITME_UNDEF;
	lit->type = ty;
	lit->free = (void (*)(struct itm_expr *))&free;
	lit->to_string = &itm_undef_to_string;
	list_push_back(c->literals, lit);
	return lit;
}

bool itm_hasvalue(struct itm_expr *e, int val)
{
	if (e->etype == ITME_UNDEF)
		return true;
	if (e->etype != ITME_LITERAL)
		return false;

	struct itm_literal *lit = (struct itm_literal *)e;
	return (int)lit->value.i == val;
}

struct itm_block *new_itm_block(struct itm_container *container)
{
	struct itm_block *res = malloc(sizeof(struct itm_block));
	res->base.type = NULL;
	res->base.etype = ITME_BLOCK;
	res->base.tags = NULL;
	res->base.free = (void (*)(struct itm_expr *))&delete_itm_block;
	res->base.to_string = &itm_blocke_to_string;
	res->previous = new_list(NULL, 0);
	res->next = new_list(NULL, 0);
	res->lexnext = NULL;
	res->lexprev = NULL;
	res->first = NULL;
	res->last = NULL;
	res->container = container;
	return res;
}

void itm_progress(struct itm_block *before, struct itm_block *after)
{
	assert(before != NULL);
	assert(after != NULL);
	list_push_back(before->next, after);
	list_push_back(after->previous, before);
}

void itm_lex_progress(struct itm_block *before, struct itm_block *after)
{
	assert(before != NULL);
	assert(after != NULL);
	before->lexnext = after;
	after->lexprev = before;
}

void cleanup_instr(struct itm_instr *i)
{
	/*struct itm_expr *op;
	it_t it = list_iterator(i->operands);
	while (iterator_next(&it, (void **)&op))
		op->free(op);*/
	delete_list(i->operands, NULL);

	if (i->next)
		cleanup_instr(i->next);
	free(i);
}

void delete_itm_block(struct itm_block *block)
{
	if (block->lexnext)
		delete_itm_block(block->lexnext);

	if (block->first)
		cleanup_instr(block->first);
	delete_list(block->previous, NULL);
	delete_list(block->next, NULL);
	free(block);
}

void itm_tag_expr(struct itm_expr *e, struct itm_tag *tag)
{
	assert(tag != NULL);

	if (!e->tags)
		e->tags = new_list(NULL, 0);
	list_push_back(e->tags, tag);
}

void itm_untag_expr(struct itm_expr *e, const char *const *ty)
{
	struct itm_tag *tag;
	for (it_t it = list_iterator(e->tags); iterator_next(&it, (void **)&tag);) {
		if (itm_tag_type(tag) == ty) {
			list_remove(e->tags, tag);
			delete_itm_tag(tag);
			return;
		}
	}
}

struct itm_tag *itm_get_tag(struct itm_expr *e, const char *const *ty)
{
	if (!e->tags)
		return NULL;

	struct itm_tag *tag;
	it_t it = list_iterator(e->tags);
	while (iterator_next(&it, (void **)&tag))
		if (itm_tag_type(tag) == ty)
			return tag;

	return NULL;
}

static void free_dummy(struct itm_expr *e)
{
}

void itm_remi(struct itm_instr *a)
{
	if (a->previous)
		a->previous->next = a->next;
	if (a->next)
		a->next->previous = a->previous;
	if (a->block->first == a)
		a->block->first = a->next;
	if (a->block->last == a)
		a->block->last = a->previous;

	a->previous = NULL;
	a->next = NULL;
	cleanup_instr(a);
}

void itm_replocc(struct itm_expr *a, struct itm_expr *b, struct itm_block *bl)
{
	struct itm_block *first = bl;
	while (first->lexprev)
		first = first->lexprev;

	struct itm_instr *instr = first->first;
	while (instr) {
		struct itm_expr *e;
		it_t it = list_iterator(instr->operands);
		int i = 0;
		while (iterator_next(&it, (void **)&e)) {
			if (e == a)
				set_list_item(instr->operands, i, b);
			++i;
		}

		if (!instr->next) {
			if (instr->block->lexnext)
				instr = instr->block->lexnext->first;
			else
				break;
		} else {
			instr = instr->next;
		}
	}
}

void itm_repli(struct itm_instr *a, struct itm_expr *b)
{
	itm_replocc(&a->base, b, a->block);
	itm_remi(a);
}

void itm_inserti(struct itm_instr *a, struct itm_instr *before)
{
	if (a->previous)
		a->previous->next = a->next;
	if (a->next)
		a->next->previous = a->previous;
	if (a->block->last == a)
		a->block->last = a->previous;

	if (before->previous)
		before->previous->next = a;
	else
		a->block->first = a;

	a->previous = before->previous;
	a->next = before;
	before->previous = a;
}


bool itm_isconst(struct itm_expr *e)
{
	if (e->etype != ITME_INSTRUCTION)
		return true;

	struct itm_instr *i = (struct itm_instr *)e;
	if (i->id != ITM_ID(itm_add) && i->id != ITM_ID(itm_sub) &&
	    i->id != ITM_ID(itm_mul) && i->id != ITM_ID(itm_div) &&
	    i->id != ITM_ID(itm_shl) && i->id != ITM_ID(itm_shr) &&
	    i->id != ITM_ID(itm_sal) && i->id != ITM_ID(itm_sar) &&
	    i->id != ITM_ID(itm_cmpeq) && i->id != ITM_ID(itm_cmpneq) &&
	    i->id != ITM_ID(itm_cmpgt) && i->id != ITM_ID(itm_cmpgte) &&
	    i->id != ITM_ID(itm_cmplt) && i->id != ITM_ID(itm_cmplte))
		return false;

	return itm_isconst(list_head(i->operands)) &&
	       itm_isconst(list_last(i->operands));
}

struct itm_expr *itm_eval(struct itm_expr *e)
{
	assert(itm_isconst(e));

	if (e->etype != ITME_INSTRUCTION)
		return e;

	struct itm_instr *i = (struct itm_instr *)e;

	struct itm_expr *first = list_head(i->operands);
	struct itm_expr *second = list_last(i->operands);

	if (first->etype == ITME_UNDEF)
		return first;
	else if (second->etype == ITME_UNDEF)
		return second;

	struct itm_literal *l = (struct itm_literal *)itm_eval(first);
	struct itm_literal *r = (struct itm_literal *)itm_eval(second);

	uint64_t li = l->value.i;
	uint64_t ri = r->value.i;
	uint64_t resi;

	// TODO: floating point emulation
	if (i->id == ITM_ID(itm_add))
		resi = li + ri;
	else if (i->id == ITM_ID(itm_sub))
		resi = li - ri;
	else if (i->id == ITM_ID(itm_mul))
		resi = li * ri;
	else if (i->id == ITM_ID(itm_div))
		resi = li / ri;
	else if (i->id == ITM_ID(itm_sub))
		resi = li - ri;
	else if (i->id == ITM_ID(itm_shl))
		resi = li << ri;
	else if (i->id == ITM_ID(itm_shr))
		resi = li >> ri;
	else if (i->id == ITM_ID(itm_sal))
		resi = (int64_t)li / (1l >> ri);
	else if (i->id == ITM_ID(itm_sar))
		resi = (int64_t)li * (1l >> ri);
	else if (i->id == ITM_ID(itm_cmpeq))
		resi = li == ri;
	else if (i->id == ITM_ID(itm_cmpneq))
		resi = li != ri;
	else if (i->id == ITM_ID(itm_cmpgt))
		resi = li > ri;
	else if (i->id == ITM_ID(itm_cmpgte))
		resi = li >= ri;
	else if (i->id == ITM_ID(itm_cmplt))
		resi = li < ri;
	else if (i->id == ITM_ID(itm_cmplte))
		resi = li <= ri;
	else
		return &i->base;

	uint64_t mask = 1ul << (i->base.type->size - 1);
	mask |= mask - 1;
	resi &= mask;

	struct itm_literal *res = new_itm_literal(i->block->container, i->base.type);
	res->value.i = resi;
	return &res->base;
}


// instruction initializers and instructions
enum opflags {
	OF_NONE = 0x0,
	OF_TERMINAL = 0x1,
	OF_START = 0x2, // only really used for alloca
	OF_START_OF_BLOCK = 0x4 // only really used for phi
};

static struct itm_instr *impl_op(struct itm_block *b, struct ctype *type, void (*id)(void),
	const char *operation, enum opflags opflags)
{
	struct itm_instr *res = malloc(sizeof(struct itm_instr));

	assert(type != NULL);
	assert(b != NULL);

	res->base.tags = NULL;
	res->base.etype = ITME_INSTRUCTION;
	res->base.type = type;
	res->base.free = &free_dummy;
	res->base.to_string = &itm_instr_expr_to_string;

	res->id = id;
	res->operation = operation;
	res->isterminal = opflags & OF_TERMINAL;

	res->operands = new_list(NULL, 0);
	res->typeoperand = NULL;
	res->next = NULL;
	if ((opflags & OF_START) || (opflags & OF_START_OF_BLOCK)) {
		struct itm_instr *before = NULL;
		struct itm_instr *after;
		if (opflags & OF_START)
			b = b->container->block;

		// find first non-id instruction
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
	return impl_aop(b, l, r, ITM_ID(itm_add), "add");
}

struct itm_instr *itm_sub(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_sub), "sub");
}

struct itm_instr *itm_mul(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_mul), "mul");
}

struct itm_instr *itm_imul(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_imul), "imul");
}

struct itm_instr *itm_div(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_div), "div");
}

struct itm_instr *itm_idiv(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_idiv), "idiv");
}

struct itm_instr *itm_rem(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_rem), "rem");
}

struct itm_instr *itm_shl(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_shl), "shl");
}

struct itm_instr *itm_shr(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_shr), "shr");
}

struct itm_instr *itm_sal(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_sal), "sal");
}

struct itm_instr *itm_sar(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_sar), "sar");
}

struct itm_instr *itm_or(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_or), "or");
}

struct itm_instr *itm_and(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_and), "and");
}

struct itm_instr *itm_xor(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_xor), "xor");
}


struct itm_instr *itm_cmpeq(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, ITM_ID(itm_cmpeq), "cmp eq");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmpneq(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, ITM_ID(itm_cmpneq), "cmp neq");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmpgt(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, ITM_ID(itm_cmpgt), "cmp gt");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmpgte(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, ITM_ID(itm_cmpgte), "cmp gte");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmplt(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, ITM_ID(itm_cmplt), "cmp lt");
	i->base.type = &cbool;
	return i;
}

struct itm_instr *itm_cmplte(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr * i = impl_aop(b, l, r, ITM_ID(itm_cmplte), "cmp lte");
	i->base.type = &cbool;
	return i;
}


struct itm_instr *itm_bitcast(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_bitcast), "bitcast");
}

struct itm_instr *itm_trunc(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_trunc), "trunc");
}

struct itm_instr *itm_ftrunc(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_ftrunc), "ftrunc");
}

struct itm_instr *itm_zext(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_zext), "zext");
}

struct itm_instr *itm_sext(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_sext), "sext");
}

struct itm_instr *itm_fext(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_fext), "fext");
}

struct itm_instr *itm_itof(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_itof), "itof");
}

struct itm_instr *itm_ftoi(struct itm_block *b, struct itm_expr *l, struct ctype *to)
{
	return impl_cast(b, l, to, ITM_ID(itm_ftoi), "ftoi");
}


struct itm_instr *itm_getptr(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	return impl_aop(b, l, r, ITM_ID(itm_getptr), "getptr");
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
	res = impl_op(b, deeptype, ITM_ID(itm_deepptr), "deepptr", OF_NONE);
	list_push_back(res->operands, l);
	list_push_back(res->operands, r);
	return res;
}

struct itm_instr *itm_alloca(struct itm_block *b, struct ctype *ty)
{
	struct itm_instr *res;
	res = impl_op(b, new_pointer(ty), ITM_ID(itm_alloca), "alloca", OF_START);
	res->typeoperand = ty;
	return res;
}

struct itm_instr *itm_load(struct itm_block *b, struct itm_expr *l)
{
	struct itm_instr *res;
	assert(l->type->type == POINTER);
	res = impl_op(b, ((struct cpointer *)l->type)->pointsto,
		ITM_ID(itm_load), "load", OF_NONE);

	list_push_back(res->operands, l);

	return res;
}

struct itm_instr *itm_store(struct itm_block *b, struct itm_expr *l, struct itm_expr *r)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, ITM_ID(itm_store), "store", OF_NONE);

	list_push_back(res->operands, l);
	list_push_back(res->operands, r);

	return res;
}

struct itm_instr *itm_phi(struct itm_block *b, struct ctype *ty, struct list *dict)
{
	struct itm_instr *res;
	res = impl_op(b, ty, ITM_ID(itm_phi), "phi", OF_START_OF_BLOCK);

	struct itm_expr *ex;
	it_t it = list_iterator(dict);
	while (iterator_next(&it, (void **)&ex))
		list_push_back(res->operands, ex);

	return res;
}

struct itm_instr *itm_jmp(struct itm_block *b, struct itm_block *to)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, ITM_ID(itm_jmp), "jmp", OF_TERMINAL);

	list_push_back(res->operands, &to->base);

	return res;
}

struct itm_instr *itm_split(struct itm_block *b, struct itm_expr *c, struct itm_block *t, struct itm_block *e)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, ITM_ID(itm_split), "split", OF_TERMINAL);

	list_push_back(res->operands, c);
	list_push_back(res->operands, &t->base);
	list_push_back(res->operands, &e->base);

	return res;
}

struct itm_instr *itm_ret(struct itm_block *b, struct itm_expr *l)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, ITM_ID(itm_ret), "ret", OF_TERMINAL);
	list_push_back(res->operands, l);
	return res;
}

struct itm_instr *itm_leave(struct itm_block *b)
{
	struct itm_instr *res;
	res = impl_op(b, &cvoid, ITM_ID(itm_leave), "leave", OF_TERMINAL);
	return res;
}


struct itm_instr *itm_mov(struct itm_block *b, struct itm_expr *l)
{
	struct itm_instr *res;
	res = impl_op(b, l->type, ITM_ID(itm_mov), "mov", OF_NONE);
	list_push_back(res->operands, l);
	return res;
}

struct itm_instr *itm_clobb(struct itm_block *b)
{
	return impl_op(b, &cvoid, ITM_ID(itm_clobb), "clobb", OF_NONE);
}
