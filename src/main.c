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
#include <acc/parsing/file.h>

static void compilefile(FILE * f)
{
	struct list * syms = new_list(NULL, 0);
	parsefile(f, syms);
	delete_list(syms, NULL);
}

int main(int argc, char *argv[])
{
	char * filename;
	void * li;

	setlocale(LC_ALL, "C");
	options_init(argc, argv);
	ast_init();

	li = list_iterator(option_input());
	while (iterator_next(&li, (void **)&filename)) {
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

	ast_destroy();
	options_destroy();
	return EXIT_SUCCESS;
}
