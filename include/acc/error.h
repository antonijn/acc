#ifndef ERROR_H
#define ERROR_H

enum errorty {
	E_FATAL = 0x01,
	E_WARNING = 0x02,
	E_HIDE_TOKEN = 0x04,
	E_HIDE_LOCATION = 0x08,

	E_TOKENIZER = E_FATAL | E_HIDE_TOKEN,
};

struct token;
void report(enum errorty ty, struct token * tok, const char * frmt, ...);

#endif
