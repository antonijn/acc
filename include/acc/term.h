/*
 * ANSI color attribute definitions
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

#ifndef ACC_TERM_H
#define ACC_TERM_H

#define ANSI_RESET(x)	((x) ? "\033[0m" : "")
#define ANSI_BOLD(x)	((x) ? "\033[1m" : "")
#define ANSI_BLACK(x)	((x) ? "\033[30m" : "")
#define ANSI_RED(x)	((x) ? "\033[31m" : "")
#define ANSI_GREEN(x)	((x) ? "\033[32m" : "")
#define ANSI_YELLOW(x)	((x) ? "\033[33m" : "")
#define ANSI_BLUE(x)	((x) ? "\033[34m" : "")
#define ANSI_MAGENTA(x)	((x) ? "\033[35m" : "")
#define ANSI_CYAN(x)	((x) ? "\033[36m" : "")
#define ANSI_WHITE(x)	((x) ? "\033[37m" : "")

#endif
