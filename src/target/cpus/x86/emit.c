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

#include <acc/target/cpu/cpus.h>
#include <acc/target/emit.h>
#include <acc/target/cpu.h>
#include <acc/target/asm.h>
#include <acc/itm/ast.h>
#include <acc/itm/analyze.h>
#include <acc/parsing/ast.h>
#include <acc/options.h>

asme_type_t asme_x86ea;

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

static const struct asmreg ah = NEW_REG(1, "ah", &ax, NULL, NULL, 0);
static const struct asmreg bh = NEW_REG(1, "bh", &bx, NULL, NULL, 1);
static const struct asmreg ch = NEW_REG(1, "ch", &cx, NULL, NULL, 2);
static const struct asmreg dh = NEW_REG(1, "dh", &dx, NULL, NULL, 3);
static const struct asmreg al = NEW_REG(1, "al", &ax, NULL, NULL, 0);
static const struct asmreg bl = NEW_REG(1, "bl", &bx, NULL, NULL, 1);
static const struct asmreg cl = NEW_REG(1, "cl", &cx, NULL, NULL, 2);
static const struct asmreg dl = NEW_REG(1, "dl", &dx, NULL, NULL, 3);
static const struct asmreg spl = NEW_REG(1, "spl", &sp, NULL, NULL, 0);
static const struct asmreg bpl = NEW_REG(1, "bpl", &bp, NULL, NULL, 0);
static const struct asmreg sil = NEW_REG(1, "sil", &si, NULL, NULL, 4);
static const struct asmreg dil = NEW_REG(1, "dil", &di, NULL, NULL, 5);
static const struct asmreg r8b = NEW_REG(1, "r8b", &r8w, NULL, NULL, 6);
static const struct asmreg r9b = NEW_REG(1, "r9b", &r9w, NULL, NULL, 7);
static const struct asmreg r10b = NEW_REG(1, "r10b", &r10w, NULL, NULL, 8);
static const struct asmreg r11b = NEW_REG(1, "r11b", &r11w, NULL, NULL, 9);
static const struct asmreg r12b = NEW_REG(1, "r12b", &r12w, NULL, NULL, 10);
static const struct asmreg r13b = NEW_REG(1, "r13b", &r13w, NULL, NULL, 11);
static const struct asmreg r14b = NEW_REG(1, "r14b", &r14w, NULL, NULL, 12);
static const struct asmreg r15b = NEW_REG(1, "r15b", &r15w, NULL, NULL, 13);

static const struct asmreg ax = NEW_REG(2, "ax", &eax, &al, &ah, 0);
static const struct asmreg bx = NEW_REG(2, "bx", &ebx, &bl, &bh, 1);
static const struct asmreg cx = NEW_REG(2, "cx", &ecx, &cl, &ch, 2);
static const struct asmreg dx = NEW_REG(2, "dx", &edx, &dl, &dh, 3);
static const struct asmreg sp = NEW_REG(2, "sp", &esp, &spl, NULL, 0);
static const struct asmreg bp = NEW_REG(2, "bp", &ebp, &bpl, NULL, 0);
static const struct asmreg si = NEW_REG(2, "si", &esi, &sil, NULL, 4);
static const struct asmreg di = NEW_REG(2, "di", &edi, &dil, NULL, 5);
static const struct asmreg r8w = NEW_REG(2, "r8w", &r8d, &r8b, NULL, 6);
static const struct asmreg r9w = NEW_REG(2, "r9w", &r9d, &r9b, NULL, 7);
static const struct asmreg r10w = NEW_REG(2, "r10w", &r10d, &r10b, NULL, 8);
static const struct asmreg r11w = NEW_REG(2, "r11w", &r11d, &r11b, NULL, 9);
static const struct asmreg r12w = NEW_REG(2, "r12w", &r12d, &r12b, NULL, 10);
static const struct asmreg r13w = NEW_REG(2, "r13w", &r13d, &r13b, NULL, 11);
static const struct asmreg r14w = NEW_REG(2, "r14w", &r14d, &r14b, NULL, 12);
static const struct asmreg r15w = NEW_REG(2, "r15w", &r15d, &r15b, NULL, 13);

static const struct asmreg eax = NEW_REG(4, "eax", &rax, &ax, NULL, 0);
static const struct asmreg ebx = NEW_REG(4, "ebx", &rbx, &bx, NULL, 1);
static const struct asmreg ecx = NEW_REG(4, "ecx", &rcx, &cx, NULL, 2);
static const struct asmreg edx = NEW_REG(4, "edx", &rdx, &dx, NULL, 3);
static const struct asmreg esp = NEW_REG(4, "esp", &rsp, &sp, NULL, 0);
static const struct asmreg ebp = NEW_REG(4, "ebp", &rbp, &bp, NULL, 0);
static const struct asmreg esi = NEW_REG(4, "esi", &rsi, &si, NULL, 4);
static const struct asmreg edi = NEW_REG(4, "edi", &rdi, &di, NULL, 5);
static const struct asmreg r8d = NEW_REG(4, "r8d", &r8, &r8w, NULL, 6);
static const struct asmreg r9d = NEW_REG(4, "r9d", &r9, &r9w, NULL, 7);
static const struct asmreg r10d = NEW_REG(4, "r10d", &r10, &r10w, NULL, 8);
static const struct asmreg r11d = NEW_REG(4, "r11d", &r11, &r11w, NULL, 9);
static const struct asmreg r12d = NEW_REG(4, "r12d", &r12, &r12w, NULL, 10);
static const struct asmreg r13d = NEW_REG(4, "r13d", &r13, &r13w, NULL, 11);
static const struct asmreg r14d = NEW_REG(4, "r14d", &r14, &r14w, NULL, 12);
static const struct asmreg r15d = NEW_REG(4, "r15d", &r15, &r15w, NULL, 13);

static const struct asmreg rax = NEW_REG(8, "rax", NULL, &eax, NULL, 0);
static const struct asmreg rbx = NEW_REG(8, "rbx", NULL, &ebx, NULL, 1);
static const struct asmreg rcx = NEW_REG(8, "rcx", NULL, &ecx, NULL, 2);
static const struct asmreg rdx = NEW_REG(8, "rdx", NULL, &edx, NULL, 3);
static const struct asmreg rsp = NEW_REG(8, "rsp", NULL, &esp, NULL, 0);
static const struct asmreg rbp = NEW_REG(8, "rbp", NULL, &ebp, NULL, 0);
static const struct asmreg rsi = NEW_REG(8, "rsi", NULL, &esi, NULL, 4);
static const struct asmreg rdi = NEW_REG(8, "rdi", NULL, &edi, NULL, 5);
static const struct asmreg r8 = NEW_REG(8, "r8", NULL, &r8d, NULL, 6);
static const struct asmreg r9 = NEW_REG(8, "r9", NULL, &r9d, NULL, 7);
static const struct asmreg r10 = NEW_REG(8, "r10", NULL, &r10d, NULL, 8);
static const struct asmreg r11 = NEW_REG(8, "r11", NULL, &r11d, NULL, 9);
static const struct asmreg r12 = NEW_REG(8, "r12", NULL, &r12d, NULL, 10);
static const struct asmreg r13 = NEW_REG(8, "r13", NULL, &r13d, NULL, 11);
static const struct asmreg r14 = NEW_REG(8, "r14", NULL, &r14d, NULL, 12);
static const struct asmreg r15 = NEW_REG(8, "r15", NULL, &r15d, NULL, 13);

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

static void x86_emit_container(FILE *f, struct itm_container *sym);
static void x86_restrict(struct itm_block *b);

static void new_x86_ea(struct x86ea *res, int size,
	const struct asmreg *base,
	struct asmimm *displacement,
	const struct asmreg *offset,
	int mult)
{
	assert(res != NULL);
	assert(getcpu()->offset >= cpui386.offset || mult == 1);

	res->base.type = &asme_x86ea;
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

void emit(FILE *f, struct list *containers)
{
	void *cont;
	it_t it = list_iterator(containers);
	while (iterator_next(&it, &cont))
		x86_emit_container(f, cont);
}

static void x86_archdes(struct archdes *ades)
{
	// TODO: floating point regs

	memset(ades, 0, sizeof(struct archdes));
	int offs = getcpu()->offset;

	if (offs < cpux86_64.offset) {
		ades->all_iregs =
			eax.id | ebx.id | ecx.id | edx.id | edi.id | esi.id;
		ades->saved_iregs = ades->all_iregs & ~(eax.id | edx.id);
		return;
	}

	ades->all_iregs =
		rax.id | rbx.id | rcx.id | rdx.id | rdi.id | rsi.id | r15.id |
		r8.id | r9.id | r10.id | r11.id | r12.id | r13.id | r14.id;
	ades->saved_iregs =
		rbx.id | r12.id | r13.id | r14.id | r15.id;
}

static bool x86_isarith(struct itm_instr *i)
{
	return i->id == ITM_ID(itm_add) || i->id == ITM_ID(itm_sub) ||
	       i->id == ITM_ID(itm_mul) || i->id == ITM_ID(itm_div) ||
	       i->id == ITM_ID(itm_xor) || i->id == ITM_ID(itm_and) ||
	       i->id == ITM_ID(itm_or);
}

static bool x86_issymm(struct itm_instr *i)
{
	return i->id == ITM_ID(itm_add) || i->id == ITM_ID(itm_mul) ||
	       i->id == ITM_ID(itm_xor) || i->id == ITM_ID(itm_and) ||
	       i->id == ITM_ID(itm_or);
}

static void x86_emit_block(FILE *f, struct itm_block *b, struct list *bldict);

static void x86_emit_container(FILE *f, struct itm_container *c)
{
	x86_restrict(c->block);

	struct archdes des;
	x86_archdes(&des);
	regalloc(c->block, des);

	struct list *dict = new_list(NULL, 0);
	x86_emit_block(f, c->block, dict);
	delete_list(dict, NULL);

	//itm_container_to_string(f, c);
}

static void x86_restrictarith(struct itm_instr *i)
{
	if (!x86_isarith(i))
		return;

	if (x86_issymm(i))
		return;

	struct itm_expr *head = list_head(i->operands);
	if (head->etype == ITME_INSTRUCTION)
		return;

	struct itm_instr *mov = itm_mov(i->block, head);
	itm_inserti(mov, i);
	set_list_item(i->operands, 0, mov);
}

static void x86_restrictmul(struct itm_instr *i)
{
	if (i->id != ITM_ID(itm_mul))
		return;

	struct itm_expr *l = list_head(i->operands);
	if (!hastc(l->type, TC_UNSIGNED))
		return;

	struct location *actloc = new_loc_reg(i->base.type->size, rax.id);
	struct itm_instr *mov = itm_mov(i->block, l);
	struct itm_tag *loc = new_itm_tag(&tt_loc, TO_USER_PTR);
	itm_tag_set_user_ptr(loc, actloc, (void (*)(FILE *, void *))&loc_to_string);
	itm_tag_expr(&mov->base, loc);
	itm_inserti(mov, i);
	set_list_item(i->operands, 0, mov);

	actloc = new_loc_reg(i->base.type->size, rdx.id);
	struct itm_instr *clobb = itm_clobb(i->block);
	loc = new_itm_tag(&tt_loc, TO_USER_PTR);
	itm_tag_set_user_ptr(loc, actloc, (void (*)(FILE *, void *))&loc_to_string);
	itm_tag_expr(&clobb->base, loc);
	itm_inserti(clobb, i->next);
}

static void x86_restrictret(struct itm_instr *i)
{
	if (i->id != ITM_ID(itm_ret))
		return;

	if (hastc(i->base.type, TC_POINTER) ||
	    hastc(i->base.type, TC_INTEGRAL)) {
		struct itm_instr *mov = itm_mov(i->block, list_head(i->operands));
		struct itm_tag *loc = new_itm_tag(&tt_loc, TO_USER_PTR);
		struct location *actloc = new_loc_reg(i->base.type->size, rax.id);
		itm_tag_set_user_ptr(loc, actloc, (void (*)(FILE *, void *))&loc_to_string);
		itm_tag_expr(&mov->base, loc);
		itm_inserti(mov, i);
		set_list_item(i->operands, 0, mov);
	}
}

static void x86_restrict(struct itm_block *b)
{
	struct itm_instr *i = b->first;
	while (i) {
		// TODO: restrict for function calls
		x86_restrictmul(i);
		x86_restrictret(i);
		x86_restrictarith(i);
		i = i->next;
	}

	if (b->lexnext)
		x86_restrict(b->lexnext);
}


static struct itm_instr *x86_emiti(FILE *f, struct itm_instr *i);

static void x86_emit_block(FILE *f, struct itm_block *b, struct list *bldict)
{
	struct asmimm *lbl;

	if (!dict_get(bldict, b, (void **)&lbl)) {
		lbl = malloc(sizeof(struct asmimm));
		char lblid[3 + sizeof(int) * 3]; // size estimate
		sprintf(lblid, ".L%d", list_length(bldict) / 2);
		new_asm_label(lbl, lblid);
		dict_push_back(bldict, b, lbl);
	}

	emit_label(f, lbl);


	for (struct itm_instr *i = b->first; i; i = x86_emiti(f, i))
		;

	if (b->lexnext)
		x86_emit_block(f, b->lexnext, bldict);

	delete_asm_imm(lbl);
	free(lbl);
	list_pop_back(bldict);
	list_pop_back(bldict);
}

static struct asme *x86_getloce(struct location *loc, int size);
static struct asme *x86_getasme(struct asmimm *imm, struct itm_expr *e);
static struct itm_instr *x86_emiti_arith(FILE *f, struct itm_instr *i);
static struct itm_instr *x86_emiti_ret(FILE *f, struct itm_instr *i);

static struct asme *x86_getloce(struct location *loc, int size)
{
	// TODO: stack and parameter space memory
	struct loc_reg *lreg;

	switch (loc->type) {
	case LT_REG:
		lreg = loc->extended;

		const struct asmreg **av = regav[getcpu()->offset];
		for (int i = 0; av[i]; ++i) {
			const struct asmreg *reg = av[i];
			if (reg->id == lreg->rid && reg->base.size == size)
				return (struct asme *)&reg->base;
		}

		break;
	}

	assert(false);
}

static struct asme *x86_getasme(struct asmimm *imm, struct itm_expr *e)
{
	if (e->etype != ITME_INSTRUCTION) {
		if (!imm)
			return NULL;

		struct itm_literal *lit = (struct itm_literal *)e;
		new_asm_imm(imm, e->type->size, lit->value.i);
		return &imm->base;
	}

	struct itm_tag *restag = itm_get_tag(e, &tt_loc);
	assert(restag != NULL);
	struct location *loc = itm_tag_get_user_ptr(restag);
	assert(loc != NULL);
	return x86_getloce(loc, e->type->size);
}

static struct itm_instr *x86_emiti(FILE *f, struct itm_instr *i)
{
	assert(i != NULL);

	struct itm_instr *nxt;

	if ((nxt = x86_emiti_arith(f, i)) != i)
		return nxt;
	if ((nxt = x86_emiti_ret(f, i)) != i)
		return nxt;

	if (i->id == ITM_ID(itm_mov)) {
		struct asme *result = x86_getasme(NULL, &i->base);
		struct asmimm imm;
		struct itm_expr *firstop = list_head(i->operands);
		struct asme *firstope;
		if ((firstope = x86_getasme(&imm, firstop)) != result) {
			emit_sdi(f, "mov", result, firstope);
			if (firstope == &imm.base)
				delete_asm_imm(&imm);
		}
	}

	return i->next;
}

static struct itm_instr *x86_emiti_ret(FILE *f, struct itm_instr *i)
{
	if (i->id == ITM_ID(itm_leave) || i->id == ITM_ID(itm_ret)) {
		emit_i(f, "ret", 0);
		return i->next;
	}

	return i;
}

static struct itm_instr *x86_emiti_arith(FILE *f, struct itm_instr *i)
{
	const char *instrstr;

	instr_id_t id = i->id;
	if (id == ITM_ID(itm_add))
		instrstr = "add";
	else if (id == ITM_ID(itm_sub))
		instrstr = "sub";
	else if (id == ITM_ID(itm_mul))
		instrstr = "imul"; // TODO: unsigned multiply
	else if (id == ITM_ID(itm_div))
		instrstr = "idiv"; // TODO: unsigned divide
	else if (id == ITM_ID(itm_and))
		instrstr = "and";
	else if (id == ITM_ID(itm_xor))
		instrstr = "xor";
	else if (id == ITM_ID(itm_or))
		instrstr = "or";
	else
		return i;

	struct asmimm limm, rimm;
	struct asme *result = x86_getasme(NULL, &i->base);
	struct asme *le = x86_getasme(&limm, list_head(i->operands));
	struct asme *re = x86_getasme(&rimm, list_last(i->operands));

	// all the stuff that checks for this variable is basically dirty
	// it introduces a set of xchg instructions, which... isn't ideal...
	bool xchg = false;

	if (re == result) {
		if (x86_issymm(i)) {
			struct asme *tmpe = le;
			le = re;
			re = tmpe;
		} else {
			xchg = true;
		}
	}

	if (xchg) {
		// re == result here, just keep that in mind
		emit_sdi(f, "xchg", le, re);
		emit_sdi(f, instrstr, le, re);
		emit_sdi(f, "xchg", le, re);
	} else {
		if (le != result)
			emit_sdi(f, "mov", result, le);
		emit_sdi(f, instrstr, result, re);
	}

	if (re == &rimm.base)
		delete_asm_imm(&rimm);
	if (le == &limm.base)
		delete_asm_imm(&limm);

	return i->next;
}
