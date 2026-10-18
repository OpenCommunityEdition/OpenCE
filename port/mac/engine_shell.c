#include "engine_shell.h"
#include <string.h>

enum { MAC_ENGINE_SHELL_COMMAND_LINE_CAPACITY = 1024 };

static char command_line[MAC_ENGINE_SHELL_COMMAND_LINE_CAPACITY];
static mac_engine_shell_idle_proc idle_proc;
static void *idle_context;

int mac_engine_shell_set_command_line(const char *value)
{
	size_t length;
	if (!value)
	{
		command_line[0] = '\0';
		return 1;
	}
	length = strlen(value);
	if (length >= sizeof(command_line))
		return 0;
	memcpy(command_line, value, length + 1);
	return 1;
}

void mac_engine_shell_set_idle_proc(mac_engine_shell_idle_proc proc, void *context)
{
	idle_proc = proc;
	idle_context = proc ? context : NULL;
}

const char *shell_get_command_line(void)
{
	return command_line;
}

void shell_idle(void)
{
	if (idle_proc)
		idle_proc(idle_context);
}
