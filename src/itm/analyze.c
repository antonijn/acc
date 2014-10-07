/*
 * Intermediate code analyzation
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

#include <acc/itm/analyze.h>
#include <acc/itm/ast.h>
#include <acc/itm/tag.h>
#include <acc/parsing/ast.h>

itm_tag_type_t tt_used, tt_acc, tt_alive, tt_endlife, tt_phiable;

static void force_analyze(struct itm_block *strt, enum analysis a);

static void canalias(struct itm_expr *l, struct itm_expr *r);

static void a_used(struct itm_instr *strt);
static void a_acc(struct itm_instr *strt);
static void a_lifetime(struct itm_instr *instr);
static void a_phiable(struct itm_instr *instr);

static bool lifetime(struct itm_instr *instr, struct itm_block *block, struct list *done);

static void force_analyze(struct itm_block *strt, enum analysis a)
{
	if ((a & A_USED) == A_USED)
		a_used(strt->first);

	if ((a & A_ACC) == A_ACC)
		a_acc(strt->first);

	// TODO: can lifetime analysis not be merged with usage analysis?
	if ((a & A_LIFETIME) == A_LIFETIME)
		a_lifetime(strt->first);

	if ((a & A_PHIABLE) == A_PHIABLE)
		a_phiable(strt->first);
}

static void a_used(struct itm_instr *i)
{
	if (!i)
		return;

	if (i->base.type != &cvoid &&
	   !itm_get_tag((struct itm_expr *)i, &tt_used)) {
		struct itm_tag *tag = new_itm_tag(&tt_used, "used", TO_INT);
		itm_tag_expr((struct itm_expr *)i, tag);
	}

	struct itm_expr *e;
	it_t it = list_iterator(i->operands);
	while (iterator_next(&it, (void **)&e)) {
		struct itm_tag *tag = itm_get_tag(e, &tt_used);
		if (!tag) {
			tag = new_itm_tag(&tt_used, "used", TO_INT);
			itm_tag_expr(e, tag);
		}
		itm_tag_seti(tag, itm_tag_geti(tag) + 1);
	}

	if (i->next) {
		a_used(i->next);
		return;
	}

	if (i->block->lexnext)
		a_used(i->block->lexnext->first);
}

static void a_acc(struct itm_instr *i)
{
	if (!i)
		return;

	struct itm_tag *t = itm_get_tag((struct itm_expr *)i, &tt_used);

	if (!t || !i->next || itm_tag_geti(t) != 1)
		goto exit;

	struct itm_expr *e;
	it_t it = list_iterator(i->next->operands);
	while (iterator_next(&it, (void **)&e)) {
		if (e == &i->base) {
			struct itm_tag *acc = new_itm_tag(&tt_acc, "acc", TO_NONE);
			itm_tag_expr((struct itm_expr *)i, acc);
			break;
		}
	}

exit:
	if (i->next) {
		a_acc(i->next);
		return;
	}

	if (i->block->lexnext)
		a_acc(i->block->lexnext->first);
}

static void a_lifetime(struct itm_instr *instr)
{
	// FIXME: for unused instructions

	if (!instr)
		return;

	struct itm_instr *i = instr;
	do {
		if (i->id == ITM_ID(itm_alloca) || i->base.type == &cvoid)
			continue;
		struct list *li = new_list(NULL, 0);
		lifetime(i, i->block, li);
		delete_list(li, NULL);
	} while (i = i->next);

	if (instr->block->lexnext)
		a_lifetime(instr->block->lexnext->first);
}

static bool lifetime(struct itm_instr *instr, struct itm_block *block, struct list *done)
{
	assert(done != NULL);


	bool localuse = false;
	bool orf = false;
	struct itm_instr *bi = block->first;

	for (bi = block->first; bi && bi->id == ITM_ID(itm_phi); bi = bi->next) {
		localuse = list_contains(bi->operands, &instr->base);
		if (localuse)
			break;
	}

	if (list_contains(done, block))
		goto tag;

	list_push_back(done, block);

	/*
	 * Backwards is faster
	 */
	for (bi = block->last; bi && bi->id != ITM_ID(itm_phi); bi = bi->previous) {
		localuse = list_contains(bi->operands, &instr->base);
		if (localuse)
			break;
	}

	struct itm_block *baft;
	it_t it = list_iterator(block->next);
	while (iterator_next(&it, (void **)&baft))
		orf |= lifetime(instr, baft, done);

tag:
	// used in block only
	if (localuse && !orf) {
		struct itm_tag *endlife = itm_get_tag(&bi->base, &tt_endlife);
		if (!endlife) {
			endlife = new_itm_tag(&tt_endlife, "endlife", TO_EXPR_LIST);
			itm_tag_expr(&bi->base, endlife);
		}
		itm_tag_add_item(endlife, instr);
	}

	return orf || localuse;
}

static bool isreferenced(struct itm_instr *instr, struct itm_block *b)
{
	assert(instr->id == ITM_ID(itm_alloca));

	struct itm_instr *i = b->first;
	while (i) {
		if (i->id == ITM_ID(itm_load))
			goto skip;

		struct itm_expr *e;
		it_t it = list_iterator(i->operands);
		int j = 0;
		while (iterator_next(&it, (void **)&e)) {
			if (i->id == ITM_ID(itm_store) && j > 0)
				goto skip;
			if (e == &instr->base)
				return true;
			++j;
		}

	skip:
		i = i->next;
	}

	if (b->lexnext)
		return isreferenced(instr, b->lexnext);

	return false;
}

static void a_phiable(struct itm_instr *instr)
{
	if (instr->id != ITM_ID(itm_alloca))
		return;

	if (!isreferenced(instr, instr->block)) {
		struct itm_tag *phi = new_itm_tag(&tt_phiable, "phiable", TO_NONE);
		itm_tag_expr(&instr->base, phi);
	}

	if (instr->next)
		a_phiable(instr->next);
}

void analyze(struct itm_block *strt, enum analysis a)
{
	force_analyze(strt, a);
}

void reanalyze(struct itm_block *strt, enum analysis a)
{

}
