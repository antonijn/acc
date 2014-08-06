
#include <acc/token.h>
#include <acc/error.h>
#include <stdlib.h>
#include <ctype.h>

/* string stream */
typedef struct sstream {
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

static int exc(FILE * f, SFILE * t, const char * chs);

static int exid(FILE * f, SFILE * t, enum tokenty * tt);
static int exnum(FILE * f, SFILE * t, enum tokenty * tt);
static int exstr(FILE * f, SFILE * t, enum tokenty * tt);
static int exop(FILE * f, SFILE * t, enum tokenty * tt);

static int readnum(FILE * f, SFILE * t, const char * allowed,
	enum tokenty * tt);
static int readch(FILE * f, SFILE * t, char terminator);

static int line;
static int column;

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
 	res->av = 128;
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
 * Expect character
 * If next char is in chs, returns 1 and advances stream
 * If not, returns 0
 */
static int exc(FILE * f, SFILE * t, const char * chs)
{
	char ch;
	int act = fgetc(f);
	++column;
	if (!chs) {
		if (act == '\n') {
			++line;
			column = 0;
		}
		if (t)
			ssputc(t, act);
		return 1;
	}

	for (; ch = *chs; ++chs) {
		if (ch != act)
			continue;
		if (ch == '\n') {
			++line;
			column = 0;
		}
		if (t)
			ssputc(t, ch);
		return 1;
	}
	--column;
	ungetc(act, f);
	return 0;
}

/*
 * Skip formatting characters
 */
static void skipf(FILE * f)
{
	while (exc(f, NULL, " \n\t\v\r\f"))
		;
}

/*
 * Expect operator
 * Returns 1 if an operator was read
 */
static int exop(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (exc(f, t, "*/%^!=")) {
		exc(f, t, "=");
		goto ret;
	}

	if (exc(f, t, "-")) {
		exc(f, t, ">-=");
		goto ret;
	}

	if (exc(f, t, "+")) {
		exc(f, t, "+=");
		goto ret;
	}

	if (exc(f, t, ">")) {
		exc(f, t, ">=");
		goto ret;
	}

	if (exc(f, t, "<")) {
		exc(f, t, "<=");
		goto ret;
	}

	if (exc(f, t, "&")) {
		exc(f, t, "&=");
		goto ret;
	}

	if (exc(f, t, "|")) {
		exc(f, t, "|=");
		goto ret;
	}

	return exc(f, t, "{};:()?#.,[]");

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
	       !strcmp(str, "extern") ||
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
	"ABCEEFGHIJKLMNOPQRSTUVWXYZ_";
static const char * alphanum =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCEEFGHIJKLMNOPQRSTUVWXYZ"
	"_0123456789";

/*
 * Expect identifier
 * Returns 1 if an identifier was read
 */
static int exid(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (exc(f, t, alpha)) {
		while (exc(f, t, alphanum))
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
	while (exc(f, t, allowed))
		++i;
	if (((i == 1 && allowed == octchars) || allowed == decchars) &&
		exc(f, t, ".")) {
		++i;
		*tt = T_DOUBLE;
		while (exc(f, t, decchars))
			++i;
		if (exc(f, t, "f")) {
			++i;
			*tt = T_FLOAT;
		}
	}

	if (exc(f, NULL, alphanum))
		report(E_TOKENIZER, NULL, "unexpected character in numeric literal");
	return i;
}

/*
 * Expect numbers
 * Returns 1 if a number is read
 */
static int exnum(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (exc(f, t, "0")) {
		/* octal, hex or zero */
		if (exc(f, t, "x")) {
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
	if (exc(f, t, termstr))
		return 0;

	if (!exc(f, t, "\\")) {
		exc(f, t, NULL);
		return 1;
	}

	/* escape sequence */
	if (exc(f, t, "x")) {
		for (i = 0; exc(f, t, hexchars); ++i)
			;
		if (i == 0)
			report(E_TOKENIZER, NULL, "hexadecimal escape sequences cannot be empty");
		return 1;
	}

	/* octal number */
	for (i = 0; i < 3 && exc(f, t, octchars); ++i)
		;
	if (i != 0)
		return 1;

	if (exc(f, NULL, "\n")) {
		ssunputc(t);
		return 1;
	}

	if (!exc(f, t, "abfnrtv\\'\"?"))
		report(E_TOKENIZER, NULL, "invalid escape sequence");
	
	return 1;
}

/*
 * Expect string
 * Returns 1 if a string or character literal is read
 */
static int exstr(FILE * f, SFILE * t, enum tokenty * tt)
{
	if (exc(f, t, "\"")) {
		while (readch(f, t, '"'))
			;
		*tt = T_STRING;
		return 1;
	}

	if (exc(f, t, "'")) {
		if (!readch(f, t, '\''))
			report(E_TOKENIZER, NULL, "character literals may not be empty");
		if (!exc(f, t, "'"))
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

	/* look for comments */
	if (exc(f, NULL, "/")) {
		if (exc(f, NULL, "*")) {
			while (!exc(f, NULL, "*") || !exc(f, NULL, "/"))
				exc(f, NULL, NULL);
			skipf(f);
		} else
			ungetc('/', f);
	}

	if (exop(f, t, &res.type))
		;
	else if (exid(f, t, &res.type))
		;
	else if (exnum(f, t, &res.type))
		;
	else if (exstr(f, t, &res.type))
		;
	else if (exc(f, t, eof))
		res.type = T_EOF;
	else
		report(E_TOKENIZER, NULL, "character out of place");

	res.lexeme = ssclose(t);
	return res;
}

void freetok(struct token * t)
{
	free(t->lexeme);
}
