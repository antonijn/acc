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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdlib.h>
#include <stdio.h>

#include <acc/list.h>

/*
 * Initialize options from command-line args
 */
void options_init(int argc, char *argv[]);
/*
 * Options cleanup
 */
void options_destroy(void);


/*
 * The output file ('-o')
 */
char *option_outfile(void);
/*
 * The optimization number (the X in '-OX')
 */
int option_optimize(void);
/*
 * The list of input files
 */
struct list *option_input(void);
/*
 * Indicates whether to give warnings ('-W')
 */
bool option_warnings(void);
/*
 * Indicates whether to emit IR ('-Sir')
 */
bool option_emit_ir(void);
/*
 * Indicates whether to emit assembly ('-S')
 */
bool option_emit_asm(void);

#endif
