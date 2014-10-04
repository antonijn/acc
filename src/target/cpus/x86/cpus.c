/*
 * x86 CPU definitions
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

#include <acc/target/cpu.h>

const struct cpu cpu8086 = { "8086", 16, 0 };
const struct cpu cpui386 = { "i386", 32, 1 };
const struct cpu cpui686 = { "i686", 32, 2 };
const struct cpu cpux86_64 = { "x86_64", 64, 3 };

const struct cpu *cpus[] = {
	&cpu8086, &cpui386, &cpui686, &cpux86_64, NULL
};

int gettypesize(struct ctype *ty)
{
	if (ty == &cbool)
		return 1;
	if (ty == &cfloat)
		return 4;
	if (ty == &cdouble)
		return 8;
	if (ty == &clonglong)
		return 8;

	if (ty == &clongdouble)
		return 10;

	switch (cpu->bits) {
	case 16:
		if (ty == &cchar)
			return 1;
		if (ty == &cshort)
			return 2;
		if (ty == &cint)
			return 2;
		if (ty == &clong)
			return 4;
		break;
	case 32:
		/*if (os == &oswindows && ty == &clong)
			return 4;*/
	case 64:
		if (ty == &cchar)
			return 1;
		if (ty == &cshort)
			return 2;
		if (ty == &cint)
			return 4;
		if (ty == &clong)
			return 8;
		break;
	}

	if (ty->type == POINTER)
		return cpu->bits / 8;

	return -1;
}

int getfalign(struct ctype *ty)
{
	/*if (arch == &archx86 && os == &oslinux && ty == &cdouble)
		return 4;*/
	return gettypesize(ty);
}
