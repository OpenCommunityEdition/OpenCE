/* Config file read/modify/write for OpenCE Controller Setup.
   Pure C89, no platform calls beyond stdio/stdlib. */
#ifndef CONTROLLER_SETUP_CONFIG_H
#define CONTROLLER_SETUP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Replaces the value of each key (a line "key = value", whitespace around
   the key and '=' ignored; comment and [section] lines are never matched) or
   adds the key before the first [section] line (at the end if there is none).
   All other lines are kept byte for byte. The file is created if missing and
   is replaced through "<path>.tmp". Returns 0, or -1 on an error. */
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
