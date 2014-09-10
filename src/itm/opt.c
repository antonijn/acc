/*
 * Intermediate code optimization
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

#include <assert.h>

#include <acc/itm/opt.h>
#include <acc/itm/ast.h>
#include <acc/itm/analyze.h>
#include <acc/itm/tag.h>
#include <acc/list.h>

static void repli(struct itm_instr *a, struct itm_expr *b);

static void o_phiable(struct itm_instr *strt, struct list *dict);

void optimize(struct itm_block *strt, enum optimization o)
{
	if (o & OPT_PHIABLE) {
		struct list *dict = new_list(NULL, 0);
		o_phiable(strt->first, dict);
		delete_list(dict, NULL);
	}
}

static void remi(struct itm_instr *a)
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

static void repli(struct itm_instr *a, struct itm_expr *b)
{
	struct itm_block *first = a->block;
	while (first->lexprev)
		first = first->lexprev;

	struct itm_instr *instr = first->first;
	while (instr) {
		struct itm_expr *e;
		void *it = list_iterator(instr->operands);
		int i = 0;
		while (iterator_next(&it, (void **)&e)) {
			if (e == &a->base) {
				set_list_item(instr->operands, i, b);
				break;
			}
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

	remi(a);
}

static struct itm_expr *traceload(struct itm_instr *ld, struct itm_instr *i,
	struct list *dict);

static struct itm_expr *replload(struct itm_instr *ld, struct list *dict)
{
	struct itm_instr *key;
	struct itm_expr *value = NULL;
	void *iter = list_iterator(dict);
	while (iterator_next(&iter, (void **)&key)) {
		iterator_next(&iter, (void **)&value);
		if (key == ld)
			break;
	}

	if (!value)
		value = traceload(ld, ld, dict);

	repli(ld, value);
	return value;
}

static struct itm_expr *traceload(struct itm_instr *ld, struct itm_instr *i,
	struct list *dict)
{
	struct itm_expr *ptr = list_head(ld->operands);
	if (ld != i) {
		if (i->id == ITM_ID(itm_store) &&
		    ptr == list_last(i->operands)) {
			struct itm_expr *res = list_head(i->operands);
			remi(i);
			return clone_itm_expr(res);
		}

		if (i->id == ITM_ID(itm_load) &&
		    ptr == list_head(i->operands))
			return replload(i, dict);
	}

	if (i->previous)
		return traceload(ld, i->previous, dict);

	switch (list_length(i->block->previous)) {
	case 0:
		// TODO: return __undef
		assert(false);
	case 1:
		return traceload(ld, ((struct itm_block *)list_head(i->block->previous))->last,
			dict);
	}

	struct list *li = new_list(NULL, 0);
	struct itm_instr *phi = itm_phi(i->block, ld->base.type, li);
	delete_list(li, NULL);

	list_push_back(dict, ld);
	list_push_back(dict, phi);

	struct itm_block *pb;
	void *it = list_iterator(i->block->previous);
	while (iterator_next(&it, (void **)&pb)) {
		list_push_back(phi->operands, pb);
		list_push_back(phi->operands, traceload(ld, pb->last, dict));
	}
	return &phi->base;
}

static void remphiables(struct itm_instr *strt)
{
	struct itm_block *first = strt->block;
	while (first->lexprev)
		first = first->lexprev;

	strt = first->first;
	while (strt) {
		struct itm_instr *nnxt = strt->next;
		if (strt->id == ITM_ID(itm_alloca) &&
		    itm_get_tag(&strt->base, &tt_phiable))
			remi(strt);
		strt = nnxt;
	}
}

static void o_phiable(struct itm_instr *strt, struct list *dict)
{
	struct itm_instr *nxt = strt->next;

	if (strt->id == ITM_ID(itm_load) &&
	    itm_get_tag(list_head(strt->operands), &tt_phiable))
		replload(strt, dict);

	if (nxt)
		o_phiable(nxt, dict);
	else if (strt->block->lexnext)
		o_phiable(strt->block->lexnext->first, dict);
	else
		remphiables(strt);
}
