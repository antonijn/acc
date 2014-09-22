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
	uint64_t rid;
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

struct location *new_loc_reg(size_t size, int rid)
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
	struct list *overlapdict);

/*
 * The only exported register allocation functions, calling in sequence the
 * three basic components.
 */
void regalloc(struct itm_block *b, enum raflags flags)
{
	struct list *overlapdict = new_list(NULL, 0);
	getovlps(b, flags, overlapdict);
	regasn(b, flags, overlapdict);
	it_t it = list_iterator(overlapdict);
	struct list *li;
	while (iterator_next(&it, (void **)&li)) {
		iterator_next(&it, (void **)&li);
		delete_list(li, NULL);
	}
	delete_list(overlapdict, NULL);
}


static void rgetovlps(struct itm_instr *i, enum raflags flags, int *h,
	struct list *alive, struct list *overlapdicth);
static void killinstrs(struct itm_instr *i, struct list *alive);
static bool samety(struct ctype *a, struct ctype *b);

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

	if (i->base.type == &cvoid || i->id == ITM_ID(itm_alloca))
		goto nxti;

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

nxti:
	if (i->next) {
		rgetovlps(i->next, flags, h, alive, overlapdict);
		return;
	}

	struct itm_block *nxt;
	it_t bit = list_iterator(i->block->next);
	while (iterator_next(&bit, (void **)&nxt))
		rgetovlps(nxt->first, flags, h, alive, overlapdict);
}

static bool samety(struct ctype *a, struct ctype *b)
{
	// TODO: proper implementation...
	return a == b;
}


static void regasn(struct itm_block *b, enum raflags flags,
	struct list *overlapdict)
{
}
