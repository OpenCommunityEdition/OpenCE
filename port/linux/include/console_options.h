/* Opt-in, local display options. Console aliases do not occupy map-script
function/global indices. Values use the same config store as Settings. */
#ifndef CONSOLE_OPTIONS_H
#define CONSOLE_OPTIONS_H

enum console_option_type { console_option_boolean, console_option_real };
struct console_option
{
	const char *command;
	const char *setting;
	enum console_option_type type;
	double minimum, maximum;
	int allow_zero;
	const char *help;
};
const struct console_option *console_option_get(unsigned index);
/* Returns whether this is a registered option command, including invalid
arguments. succeeded receives whether the query/write succeeded. */
int console_option_execute(const char *expression, int *succeeded);
int console_option_help(const char *command);

#endif
