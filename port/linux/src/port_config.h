/*
PORT_CONFIG.H

The native ports' settings, read from config.toml (port_config.c): next to
the executable on the desktop, in the data folder (the one holding maps/)
on Android. A missing file is written with the defaults. Each setting can
also be set for one run with its HALO_* environment variable, which wins
over the file (the tools and the Android app pass settings that way).

Settings are named "section.key", as in the file: "display.vsync".
*/

#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

#include <stddef.h>

int config_boolean(const char *name);
long config_integer(const char *name);
double config_real(const char *name);
/* never NULL; "" when unset */
const char *config_string(const char *name);
/* sets a setting from its value as text ("true", "60", "1.5", "all"), and
writes it into config.toml (only its line changes); 1 on success */
int config_write(const char *name, const char *value);
int config_write_boolean(const char *name, int value);
/* a setting's value as text ("true", "60", "1.5", "all"); 0 if there is no
such setting */
int config_text(const char *name, char *text, size_t size);
/* the folder config.toml is in, with its separator */
void config_folder(char *path, size_t size);
/* a whole file (a UTF-8 path), NUL terminated, or NULL; free() it (the Android
guest has no SDL_LoadFile) */
char *config_file_read(const char *path, size_t *size);
/* a setting's default, as text; 0 if there is no such setting */
int config_default(const char *name, char *text, size_t size);
/* how many times config_write has changed a setting: what keeps one reads
it again when this moves */
unsigned long config_changes(void);

/* whether this is a dedicated server (server.enabled, --server,
HALO_SERVER: port/linux/src/server_mode.c), whose settings the file holds */
int config_server_mode_enabled(void);

/* a setting, for a list of them (a dedicated server's Settings page): the
one at index; 0 past the last */
struct config_setting_info
{
	const char *name;
	/* "boolean", "integer", "real" or "string" */
	const char *type;
	/* NULL: none */
	const char *environment;
	const char *comment;
	/* as text, a string's without its quotes */
	char default_text[300];
	/* its environment variable sets it for this run */
	int pinned;
	/* this build writes it into config.toml */
	int written;
};
int config_setting_info(int index, struct config_setting_info *info);

/* the settings whose names begin with prefix ("server.") read from
config.toml again, but for those their environment variables set; 1 on
success (a file that cannot be read leaves them as they were) */
int config_reload(const char *prefix);

#endif
