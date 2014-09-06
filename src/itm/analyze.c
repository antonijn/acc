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

#include <acc/itm/analyze.h>
#include <acc/itm/ast.h>
#include <acc/itm/tag.h>
#include <acc/parsing/ast.h>

static void force_analyze(struct itm_block *strt, enum analysis a);

static void canalias(struct itm_expr *l, struct itm_expr *r);

static void a_used(struct itm_instr *strt);
static void a_acc(struct itm_instr *strt);

static void force_analyze(struct itm_block *strt, enum analysis a)
{
	if (a & A_USED)
		a_used(strt->first);

	if ((a & A_ACC) == A_ACC)
		a_acc(strt->first);
}

static void a_used(struct itm_instr *i)
{
	if (!i)
		return;

	if (i->base.type != &cvoid &&
	   !itm_get_tag((struct itm_expr *)i, TT_USED)) {
		struct itm_tag *tag = malloc(sizeof(struct itm_tag));
		new_itm_tag(tag, TT_USED, "used");
		itm_tag_expr((struct itm_expr *)i, tag);
	}

	struct itm_expr *e;
	void *it = list_iterator(i->operands);
	while (iterator_next(&it, (void **)&e)) {
		struct itm_tag *tag = itm_get_tag(e, TT_USED);
		if (!tag) {
			tag = malloc(sizeof(struct itm_tag));
			new_itm_tag(tag, TT_USED, "used");
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

	struct itm_tag *t = itm_get_tag((struct itm_expr *)i, TT_USED);

	if (!t || !i->next || itm_tag_geti(t) != 1)
		goto exit;

	struct itm_expr *e;
	void *it = list_iterator(i->next->operands);
	while (iterator_next(&it, (void **)&e)) {
		if (e == &i->base) {
			struct itm_tag *acc = malloc(sizeof(struct itm_tag));
			new_itm_tag(acc, TT_ACC, "acc");
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

void analyze(struct itm_block *strt, enum analysis a)
{
	force_analyze(strt, a);
}

void reanalyze(struct itm_block *strt, enum analysis a)
{

}
