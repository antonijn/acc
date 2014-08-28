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
#include <locale.h>

#include <acc/options.h>
#include <acc/token.h>
#include <acc/error.h>
#include <acc/target.h>
#include <acc/parsing/file.h>

static void compilefile(FILE *f)
{
	struct list *syms = new_list(NULL, 0);
	resettok();
	ast_init();
	parsefile(f, syms);
	getarch()->emitter(stdout, syms);
	delete_list(syms, NULL);
	ast_destroy();
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "C");
	options_init(argc, argv);

	void *li = list_iterator(option_input());
	while (iterator_next(&li, (void **)&currentfile)) {
		FILE *file;

		if (!strcmp(currentfile, "-")) {
			currentfile = NULL;
			compilefile(stdin);
			continue;
		}

		file = fopen(currentfile, "rb");
		if (!file)
			report(E_OPTIONS, NULL, "file not found: \"%s\"", currentfile);

		compilefile(file);
		fclose(file);
	}

	options_destroy();
	return EXIT_SUCCESS;
}
