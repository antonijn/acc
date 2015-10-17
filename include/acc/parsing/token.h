/*
 * Tokenizer
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

#ifndef PARSING_TOKEN_H
#define PARSING_TOKEN_H

#include <stdio.h>
#include <stdbool.h>

enum tokenty {
	T_IDENTIFIER,
	T_RESERVED,
	T_OPERATOR,
	T_PREPROC,
	T_OCT,
	T_HEX,
	T_DEC,
	T_FLOAT,
	T_DOUBLE,
	T_CHAR,
	T_STRING,
	T_EOF
};

struct token {
	enum tokenty type;
	char *lexeme;
	char *linestr;
	int column;
	int line;
};

/*
 * Check if next token has certain lexeme
 * Advance stream if so
 */
bool chkt(FILE *f, const char *t);
/*
 * Check if next token has certain type
 * Advance stream if so
 */
bool chktt(FILE *f, enum tokenty tt);
/*
 * Same as above, except for specified tokens, and doesn't advance stream
 */
bool chktp(FILE * f, const char *t, struct token *nxt);
bool chkttp(FILE * f, enum tokenty tt, struct token *nxt);

/*
 * Get next token from stream
 */
struct token gettok(FILE *f);
/*
 * Revert getting token from stream
 */
void ungettok(struct token *t, FILE *f);
/*
 * Delete token retrieved from stream
 */
void freetok(struct token *t);

/*
 * Reset the entire tokenizer
 */
void resettok(void);
/*
 * Get current line number (starting at 1)
 */
int get_line(void);
/*
 * Get current column number (starting at 1)
 */
int get_column(void);

#endif
