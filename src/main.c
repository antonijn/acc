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

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <locale.h>
#if !defined(NDEBUG) && defined(__GNU_LIBRARY__)
#include <execinfo.h>
#include <unistd.h>
#endif

#include <acc/itm/analyze.h>
#include <acc/itm/opt.h>
#include <acc/itm/ast.h>
#include <acc/parsing/file.h>
#include <acc/parsing/token.h>
#include <acc/options.h>
#include <acc/error.h>
#include <acc/target.h>

static void opt_and_dump(struct list *syms)
{
	char ofname[strlen(currentfile ? currentfile : "-") + 4];
	sprintf(ofname, "%s.ir", currentfile ? currentfile : "-");
	FILE *out;
	if (option_emit_ir()) {
		if (option_outfile())
			out = fopen(option_outfile(), "wb");
		else
			out = fopen(ofname, "wb");
	}

	struct itm_container *sym;
	void *it = list_iterator(syms);
	while (iterator_next(&it, (void **)&sym)) {
		if (!sym->block)
			continue;

		optimize(sym->block);
		if (option_emit_ir())
			itm_container_to_string(out, sym);
	}

	if (option_emit_ir())
		fclose(out);
}

static void compilefile(FILE *f)
{
	struct list *syms = new_list(NULL, 0);

	resettok();
	ast_init();

	if (setjmp(fatal_env))
		goto cleanup;

	parsefile(f, syms);
	opt_and_dump(syms);

	FILE *s;
	if (option_emit_asm()) {
		if (option_outfile()) {
			s = fopen(option_outfile(), "wb");
		} else {
			char ofname[strlen(currentfile ? currentfile : "-") + 3];
			sprintf(ofname, "%s.s", currentfile ? currentfile : "-");
			s = fopen(ofname, "wb");
		}
	} else {
		s = tmpfile();
	}
	getarch()->emitter(s, syms);
	fclose(s);

cleanup:
	delete_list(syms, NULL);
	ast_destroy();
}

#ifndef NDEBUG
static void segvcatcher(int signo)
{
	fprintf(stderr, "\n"
	                "\t\\|/ ____ \\|/\n"
	                "\t\"@'/ .. \\`@\"\n"
                        "\t/_| \\__/ |_\\\n"
	                "\t   \\__U_/\n\n");
	fprintf(stderr, "Oops! Received SIGSEGV during compilation!\n");
	fprintf(stderr, "\tFile:\t%s\n", currentfile ? currentfile : "<stdin>");
	fprintf(stderr, "\tLine:\t%d\n", get_line());
	fprintf(stderr, "\tColumn:\t%d\n\n", get_column());

#ifdef __GNU_LIBRARY__
	fprintf(stderr, "======= Backtrace: =======\n");

	void *buffer[100];
	size_t nptrs = backtrace(buffer, sizeof(buffer) / sizeof(void *));
	backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
	fprintf(stderr, "\n");
#endif

	abort();
}
#endif

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "C");

#ifndef NDEBUG
	if (signal(SIGSEGV, &segvcatcher) == SIG_ERR)
		fprintf(stderr, "warning: failed to register"
		                "segmentation fault handler\n");
#endif

	if (setjmp(fatal_env))
		exit(EXIT_FAILURE);

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
