/*
 * x86 code emission
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <acc/emit/x86.h>
#include <acc/itm.h>
#include <acc/options.h>
#include <acc/target.h>

enum x86section {
	SECTION_INVALID,
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_RODATA,
	SECTION_BSS
};

enum x86etype {
	XREGISTER,
	XEA,
	XIMMEDIATE
};

struct x86e {
	enum x86etype type;
	int size;
};

struct x86reg {
	struct x86e base;
	const char *name;
	const struct x86reg *parent;
	const struct x86reg *child1;
	const struct x86reg *child2;
};

struct x86imm {
	struct x86e base;
	struct x86imm *l, *r;
	const char *op;
	char *label;
	long value;
};

struct x86ea {
	struct x86e base;
	const struct x86reg *basereg;
	struct x86imm *displacement;
	const struct x86reg *offset;
	int mult;
};

static const struct x86reg ah, bh, ch, dh;
static const struct x86reg al, bl, cl, dl, spl, bpl, sil, dil,
	r8b, r9b, r10b, r11b, r12b, r13b, r14b, r15b;
static const struct x86reg ax, bx, cx, dx, sp, bp, si, di,
	r8w, r9w, r10w, r11w, r12w, r13w, r14w, r15w;
static const struct x86reg eax, ebx, ecx, edx, esp, ebp, esi, edi,
	r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d;
static const struct x86reg rax, rbx, rcx, rdx, rsp, rbp, rsi, rdi,
	r8, r9, r10, r11, r12, r13, r14, r15;

static const struct x86reg ah = { { XREGISTER, 1 }, "ah", &ax, NULL, NULL };
static const struct x86reg bh = { { XREGISTER, 1 }, "bh", &bx, NULL, NULL };
static const struct x86reg ch = { { XREGISTER, 1 }, "ch", &cx, NULL, NULL };
static const struct x86reg dh = { { XREGISTER, 1 }, "dh", &dx, NULL, NULL };
static const struct x86reg al = { { XREGISTER, 1 }, "al", &ax, NULL, NULL };
static const struct x86reg bl = { { XREGISTER, 1 }, "bl", &bx, NULL, NULL };
static const struct x86reg cl = { { XREGISTER, 1 }, "cl", &cx, NULL, NULL };
static const struct x86reg dl = { { XREGISTER, 1 }, "dl", &dx, NULL, NULL };
static const struct x86reg spl = { { XREGISTER, 1 }, "spl", &sp, NULL, NULL };
static const struct x86reg bpl = { { XREGISTER, 1 }, "bpl", &bp, NULL, NULL };
static const struct x86reg sil = { { XREGISTER, 1 }, "sil", &si, NULL, NULL };
static const struct x86reg dil = { { XREGISTER, 1 }, "dil", &di, NULL, NULL };
static const struct x86reg r8b = { { XREGISTER, 1 }, "r8b", &r8w, NULL, NULL };
static const struct x86reg r9b = { { XREGISTER, 1 }, "r9b", &r9w, NULL, NULL };
static const struct x86reg r10b = { { XREGISTER, 1 }, "r10b", &r10w, NULL, NULL };
static const struct x86reg r11b = { { XREGISTER, 1 }, "r11b", &r11w, NULL, NULL };
static const struct x86reg r12b = { { XREGISTER, 1 }, "r12b", &r12w, NULL, NULL };
static const struct x86reg r13b = { { XREGISTER, 1 }, "r13b", &r13w, NULL, NULL };
static const struct x86reg r14b = { { XREGISTER, 1 }, "r14b", &r14w, NULL, NULL };
static const struct x86reg r15b = { { XREGISTER, 1 }, "r15b", &r15w, NULL, NULL };

static const struct x86reg ax = { { XREGISTER, 2 }, "ax", &eax, &ah, &al };
static const struct x86reg bx = { { XREGISTER, 2 }, "bx", &ebx, &bh, &bl };
static const struct x86reg cx = { { XREGISTER, 2 }, "cx", &ecx, &ch, &cl };
static const struct x86reg dx = { { XREGISTER, 2 }, "dx", &edx, &dh, &dl };
static const struct x86reg sp = { { XREGISTER, 2 }, "sp", &esp, &spl, NULL };
static const struct x86reg bp = { { XREGISTER, 2 }, "bp", &ebp, &bpl, NULL };
static const struct x86reg si = { { XREGISTER, 2 }, "si", &esi, &sil, NULL };
static const struct x86reg di = { { XREGISTER, 2 }, "di", &edi, &dil, NULL };
static const struct x86reg r8w = { { XREGISTER, 2 }, "r8w", &r8d, &r8b, NULL };
static const struct x86reg r9w = { { XREGISTER, 2 }, "r9w", &r9d, &r9b, NULL };
static const struct x86reg r10w = { { XREGISTER, 2 }, "r10w", &r10d, &r10b, NULL };
static const struct x86reg r11w = { { XREGISTER, 2 }, "r11w", &r11d, &r11b, NULL };
static const struct x86reg r12w = { { XREGISTER, 2 }, "r12w", &r12d, &r12b, NULL };
static const struct x86reg r13w = { { XREGISTER, 2 }, "r13w", &r13d, &r13b, NULL };
static const struct x86reg r14w = { { XREGISTER, 2 }, "r14w", &r14d, &r14b, NULL };
static const struct x86reg r15w = { { XREGISTER, 2 }, "r15w", &r15d, &r15b, NULL };

static const struct x86reg eax = { { XREGISTER, 4 }, "eax", &rax, &ax, NULL };
static const struct x86reg ebx = { { XREGISTER, 4 }, "ebx", &rbx, &bx, NULL };
static const struct x86reg ecx = { { XREGISTER, 4 }, "ecx", &rcx, &cx, NULL };
static const struct x86reg edx = { { XREGISTER, 4 }, "edx", &rdx, &dx, NULL };
static const struct x86reg esp = { { XREGISTER, 4 }, "esp", &rsp, &sp, NULL };
static const struct x86reg ebp = { { XREGISTER, 4 }, "ebp", &rbp, &bp, NULL };
static const struct x86reg esi = { { XREGISTER, 4 }, "esi", &rsi, &si, NULL };
static const struct x86reg edi = { { XREGISTER, 4 }, "edi", &rdi, &di, NULL };
static const struct x86reg r8d = { { XREGISTER, 4 }, "r8d", &r8, &r8w, NULL };
static const struct x86reg r9d = { { XREGISTER, 4 }, "r9d", &r9, &r9w, NULL };
static const struct x86reg r10d = { { XREGISTER, 4 }, "r10d", &r10, &r10w, NULL };
static const struct x86reg r11d = { { XREGISTER, 4 }, "r11d", &r11, &r11w, NULL };
static const struct x86reg r12d = { { XREGISTER, 4 }, "r12d", &r12, &r12w, NULL };
static const struct x86reg r13d = { { XREGISTER, 4 }, "r13d", &r13, &r13w, NULL };
static const struct x86reg r14d = { { XREGISTER, 4 }, "r14d", &r14, &r14w, NULL };
static const struct x86reg r15d = { { XREGISTER, 4 }, "r15d", &r15, &r15w, NULL };

static const struct x86reg rax = { { XREGISTER, 8 }, "rax", NULL, &eax, NULL };
static const struct x86reg rbx = { { XREGISTER, 8 }, "rbx", NULL, &ebx, NULL };
static const struct x86reg rcx = { { XREGISTER, 8 }, "rcx", NULL, &ecx, NULL };
static const struct x86reg rdx = { { XREGISTER, 8 }, "rdx", NULL, &edx, NULL };
static const struct x86reg rsp = { { XREGISTER, 8 }, "rsp", NULL, &esp, NULL };
static const struct x86reg rbp = { { XREGISTER, 8 }, "rbp", NULL, &ebp, NULL };
static const struct x86reg rsi = { { XREGISTER, 8 }, "rsi", NULL, &esi, NULL };
static const struct x86reg rdi = { { XREGISTER, 8 }, "rdi", NULL, &edi, NULL };
static const struct x86reg r8 = { { XREGISTER, 8 }, "r8", NULL, &r8d, NULL };
static const struct x86reg r9 = { { XREGISTER, 8 }, "r9", NULL, &r9d, NULL };
static const struct x86reg r10 = { { XREGISTER, 8 }, "r10", NULL, &r10d, NULL };
static const struct x86reg r11 = { { XREGISTER, 8 }, "r11", NULL, &r11d, NULL };
static const struct x86reg r12 = { { XREGISTER, 8 }, "r12", NULL, &r12d, NULL };
static const struct x86reg r13 = { { XREGISTER, 8 }, "r13", NULL, &r13d, NULL };
static const struct x86reg r14 = { { XREGISTER, 8 }, "r14", NULL, &r14d, NULL };
static const struct x86reg r15 = { { XREGISTER, 8 }, "r15", NULL, &r15d, NULL };

/* list of available registers per platform */
static const struct x86reg *regav8086[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di, NULL
};

static const struct x86reg *regavi386[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di,
	&eax, &ebx, &edx, &ecx, &esp, &ebp, &esi, &edi, NULL
};

static const struct x86reg *regavi686[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di,
	&eax, &ebx, &edx, &ecx, &esp, &ebp, &esi, &edi, NULL
};

static const struct x86reg *regavamd64[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di,
	&eax, &ebx, &edx, &ecx, &esp, &ebp, &esi, &edi,
	
	&r8b, &r9b, &r10b, &r11b, &r12b, &r14b, &r15b,
	&r8w, &r9w, &r10w, &r11w, &r12w, &r14w, &r15w,
	&r8d, &r9d, &r10d, &r11d, &r12d, &r14d, &r15d,
	&r8, &r9, &r10, &r11, &r12, &r14, &r15, NULL
};

static const struct x86reg **regav[] = {
	&regav8086[0], &regavi386[0], &regavi686[0], &regavamd64[0]
};

static void new_x86_imm(struct x86imm *res, int size, long value);
static void new_x86_label(struct x86imm *res, char *value);
static void new_x86_cop(struct x86imm *res, const char *op,
	struct x86imm *l, struct x86imm *r);
static void delete_x86_imm(struct x86imm *imm);

static void new_x86_ea(struct x86ea *res, int size,
	const struct x86reg *base,
	struct x86imm *displacement,
	const struct x86reg *offset,
	int mult);
static void delete_x86_ea(struct x86ea *ea);

static void x86l(FILE *f, struct x86imm *imm);
static void x86i(FILE *f, const char *instr, int numops, ...);
static void x86sdi(FILE *f, const char *instr,
	struct x86e *dest, struct x86e *src);
static void x86global(FILE *f, struct x86imm *imm);
static void x86extern(FILE *f, struct x86imm *imm);
static void x86sect(FILE *f, enum x86section sec);
static void x86reslike(FILE *f, const char *dir, size_t cnt, va_list ap);
static void x86byte(FILE *f, size_t cnt, ...);
static void x86short(FILE *f, size_t cnt, ...);
static void x86long(FILE *f, size_t cnt, ...);
static void x86quad(FILE *f, size_t cnt, ...);
static void x86asciiz(FILE *f, const char *str);
static void x86etostr(FILE *f, struct x86e *e);
static void x86eatostr(FILE *f, struct x86ea *ea);
static void x86immtostr(FILE *f, struct x86imm *imm, bool attprefix);

static void x86_emit_symbol(FILE *f, struct symbol *sym);


static void new_x86_imm(struct x86imm *res, int size, long value)
{
	assert(res != NULL);
	
	res->base.type = XIMMEDIATE;
	res->base.size = size;
	res->l = res->r = NULL;
	res->op = NULL;
	res->label = NULL;
	res->value = value;
}

static void new_x86_label(struct x86imm *res, char *value)
{
	/* I don't know if ISO true values are necessarily 1... */
	int uscorepfix = ((getos() == &oswindows) && value[0] != '.') ? 1 : 0;
	
	assert(res != NULL);
	assert(value != NULL);
	
	res->base.type = XIMMEDIATE;
	res->base.size = getcpu()->bits / 8;
	res->l = res->r = NULL;
	res->op = NULL;
	res->label = calloc(strlen(value) + 1 + uscorepfix, sizeof(char));
	if (uscorepfix)
		sprintf(res->label, "_%s", value);
	else
		sprintf(res->label, "%s", value);
	res->value = -1;
}

static void new_x86_cop(struct x86imm *res, const char *op,
	struct x86imm *l, struct x86imm *r)
{
	assert(res != NULL);
	assert(op != NULL);
	assert(l != NULL);
	assert(r != NULL);
	assert(l->base.size == r->base.size);
	
	res->base.type = XIMMEDIATE;
	res->base.size = l->base.size;
	res->l = l;
	res->r = r;
	res->op = op;
	res->label = NULL;
	res->value = -1;
}

static void delete_x86_imm(struct x86imm *imm)
{
	assert(imm != NULL);
	if (imm->label)
		free(imm->label);
}

static void new_x86_ea(struct x86ea *res, int size,
	const struct x86reg *base,
	struct x86imm *displacement,
	const struct x86reg *offset,
	int mult)
{
	assert(res != NULL);
	assert(getcpu()->offset >= cpui386.offset || mult == 1);
	
	res->base.type = XEA;
	res->base.size = size;
	res->basereg = base;
	res->displacement = displacement;
	res->offset = offset;
	res->mult = mult;
}

static void delete_x86_ea(struct x86ea *ea)
{
	/* stub for future compatibility */
}


static void x86l(FILE *f, struct x86imm *imm)
{
	assert(imm != NULL);
	assert(imm->label != NULL);
	
	fprintf(f, "%s:\n", imm->label);
}

static void x86i(FILE *f, const char *instr, int numops, ...)
{
	va_list ap;
	bool reqsuf = 0;
	struct x86e **ops = malloc(sizeof(struct x86e *) * numops);
	
	va_start(ap, numops);
	for (int i = 0; i < numops; ++i) {
		struct x86e *e = va_arg(ap, struct x86e *);
		assert(e != NULL);
		ops[i] = e;
		reqsuf |= (e->type != XIMMEDIATE);
	}
	
	fprintf(f, "\t%s", instr);
	if (option_asmflavor() == AF_ATT && reqsuf) {
		switch (ops[0]->size) {
		case 1:
			putc('b', f);
			break;
		case 2:
			putc('w', f);
			break;
		case 4:
			putc('l', f);
			break;
		case 8:
			putc('q', f);
			break;
		case 10:
			putc('t', f);
			break;
		}
	}

	if (numops)
		fprintf(f, " ");
	
	for (int i = 0; i < numops; ++i) {
		x86etostr(f, ops[i]);
		if (i != numops - 1)
			fprintf(f, ", ");
	}
	
	fprintf(f, "\n");
	
	va_end(ap);
	free(ops);
}

static void x86sdi(FILE *f, const char *instr,
	struct x86e *dest, struct x86e *src)
{
	if (option_asmflavor() == AF_ATT)
		x86i(f, instr, 2, src, dest);
	else
		x86i(f, instr, 2, dest, src);
}

static void x86etostr(FILE *f, struct x86e *e)
{
	struct x86reg *reg;
	switch (e->type) {
	case XREGISTER:
		reg = (struct x86reg *)e;
		if (option_asmflavor() == AF_ATT)
			putc('%', f);
		fprintf(f, "%s", reg->name);
		break;
	case XIMMEDIATE:
		x86immtostr(f, (struct x86imm *)e, 1);
		break;
	case XEA:
		x86eatostr(f, (struct x86ea *)e);
		break;
	}
}

static void x86immtostr(FILE *f, struct x86imm *imm, bool attprefix)
{
	if (imm->op) {
		fprintf(f, "(");
		x86immtostr(f, imm->l, attprefix);
		fprintf(f, " %s ", imm->op);
		x86immtostr(f, imm->r, attprefix);
		fprintf(f, ")");
	} else if (imm->label) {
		fprintf(f, "%s", imm->label);
	} else if (attprefix && option_asmflavor() == AF_ATT) {
		fprintf(f, "$%ld", imm->value);
	} else {
		fprintf(f, "%ld", imm->value);
	}
}

static void x86eatostr(FILE *f, struct x86ea *ea)
{
	if (option_asmflavor() == AF_ATT) {
		if (ea->displacement && (ea->basereg || ea->offset))
			x86immtostr(f, ea->displacement, false);
		fprintf(f, "(");
		if (ea->displacement && !(ea->basereg || ea->offset))
			x86immtostr(f, ea->displacement, false);
		if (ea->basereg)
			x86etostr(f, (struct x86e *)ea->basereg);
		if (ea->offset) {
			fprintf(f, ", ");
			x86etostr(f, (struct x86e *)ea->offset);
		}
		if (ea->mult > 1)
			fprintf(f, ", %d", ea->mult);
		fprintf(f, ")");
		return;
	}
	
	switch (ea->base.size) {
	case 1:
		fprintf(f, "byte");
		break;
	case 2:
		fprintf(f, "word");
		break;
	case 4:
		fprintf(f, "dword");
		break;
	case 8:
		fprintf(f, "qword");
		break;
	case 10:
		/* TODO: ? */
		break;
	}
	fprintf(f, " [");
	if (ea->basereg) {
		x86etostr(f, (struct x86e *)ea->basereg);
		if (ea->displacement || ea->offset)
			fprintf(f, " + ");
	}
	if (ea->displacement) {
		x86etostr(f, (struct x86e *)ea->displacement);
		if (ea->offset)
			fprintf(f, " + ");
	}
	if (ea->offset) {
		x86etostr(f, (struct x86e *)ea->offset);
		if (ea->mult > 1)
			fprintf(f, " * %d", ea->mult);
	}
	fprintf(f, "]");
}

static void x86global(FILE *f, struct x86imm *imm)
{
	assert(imm != NULL);
	assert(imm->label != NULL);
	
	if (option_asmflavor() == AF_ATT)
		fprintf(f, "\t.globl\t%s\n", imm->label);
	else 	
		fprintf(f, "global\t%s\n", imm->label);
}

static void x86extern(FILE *f, struct x86imm *imm)
{
	assert(imm != NULL);
	assert(imm->label != NULL);
	
	if (option_asmflavor() == AF_ATT)
		return;
	
	fprintf(f, "extern\t%s\n", imm->label);
}

static void x86sect(FILE *f, enum x86section sec)
{
	static enum x86section cursect = SECTION_INVALID;
	
	assert(sec != SECTION_INVALID);
	if (sec == cursect)
		return;
	cursect = sec;
	
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

static void x86reslike(FILE *f, const char *dir, size_t cnt, va_list ap)
{
	fprintf(f, "\t%s\t");
	for (int i = 0; i < cnt; ++i) {
		fprintf(f, "%s", va_arg(ap, char *));
		if (i != cnt - 1)
			fprintf(f, ", ");
	}
	fprintf(f, "\n");
}

static void x86byte(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		x86reslike(f, ".byte", cnt, ap);
	else
		x86reslike(f, "db", cnt, ap);
	va_end(ap);
}

static void x86short(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		x86reslike(f, ".short", cnt, ap);
	else
		x86reslike(f, "dw", cnt, ap);
	va_end(ap);
}

static void x86long(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		x86reslike(f, ".long", cnt, ap);
	else
		x86reslike(f, "dd", cnt, ap);
	va_end(ap);
}

static void x86quad(FILE *f, size_t cnt, ...)
{
	va_list ap;
	va_start(ap, cnt);
	if (option_asmflavor() == AF_ATT)
		x86reslike(f, ".quad", cnt, ap);
	else
		x86reslike(f, "dq", cnt, ap);
	va_end(ap);
}

static void x86asciizchar(FILE *f, char ch)
{
	if (isprint(ch) && ch != '"')
		fprintf(f, "%c", ch);
	else
		fprintf(f, "\\x%02x", (int)ch);
}

static void x86asciiz(FILE *f, const char *str)
{
	if (option_asmflavor() == AF_ATT)
		fprintf(f, "\t.asciz\t\"");
	else
		fprintf(f, "\tdb\t\"");
	
	size_t len = strlen(str);
	for (int i = 0; i < len; ++i) {
		x86asciizchar(f, str[i]);
	}
	
	if (option_asmflavor() != AF_ATT)
		fprintf(f, "\\0");
	fprintf(f, "\"\n");
}

void x86_emit(FILE *f, struct list *blocks)
{
	void *sym, *it = list_iterator(blocks);
	while (iterator_next(&it, &sym))
		x86_emit_symbol(f, sym);
}

static void x86_emit_symbol(FILE *f, struct symbol *sym)
{
	/* TODO: implement */
}
