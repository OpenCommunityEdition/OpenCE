/*
CONFIG_CHECK.C

port/linux/src/port_config.c on its own, for tools/test_linux_port.py's
tests of config.toml: built with stand-ins for SDL and the platform layer
(the test writes them), the file in the folder given.

    config_check <folder> get <setting>          prints its value
    config_check <folder> write <setting> <value>
*/

#include "port_config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char folder[1024];

/* (SDL's: the executable's folder, which holds config.toml) */
const char *SDL_GetBasePath(void)
{
	return folder;
}

void platform_log(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	fprintf(stderr, "log: ");
	vfprintf(stderr, format, arguments);
	fprintf(stderr, "\n");
	va_end(arguments);
}

int main(int argc, char **argv)
{
	char text[512];

	if (argc < 4)
		return 2;
	snprintf(folder, sizeof(folder), "%s/", argv[1]);
	if (!strcmp(argv[2], "get") && config_text(argv[3], text, sizeof(text)))
	{
		printf("%s\n", text);
		return 0;
	}
	if (!strcmp(argv[2], "write") && argc == 5)
		return config_write(argv[3], argv[4]) ? 0 : 1;
	return 2;
}
