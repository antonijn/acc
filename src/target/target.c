/*
 * Getting the default CPU
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

#include <acc/target/cpu.h>
#include <acc/options.h>
#include <acc/error.h>

const struct cpu *getcpu(void)
{
	return cpu;
}

void archoption(const char *opt)
{
	if (strncmp(opt, "cpu", 3)) {
		xarchoption(opt);
		return;
	}

	const char *scpu = opt + 3;

	for (int i = 0; cpus[i]; ++i) {
		if (!strcmp(cpus[i]->name, scpu)) {
			cpu = cpus[i];
			return;
		}
	}

	report(E_OPTIONS | E_FATAL, NULL, "no such CPU: '%s'", scpu);
}
