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

#include <acc/target.h>

struct target target = { &oslinux, &abignu, &archx86, &pc };

os oslinux;
os oswindows;
os osx;

abi abignu;
abi abivc;

arch archx86;
arch archx86_64;

platform pc;

int gettypesize(struct ctype * ty)
{
	if (ty == &cchar || ty == &cuchar)
		return 1;
	if (ty == &cshort || ty == &cushort)
		return 2;
	if (ty == &cint || ty == &cuint)
		return 4;
	if (ty == &clong || ty == &culong)
		return (target.abi == &abivc) ? 4 : 8;
	
	if (ty == &cfloat)
		return 4;
	if (ty == &cdouble)
		return 8;
	
	/*if (ty == &cbool)
		return 1;*/
	
	if (ty->type == POINTER)
		return (target.arch == &archx86_64) ? 8 : 4;
	
	return 0;
}

int getfalign(struct ctype * ty)
{
	if (ty == &cdouble && target.abi == &abignu && target.arch == &archx86)
		return 4;
	return gettypesize(ty);
}
