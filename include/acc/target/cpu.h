#ifndef TARGET_CPU_H
#define TARGET_CPU_H

#include <acc/parsing/ast.h>

struct cpu {
	const char *name;
	int bits;
	int offset;
};

extern const struct cpu *cpu;

const struct cpu *getcpu(void);

int gettypesize(struct ctype *ty);
int getfalign(struct ctype *ty);

#endif
