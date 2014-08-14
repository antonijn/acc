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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <acc/token.h>
#include <acc/error.h>
#include <acc/ext.h>

/* string stream */
typedef struct {
	char * buf;
	int av;
	int count;
} SFILE;

static SFILE * ssopen(void);
static char * ssclose(SFILE * ss);
static char * ssgets(SFILE * ss);
static void ssputc(SFILE * ss, char ch);
static void ssunputc(SFILE * ss);

/* local functions */
static void skipf(FILE * f);

struct lchar {
	int ch;
	int chars[3];
	size_t len;
};

static struct lchar fgetlc(FILE * f);
static void ungetlc(struct lchar * lc, FILE * f);

static int chkc(FILE * f, SFILE * t, const char * chs);
static int chks(FILE * f, SFILE * t, const char * s);

static int chkid(FILE * f, SFILE * t, enum tokenty * tt);
static int chknum(FILE * f, SFILE * t, enum tokenty * tt);
static int chkstr(FILE * f, SFILE * t, enum tokenty * tt);
static int chkop(FILE * f, SFILE * t, enum tokenty * tt);
static int chkppdir(FILE * f, SFILE * t, enum tokenty * tt);

static int readnum(FILE * f, SFILE * t, const char * allowed,
	enum tokenty * tt);
static int readch(FILE * f, SFILE * t, char terminator);

static int line = 1;
static int column = 0;

int get_line(void)
{
	return line;
}

int get_column(void)
{
	return column;
}

/*
 * Open string stream
 */
static SFILE * ssopen(void)
{
	SFILE * res = malloc(sizeof(SFILE));
	res->count = 0;
	res->av = 16;
	res->buf = calloc(res->av + 1, sizeof(char));
	return res;
}

 /*
  * Close string stream
  */
static char * ssclose(SFILE * ss)
{
	char * buf = ss->buf;
	free(ss);
	return buf;
}

/*
 * Get stored string
 */
static char * ssgets(SFILE * ss)
{
	return ss->buf;
}

/*
 * Write character to a string stream
 */
static void ssputc(SFILE * ss, char ch)
{
	int i;
	if (ss->av == 0) {
		ss->av = ss->count;
		ss->buf = realloc(ss->buf, ss->count * 2 + 1);
		for (i = ss->count; i < ss->count * 2 + 1; ++i)
			ss->buf[i] = '\0';
	}
	ss->buf[ss->count++] = ch;
	--ss->av;
}

/*
 * Remove last character from string stream
 */
static void ssunputc(SFILE * ss)
{
	ss->buf[--ss->count] = '\0';
}

/*
 * Get logical C character
 * Filters out trigraphs.
 */
static struct lchar fgetlc(FILE * f)
{
	struct lchar res;
	res.chars[0] = res.ch = fgetc(f);
	++column;
	if (res.chars[0] != '?') {
		if (res.chars[0] == '\n') {
			++line;
			column = 0;
		}
		res.len = 1;
		return res;
	}
	res.chars[1] = fgetc(f);
	if (res.chars[1] != '?') {
		ungetc(res.chars[1], f);
		res.len = 1;
		return res;
	}
	column += 2;
	res.chars[2] = fgetc(f);
	switch (res.chars[2]) {
	case '=':
		res.ch = '#';
		res.len = 3;
		return res;
	case '/':
		res.ch = '\\';
		res.len = 3;
		return res;
	case '\'':
		res.ch = '^';
		res.len = 3;
		return res;
	case '(':
		res.ch = '[';
		res.len = 3;
		return res;
	case ')':
		res.ch = ']';
		res.len = 3;
		return res;
	case '!':
		res.ch = '|';
		res.len = 3;
		return res;
	case '<':
		res.ch = '{';
		res.len = 3;
		return res;
	case '>':
		res.ch = '}';
		res.len = 3;
		return res;
	case '-':
		res.ch = '~';
		res.len = 3;
		return res;
	}
	report(E_TOKENIZER, NULL, "invalid trigraph sequence: \"??%c\"", res.chars[2]);
	ungetc(res.chars[2], f);
	ungetc(res.chars[1], f);
	column -= 2;
	res.len = 1;
	return res;
}

static void ungetlc(struct lchar * lc, FILE * f)
{
	int i;
	for (i = lc->len - 1; i >= 0; --i) {
		int c = lc->chars[i];
		if (c == '\n')
			--line;
		else
			--column;
		ungetc(c, f);
	}
}

/*
 * Check for character
 * If next char is in chs, returns 1 and advances stream
 * If not, returns 0
 */
static int chkc(FILE * f, SFILE * t, const char * chs)
{
	char ch;
	struct lchar act = fgetlc(f);

	if (act.ch == '\\') {
		struct lchar nxt = fgetlc(f);
		if (nxt.ch == '\n')
			act = fgetlc(f);
		else
			ungetlc(&nxt, f);
	}

	if (!chs) {
		if (t)
			ssputc(t, act.ch);
		return 1;
	}

	for (; ch = *chs; ++chs) {
		if (ch != act.ch)
			continue;
		if (t)
			ssputc(t, ch);
		return 1;
	}
	ungetlc(&act, f);
	return 0;
}

/*
 * Check for string
 * Like chkc but checks for an entire sequence.
 */
static int chks(FILE * f, SFILE * t, const char * s)
{
	size_t len = strlen(s);
	struct lchar *lcs = malloc(sizeof(struct lchar) * len);
	int i;
	for (i = 0; s[i]; ++i) {
		lcs[i] = fgetlc(f);
		if (lcs[i].ch != s[i])
			goto cleanup;
		if (t)
			ssputc(t, s[i]);
	}
	free(lcs);
	return 1;

cleanup:
	for (; i >= 0; --i) {
		if (t)
			ssunputc(t);
		ungetlc(&lcs[i], f);
	}

	free(lcs);
	return 0;
}

/*
 * Skip formatting characters
 */
static void skipf(FILE * f)
{
	while (chkc(f, NULL, " \n\t\v\r\f"))
		;
	
	/* look for comments */
	if (chks(f, NULL, "/*")) {
		while (!chks(f, NULL, "*/"))
			chkc(f, NULL, NULL);
		skipf(f);
	}
}

/*
 * Check for preprocessor directive
 * Returns 1 if a preprocessor directive was read
 */
static int chkppdir(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (chkc(f, t, "#")) {
		while (!(!chkc(f, NULL, "\\") && chkc(f, NULL, "\n")))
			chkc(f, t, NULL);
		*tt = T_PREPROC;
		return 1;
	}
	return 0;
}

/*
 * Check for operator
 * Returns 1 if an operator was read
 */
static int chkop(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (chkc(f, t, "*/%^!=~")) {
		chkc(f, t, "=");
		goto ret;
	}

	if (chkc(f, t, "-")) {
		chkc(f, t, ">-=");
		goto ret;
	}

	if (chkc(f, t, "+")) {
		chkc(f, t, "+=");
		goto ret;
	}

	if (chkc(f, t, ">")) {
		chkc(f, t, ">=");
		goto ret;
	}

	if (chkc(f, t, "<")) {
		chkc(f, t, "<=");
		goto ret;
	}

	if (chkc(f, t, "&")) {
		chkc(f, t, "&=");
		goto ret;
	}

	if (chkc(f, t, "|")) {
		chkc(f, t, "|=");
		goto ret;
	}

	if (chkc(f, t, "{};:()?.,[]"))
		goto ret;

	return 0;

ret:
	*tt = T_OPERATOR;
	return 1;
}

/*
 * Returns 1 if str is a reserved word
 */
static int isreserved(char * str)
{
	if (str[0] == '_' && isupper(str[1]))
		return 1;

	if (isext(EX_RESTRICT) && !strcmp(str, "restrict"))
		return 1;

	return !strcmp(str, "auto") ||
	       !strcmp(str, "break") ||
	       !strcmp(str, "case") ||
	       !strcmp(str, "char") ||
	       !strcmp(str, "const") ||
	       !strcmp(str, "continue") ||
	       !strcmp(str, "default") ||
	       !strcmp(str, "do") ||
	       !strcmp(str, "double") ||
	       !strcmp(str, "else") ||
	       !strcmp(str, "enum") ||
	       !strcmp(str, "chktern") ||
	       !strcmp(str, "float") ||
	       !strcmp(str, "for") ||
	       !strcmp(str, "goto") ||
	       !strcmp(str, "if") ||
	       !strcmp(str, "int") ||
	       !strcmp(str, "long") ||
	       !strcmp(str, "register") ||
	       !strcmp(str, "return") ||
	       !strcmp(str, "short") ||
	       !strcmp(str, "signed") ||
	       !strcmp(str, "sizeof") ||
	       !strcmp(str, "static") ||
	       !strcmp(str, "struct") ||
	       !strcmp(str, "switch") ||
	       !strcmp(str, "typedef") ||
	       !strcmp(str, "union") ||
	       !strcmp(str, "unsigned") ||
	       !strcmp(str, "void") ||
	       !strcmp(str, "volatile") ||
	       !strcmp(str, "while");
}

static const char * alpha =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ_";
static const char * alphanum =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"_0123456789";

/*
 * Check for identifier
 * Returns 1 if an identifier was read
 */
static int chkid(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (chkc(f, t, alpha)) {
		while (chkc(f, t, alphanum))
			;
		*tt = isreserved(ssgets(t)) ?
			T_RESERVED : T_IDENTIFIER;
		return 1;
	}
	return 0;
}

const char * hexchars = "0123456789abcdefABCDEF";
const char * decchars = "0123456789";
const char * octchars = "01234567";

/*
 * Read a number composed of the characters in allowed
 */
static int readnum(FILE * f, SFILE * t, const char * allowed,
	enum tokenty * tt)
{
	int i = 0;
	while (chkc(f, t, allowed))
		++i;
	if (((i == 1 && allowed == decchars) || allowed == octchars) &&
		chkc(f, t, ".")) {
		++i;
		*tt = T_DOUBLE;
		while (chkc(f, t, decchars))
			++i;
		if (chkc(f, t, "f")) {
			++i;
			*tt = T_FLOAT;
		}
	}

	if (chkc(f, NULL, alphanum))
		report(E_TOKENIZER, NULL, "unexpected character in numeric literal");
	return i;
}

/*
 * Check for number
 * Returns 1 if a number is read
 */
static int chknum(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (chkc(f, t, "0")) {
		/* octal, hex or zero */
		if (chkc(f, t, "xX")) {
			*tt = T_HEX;
			if (readnum(f, t, hexchars, tt) == 0)
				report(E_TOKENIZER, NULL, "unfinished hexadecimal literal");
			return 1;
		}

		*tt = T_OCT;
		if (readnum(f, t, octchars, tt) == 0) {
			/* zero literal */
			*tt = T_DEC;
		}
		return 1;
	}
	*tt = T_DEC;
	return readnum(f, t, decchars, tt) != 0;
}

/*
 * Returns 1 if a non-terminator character is read
 */
static int readch(FILE * f, SFILE * t, char terminator)
{
	int i;
	char termstr[2];

	termstr[0] = terminator;
	termstr[1] = '\0';
	if (chkc(f, t, termstr))
		return 0;

	if (chkc(f, t, "\n"))
		report(E_TOKENIZER, NULL, "newline in string literal");

	if (!chkc(f, t, "\\")) {
		chkc(f, t, NULL);
		return 1;
	}

	/* escape sequence */
	if (chkc(f, t, "x")) {
		for (i = 0; chkc(f, t, hexchars); ++i)
			;
		if (i == 0)
			report(E_TOKENIZER, NULL, "hexadecimal escape sequences cannot be empty");
		return 1;
	}

	/* octal number */
	for (i = 0; i < 3 && chkc(f, t, octchars); ++i)
		;
	if (i != 0)
		return 1;

	if (chkc(f, NULL, "\n")) {
		ssunputc(t);
		return 1;
	}

	if (!chkc(f, t, "abfnrtv\\'\"?"))
		report(E_TOKENIZER, NULL, "invalid escape sequence");
	
	return 1;
}

/*
 * Check for string
 * Returns 1 if a string or character literal is read
 */
static int chkstr(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (chkc(f, t, "\"")) {
		while (readch(f, t, '"'))
			;
		*tt = T_STRING;
		return 1;
	}

	if (chkc(f, t, "'")) {
		if (!readch(f, t, '\''))
			report(E_TOKENIZER, NULL, "character literals may not be empty");
		if (!chkc(f, t, "'"))
			report(E_TOKENIZER, NULL,
				"character literals may only contain one character");
		*tt = T_CHAR;
		return 1;
	}

	return 0;
}

const char eof[] = { EOF, '\0' };

struct token gettok(FILE * f)
{
	struct token res;
	SFILE * t;

	skipf(f);

	t = ssopen();

	if (chkop(f, t, &res.type))
		;
	else if (chkid(f, t, &res.type))
		;
	else if (chknum(f, t, &res.type))
		;
	else if (chkstr(f, t, &res.type))
		;
	else if (chkppdir(f, t, &res.type))
		;
	else if (chkc(f, t, eof)) {
		res.type = T_EOF;
		line = 1;
		column = 0;
	} else
		report(E_TOKENIZER, NULL, "character out of place: '%c'", fgetc(f));

	res.lexeme = ssclose(t);
	return res;
}

void ungettok(struct token * t, FILE * f)
{
	size_t len = strlen(t->lexeme);
	int i;
	for (i = len - 1; i >= 0; --i) {
		ungetc(t->lexeme[i], f);
		--column;
	}
}

void freetok(struct token * t)
{
	free(t->lexeme);
}
