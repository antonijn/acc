/*
 * Compiler entry
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <acc/options.h>
#include <acc/token.h>
#include <acc/error.h>

static void compilefile(FILE * f)
{
	struct token tok;
	while (1) {
		tok = gettok(f);
		if (tok.type == T_EOF) {
			freetok(tok);
			break;
		}

		printf("%2d: %s\n", tok.type, tok.lexeme);

		freetok(tok);
	}
}

int main(int argc, char *argv[])
{
	char * filename;
	void * li;

	setlocale(LC_ALL, "C");
	options_init(argc, argv);

	li = list_iterator(option_input());
	while (filename = iterator_next(&li)) {
		FILE * file;

		if (!strcmp(filename, "-")) {
			compilefile(stdin);
			continue;
		}

		file = fopen(filename, "rb");
		if (!file)
			report(E_OPTIONS, NULL, strerror(errno));

		compilefile(file);
	}

	options_destroy();
	return EXIT_SUCCESS;
}
