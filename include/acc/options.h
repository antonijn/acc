#ifndef OPTIONS_H
#define OPTIONS_H

#include <acc/list.h>
#include <stdlib.h>
#include <stdio.h>

enum arch {
	ARCH_INVALID,
	ARCH_I386,
	ARCH_I686,
	ARCH_8086,
	ARCH_AMD64
};

void options_init(int argc, char * argv[]);
void options_destroy(void);

char * option_outfile(void);
int option_optimize(void);
struct list * option_input(void);
enum arch option_arch(void);
int option_warnings(void);

#endif
