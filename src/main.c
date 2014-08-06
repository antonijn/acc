#include <acc/options.h>

int main(int argc, char *argv[])
{
	options_init(argc, argv);

	options_destroy();
	return 0;
}
