#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

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
	char * lexeme;
};

struct token gettok(FILE * f);
void freetok(struct token * t);

int get_line(void);
int get_column(void);

#endif
