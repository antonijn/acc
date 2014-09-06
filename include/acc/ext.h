/*
 * Utilities for managing language extensions
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

#ifndef ACC_EXT_H
#define ACC_EXT_H

enum exno {
	EX_MIXED_DECLARATIONS = 0x1,
	EX_PURE = 0x2,
	EX_BOOL = 0x4,
	EX_UNDEF = 0x8,
	EX_INLINE = 0x10,
	EX_LONG_LONG = 0x20,
	EX_VLAS = 0x40,
	EX_COMPLEX = 0x80,
	EX_ONE_LINE_COMMENTS = 0x100,
	EX_HEX_FLOAT = 0x200,
	EX_LONG_DOUBLE = 0x400,
	EX_DESIGNATED_INITIALIZERS = 0x800,
	EX_COMPOUND_LITERALS = 0x1000,
	EX_VARIADIC_MACROS = 0x2000,
	EX_RESTRICT = 0x4000,
	EX_UNIVERSAL_CHARACTER_NAMES = 0x8000,
	EX_UNICODE_STRINGS = 0x10000,
	EX_UNSIGNED_CHAR = 0x20000,
	EX_BINARY_LITERALS = 0x40000
};

enum exno getex(const char *ext);
void enableext(const char *ext);
void disableext(const char *ext);
bool isext(enum exno ext);

#endif
