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

#ifndef EMIT_ASM_H
#define EMIT_ASM_H

#include <stdio.h>

typedef void *asme_type_t;

extern asme_type_t asme_reg;
extern asme_type_t asme_imm;

struct asme {
	asme_type_t *type;
	int size;
	void (*to_string)(FILE *f, struct asme *e);
	void (*to_string_d)(FILE *f, struct asme *e);
};

struct asmreg {
	struct asme base;
	const char *name;
	const struct asmreg *parent;
	const struct asmreg *child1;
	const struct asmreg *child2;
};

#define NEW_REG(size, name, parent, ch1, ch2) 				\
	{ { &asme_reg, size, &asmregtostr, &asmregtostr }, 		\
		name, parent, ch1, ch2 }

struct asmimm {
	struct asme base;
	struct asmimm *l, *r;
	const char *op;
	char *label;
	long value;
};

enum section {
	SECTION_INVALID,
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_RODATA,
	SECTION_BSS
};

void new_asm_imm(struct asmimm *res, int size, long value);
void new_asm_label(struct asmimm *res, char *value);
void new_asm_cop(struct asmimm *res, const char *op,
	struct asmimm *l, struct asmimm *r);
void delete_asm_imm(struct asmimm *imm);

void asmregtostr(FILE *f, struct asme *e);

void emit_label(FILE *f, struct asmimm *imm);
void emit_i(FILE *f, const char *instr, int numops, ...);
void emit_sdi(FILE *f, const char *instr, struct asme *src, struct asme *dest);
void emit_global(FILE *f, struct asmimm *imm);
void emit_extern(FILE *f, struct asmimm *imm);
void emit_section(FILE *f, enum section sec);
void emit_byte(FILE *f, size_t cnt, ...);
void emit_short(FILE *f, size_t cnt, ...);
void emit_long(FILE *f, size_t cnt, ...);
void emit_quad(FILE *f, size_t cnt, ...);
void emit_asciiz(FILE *f, const char *str);

#endif
