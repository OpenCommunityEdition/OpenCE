/* server_console_line_is_secret (port/linux/src/server_console.c): which
command lines are kept out of the log, as their command is matched (in any
case) */
#include "harness.h"

#include <ctype.h>
#include <string.h>

#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("secrets")
	{
		static const char *const secret[] = {
			"sv_password hunter22", "SV_PASSWORD x", "  sv_password", "sv_set server.web_password abcdefgh",
			"SV_SET server.web_password abcdefgh", "sv_set SERVER.WEB_MODERATOR_PASSWORD abcdefgh",
			"sv_set server.password x", "\tsv_set  server.web_viewer_password x", "sv_set server.publisher_key_file x",
			"sv_set \"server.web_password\" abcdefgh", "\"sv_password\" x", "\"SV_SET\" \"server.password\" x",
			NULL,
		};
		static const char *const plain[] = {
			"sv_set server.name Passwords", "sv_status", "say my password is safe", "sv_passwordless",
			"sv_settings", "sv_set", "", "kick password", "sv_pass x", "sv_se server.web_password x", NULL,
		};
		int index;

		for (index = 0; secret[index]; index++)
			CHECK(server_console_line_is_secret(secret[index]), "\"%s\" was not a secret's", secret[index]);
		for (index = 0; plain[index]; index++)
			CHECK(!server_console_line_is_secret(plain[index]), "\"%s\" was taken for a secret's", plain[index]);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
