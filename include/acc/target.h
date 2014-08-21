/*
 * Target specifications
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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <acc/ast.h>

struct os {
	const char *name;
};

struct arch {
	const char *name;
	const struct cpu **cpus;
};

struct cpu {
	const char *name;
	const struct arch *arch;
	int bits;
	int offset;
};

extern const struct os oslinux, oswindows, osx;
extern const struct arch archx86;
extern const struct cpu cpu8086, cpui386, cpui686, cpuamd64;

const struct os *osbyname(const char *name);
const struct arch *archbyname(const char *name);
const struct cpu *cpubyname(const char *name);

const struct os *getos(void);
const struct arch *getarch(void);
const struct cpu *getcpu(void);

void setos(const struct os *o);
void setarch(const struct arch *a);
void setcpu(const struct cpu *c);

int gettypesize(struct ctype *ty);
int getfalign(struct ctype *ty);

#endif
