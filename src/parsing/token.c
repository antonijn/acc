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
 * 
 * 
 * 
 * The acc tokenizer tokenizes on the fly in order to decrease memory usage.
 * A token can be read from a file using readtok().
 * 
 * The standard interface provides buffered access to tokens, although this is
 * abstracted away through an interface that seems unbuffered. The variable
 * "isbuffered" indicates whether the next token is already buffered or not. If
 * this is so, the next token is stored in the global variable "buffer". Using
 * readtok() immediately is not preferred. It is more performance efficient to
 * make certain input is buffered first (call validatebuf()) and then read from
 * "buffer". To then advance the token for the next function, set "isbuffered"
 * to false, so that validatebuf() forces a new token into "buffer".
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <acc/parsing/token.h>
#include <acc/error.h>
#include <acc/ext.h>

/* string stream */
typedef struct {
	char * buf;
	size_t av;
	size_t count;
} SFILE;

static SFILE *ssopen(void);
static char *ssclose(SFILE *ss);
static char *ssgets(SFILE *ss);
static void ssputc(SFILE *ss, char ch);
static void ssunputc(SFILE *ss);

/* local functions */
static void skipf(FILE *f);

struct lchar {
	int ch;
	int chars[3];
	size_t len;
};

static void readline(FILE *f);

static struct lchar fgetlc(FILE *f);
static void ungetlc(struct lchar *lc, FILE *f);

static bool chkc(FILE *f, SFILE *t, const char *chs);
static bool chks(FILE *f, SFILE *t, const char *s);

static bool chkid(FILE *f, SFILE *t, enum tokenty *tt);
static bool chknum(FILE *f, SFILE *t, enum tokenty *tt);
static bool chkstr(FILE *f, SFILE *t, enum tokenty *tt);
static bool chkop(FILE *f, SFILE *t, enum tokenty *tt);
static bool chkppdir(FILE *f, SFILE *t, enum tokenty *tt);

static int readnum(FILE *f, SFILE *t, const char *allowed,
	enum tokenty *tt);
static bool readch(FILE *f, SFILE *t, char terminator);

static struct token clonetok(struct token *tok);

static int line = 1;
static int column = 1;
static char *linestr = NULL;

static struct token buffer = { 0 };
static bool isbuffered = false;

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
static SFILE *ssopen(void)
{
	SFILE *res = malloc(sizeof(SFILE));
	res->count = 0;
	res->av = 16;
	res->buf = malloc((res->av + 1) * sizeof(char));
	res->buf[0] = '\0';
	return res;
}

/*
 * Close string stream
 */
static char *ssclose(SFILE *ss)
{
	char *buf = ss->buf;
	free(ss);
	return buf;
}

/*
 * Get stored string
 */
static char *ssgets(SFILE *ss)
{
	return ss->buf;
}

/*
 * Write character to a string stream
 */
static void ssputc(SFILE *ss, char ch)
{
	if (ss->av == 0) {
		ss->av = ss->count;
		ss->buf = realloc(ss->buf, (ss->count * 2 + 1) * sizeof(char));
	}
	ss->buf[ss->count++] = ch;
	ss->buf[ss->count] = '\0';
	--ss->av;
}

/*
 * Remove last character from string stream
 */
static void ssunputc(SFILE *ss)
{
	ss->buf[--ss->count] = '\0';
}

static void readline(FILE *f)
{
	if (linestr)
		free(linestr);

	SFILE *sf = ssopen();

	int i = 0, nxt;
	while ((nxt = fgetc(f)) != '\n' && nxt != EOF) {
		ssputc(sf, nxt);
		++i;
	}
	ungetc(nxt, f);

	linestr = ssclose(sf);
	for (; i > 0; --i)
		ungetc(linestr[i - 1], f);
}

/*
 * Get logical C character
 * Filters out trigraphs.
 */
static struct lchar fgetlc(FILE *f)
{
	struct lchar res;
	res.chars[0] = res.ch = fgetc(f);
	++column;
	if (res.chars[0] != '?') {
		if (res.chars[0] == '\n') {
			readline(f);
			++line;
			column = 1;
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
	case '?':
		ungetc('?', f);
		ungetc('?', f);
		column -= 2;
		res.ch = '?';
		res.len = 1;
		return res;
	}
	report(E_TOKENIZER, NULL, "invalid trigraph sequence: \"\?\?%c\"", res.chars[2]);
	ungetc(res.chars[2], f);
	ungetc(res.chars[1], f);
	column -= 2;
	res.len = 1;
	return res;
}

static void ungetlc(struct lchar *lc, FILE *f)
{
	for (int i = lc->len - 1; i >= 0; --i) {
		int c = lc->chars[i];
		if (c == '\n') {
			--line;
			column = 1;
		} else {
			--column;
		}
		ungetc(c, f);
	}
}

/*
 * Check for character
 * If next char is in chs, returns 1 and advances stream
 * If not, returns 0
 */
static bool chkc(FILE *f, SFILE *t, const char *chs)
{
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
		return true;
	}

	for (char ch; ch = *chs; ++chs) {
		if (ch != act.ch)
			continue;
		if (t)
			ssputc(t, ch);
		return true;
	}
	ungetlc(&act, f);
	return false;
}

/*
 * Check for string
 * Like chkc but checks for an entire sequence.
 */
static bool chks(FILE *f, SFILE *t, const char *s)
{
	int i;
	size_t len = strlen(s);
	struct lchar *lcs = malloc(sizeof(struct lchar) * len);
	for (i = 0; s[i]; ++i) {
		lcs[i] = fgetlc(f);
		if (lcs[i].ch != s[i])
			goto cleanup;
		if (t)
			ssputc(t, s[i]);
	}
	free(lcs);
	return true;

cleanup:
	for (; i >= 0; --i) {
		if (t)
			ssunputc(t);
		ungetlc(&lcs[i], f);
	}

	free(lcs);
	return false;
}

/*
 * Skip formatting characters
 */
static void skipf(FILE *f)
{
	while (chkc(f, NULL, " \n\t\v\r\f"))
		;
	
	// look for comments
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
static bool chkppdir(FILE *f, SFILE *t, enum tokenty *tt)
{
	if (chkc(f, t, "#")) {
		while (!(!chkc(f, NULL, "\\") && chkc(f, NULL, "\n")))
			chkc(f, t, NULL);
		*tt = T_PREPROC;
		return true;
	}
	return false;
}

/*
 * Check for operator
 * Returns 1 if an operator was read
 */
static bool chkop(FILE *f, SFILE *t, enum tokenty *tt)
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

	return false;

ret:
	*tt = T_OPERATOR;
	return true;
}

/*
 * Returns 1 if str is a reserved word
 */
static bool isreserved(char *str)
{
	if (isext(EX_RESTRICT) && !strcmp(str, "restrict"))
		return true;

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

static const char alpha[] =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ_";
static const char alphanum[] =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"_0123456789";

/*
 * Check for identifier
 * Returns 1 if an identifier was read
 */
static bool chkid(FILE *f, SFILE *t, enum tokenty *tt)
{
	if (chkc(f, t, alpha)) {
		while (chkc(f, t, alphanum))
			;
		*tt = isreserved(ssgets(t)) ?
			T_RESERVED : T_IDENTIFIER;
		return true;
	}
	return false;
}

const char hexchars[] = "0123456789abcdefABCDEF";
const char decchars[] = "0123456789";
const char octchars[] = "01234567";

/*
 * Read a number composed of the characters in allowed
 */
static int readnum(FILE *f, SFILE *t, const char *allowed,
	enum tokenty *tt)
{
	int i = 0;
	while (chkc(f, t, allowed))
		++i;
	if ((allowed == decchars || allowed == octchars) &&
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
static bool chknum(FILE *f, SFILE *t, enum tokenty *tt)
{
	if (chkc(f, t, "0")) {
		// octal, hex or zero
		if (chkc(f, t, "xX")) {
			*tt = T_HEX;
			if (readnum(f, t, hexchars, tt) == 0)
				report(E_TOKENIZER, NULL, "unfinished hexadecimal literal");
			return true;
		}

		*tt = T_OCT;
		if (readnum(f, t, octchars, tt) == 0) {
			// zero literal
			*tt = T_DEC;
		}
		return true;
	}
	*tt = T_DEC;
	return readnum(f, t, decchars, tt) != 0;
}

/*
 * Returns 1 if a non-terminator character is read
 */
static bool readch(FILE *f, SFILE *t, char terminator)
{
	int i;
	char termstr[2];

	termstr[0] = terminator;
	termstr[1] = '\0';
	if (chkc(f, t, termstr))
		return false;

	if (chkc(f, t, "\n"))
		report(E_TOKENIZER, NULL, "newline in string literal");

	if (!chkc(f, t, "\\")) {
		chkc(f, t, NULL);
		return true;
	}

	// escape sequence
	if (chkc(f, t, "x")) {
		for (i = 0; chkc(f, t, hexchars); ++i)
			;
		if (i == 0)
			report(E_TOKENIZER, NULL, "hexadecimal escape sequences cannot be empty");
		return true;
	}

	// octal number
	for (i = 0; i < 3 && chkc(f, t, octchars); ++i)
		;
	if (i != 0)
		return true;

	if (chkc(f, NULL, "\n")) {
		ssunputc(t);
		return true;
	}

	if (!chkc(f, t, "abfnrtv\\'\"?"))
		report(E_TOKENIZER, NULL, "invalid escape sequence");
	
	return true;
}

/*
 * Check for string
 * Returns 1 if a string or character literal is read
 */
static bool chkstr(FILE *f, SFILE *t, enum tokenty *tt)
{
	if (chkc(f, t, "\"")) {
		while (readch(f, t, '"'))
			;
		*tt = T_STRING;
		return true;
	}

	if (chkc(f, t, "'")) {
		if (!readch(f, t, '\''))
			report(E_TOKENIZER, NULL, "character literals may not be empty");
		if (!chkc(f, t, "'"))
			report(E_TOKENIZER, NULL,
				"character literals may only contain one character");
		*tt = T_CHAR;
		return true;
	}

	return false;
}

static struct token readtok(FILE *f)
{
	if (!linestr)
		readline(f);

	skipf(f);

	struct token res;
	res.line = line;
	res.column = column;
	res.linestr = NULL;

	SFILE *t = ssopen();

	int nxt = fgetc(f);
	if (nxt == EOF) {
		res.type = T_EOF;
		res.lexeme = NULL;
		free(ssclose(t));
		goto eofret;
	} else {
		ungetc(nxt, f);
	}

	if (chkop(f, t, &res.type))
		goto ret;
	if (chkid(f, t, &res.type))
		goto ret;
	if (chknum(f, t, &res.type))
		goto ret;
	if (chkstr(f, t, &res.type))
		goto ret;
	if (chkppdir(f, t, &res.type))
		goto ret;

	report(E_TOKENIZER, NULL, "character out of place: '%c'", fgetc(f));

ret:
	res.lexeme = ssclose(t);
	res.linestr = malloc((strlen(linestr) + 1) * sizeof(char));
	sprintf(res.linestr, "%s", linestr);
eofret:
	return res;
}

static void rewritetok(struct token *t, FILE *f)
{
	size_t len = strlen(t->lexeme);
	for (int i = len - 1; i >= 0; --i)
		ungetc(t->lexeme[i], f);

	column = t->column;
	line = t->line;
}

void ungettok(struct token *t, FILE *f)
{
	if (isbuffered) {
		rewritetok(&buffer, f);
		freetok(&buffer);
		buffer = clonetok(t);
	} else {
		buffer = clonetok(t);
		isbuffered = true;
	}
}

void freetok(struct token *t)
{
	if (t->lexeme)
		free(t->lexeme);
	if (t->linestr)
		free(t->linestr);
}

static void validatebuf(FILE *f)
{
	if (isbuffered)
		return;

	buffer = readtok(f);
	isbuffered = true;
}

void resettok(void)
{
	line = 1;
	column = 1;
	isbuffered = false;
	if (linestr)
		free(linestr);
	linestr = NULL;
}

static struct token clonetok(struct token *tok)
{
	struct token res = *tok;
	if (tok->lexeme) {
		res.lexeme = malloc((strlen(tok->lexeme) + 1) * sizeof(char));
		sprintf(res.lexeme, "%s", tok->lexeme);
	} else {
		res.lexeme = NULL;
	}
	if (tok->linestr) {
		res.linestr = malloc((strlen(tok->linestr) + 1) * sizeof(char));
		sprintf(res.linestr, "%s", tok->linestr);
	} else {
		res.linestr = NULL;
	}
	return res;
}

struct token gettok(FILE *f)
{
	validatebuf(f);
	struct token res = clonetok(&buffer);
	freetok(&buffer);
	isbuffered = false;
	return res;
}

bool chkt(FILE *f, const char *t)
{
	validatebuf(f);

	if (buffer.type == T_EOF) {
		report(E_FATAL | E_HIDE_TOKEN, NULL,
		       "unexpected end-of-file");
	}

	if (strcmp(t, buffer.lexeme))
		return false;

	freetok(&buffer);
	isbuffered = false;
	return true;
}

bool chktt(FILE *f, enum tokenty tt)
{
	validatebuf(f);

	if (buffer.type != tt) {
		if (buffer.type == T_EOF) {
			report(E_FATAL | E_HIDE_TOKEN, NULL,
				"unexpected end-of-file");
		}
		return false;
	}

	if (buffer.type == T_EOF) {
		resettok();
		return true;
	}

	freetok(&buffer);
	isbuffered = false;
	return true;
}

bool chktp(FILE *f, const char *t, struct token *nxt)
{
	validatebuf(f);
	*nxt = clonetok(&buffer);
	bool res = chkt(f, t);
	if (!res)
		freetok(nxt);
	return res;
}

bool chkttp(FILE *f, enum tokenty tt, struct token *nxt)
{
	validatebuf(f);
	*nxt = clonetok(&buffer);
	bool res = chktt(f, tt);
	if (!res)
		freetok(nxt);
	return res;
}
