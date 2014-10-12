#ifndef TARGET_X86_CPUS_H
#define TARGET_X86_CPUS_H

#include <acc/target/cpu.h>

enum asmflavor {
	AF_ATT,
	AF_NASM
};

enum asmflavor asmflavor(void);

extern const struct cpu cpu8086, cpui386, cpui686, cpux86_64;

#endif

