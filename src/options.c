/*
 * Command line argument parser and option hub
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

#include <acc/options.h>
#include <acc/error.h>
#include <acc/ext.h>

static char *outfile = "a.out";
static struct list *input;
static int optimize = 0;
static bool warnings = true;
static enum asmflavor flavor =
#if defined(BUILDFOR_LINUX) || defined(BUILDFOR_OSX)
	AF_ATT;
#elif defined(BUILDFOR_WINDOWS)
	AF_MASM;
#else
#error No target defined
	-1;
#endif

enum cversion {
	C89,
	C95,
	C99
};

static void setcversion(enum cversion version)
{
	switch (version) {
	case C99:
		orext(EX_MIXED_DECLARATIONS);
		orext(EX_BOOL);
		orext(EX_INLINE);
		orext(EX_LONG_LONG);
		orext(EX_VLAS);
		orext(EX_COMPLEX);
		orext(EX_ONE_LINE_COMMENTS);
		orext(EX_HEX_FLOAT);
		orext(EX_LONG_DOUBLE);
		orext(EX_DESIGNATED_INITIALIZERS);
		orext(EX_COMPOUND_LITERALS);
		orext(EX_VARIADIC_MACROS);
		orext(EX_RESTRICT);
		orext(EX_UNIVERSAL_CHARACTER_NAMES);
		orext(EX_DIGRAPHS);
	case C95:
		orext(EX_DIGRAPHS);
		orext(EX_UNIVERSAL_CHARACTER_NAMES);
	}
}

void options_init(int argc, char *argv[])
{
	input = new_list(NULL, 0);

	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		if (!strcmp(arg, "-o")) {
			if (++i >= argc)
				report(E_OPTIONS, NULL, "expected output file name");
			outfile = argv[++i];
		} else if (!strcmp(arg, "-w")) {
			warnings = false;
		} else if (!strcmp(arg, "-O0")) {
			optimize = 0;
		} else if (!strcmp(arg, "-O1")) {
			optimize = 1;
		} else if (!strcmp(arg, "-O2")) {
			optimize = 2;
		} else if (!strcmp(arg, "-O3")) {
			optimize = 3;
		} else if (!strcmp(arg, "-masm=att") || !strcmp(arg, "-masm=gas")) {
			flavor = AF_ATT;
		} else if (!strcmp(arg, "-masm=nasm")) {
			flavor = AF_NASM;
		} else if (!strcmp(arg, "-masm=intel") || !strcmp(arg, "-masm=masm")) {
			flavor = AF_MASM;
		} else if (!strcmp(arg, "-std=c89")) {
			setcversion(C89);
		} else if (!strcmp(arg, "-std=c95")) {
			setcversion(C95);
		} else if (!strcmp(arg, "-std=c99")) {
			setcversion(C99);
		} else if (arg[0] == '-' && arg[1] == 'f') {
			enableext(&arg[2]);
		} else if (arg[0] == '-') {
			report(E_OPTIONS, NULL,
				"invalid command-line option: %s", arg);
		} else {
			list_push_back(input, arg);
		}
	}
}

void options_destroy(void)
{
	delete_list(input, NULL);
}

char *option_outfile(void)
{
	return outfile;
}

int option_optimize(void)
{
	return optimize;
}

struct list *option_input(void)
{
	return input;
}

enum asmflavor option_asmflavor(void)
{
	return flavor;
}

bool option_warnings(void)
{
	return warnings;
}
