/*
 * Command line argument parser and option hub
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <acc/list.h>
#include <stdlib.h>
#include <stdio.h>

enum arch {
	ARCH_INVALID,
	ARCH_I386,
	ARCH_I686,
	ARCH_8086,
	ARCH_AMD64
};

void options_init(int argc, char * argv[]);
void options_destroy(void);

char * option_outfile(void);
int option_optimize(void);
struct list * option_input(void);
enum arch option_arch(void);
int option_warnings(void);

#endif
