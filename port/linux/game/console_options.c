#include "cseries.h"
#include "console_options.h"
#include "main/console.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int config_text(const char *name, char *text, size_t size);
int config_default(const char *name, char *text, size_t size);
int config_write(const char *name, const char *value);

static const struct console_option *find_option(const char *command)
{
	unsigned index;
	const struct console_option *option;

	for (index = 0; (option = console_option_get(index)) != NULL; index++)
		if (!strcmp(command, option->command))
			return option;
	return NULL;
}

int console_option_help(const char *command)
{
	const struct console_option *option = find_option(command);

	if (!option)
		return 0;
	console_printf(FALSE, "%s [value|default]: %s", option->command, option->help);
	console_printf(FALSE, "No value prints the current value. Changes are saved in config.toml.");
	return 1;
}

int console_option_execute(const char *expression, int *succeeded)
{
	char command[64], value[128], text[128];
	const char *cursor = expression;
	const struct console_option *option;
	size_t length = 0;
	int parenthesized;

	*succeeded = 0;
	while (isspace((unsigned char)*cursor)) cursor++;
	parenthesized = *cursor == '(';
	if (parenthesized) cursor++;
	while (isspace((unsigned char)*cursor)) cursor++;
	while (*cursor && !isspace((unsigned char)*cursor) && *cursor != ')')
	{
		if (length + 1 >= sizeof(command)) return 0;
		command[length++] = (char)tolower((unsigned char)*cursor++);
	}
	command[length] = 0;
	option = find_option(command);
	if (!option) return 0;
	while (isspace((unsigned char)*cursor)) cursor++;
	length = 0;
	while (*cursor && !isspace((unsigned char)*cursor) && *cursor != ')')
	{
		if (length + 1 >= sizeof(value)) goto invalid;
		value[length++] = (char)tolower((unsigned char)*cursor++);
	}
	value[length] = 0;
	while (isspace((unsigned char)*cursor)) cursor++;
	if (parenthesized)
	{
		if (*cursor++ != ')') goto invalid;
		while (isspace((unsigned char)*cursor)) cursor++;
	}
	if (*cursor) goto invalid;
	if (!length)
	{
		*succeeded = config_text(option->setting, text, sizeof(text));
		if (*succeeded) console_printf(FALSE, "%s = %s", command, text);
		return 1;
	}
	if (!strcmp(value, "default"))
	{
		if (!config_default(option->setting, value, sizeof(value))) goto invalid;
	}
	if (option->type == console_option_boolean)
	{
		if (!strcmp(value, "0") || !strcmp(value, "false") || !strcmp(value, "off"))
			strcpy(text, "false");
		else if (!strcmp(value, "1") || !strcmp(value, "true") || !strcmp(value, "on"))
			strcpy(text, "true");
		else goto invalid;
	}
	else
	{
		char *end;
		double number;

		errno = 0;
		number = strtod(value, &end);
		if (end == value || *end || errno == ERANGE || !isfinite(number) ||
			(!((number >= option->minimum && number <= option->maximum) ||
				(option->allow_zero && number == 0.0)))) goto invalid;
		snprintf(text, sizeof(text), "%.15g", number);
	}
	*succeeded = config_write(option->setting, text);
	if (*succeeded) console_printf(FALSE, "%s = %s (saved)", command, text);
	else console_warning("could not save %s in config.toml; setting unchanged", command);
	return 1;
invalid:
	console_warning("invalid value or syntax for %s", command);
	console_option_help(command);
	return 1;
}
