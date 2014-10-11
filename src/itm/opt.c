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
#include <acc/options.h>
#include <acc/list.h>

/*
 * Replaces SSA alloca/load/store system with a phi node system where possible
 */
static void o_phiable(struct itm_instr *strt, struct list *dict);
/*
 * Removes unused blocks
 */
static void o_prune(struct itm_block *blk);
/*
 * Performs constant folding
 */
static void o_cfld(struct itm_instr *strt);

void optimize(struct itm_block *strt)
{
	if (option_optimize() > 0) {
		analyze(strt, A_PHIABLE);
		struct list *dict = new_list(NULL, 0);
		o_phiable(strt->first, dict);
		delete_list(dict, NULL);

		o_cfld(strt->first);
		o_prune(strt->lexnext);
	}
}

static struct itm_expr *traceload(struct itm_instr *ld, struct itm_instr *i,
	struct list *dict)
{
	struct itm_expr *ptr = list_head(ld->operands);
	if (ld != i) {
		if (i->id == ITM_ID(itm_store) &&
		    ptr == list_last(i->operands))
			return list_head(i->operands);

		if (i->id == ITM_ID(itm_load) &&
		    ptr == list_head(i->operands)) {
			struct itm_expr *repl = traceload(i, i, dict);
			itm_replocc(&i->base, repl, i->block);
			return repl;
		}
	}

	if (i->previous && i->previous->id != ITM_ID(itm_phi))
		return traceload(ld, i->previous, dict);

	switch (list_length(i->block->previous)) {
	case 0:
		return new_itm_undef(i->block->container, ld->base.type);
	case 1:
		return traceload(ld, ((struct itm_block *)list_head(i->block->previous))->last,
			dict);
	}

	struct itm_block *bkey;
	struct itm_expr *pkey, *dval;
	it_t iter = list_iterator(dict);
	while (iterator_next(&iter, (void **)&bkey)) {
		iterator_next(&iter, (void **)&pkey);
		iterator_next(&iter, (void **)&dval);
		if (bkey == i->block && pkey == ptr)
			return dval;
	}

	struct list *li = new_list(NULL, 0);
	struct itm_instr *phi = itm_phi(i->block, ld->base.type, li);
	delete_list(li, NULL);

	list_push_back(dict, i->block);
	list_push_back(dict, ptr);
	list_push_back(dict, phi);

	struct itm_block *pb;
	it_t it = list_iterator(i->block->previous);
	while (iterator_next(&it, (void **)&pb)) {
		list_push_back(phi->operands, pb);
		list_push_back(phi->operands, traceload(ld, pb->last, dict));
	}
	return &phi->base;
}

static void remphiables(struct itm_instr *strt)
{
	/*
	 * Removes all stores to phiables, and then all allocas
	 */
	struct itm_block *first = strt->block;
	while (first->lexprev)
		first = first->lexprev;

	strt = first->first;
	while (strt) {
		struct itm_instr *nnxt;
		if (!strt->next) {
			if (strt->block->lexnext)
				nnxt = strt->block->lexnext->first;
			else
				nnxt = NULL;
		} else {
			nnxt = strt->next;
		}

		if (strt->id == ITM_ID(itm_store) &&
		    itm_get_tag(list_last(strt->operands), &tt_phiable))
			itm_remi(strt);
		else if (strt->id == ITM_ID(itm_load) &&
		         itm_get_tag(list_head(strt->operands), &tt_phiable))
			itm_remi(strt);

		strt = nnxt;
	}
	
	strt = first->first;
	while (strt) {
		struct itm_instr *nnxt = strt->next;
		if (strt->id == ITM_ID(itm_alloca) &&
		    itm_get_tag(&strt->base, &tt_phiable))
			itm_remi(strt);
		strt = nnxt;
	}
}

static void o_phiable(struct itm_instr *strt, struct list *dict)
{
	struct itm_instr *nxt = strt->next;

	if (strt->id == ITM_ID(itm_load) &&
	    itm_get_tag(list_head(strt->operands), &tt_phiable))
		itm_replocc(&strt->base, traceload(strt, strt, dict), strt->block);

	if (nxt)
		o_phiable(nxt, dict);
	else if (strt->block->lexnext)
		o_phiable(strt->block->lexnext->first, dict);
	else
		remphiables(strt);
}

static void rmfromphi(struct itm_block *whichblk, struct itm_instr *phi)
{
	int i = 0;
	struct itm_block *b;
	void *dummy;
	it_t it = list_iterator(phi->operands);
	while (iterator_next(&it, (void **)&b)) {
		iterator_next(&it, &dummy);
		if (b == whichblk) {
			list_remove_at(phi->operands, i);
			list_remove_at(phi->operands, i);
			break;
		}
		i += 2;
	}

	if (list_length(phi->operands) == 2)
		itm_repli(phi, list_last(phi->operands));
}

static void o_prune(struct itm_block *blk)
{
	if (!blk)
		return;

	if (blk->previous && list_length(blk->previous)) {
		o_prune(blk->lexnext);
		return;
	}

	struct itm_block *aft;
	it_t it = list_iterator(blk->next);
	while (iterator_next(&it, (void **)&aft)) {
		struct itm_instr *i = aft->first;
		while (i->id == ITM_ID(itm_phi)) {
			struct itm_instr *nxti = i->next;
			rmfromphi(blk, i);
			i = nxti;
		}

		list_remove(aft->previous, blk);
	}

	struct itm_block *nxt = blk->lexnext;
	blk->lexprev->lexnext = nxt;
	blk->lexnext = NULL;
	if (nxt) {
		nxt->lexprev = blk->lexprev;
		delete_itm_block(blk);
		o_prune(nxt);
	}
}


static void o_cfld(struct itm_instr *strt)
{
	struct itm_instr *nxt = strt->next;

	if (itm_isconst(&strt->base)) {
		struct itm_expr *repl = itm_eval(&strt->base);
		if (repl != &strt->base)
			itm_repli(strt, repl);
	}

	if (nxt)
		o_cfld(nxt);
	else if (strt->block->lexnext)
		o_cfld(strt->block->lexnext->first);
}
