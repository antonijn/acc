/*
 * Error reporting utility
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

#ifndef ERROR_H
#define ERROR_H

#include <acc/token.h>

enum errorty {
	E_ERROR = 0x00,
	E_FATAL = 0x01,
	E_WARNING = 0x02,
	E_HIDE_TOKEN = 0x04,
	E_HIDE_LOCATION = 0x08,

	E_TOKENIZER = E_FATAL | E_HIDE_TOKEN,
	E_OPTIONS = E_FATAL | E_HIDE_TOKEN | E_HIDE_LOCATION,
	E_INTERNAL = E_FATAL | E_HIDE_TOKEN | E_HIDE_LOCATION,
	E_PARSER = E_ERROR
};

void report(enum errorty ty, struct token * tok, const char * frmt, ...);

#endif
