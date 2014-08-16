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

#include <string.h>

#include <acc/ext.h>
#include <acc/error.h>

struct extension {
	const char * str;
	enum exno no;
};

static struct extension extensions[] = {
	{ "mixed-declarations", EX_MIXED_DECLARATIONS },
	{ "pure", EX_PURE },
	{ "bool", EX_BOOL },
	{ "undef", EX_UNDEF },
	{ "inline", EX_INLINE },
	{ "long-long", EX_LONG_LONG },
	{ "vlas", EX_VLAS },
	{ "complex", EX_COMPLEX },
	{ "one-line-comments", EX_ONE_LINE_COMMENTS },
	{ "hex-float", EX_HEX_FLOAT },
	{ "long-double", EX_LONG_DOUBLE },
	{ "designated-initializers", EX_DESIGNATED_INITIALIZERS },
	{ "compound-literals", EX_COMPOUND_LITERALS },
	{ "variadic-macros", EX_VARIADIC_MACROS },
	{ "restrict", EX_RESTRICT },
	{ "universal-character-names", EX_UNIVERSAL_CHARACTER_NAMES },
	{ "unicode-strings", EX_UNICODE_STRINGS },
	{ "unsigned-char", EX_UNSIGNED_CHAR },
	{ "binary-literals", EX_BINARY_LITERALS }
};

static enum exno enabled = 0;

enum exno getex(const char * ext)
{
	int i;
	for (i = 0; i < sizeof(extensions) / sizeof(struct extension); ++i) {
		struct extension ex = extensions[i];
		if (!strcmp(ext, ex.str))
			return ex.no;
	}
	report(E_WARNING | E_HIDE_TOKEN | E_HIDE_LOCATION,
		NULL, "extension not found: \"%s\"", ext);
	return -1;
}

void enableext(const char * ext)
{
	if (!strncmp(ext, "no-", 3))
		enabled &= ~getex(&ext[3]);
	else
		enabled |= getex(ext);
}

void disableext(const char * ext)
{
	if (!strncmp(ext, "no-", 3))
		enabled |= getex(&ext[3]);
	else
		enabled &= ~getex(ext);
}

int isext(enum exno ext)
{
	return (enabled & ext) == ext;
}
