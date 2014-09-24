/*
 * Assembly emission
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <acc/itm/analyze.h>
#include <acc/emit/asm.h>
#include <acc/options.h>
#include <acc/target.h>

asme_type_t asme_reg;
asme_type_t asme_imm;

itm_tag_type_t tt_loc, tt_color;
static itm_tag_type_t tt_lochint;

enum locty {
	LT_REG,
	LT_LMEM,
	LT_PMEM,
	LT_MULTIPLE
};

struct location {
	enum locty type;
	size_t size;
	void *extended;
};

struct loc_reg {
	struct location base;
	regid_t rid;
};

struct loc_mem {
	struct location base;
	int offset;
};

struct loc_multiple {
	struct location base;
	struct list *locs;
};

static inline void loc_init(struct location *loc, enum locty type,
	size_t size, void *ex)
{
	loc->type = type;
	loc->size = size;
	loc->extended = ex;
}

struct location *new_loc_reg(size_t size, regid_t rid)
{
	struct loc_reg *loc = malloc(sizeof(struct loc_reg));
	loc_init(&loc->base, LT_REG, size, loc);
	loc->rid = rid;
	return &loc->base;
}

struct location *new_loc_lmem(size_t size, int offset)
{
	struct loc_mem *loc = malloc(sizeof(struct loc_mem));
	loc_init(&loc->base, LT_LMEM, size, loc);
	loc->offset = offset;
	return &loc->base;
}

struct location *new_loc_pmem(size_t size, int offset)
{
	struct loc_mem *loc = malloc(sizeof(struct loc_mem));
	loc_init(&loc->base, LT_PMEM, size, loc);
	loc->offset = offset;
	return &loc->base;
}

struct location *new_loc_multiple(size_t size, struct list *locs)
{
	struct loc_multiple *loc = malloc(sizeof(struct loc_multiple));
	loc_init(&loc->base, LT_MULTIPLE, size, loc);
	loc->locs = clone_list(locs);
	return &loc->base;
}

void delete_loc(struct location *loc)
{
	if (loc->type == LT_MULTIPLE) {
		struct loc_multiple *ext = loc->extended;
		delete_list(ext->locs, (void (*)(void *))&delete_loc);
	}
	free(loc);
}

void loc_to_string(FILE *f, struct location *loc)
{
	struct loc_reg *reg;
	struct loc_mem *mem;
	struct loc_multiple *mul;

	switch (loc->type) {
	case LT_REG:
		reg = loc->extended;
		regid_t rid = reg->rid;
		bool prevprint = false;
		for (int i = 0; i < sizeof(rid) * 8; ++i) {
			if (!(rid & (1ul << i)))
				continue;

			if (prevprint)
				fprintf(f, " | ");
			fprintf(f, "r%d", i);
			prevprint = true;
			rid &= ~(1ul << i);
		}
		break;
	}
}

static bool loc_eq(struct location *a, struct location *b)
{
	assert(a != NULL);
	assert(b != NULL);
	
	if (a->type != b->type)
		return false;

	struct loc_reg *reg1, *reg2;
	struct loc_mem *mem1, *mem2;
	struct loc_multiple *mul1, *mul2;
	
	switch (a->type) {
	case LT_REG:
		reg1 = a->extended;
		reg2 = b->extended;
		return (reg1->rid & reg2->rid) != 0;
	case LT_LMEM:
	case LT_PMEM:
		mem1 = a->extended;
		mem2 = b->extended;
		// FIXME: better overlap check
		return mem1->offset == mem2->offset;
	case LT_MULTIPLE:
		// TODO: implement LT_MULTIPLE
		return true;
	}
}

static struct location *copy_loc(struct location *loc)
{
	assert(loc != NULL);

	struct loc_reg *reg1, *reg2;
	struct loc_mem *mem1, *mem2;

	switch (loc->type) {
	case LT_REG:
		reg1 = loc->extended;
		reg2 = new_loc_reg(loc->size, reg1->rid)->extended;
		return &reg2->base;
	case LT_LMEM:
		mem1 = loc->extended;
		mem2 = new_loc_lmem(loc->size, mem1->offset)->extended;
		return &mem2->base;
	case LT_PMEM:
		mem1 = loc->extended;
		mem2 = new_loc_pmem(loc->size, mem1->offset)->extended;
		return &mem2->base;
	case LT_MULTIPLE:
		assert(false); // TODO: implement LT_MULTIPLE copy
	}
}

static void asmimmtostr(FILE *f, struct asme *e);
static void asmimmtostrd(FILE *f, struct asme *e);

void new_asm_imm(struct asmimm *res, int size, long value)
{
	assert(res != NULL);

	res->base.type = &asme_imm;
	res->base.size = size;
	res->base.to_string = &asmimmtostr;
	res->base.to_string_d = &asmimmtostrd;
	res->l = res->r = NULL;
	res->op = NULL;
	res->label = NULL;
	res->value = value;
}

void new_asm_label(struct asmimm *res, char *value)
{
	assert(res != NULL);
	assert(value != NULL);

	// I don't know if ISO true values are necessarily 1...
	int uscorepfix = ((getos() == &oswindows) && value[0] != '.') ? 1 : 0;

	res->base.type = &asme_imm;
	res->base.size = getcpu()->bits / 8;
	res->base.to_string = &asmimmtostr;
	res->base.to_string_d = &asmimmtostrd;
	res->l = res->r = NULL;
	res->op = NULL;
	res->label = calloc(strlen(value) + 1 + uscorepfix, sizeof(char));
	if (uscorepfix)
		sprintf(res->label, "_%s", value);
	else
		sprintf(res->label, "%s", value);
	res->value = -1;
}

void new_asm_cop(struct asmimm *res, const char *op,
	struct asmimm *l, struct asmimm *r)
{
	assert(res != NULL);
	assert(op != NULL);
	assert(l != NULL);
	assert(r != NULL);
	assert(l->base.size == r->base.size);

	res->base.type = &asme_imm;
	res->base.size = l->base.size;
	res->base.to_string = &asmimmtostr;
	res->base.to_string_d = &asmimmtostrd;
	res->l = l;
	res->r = r;
	res->op = op;
	res->label = NULL;
	res->value = -1;
}

void delete_asm_imm(struct asmimm *imm)
{
	assert(imm != NULL);
	if (imm->label)
		free(imm->label);
}


// To string functions

void asmregtostr(FILE *f, struct asme *e)
{
	struct asmreg *r = (struct asmreg *)e;
	if (option_asmflavor() == AF_ATT)
		fprintf(f, "%%");
	fprintf(f, "%s", r->name);
}

static void asmimmtostrpfix(FILE *f, struct asmimm *imm, bool attprefix)
{
	if (imm->op) {
		fprintf(f, "(");
		asmimmtostrpfix(f, imm->l, attprefix);
		fprintf(f, " %s ", imm->op);
		asmimmtostrpfix(f, imm->r, attprefix);
		fprintf(f, ")");
	} else if (imm->label) {
		fprintf(f, "%s", imm->label);
	} else if (attprefix && option_asmflavor() == AF_ATT) {
		fprintf(f, "$%ld", imm->value);
	} else {
		fprintf(f, "%ld", imm->value);
	}
}

static void asmimmtostr(FILE *f, struct asme *imm)
{
	asmimmtostrpfix(f, (struct asmimm *)imm, true);
}

static void asmimmtostrd(FILE *f, struct asme *imm)
{
	asmimmtostrpfix(f, (struct asmimm *)imm, false);
}


void emit_label(FILE *f, struct asmimm *imm)
{
	assert(imm != NULL);
	assert(imm->label != NULL);

	fprintf(f, "%s:\n", imm->label);
}

void emit_i(FILE *f, const char *instr, int numops, ...)
{
	va_list ap;
	bool reqsuf = 0;
	struct asme **ops = malloc(sizeof(struct asme *) * numops);

	va_start(ap, numops);
	for (int i = 0; i < numops; ++i) {
		struct asme *e = va_arg(ap, struct asme *);
		assert(e != NULL);
		ops[i] = e;
		reqsuf |= (e->type != &asme_imm);
	}

	fprintf(f, "\t%s", instr);
	if (option_asmflavor() == AF_ATT && reqsuf) {
		switch (ops[0]->size) {
		case 1:
			fprintf(f, "b");
			break;
		case 2:
			fprintf(f, "w");
			break;
		case 4:
			fprintf(f, "l");
			break;
		case 8:
			fprintf(f, "q");
			break;
		case 10:
			fprintf(f, "t");
			break;
		}
	}

	if (numops)
		fprintf(f, " ");

	for (int i = 0; i < numops; ++i) {
		ops[i]->to_string(f, ops[i]);
		if (i != numops - 1)
			fprintf(f, ", ");
	}

	fprintf(f, "\n");

	va_end(ap);
	free(ops);
}

void emit_sdi(FILE *f, const char *instr,
	struct asme *dest, struct asme *src)
{
	if (option_asmflavor() == AF_ATT)
		emit_i(f, instr, 2, src, dest);
	else
		emit_i(f, instr, 2, dest, src);
}

void emit_global(FILE *f, struct asmimm *imm)
{
	assert(imm != NULL);
	assert(imm->label != NULL);

	if (option_asmflavor() == AF_ATT)
		fprintf(f, "\t.globl\t%s\n", imm->label);
	else
		fprintf(f, "global\t%s\n", imm->label);
}

void emit_extern(FILE *f, struct asmimm *imm)
{
	assert(imm != NULL);
	assert(imm->label != NULL);

	if (option_asmflavor() == AF_ATT)
		return;

	fprintf(f, "extern\t%s\n", imm->label);
}

void emit_sect(FILE *f, enum section sec)
{
	assert(sec != SECTION_INVALID);

	if (option_asmflavor() == AF_ATT) {
		fprintf(f, "\t");
		if (sec == SECTION_RODATA)
			fprintf(f, ".");
	}

	if (option_asmflavor() != AF_ATT || sec == SECTION_RODATA)
		fprintf(f, "section\t");

	switch (sec) {
	case SECTION_TEXT:
		fprintf(f, ".text");
		break;
	case SECTION_DATA:
		fprintf(f, ".data");
		break;
	case SECTION_RODATA:
		fprintf(f, ".rodata");
		break;
	case SECTION_BSS:
		fprintf(f, ".bss");
		break;
	}
	fprintf(f, "\n");
}

static void emit_reslike(FILE *f, const char *dir, size_t cnt, va_list ap)
{
	fprintf(f, "\t%s\t", dir);
	for (int i = 0; i < cnt; ++i) {
		fprintf(f, "%s", va_arg(ap, char *));
		if (i != cnt - 1)
			fprintf(f, ", ");
	}
	fprintf(f, "\n");
}

void emit_byte(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		emit_reslike(f, ".byte", cnt, ap);
	else
		emit_reslike(f, "db", cnt, ap);
	va_end(ap);
}

void emit_short(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		emit_reslike(f, ".short", cnt, ap);
	else
		emit_reslike(f, "dw", cnt, ap);
	va_end(ap);
}

void emit_long(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		emit_reslike(f, ".long", cnt, ap);
	else
		emit_reslike(f, "dd", cnt, ap);
	va_end(ap);
}

void emit_quad(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		emit_reslike(f, ".quad", cnt, ap);
	else
		emit_reslike(f, "dq", cnt, ap);
	va_end(ap);
}

static void emit_asciizchar(FILE *f, char ch)
{
	if (isprint(ch) && ch != '"')
		fprintf(f, "%c", ch);
	else
		fprintf(f, "\\x%02x", (int)ch);
}

void emit_asciiz(FILE *f, const char *str)
{
	if (option_asmflavor() == AF_ATT)
		fprintf(f, "\t.asciz\t\"");
	else
		fprintf(f, "\tdb\t\"");

	size_t len = strlen(str);
	for (int i = 0; i < len; ++i) {
		emit_asciizchar(f, str[i]);
	}

	if (option_asmflavor() != AF_ATT)
		fprintf(f, "\\0");
	fprintf(f, "\"\n");
}

/*
 * These are register allocation functions, used to assign a valid register to
 * each intermediate instruction.
 */

#ifndef NDEBUG
static void ovldump(struct list *overlapdict);
#endif

/*
 * Get Overlaps
 *
 * Creates a dictionary of overlaps between instruction lifetimes.
 */
static void getovlps(struct itm_block *b, enum raflags flags,
	struct list *overlapdict);

/*
 * Register Assign
 *
 * Assigns a register/memory location to each instruction.
 */
static void regasn(struct itm_block *b, enum raflags flags,
	struct list *overlapdict, regid_t callersav, regid_t calleesav);

/*
 * The only exported register allocation functions, calling in sequence the
 * three basic components.
 */
void regalloc(struct itm_block *b, enum raflags flags,
	regid_t callersav, regid_t calleesav)
{
	struct list *overlapdict = new_list(NULL, 0);
	getovlps(b, flags, overlapdict);
#ifndef NDEBUG
	ovldump(overlapdict);
#endif
	regasn(b, flags, overlapdict, callersav, calleesav);

	it_t it = list_iterator(overlapdict);
	struct list *li;
	while (iterator_next(&it, (void **)&li)) {
		iterator_next(&it, (void **)&li);
		delete_list(li, NULL);
	}
	delete_list(overlapdict, NULL);
}

#ifndef NDEBUG
static void ovldump(struct list *overlapdict)
{
	FILE *f = fopen("ovldump", "wb");
	
	struct itm_instr *i;
	it_t it = list_iterator(overlapdict);
	while (iterator_next(&it, (void **)&i)) {
		fprintf(f, "%d", itm_instr_number(i));
		struct list *ovlwith;
		iterator_next(&it, (void **)&ovlwith);
		it_t ovlit = list_iterator(ovlwith);
		while (iterator_next(&ovlit, (void **)&i))
			fprintf(f, ",\t%d", itm_instr_number(i));
		fprintf(f, "\n");
		fflush(f);
	}
	
	fclose(f);
}
#endif


static void rgetovlps(struct itm_instr *i, enum raflags flags, int *h,
	struct list *alive, struct list *overlapdicth);
static void killinstrs(struct itm_instr *i, struct list *alive);

static void getovlps(struct itm_block *b, enum raflags flags,
	struct list *overlapdict)
{
	assert(b != NULL);

	analyze(b, A_LIFETIME);

	int h = 0;
	struct list *alive = new_list(NULL, 0);
	rgetovlps(b->first, flags, &h, alive, overlapdict);
	delete_list(alive, NULL);
}

static void killinstrs(struct itm_instr *i, struct list *alive)
{
	struct itm_tag *elifet = itm_get_tag(&i->base, &tt_endlife);
	if (!elifet)
		return;

	struct list *elife = itm_tag_get_list(elifet);
	struct itm_instr *e;
	it_t it = list_iterator(elife);
	while (iterator_next(&it, (void **)&e))
		if (list_contains(alive, e))
			list_remove(alive, e);
}

static void rgetovlps(struct itm_instr *i, enum raflags flags, int *h,
	struct list *alive, struct list *overlapdict)
{
	assert(h != NULL);
	assert(i != NULL);

	if (i->base.type != &cvoid && i->id != ITM_ID(itm_alloca)) {
		killinstrs(i, alive);

		struct list *initoverl = new_list(NULL, 0);

		struct itm_instr *other;
		it_t it = list_iterator(alive);
		while (iterator_next(&it, (void **)&other)) {
			list_push_back(initoverl, other);
			struct list *otherovl;
			bool suc = dict_get(overlapdict, other, (void **)&otherovl);
			assert(suc);
			list_push_back(otherovl, i);
		}

		dict_push_back(overlapdict, i, initoverl);
		list_push_back(alive, i);
	}

	if (i->next) {
		rgetovlps(i->next, flags, h, alive, overlapdict);
	} else {
		struct itm_block *nxt;
		it_t bit = list_iterator(i->block->next);
		while (iterator_next(&bit, (void **)&nxt)) {
			struct list *nalive = clone_list(alive);
			rgetovlps(nxt->first, flags, h, nalive, overlapdict);
			delete_list(nalive, NULL);
		}
	}
}


/*
 * This function tries to "get rid of" itm_mov instructions by trying to assign
 * each instruction that is used as an itm_mov operand with the location it's
 * moved into.
 */
static void induceregs(struct itm_block *b, enum raflags flags,
	struct list *overlapdict);
static void inducereg(struct itm_instr *i, enum raflags flags,
	struct list *overlapdict);

/*
 * Resolves tt_lochint conflicts. The instruction with the highest tt_used value
 * "wins" the register.
 */
static void resolvconfls(struct itm_block *b, enum raflags flags,
	struct list *overlapdict);
// returns the winning itm_instr
static struct itm_instr *resolvconfl(struct itm_instr *i, enum raflags flags,
	struct list *overlapdict);

/*
 * Assigns locations to the remainder of registers.
 */
static void asnrems(struct itm_block *b, enum raflags flags,
	struct list *overlapdict, regid_t callersav, regid_t calleesav);
static void asnrem(struct itm_instr *i, enum raflags flags,
	struct list *overlapdict, regid_t callersav, regid_t calleesav);

static void regasn(struct itm_block *b, enum raflags flags,
	struct list *overlapdict, regid_t callersav, regid_t calleesav)
{
	analyze(b, A_USED);

	induceregs(b, flags, overlapdict);
	resolvconfls(b, flags, overlapdict);
	asnrems(b, flags, overlapdict, callersav, calleesav);
}

static void resolvconfls(struct itm_block *b, enum raflags flags,
	struct list *overlapdict)
{
	for (struct itm_instr *i = b->first; i; i = i->next) {
		struct itm_instr *win = resolvconfl(i, flags, overlapdict);
		if (!win)
			continue;

		struct itm_tag *loct = itm_get_tag(&win->base, &tt_lochint);
		struct location *loc = copy_loc(itm_tag_get_user_ptr(loct));
		struct itm_tag *nloct = new_itm_tag(&tt_loc, "loc", TO_USER_PTR);
		itm_tag_set_user_ptr(nloct, loc, (void (*)(FILE *, void *))&loc_to_string);
		itm_untag_expr(&win->base, &tt_lochint);
		itm_tag_expr(&win->base, nloct);
	}

	struct itm_block *nxt;
	it_t it = list_iterator(b->next);
	while (iterator_next(&it, (void **)&nxt))
		resolvconfls(nxt, flags, overlapdict);
}

static struct itm_instr *resolvconfl(struct itm_instr *i, enum raflags flags,
	struct list *overlapdict)
{
	struct itm_tag *lochint = itm_get_tag(&i->base, &tt_lochint);
	if (!lochint)
		return NULL;

	struct itm_instr *winner = i;
	struct itm_tag *usedt = itm_get_tag(&i->base, &tt_used);
	assert(usedt != NULL);
	int used = itm_tag_geti(usedt);
	struct location *loc = itm_tag_get_user_ptr(lochint);

	struct itm_instr *ovli;
	struct list *ovl;
	dict_get(overlapdict, i, (void **)&ovl);
	it_t it = list_iterator(ovl);
	while (iterator_next(&it, (void **)&ovli)) {
		struct itm_tag *other = itm_get_tag(&ovli->base, &tt_lochint);
		if (!other)
			continue;
		struct location *oloc = itm_tag_get_user_ptr(other);
		if (!loc_eq(loc, oloc))
			continue;

		// there is a conflict
		struct itm_tag *otherut = itm_get_tag(&ovli->base, &tt_used);
		assert(otherut != NULL);
		int oused = itm_tag_geti(otherut);
		if (oused > used && resolvconfl(ovli, flags, overlapdict) == ovli) {
			winner = i;
			used = oused;
			continue;
		}

		itm_untag_expr(&ovli->base, &tt_lochint);
	}

	return winner;
}

static void induceregs(struct itm_block *b, enum raflags flags,
	struct list *overlapdict)
{
	for (struct itm_instr *i = b->first; i; i = i->next)
		inducereg(i, flags, overlapdict);

	struct itm_block *nxt;
	it_t it = list_iterator(b->next);
	while (iterator_next(&it, (void **)&nxt))
		induceregs(nxt, flags, overlapdict);
}

static void inducereg(struct itm_instr *i, enum raflags flags,
	struct list *overlapdict)
{
	if (i->id != ITM_ID(itm_mov))
		return;

	struct itm_tag *movto = itm_get_tag(&i->base, &tt_loc);
	if (!movto)
		return;
	
	struct location *loc = itm_tag_get_user_ptr(movto);
	if (loc->type != LT_REG)
		return; // TODO: implement non-regs? multiple regs?
	
	struct loc_reg *reg = loc->extended;

	struct itm_expr *op = list_head(i->operands);
	if (op->etype != ITME_INSTRUCTION)
		return;

	struct itm_instr *opi = (struct itm_instr *)op;
	if (itm_get_tag(&opi->base, &tt_loc))
		return;

	struct location *newl = copy_loc(loc);
	struct itm_tag *newt = new_itm_tag(&tt_lochint, "lochint", TO_USER_PTR);
	itm_tag_set_user_ptr(newt, newl, (void (*)(FILE *, void *))&loc_to_string);
	itm_tag_expr(op, newt);
}

static void asnrems(struct itm_block *b, enum raflags flags,
	struct list *overlapdict, regid_t callersav, regid_t calleesav)
{
	for (struct itm_instr *i = b->first; i; i = i->next)
		asnrem(i, flags, overlapdict, callersav, calleesav);

	struct itm_block *nxt;
	it_t it = list_iterator(b->next);
	while (iterator_next(&it, (void **)&nxt))
		asnrems(nxt, flags, overlapdict, callersav, calleesav);
}

// returns 0 if unsuccessful
static regid_t getreg(struct itm_instr *i, enum raflags flags,
	struct list *overlapdict, regid_t av)
{
	// TODO: allocate multiple regs for large instructions
	struct list *ovlps;
	dict_get(overlapdict, i, (void **)&ovlps);

retry:
	if (!av)
		return 0;

	regid_t try;
	for (int i = 0; i < sizeof(regid_t) * 8; ++i) {
		if (av & (1u << i)) {
			try = 1u << i;
			break;
		}
	}

	struct itm_instr *other;
	it_t it = list_iterator(ovlps);
	while (iterator_next(&it, (void **)&other)) {
		struct itm_tag *loct = itm_get_tag(&other->base, &tt_loc);
		if (!loct)
			continue;
		struct location *loc = itm_tag_get_user_ptr(loct);
		if (loc->type != LT_REG)
			continue;
		struct loc_reg *rloc = loc->extended;
		if (try & rloc->rid) {
			av &= ~try;
			goto retry;
		}
	}

	return try;
}

static void asnrem(struct itm_instr *i, enum raflags flags,
	struct list *overlapdict, regid_t callersav, regid_t calleesav)
{
	if (itm_get_tag(&i->base, &tt_loc))
		return;

	regid_t try = getreg(i, flags, overlapdict, callersav);
	if (!try)
		try = getreg(i, flags, overlapdict, calleesav);

	struct loc_reg *reg = new_loc_reg(i->base.type->size, try)->extended;
	struct itm_tag *regt = new_itm_tag(&tt_loc, "loc", TO_USER_PTR);
	itm_tag_set_user_ptr(regt, reg, (void (*)(FILE *, void *))&loc_to_string);
	itm_tag_expr(&i->base, regt);
}
