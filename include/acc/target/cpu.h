/*
 * Skeleton for CPU implementations
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

#ifndef TARGET_CPU_H
#define TARGET_CPU_H

#include <acc/parsing/ast.h>

struct cpu {
	const char *name;
	int bits;
	int offset;
};

/*
 * List of CPUs in the architecture
 */
extern const struct cpu *cpus[];
/*
 * CPU to build for
 */
extern const struct cpu *cpu;

/*
 * Returns cpu (not to be implemented by platform implementations)
 */
const struct cpu *getcpu(void);
/*
 * Called by options_init() for options starting with '-mcpu' (not to be implemented by platform implementations)
 */
void archoption(const char *opt);

/*
 * Called by options_init() for other options starting with '-m'
 */
void xarchoption(const char *opt);

/*
 * Gets type size for the given type
 */
int gettypesize(struct ctype *ty);
/*
 * Gets field alignment for the given type
 */
int getfalign(struct ctype *ty);

#endif
