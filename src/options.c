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

static char *outfile = NULL;
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
static bool emit_ir = false;
static bool emit_asm = false;

static char *help[] = {
"Usage: acc [options] file...\n\
Options:\n\
  --help                   Display this information and exit\n\
  --help={extensions|target|warnings|optimizers}\n\
                           Display subject-specific help\n\
  --version                Display version information and exit\n\
  -std=<standard>          Interpret input files as being <standard>\n\
  -v                       Output verbose information\n\
  -Sir                     Parse only, and dump intermediate output\n\
  -S                       Parse and compile, but do not assemble or link, and\n\
                           dump assembly\n\
  -c                       Parse, compile and assemble, but do not link\n\
  -o <file>                Output to <file>\n\
\n\
Switches starting with -f, -m, -O and -W indicate extensions, target-specific\n\
 options, optimizations and warnings respectively. Information about them can be\n\
 displayed through the appropriate --help=... options.\n\
\n\
Report bugs at:\n\
<https://github.com/antonijn/acc/issues>.\n",

"  -fmixed-declarations     Allows intermingled code and declarations\n\
  -fpure                   Enables the __pure keyword used to declare functions\n\
                           without side-effects\n\
  -fbool                   Enables the _Bool type\n\
  -fundef                  Enables the __undef constant, which has no set value\n\
  -finline                 Enables the inline keyword, which hints for function\n\
                           inlining\n\
  -flong-long              Enables the long long integer type\n\
  -fvlas                   Enables variable-length arrays (VLAs)\n\
  -fcomplex                Enables the _Complex type\n\
  -fone-line-comments      Enables C++ style one-line comments\n\
  -fhex-floats             Enables hexadecimal floating point constants\n\
  -flong-double            Enables the long double type\n\
  -fdesignated-initializers\n\
                           Enables C99 designated initializers\n\
  -fcompound-literals      Enables C99 compound literals\n\
  -fvariadic-macros        Allows variadic macro definitions\n\
  -frestrict               Enables the C99 restrict keyword for no-alias\n\
                           pointers\n\
  -funiversal-character-names\n\
                           Enables unicode universal character names in string\n\
                           literals\n\
  -funicode-strings        Enables u8, u and U string-literal prefixes\n\
  -fbinary-literals        Enables 0b-prefixed binary integer literals\n\
  -fdigraphs               Enables C95 digraphs\n\
  -fdiagnostics-color      Signal the error reporter to colorize its output,\n\
                           and is enabled by default if the ACC_COLORS\n\
                           environment variable is set\n",

"  -masm=att, -masm=gas     Sets the x86 assembler dialect to AT&T/GAS syntax\n\
  -masm=nasm               Sets the x86 assembler dialect to NASM syntax\n\
  -masm=intel, -masm=masm  Sets the x86 assembler dialect to MASM syntax\n"
};

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
		if (!strcmp(arg, "--help")) {
			fprintf(stderr, help[0]);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--help=extensions")) {
			fprintf(stderr, help[1]);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--help=target")) {
			fprintf(stderr, help[2]);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--help=warnings")) {
			//fprintf(stderr, help[3]);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--help=optimizers")) {
			//fprintf(stderr, help[4]);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--version")) {
			fprintf(stderr, "acc " ACC_VERSION "\n");
			exit(EXIT_SUCCESS);
		}  else if (!strcmp(arg, "-o")) {
			if (++i >= argc)
				report(E_OPTIONS, NULL, "expected output file name");
			outfile = argv[i];
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
		} else if (!strcmp(arg, "-Sir")) {
			emit_ir = true;
		} else if (!strcmp(arg, "-S")) {
			emit_asm = true;
		} else if (arg[0] == '-' && arg[1] == 'f') {
			enableext(&arg[2]);
		} else if (arg[0] == '-' && arg[1] != '\0') {
			report(E_OPTIONS, NULL,
				"invalid command line option: %s", arg);
		} else {
			list_push_back(input, arg);
		}
	}

	if (list_length(input) == 0)
		report(E_OPTIONS | E_FATAL, NULL, "no input files specified");
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

bool option_emit_ir(void)
{
	return emit_ir;
}

bool option_emit_asm(void)
{
	return emit_asm;
}
