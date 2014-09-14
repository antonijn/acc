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
#include <assert.h>

#include <acc/parsing/token.h>
#include <acc/error.h>
#include <acc/options.h>
#include <acc/ext.h>
#include <acc/term.h>

const char *currentfile = NULL;
jmp_buf fatal_env;

void report(enum errorty ty, struct token *tok, const char *frmt, ...)
{
	va_list ap;
	if (!option_warnings() && (ty & E_WARNING))
		return;

	bool colors =
#ifndef BUILDFOR_WINDOWS
		isext(EX_DIAGNOSTICS_COLOR) || getenv("ACC_COLORS");
#else
		false;
#endif

	fprintf(stderr, ANSI_BOLD(colors));

	if (!(ty & E_HIDE_LOCATION))
		fprintf(stderr, "%s:%d:%d: ", currentfile ?
			currentfile : "<stdin>", get_line(), get_column());

	if (ty & E_FATAL) {
		fprintf(stderr, ANSI_RED(colors));
		fprintf(stderr, "FATAL: ");
	} else if (ty & E_WARNING) {
		fprintf(stderr, ANSI_MAGENTA(colors));
		fprintf(stderr, "warning: ");
	} else {
		fprintf(stderr, ANSI_RED(colors));
		fprintf(stderr, "error: ");
	}

	fprintf(stderr, ANSI_RESET(colors));

	va_start(ap, frmt);
	vfprintf(stderr, frmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
	if (!(ty & E_HIDE_TOKEN) && tok->linestr) {
		fprintf(stderr, "%s\n", tok->linestr);
		for (int i = 0; i < tok->column - 1; ++i) {
			if (tok->linestr[i] == '\t')
				fprintf(stderr, "\t");
			else
				fprintf(stderr, " ");
		}
		fprintf(stderr, ANSI_BOLD(colors));
		fprintf(stderr, ANSI_GREEN(colors));
		fprintf(stderr, "^");
		fprintf(stderr, ANSI_RESET(colors));
		fprintf(stderr, "\n");
	}

	if (ty & E_FATAL)
		longjmp(fatal_env, 1);
}
