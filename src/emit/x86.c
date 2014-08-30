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
#include <acc/itm/ast.h>
#include <acc/options.h>
#include <acc/target.h>

struct x86ea {
	struct asme base;
	const struct asmreg *basereg;
	struct asmimm *displacement;
	const struct asmreg *offset;
	int mult;
};

static const struct asmreg ah, bh, ch, dh;
static const struct asmreg al, bl, cl, dl, spl, bpl, sil, dil,
	r8b, r9b, r10b, r11b, r12b, r13b, r14b, r15b;
static const struct asmreg ax, bx, cx, dx, sp, bp, si, di,
	r8w, r9w, r10w, r11w, r12w, r13w, r14w, r15w;
static const struct asmreg eax, ebx, ecx, edx, esp, ebp, esi, edi,
	r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d;
static const struct asmreg rax, rbx, rcx, rdx, rsp, rbp, rsi, rdi,
	r8, r9, r10, r11, r12, r13, r14, r15;

static const struct asmreg ah = NEW_REG(1, "ah", &ax, NULL, NULL);
static const struct asmreg bh = NEW_REG(1, "bh", &bx, NULL, NULL);
static const struct asmreg ch = NEW_REG(1, "ch", &cx, NULL, NULL);
static const struct asmreg dh = NEW_REG(1, "dh", &dx, NULL, NULL);
static const struct asmreg al = NEW_REG(1, "al", &ax, NULL, NULL);
static const struct asmreg bl = NEW_REG(1, "bl", &bx, NULL, NULL);
static const struct asmreg cl = NEW_REG(1, "cl", &cx, NULL, NULL);
static const struct asmreg dl = NEW_REG(1, "dl", &dx, NULL, NULL);
static const struct asmreg spl = NEW_REG(1, "spl", &sp, NULL, NULL);
static const struct asmreg bpl = NEW_REG(1, "bpl", &bp, NULL, NULL);
static const struct asmreg sil = NEW_REG(1, "sil", &si, NULL, NULL);
static const struct asmreg dil = NEW_REG(1, "dil", &di, NULL, NULL);
static const struct asmreg r8b = NEW_REG(1, "r8b", &r8w, NULL, NULL);
static const struct asmreg r9b = NEW_REG(1, "r9b", &r9w, NULL, NULL);
static const struct asmreg r10b = NEW_REG(1, "r10b", &r10w, NULL, NULL);
static const struct asmreg r11b = NEW_REG(1, "r11b", &r11w, NULL, NULL);
static const struct asmreg r12b = NEW_REG(1, "r12b", &r12w, NULL, NULL);
static const struct asmreg r13b = NEW_REG(1, "r13b", &r13w, NULL, NULL);
static const struct asmreg r14b = NEW_REG(1, "r14b", &r14w, NULL, NULL);
static const struct asmreg r15b = NEW_REG(1, "r15b", &r15w, NULL, NULL);

static const struct asmreg ax = NEW_REG(2, "ax", &eax, &al, &ah);
static const struct asmreg bx = NEW_REG(2, "bx", &ebx, &bl, &bh);
static const struct asmreg cx = NEW_REG(2, "cx", &ecx, &cl, &ch);
static const struct asmreg dx = NEW_REG(2, "dx", &edx, &dl, &dh);
static const struct asmreg sp = NEW_REG(2, "sp", &esp, &spl, NULL);
static const struct asmreg bp = NEW_REG(2, "bp", &ebp, &bpl, NULL);
static const struct asmreg si = NEW_REG(2, "si", &esi, &sil, NULL);
static const struct asmreg di = NEW_REG(2, "di", &edi, &dil, NULL);
static const struct asmreg r8w = NEW_REG(2, "r8w", &r8d, &r8b, NULL);
static const struct asmreg r9w = NEW_REG(2, "r9w", &r9d, &r9b, NULL);
static const struct asmreg r10w = NEW_REG(2, "r10w", &r10d, &r10b, NULL);
static const struct asmreg r11w = NEW_REG(2, "r11w", &r11d, &r11b, NULL);
static const struct asmreg r12w = NEW_REG(2, "r12w", &r12d, &r12b, NULL);
static const struct asmreg r13w = NEW_REG(2, "r13w", &r13d, &r13b, NULL);
static const struct asmreg r14w = NEW_REG(2, "r14w", &r14d, &r14b, NULL);
static const struct asmreg r15w = NEW_REG(2, "r15w", &r15d, &r15b, NULL);

static const struct asmreg eax = NEW_REG(4, "eax", &rax, &ax, NULL);
static const struct asmreg ebx = NEW_REG(4, "ebx", &rbx, &bx, NULL);
static const struct asmreg ecx = NEW_REG(4, "ecx", &rcx, &cx, NULL);
static const struct asmreg edx = NEW_REG(4, "edx", &rdx, &dx, NULL);
static const struct asmreg esp = NEW_REG(4, "esp", &rsp, &sp, NULL);
static const struct asmreg ebp = NEW_REG(4, "ebp", &rbp, &bp, NULL);
static const struct asmreg esi = NEW_REG(4, "esi", &rsi, &si, NULL);
static const struct asmreg edi = NEW_REG(4, "edi", &rdi, &di, NULL);
static const struct asmreg r8d = NEW_REG(4, "r8d", &r8, &r8w, NULL);
static const struct asmreg r9d = NEW_REG(4, "r9d", &r9, &r9w, NULL);
static const struct asmreg r10d = NEW_REG(4, "r10d", &r10, &r10w, NULL);
static const struct asmreg r11d = NEW_REG(4, "r11d", &r11, &r11w, NULL);
static const struct asmreg r12d = NEW_REG(4, "r12d", &r12, &r12w, NULL);
static const struct asmreg r13d = NEW_REG(4, "r13d", &r13, &r13w, NULL);
static const struct asmreg r14d = NEW_REG(4, "r14d", &r14, &r14w, NULL);
static const struct asmreg r15d = NEW_REG(4, "r15d", &r15, &r15w, NULL);

static const struct asmreg rax = NEW_REG(8, "eax", NULL, &eax, NULL);
static const struct asmreg rbx = NEW_REG(8, "ebx", NULL, &ebx, NULL);
static const struct asmreg rcx = NEW_REG(8, "ecx", NULL, &ecx, NULL);
static const struct asmreg rdx = NEW_REG(8, "edx", NULL, &edx, NULL);
static const struct asmreg rsp = NEW_REG(8, "esp", NULL, &esp, NULL);
static const struct asmreg rbp = NEW_REG(8, "ebp", NULL, &ebp, NULL);
static const struct asmreg rsi = NEW_REG(8, "esi", NULL, &esi, NULL);
static const struct asmreg rdi = NEW_REG(8, "edi", NULL, &edi, NULL);
static const struct asmreg r8 = NEW_REG(8, "r8d", NULL, &r8d, NULL);
static const struct asmreg r9 = NEW_REG(8, "r9d", NULL, &r9d, NULL);
static const struct asmreg r10 = NEW_REG(8, "r10d", NULL, &r10d, NULL);
static const struct asmreg r11 = NEW_REG(8, "r11d", NULL, &r11d, NULL);
static const struct asmreg r12 = NEW_REG(8, "r12d", NULL, &r12d, NULL);
static const struct asmreg r13 = NEW_REG(8, "r13d", NULL, &r13d, NULL);
static const struct asmreg r14 = NEW_REG(8, "r14d", NULL, &r14d, NULL);
static const struct asmreg r15 = NEW_REG(8, "r15d", NULL, &r15d, NULL);

// list of available registers per platform
static const struct asmreg *regav8086[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di, NULL
};

static const struct asmreg *regavi386[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di,
	&eax, &ebx, &edx, &ecx, &esp, &ebp, &esi, &edi, NULL
};

static const struct asmreg *regavi686[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di,
	&eax, &ebx, &edx, &ecx, &esp, &ebp, &esi, &edi, NULL
};

static const struct asmreg *regavamd64[] = {
	&ah, &al, &bh, &bl, &ch, &cl, &dh, &dl,
	&spl, &bpl, &sil, &dil,
	&ax, &bx, &cx, &dx, &sp, &bp, &si, &di,
	&eax, &ebx, &edx, &ecx, &esp, &ebp, &esi, &edi,
	
	&r8b, &r9b, &r10b, &r11b, &r12b, &r14b, &r15b,
	&r8w, &r9w, &r10w, &r11w, &r12w, &r14w, &r15w,
	&r8d, &r9d, &r10d, &r11d, &r12d, &r14d, &r15d,
	&r8, &r9, &r10, &r11, &r12, &r14, &r15, NULL
};

static const struct asmreg **regav[] = {
	&regav8086[0], &regavi386[0], &regavi686[0], &regavamd64[0]
};

static void new_x86_ea(struct x86ea *res, int size,
	const struct asmreg *base,
	struct asmimm *displacement,
	const struct asmreg *offset,
	int mult);
static void delete_x86_ea(struct x86ea *ea);

static void x86eatostr(FILE *f, struct asme *ea);

static void x86_emit_symbol(FILE *f, struct symbol *sym);

static void new_x86_ea(struct x86ea *res, int size,
	const struct asmreg *base,
	struct asmimm *displacement,
	const struct asmreg *offset,
	int mult)
{
	assert(res != NULL);
	assert(getcpu()->offset >= cpui386.offset || mult == 1);
	
	res->base.to_string = &x86eatostr;
	res->base.to_string_d = &x86eatostr;
	res->base.size = size;
	res->basereg = base;
	res->displacement = displacement;
	res->offset = offset;
	res->mult = mult;
}

static void delete_x86_ea(struct x86ea *ea)
{
	// stub for future compatibility
}

static void x86eatostr(FILE *f, struct asme *e)
{
	struct x86ea *ea = (struct x86ea *)e;
	if (option_asmflavor() == AF_ATT) {
		if (ea->displacement && (ea->basereg || ea->offset))
			ea->displacement->base.to_string_d(
				f, (struct asme *)ea->displacement);
		fprintf(f, "(");
		if (ea->displacement && !(ea->basereg || ea->offset))
			ea->displacement->base.to_string(
				f, (struct asme *)ea->displacement);
		if (ea->basereg)
			ea->basereg->base.to_string(
				f, (struct asme *)ea->basereg);
		if (ea->offset) {
			fprintf(f, ", ");
			ea->offset->base.to_string(
				f, (struct asme *)ea->offset);
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
		// TODO: ?
		break;
	}
	fprintf(f, " [");
	if (ea->basereg) {
		ea->basereg->base.to_string(f, (struct asme *)ea->basereg);
		if (ea->displacement || ea->offset)
			fprintf(f, " + ");
	}
	if (ea->displacement) {
		ea->displacement->base.to_string(
			f, (struct asme *)ea->displacement);
		if (ea->offset)
			fprintf(f, " + ");
	}
	if (ea->offset) {
		ea->offset->base.to_string(f, (struct asme *)ea->offset);
		if (ea->mult > 1)
			fprintf(f, " * %d", ea->mult);
	}
	fprintf(f, "]");
}

void x86_emit(FILE *f, struct list *blocks)
{
	void *sym, *it = list_iterator(blocks);
	while (iterator_next(&it, &sym))
		x86_emit_symbol(f, sym);
}

static void x86_emit_symbol(FILE *f, struct symbol *sym)
{
	// TODO: implement
}
