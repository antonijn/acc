#include <acc/options.h>
#include <acc/error.h>
#include <string.h>

static char * outfile = "a.out";
static enum arch arch = ARCH_INVALID;
static struct list * input;
static int optimize = 0;
static int warnings = 1;

void options_init(int argc, char * argv[])
{
	int i;
	input = new_list(NULL, 0);

	for (i = 1; i < argc; ++i) {
		char * arg = argv[i];
		if (!strcmp(arg, "-o")) {
			if (++i >= argc)
				report(E_OPTIONS, NULL, "expected output file name");
			outfile = argv[++i];
		} else if (!strcmp(arg, "-w"))
			warnings = 0;
		else if (!strcmp(arg, "-O0"))
			optimize = 0;
		else if (!strcmp(arg, "-O1"))
			optimize = 1;
		else if (!strcmp(arg, "-O2"))
			optimize = 2;
		else if (!strcmp(arg, "-O3"))
			optimize = 3;
		else if (!strcmp(arg, "-march=i386"))
			arch = ARCH_I386;
		else if (!strcmp(arg, "-march=i686"))
			arch = ARCH_I686;
		else if (!strcmp(arg, "-march=8086"))
			arch = ARCH_8086;
		else if (!strcmp(arg, "-march=amd64"))
			arch = ARCH_AMD64;
		else
			list_push_back(input, arg);
	}

	if (arch != ARCH_INVALID)
		return;

	switch (sizeof(void *)) {
	case 2:
		arch = ARCH_8086;
		break;
	case 4:
		arch = ARCH_I386;
		break;
	case 8:
		arch = ARCH_AMD64;
		break;
	default:
		arch = ARCH_I386;
		break;
	}
}

void options_destroy(void)
{
	delete_list(input, NULL);
}

char * option_outfile(void)
{
	return outfile;
}

int option_optimize(void)
{
	return optimize;
}

struct list * option_input(void)
{
	return input;
}

enum arch option_arch(void)
{
	return arch;
}

int option_warnings(void)
{
	return warnings;
}
