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
 * Color Allocate
 *
 * colalloc() assigns a color to each intermediate instruction result, which
 * represents a unique location, either a register, or in memory. No two
 * instruction with an overlapping lifespan are assigned the same color.
 *
 * This algorithm "snatches" a location of an operand if the types match, and
 * the lifespan of the operand ends.
 */
static void colalloc(struct itm_block *b, enum raflags flags);

/*
 * Color Optimize
 *
 * Stub!
 */
static void colopt(struct itm_block *b, enum raflags flags);

/*
 * Register Assign
 *
 * Assigns a register/memory location to each color, based on its type, CPU
 * capabilities and lifespan.
 */
static void regasn(struct itm_block *b, enum raflags flags);

/*
 * The only exported register allocation functions, calling in sequence the
 * three basic components.
 */
void regalloc(struct itm_block *b, enum raflags flags)
{
	colalloc(b, flags);
	colopt(b, flags);
	regasn(b, flags);
}


static void rcolphi(struct itm_instr *i, enum raflags flags, int *h);
static void rcolalloc(struct itm_instr *i, enum raflags flags, int *h);
static int colsnatch(struct itm_instr *i, enum raflags flags);
static bool samety(struct ctype *a, struct ctype *b);

static void colalloc(struct itm_block *b, enum raflags flags)
{
	assert(b != NULL);

	analyze(b, A_LIFETIME);

	int h = 0;
	rcolalloc(b->first, flags, &h);
}

static int colsnatch(struct itm_instr *i, enum raflags flags)
{
	struct itm_tag *elifet = itm_get_tag(&i->base, &tt_endlife);
	if (!elifet)
		return -1;

	struct list *elife = itm_tag_get_list(elifet);
	struct itm_expr *e;
	void *it = list_iterator(i->operands);
	while (iterator_next(&it, (void **)&e)) {
		if (samety(e->type, i->base.type) && list_contains(elife, e)) {
			struct itm_tag *ct = itm_get_tag(e, &tt_color);
			return itm_tag_geti(ct);
		}
	}

	return -1;
}

static void rcolalloc(struct itm_instr *i, enum raflags flags, int *h)
{
	assert(h != NULL);
	assert(i != NULL);

	if (i->base.type == &cvoid || i->id == ITM_ID(itm_alloca))
		goto nxti;

	if (itm_get_tag(&i->base, &tt_loc))
		goto nxti;

	if (i->id == ITM_ID(itm_phi)) {
		rcolphi(i, flags, h);
		return;
	}

	int col = colsnatch(i, flags);
	if (col == -1)
		col = (*h)++;

	struct itm_tag *colt = new_itm_tag(&tt_color, "color", TO_INT);
	itm_tag_seti(colt, col);
	itm_tag_expr(&i->base, colt);

nxti:
	if (i->next) {
		rcolalloc(i->next, flags, h);
		return;
	}

	struct itm_block *nxt;
	void *bit = list_iterator(i->block->next);
	while (iterator_next(&bit, (void **)&nxt))
		rcolalloc(nxt->first, flags, h);
}

static void rcolphi(struct itm_instr *i, enum raflags flags, int *h)
{
	assert(!"rcolphi(): stub");
}

static bool samety(struct ctype *a, struct ctype *b)
{
	// TODO: proper implementation...
	return a == b;
}


static void colopt(struct itm_block *b, enum raflags flags)
{
}


static void regasn(struct itm_block *b, enum raflags flags)
{
}
