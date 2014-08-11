/*
 * Utilities for intermediate representation
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

#ifndef INTERMEDIATE_H
#define INTERMEDIATE_H

#include <acc/list.h>
#include <acc/ast.h>

enum itm_expr_type {
	ITME_LITERAL,
	ITME_SYMBOL,
	ITME_PARAMETER,
	ITME_INSTRUCTION
};

struct itm_expr {
	enum itm_expr_type etype;
	struct ctype * type;
#ifndef NDEBUG
	void (*to_string)(FILE * f, struct itm_expr * e);
#endif
};

struct itm_instr {
	struct itm_expr base;
	
	void (*id)(void);
	const char * operation;
	int isterminal;

	struct list * operands;
	struct ctype * typeoperand;

	struct itm_instr * next;
	struct itm_instr * previous;
	struct itm_block * block;
};

#ifndef NDEBUG
int itm_instr_number(struct itm_instr * l);
void itm_instr_to_string(FILE * f, struct itm_instr * i);
#endif

struct itm_literal {
	struct itm_expr base;

	union {
		unsigned long i;
		float f;
		double d;
	} value;
};

struct itm_instr *itm_add(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_sub(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_mul(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_imul(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_div(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_idiv(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_rem(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_shl(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_shr(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_sal(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_sar(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);

struct itm_instr *itm_bitcast(struct itm_instr * i, struct itm_expr * l, struct ctype * to);
struct itm_instr *itm_trunc(struct itm_instr * i, struct itm_expr * l, struct ctype * to);
struct itm_instr *itm_zext(struct itm_instr * i, struct itm_expr * l, struct ctype * to);
struct itm_instr *itm_sext(struct itm_instr * i, struct itm_expr * l, struct ctype * to);
struct itm_instr *itm_itof(struct itm_instr * i, struct itm_expr * l, struct ctype * to);
struct itm_instr *itm_ftoi(struct itm_instr * i, struct itm_expr * l, struct ctype * to);

struct itm_instr *itm_getptr(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_deepptr(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);
struct itm_instr *itm_alloca(struct itm_instr * i, struct ctype * ty);
struct itm_instr *itm_load(struct itm_instr * i, struct itm_expr * l);
struct itm_instr *itm_store(struct itm_instr * i, struct itm_expr * l, struct itm_expr * r);

struct itm_instr *itm_jmp(struct itm_instr * i, struct itm_block * b);
struct itm_instr *itm_split(struct itm_instr * i, struct itm_expr * c, struct itm_block * t, struct itm_block * e);
struct itm_instr *itm_ret(struct itm_instr * i, struct itm_expr * l);
struct itm_instr *itm_leave(struct itm_instr * i);

struct itm_block {
#ifndef NDEBUG
	int number;
#endif
	struct itm_block * previous;
	struct list * next;
	struct itm_instr * first;
};

struct itm_block new_itm_block(struct itm_block * previous);
void delete_itm_block(struct itm_block * block);

void itm_instr_to_block(struct itm_block * block, struct itm_instr * i);
#ifndef NDEBUG
void itm_block_to_string(struct itm_block * block);
#endif

enum itm_flags {
	ITMF_NONE   = 0x0000,
	ITMF_GLOBAL = 0x0001,
	ITMF_PURE   = 0x0002,
	ITMF_HIDDEN = 0x0004,
	ITMF_CONST  = 0x0008
};

struct itm_symbol {
	struct itm_expr base;

	enum itm_flags flags;
	char * id;
	struct itm_expr * initial;
	struct itm_block * block;
};

struct itm_symbol * new_itm_symbol(enum itm_flags flags,
                                   char * id,
                                   struct itm_expr * initial,
                                   struct itm_block * block);
#ifndef NDEBUG
void itm_symbol_to_string(struct itm_symbol * sym);
#endif

struct itm_module {
	struct list * symbols;
};

struct itm_module new_itm_module(void);
void delete_itm_module(struct itm_module mod);
#ifndef NDEBUG
void itm_module_to_string(struct itm_module * itm);
#endif

#endif
