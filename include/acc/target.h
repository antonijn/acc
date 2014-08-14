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

typedef const int os;
typedef const int abi;
typedef const int arch;
typedef const int platform;

struct target {
	os *os;
	abi *abi;
	arch *arch;
	platform *platform;
};

extern struct target target;

extern os oslinux;
extern os oswindows;
extern os osx;

extern abi abignu;
extern abi abivc;

extern arch archx86;
extern arch archx86_64;

extern platform pc;

int gettypesize(struct ctype * ty);
int getfalign(struct ctype * ty);

#endif
