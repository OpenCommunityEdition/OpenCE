/* Config file read/modify/write for OpenCE Controller Setup.
   Pure C89, no platform calls beyond stdio/stdlib. */
#ifndef CONTROLLER_SETUP_CONFIG_H
#define CONTROLLER_SETUP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Sets each key "section.name" to its value (TOML text) under [section] of
   the file: the line is replaced in place if it is there, else added after the
   section's last line, else the section is added at the end. Dotted keys at
   the top of a file that has the table are moved into it first (an older
   version wrote them there, and the game then refuses the whole file). All
   other lines are kept. The file is created if missing and is replaced
   through "<path>.tmp". Returns 0, or -1 on an error. */
int cs_config_set(const char *path, const char *const *keys,
	const char *const *values, int count);

/* Copies the value of key (trimmed) into out. Returns 1 if found, 0 if not
   found or the file is missing, -1 on an error (out too small). */
int cs_config_get(const char *path, const char *key, char *out, int out_size);

/* Writes the settings of the calibration screen. */
int cs_config_save_calibration(const char *path, int left_dead_zone,
	int right_dead_zone, int invert_look);

/* Writes only input.gamepad_setup_done = true. */
int cs_config_save_skip(const char *path);

/* Clamps a dead zone to 0..16000. */
int cs_config_clamp_dead_zone(int value);

#ifdef __cplusplus
}
#endif
#endif
