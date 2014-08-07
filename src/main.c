#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <acc/options.h>
#include <acc/token.h>
#include <acc/error.h>

static void compilefile(FILE * f)
{
	struct token tok;
	while (1) {
		tok = gettok(f);
		if (tok.type == T_EOF) {
			freetok(&tok);
			break;
		}

		printf("%s\n", tok.lexeme);

		freetok(&tok);
	}
}

int main(int argc, char *argv[])
{
	char * filename;
	void * li;

	setlocale(LC_ALL, "C");
	options_init(argc, argv);

	li = list_iterator(option_input());
	while (filename = iterator_next(&li)) {
		FILE * file;

		if (!strcmp(filename, "-")) {
			compilefile(stdin);
			continue;
		}

		file = fopen(filename, "rb");
		if (!file)
			report(E_OPTIONS, NULL, strerror(errno));

		compilefile(file);
	}

	options_destroy();
	return EXIT_SUCCESS;
}
