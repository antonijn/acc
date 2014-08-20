/*
 * Target specifications
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

#include <acc/target.h>

static const struct os * oses[] = {
	&oslinux, &oswindows, &osx
};

static const struct arch * arches[] = {
	&archx86
};

static const struct cpu * cpus[] = {
	&cpu8086, &cpui386, &cpui686, &cpuamd64
};

const struct os oslinux = { "linux" };
const struct os oswindows = { "windows" };
const struct os osx = { "osx" };

static const struct cpu * x86cpus[] = {
	&cpu8086,
	&cpui386,
	&cpui686,
	&cpuamd64,
	NULL
};

const struct arch archx86 = { "x86", &x86cpus[0] };

const struct cpu cpu8086 = { "8086", &archx86, 16, 0 };
const struct cpu cpui386 = { "i386", &archx86, 32, 1 };
const struct cpu cpui686 = { "i686", &archx86, 32, 2 };
const struct cpu cpuamd64 = { "amd64", &archx86, 64, 3 };

static const struct os * os =
#if defined(BUILDFOR_LINUX)
	&oslinux;
#elif defined(BUILDFOR_WINDOWS)
	&oswindows;
#elif defined(BUILDFOR_OSX)
	&osx;
#else
#error No target defined
	NULL;
#endif

static const struct arch * arch = &archx86;
static const struct cpu * cpu = &cpui386;

const struct os * osbyname(const char * name)
{
	int i;
	for (i = 0; i < sizeof(oses) / sizeof(struct os); ++i)
		if (!strcmp(oses[i]->name, name))
			return oses[i];
	
	return NULL;
}

const struct arch * archbyname(const char * name)
{
	int i;
	for (i = 0; i < sizeof(arches) / sizeof(struct arch); ++i)
		if (!strcmp(arches[i]->name, name))
			return arches[i];
	
	return NULL;
}

const struct cpu * cpubyname(const char * name)
{
	int i;
	for (i = 0; i < sizeof(cpus) / sizeof(struct cpu); ++i)
		if (!strcmp(cpus[i]->name, name))
			return cpus[i];
	
	return NULL;
}


const struct os * getos(void)
{
	return os;
}

const struct arch * getarch(void)
{
	return arch;
}

const struct cpu * getcpu(void)
{
	return cpu;
}


void setos(const struct os * o)
{
	os = o;
}

void setarch(const struct arch * a)
{
	if (arch == a)
		return;
	
	arch = a;
	cpu = a->cpus[0];
}

void setcpu(const struct cpu * c)
{
	arch = c->arch;
	cpu = c;
}


int gettypesize(struct ctype * ty)
{
	if (ty == &cbool)
		return 1;
	if (ty == &cfloat)
		return 4;
	if (ty == &cdouble)
		return 8;
	
	switch (cpu->bits) {
	case 16:
		if (ty == &cchar)
			return 1;
		if (ty == &cshort)
			return 2;
		if (ty == &cint)
			return 2;
		if (ty == &clong)
			return 4;
		break;
	case 32:
		if (os == &oswindows && ty == &clong)
			return 4;
	case 64:
		if (ty == &cchar)
			return 1;
		if (ty == &cshort)
			return 2;
		if (ty == &cint)
			return 4;
		if (ty == &clong)
			return 8;
		break;
	}
	
	if (ty->type == POINTER)
		return cpu->bits / 8;
	
	return -1;
}

int getfalign(struct ctype * ty)
{
	if (arch == &archx86 && os == &oslinux && ty == &cdouble)
		return 4;
	return gettypesize(ty);
}
