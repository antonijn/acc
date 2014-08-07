#include <acc/error.h>
#include <acc/token.h>
#include <acc/options.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void report(enum errorty ty, struct token * tok, const char * frmt, ...)
{
	va_list ap;
	if (!option_warnings() && (ty & E_WARNING))
		return;

	if ((ty & E_HIDE_LOCATION) == 0)
		fprintf(stderr, "%s:%d:%d: ", "???", get_line(), get_column());
	if ((ty & E_HIDE_TOKEN) == 0)
		fprintf(stderr, " at token '%s': ", tok->lexeme);

	if (ty & E_FATAL)
		fprintf(stderr, "FATAL: ");
	else if (ty & E_WARNING)
		fprintf(stderr, "warning: ");
	else
		fprintf(stderr, "error: ");

	va_start(ap, frmt);
	vfprintf(stderr, frmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	if (ty & E_FATAL)
		exit(1);
}
