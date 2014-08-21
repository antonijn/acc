/*
 * Error reporting utility
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <acc/error.h>
#include <acc/token.h>
#include <acc/options.h>

const char *currentfile = NULL;

void report(enum errorty ty, struct token *tok, const char *frmt, ...)
{
	va_list ap;
	if (!option_warnings() && (ty & E_WARNING))
		return;

	if ((ty & E_HIDE_LOCATION) == 0)
		fprintf(stderr, "%s:%d:%d: ", currentfile ?
			currentfile : "\?\?\?", get_line(), get_column());

	if (ty & E_FATAL)
		fprintf(stderr, "FATAL: ");
	else if (ty & E_WARNING)
		fprintf(stderr, "warning: ");
	else
		fprintf(stderr, "error: ");

	va_start(ap, frmt);
	vfprintf(stderr, frmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
	if ((ty & E_HIDE_TOKEN) == 0)
		fprintf(stderr, "\t%s\n", tok->lexeme);

	if (ty & E_FATAL)
		exit(1);
}
