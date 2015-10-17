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

/*
 * Extension codes
 */
enum exno {
	EX_MIXED_DECLARATIONS,
	EX_PURE,
	EX_BOOL,
	EX_UNDEF,
	EX_INLINE,
	EX_LONG_LONG,
	EX_VLAS,
	EX_COMPLEX,
	EX_ONE_LINE_COMMENTS,
	EX_HEX_FLOAT,
	EX_LONG_DOUBLE,
	EX_DESIGNATED_INITIALIZERS,
	EX_COMPOUND_LITERALS,
	EX_VARIADIC_MACROS,
	EX_RESTRICT,
	EX_UNIVERSAL_CHARACTER_NAMES,
	EX_UNICODE_STRINGS,
	EX_UNSIGNED_CHAR,
	EX_BINARY_LITERALS,
	EX_DIGRAPHS,
	EX_DIAGNOSTICS_COLOR,
	EX_COUNT
};

/*
 * Get extension code from string
 */
enum exno getex(const char *ext);

/*
 * Enable/disable extension
 */
void enableext(const char *ext);
void disableext(const char *ext);

/*
 * Enable extension by extension code
 */
void orext(enum exno ext);

/*
 * Returns whether extension is enabled
 */
bool isext(enum exno ext);

#endif
