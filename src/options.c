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

static char *outfile = "a.out";
static struct list *input;
static int optimize = 0;
static int warnings = 1;
static enum asmflavor flavor =
#if defined(BUILDFOR_LINUX) || defined(BUILDFOR_OSX)
	AF_ATT;
#elif defined(BUILDFOR_WINDOWS)
	AF_MASM;
#else
#error No target defined
	-1;
#endif

void options_init(int argc, char *argv[])
{
	int i;
	input = new_list(NULL, 0);

	for (i = 1; i < argc; ++i) {
		char *arg = argv[i];
		if (!strcmp(arg, "-o")) {
			if (++i >= argc)
				report(E_OPTIONS, NULL, "expected output file name");
			outfile = argv[++i];
		} else if (!strcmp(arg, "-w")) {
			warnings = 0;
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
		} else if (arg[0] == '-' && arg[1] == 'f') {
			enableext(&arg[2]);
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

int option_warnings(void)
{
	return warnings;
}
