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
#include <stdint.h>

#include <acc/itm/ast.h>
#include <acc/itm/tag.h>

typedef void *asme_type_t;

extern asme_type_t asme_reg;
extern asme_type_t asme_imm;

extern itm_tag_type_t tt_loc, tt_color;

struct asme {
	asme_type_t *type;
	int64_t size;
	void (*to_string)(FILE *f, struct asme *e);
	void (*to_string_d)(FILE *f, struct asme *e);
};

typedef uint64_t regid_t;

struct asmreg {
	struct asme base;
	regid_t id;
	const char *name;
	const struct asmreg *parent;
	const struct asmreg *child1;
	const struct asmreg *child2;
};

#define NEW_REG(size, name, parent, ch1, ch2, id)			\
	{ { &asme_reg, size, &asmregtostr, &asmregtostr }, 		\
		1ul << (id), name, parent, ch1, ch2 }

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

struct location *new_loc_reg(size_t size, regid_t rid);
struct location *new_loc_regany(size_t size, int of);
struct location *new_loc_lmem(size_t size, int offset);
struct location *new_loc_pmem(size_t size, int offset);
struct location *new_loc_multiple(size_t size, struct list *locs);
void delete_loc(struct location *loc);
void loc_to_string(FILE *f, struct location *loc);

struct archdes {
	regid_t all_iregs;
	regid_t saved_iregs;
	regid_t all_fregs;
	regid_t saved_fregs;
};

void regalloc(struct itm_block *b, struct archdes rset);

#endif
