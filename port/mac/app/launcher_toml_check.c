/* Checks config.toml as the game does: with the vendored TOML parser
   (port/third_party/tomlc17), so that a file the game would refuse (and
   answer by using the defaults) is found before it starts. gnu99. */
#include "launcher_config.h"
#include "tomlc17.h"
#include <stdio.h>
#include <string.h>

int lc_text_check(const char *text, size_t length, char *message, size_t size)
{
	toml_result_t result = toml_parse(text, (int)length);
	int ok = result.ok ? 1 : 0;

	if (!ok && size)
		snprintf(message, size, "%s", result.errmsg);
	else if (size)
		message[0] = 0;
	toml_free(result);
	return ok;
}
