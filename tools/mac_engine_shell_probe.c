#include "engine_shell.h"
#include <stdio.h>
#include <string.h>

const char *shell_get_command_line(void);
void shell_idle(void);

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while (0)

static int idle_calls;
static void idle_probe(void *context)
{
	CHECK(context == &idle_calls);
	++idle_calls;
}

int main(void)
{
	char text[1025];
	memset(text, 'x', sizeof(text));
	text[sizeof(text)-1] = '\0';
	CHECK(strcmp(shell_get_command_line(), "") == 0);
	CHECK(mac_engine_shell_set_command_line("-windowed map=\"Blood Gulch\""));
	CHECK(strcmp(shell_get_command_line(), "-windowed map=\"Blood Gulch\"") == 0);
	CHECK(!mac_engine_shell_set_command_line(text));
	CHECK(strcmp(shell_get_command_line(), "-windowed map=\"Blood Gulch\"") == 0);
	CHECK(mac_engine_shell_set_command_line(NULL));
	CHECK(strcmp(shell_get_command_line(), "") == 0);
	mac_engine_shell_set_idle_proc(idle_probe, &idle_calls);
	shell_idle();
	CHECK(idle_calls == 1);
	mac_engine_shell_set_idle_proc(NULL, &idle_calls);
	shell_idle();
	CHECK(idle_calls == 1);
	printf("Mac engine shell: %d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
