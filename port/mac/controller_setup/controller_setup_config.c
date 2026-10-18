#include "controller_setup_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CS_MAX_KEYS 16

/* The file's text is read and written with the launcher's code
   (../app/launcher_config.c), so that both keep one layout: each key lives
   in its [section] ("input.gamepad_setup_done" under [input]). */
#include "../app/launcher_config.h"

static char *read_file(const char *path, long *size_out)
{
	FILE *f = fopen(path, "rb");
	char *data;
	long size;

	*size_out = 0;
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size < 0) {
		fclose(f);
		return NULL;
	}
	data = (char *)malloc((size_t)size + 1);
	if (!data) {
		fclose(f);
		return NULL;
	}
	if (size > 0 && fread(data, 1, (size_t)size, f) != (size_t)size) {
		free(data);
		fclose(f);
		return NULL;
	}
	fclose(f);
	data[size] = 0;
	*size_out = size;
	return data;
}

int cs_config_set(const char *path, const char *const *keys,
	const char *const *values, int count)
{
	char *data;
	long size = 0;
	size_t length;
	char *tmp_path;
	FILE *out;
	int k, ok = 1;

	if (!path || count < 0 || count > CS_MAX_KEYS)
		return -1;
	data = read_file(path, &size);
	if (!data) {
		FILE *probe = fopen(path, "rb");
		if (probe) {
			fclose(probe);
			return -1;
		}
		data = (char *)calloc(1, 1);
		if (!data)
			return -1;
		size = 0;
	}
	length = (size_t)size;
	/* an older version wrote dotted keys above the file's [input] table */
	if (lc_text_repair(&data, &length) < 0)
		ok = 0;
	for (k = 0; k < count && ok; k++)
		if (lc_text_set(&data, &length, keys[k], values[k]) != 0)
			ok = 0;
	if (!ok) {
		free(data);
		return -1;
	}
	tmp_path = (char *)malloc(strlen(path) + 5);
	if (!tmp_path) {
		free(data);
		return -1;
	}
	sprintf(tmp_path, "%s.tmp", path);
	out = fopen(tmp_path, "wb");
	if (!out) {
		free(tmp_path);
		free(data);
		return -1;
	}
	if (length > 0 && fwrite(data, 1, length, out) != length)
		ok = 0;
	free(data);
	if (fclose(out) != 0)
		ok = 0;
	if (ok && rename(tmp_path, path) != 0)
		ok = 0;
	if (!ok)
		remove(tmp_path);
	free(tmp_path);
	return ok ? 0 : -1;
}

int cs_config_get(const char *path, const char *key, char *out, int out_size)
{
	long size;
	char *data = read_file(path, &size);
	int result;

	if (!data)
		return 0;
	if (out_size <= 0) {
		free(data);
		return -1;
	}
	result = lc_text_get(data, key, out, (size_t)out_size);
	/* (lc_text_get cuts a value that is too long: say so) */
	if (result == 1 && strlen(out) + 1 >= (size_t)out_size)
		result = -1;
	free(data);
	return result;
}

int cs_config_clamp_dead_zone(int value)
{
	if (value < 0)
		return 0;
	if (value > 16000)
		return 16000;
	return value;
}

int cs_config_save_calibration(const char *path, int left_dead_zone,
	int right_dead_zone, int invert_look)
{
	static const char *const keys[4] = {
		"input.gamepad_left_dead_zone",
		"input.gamepad_right_dead_zone",
		"input.gamepad_invert_look",
		"input.gamepad_setup_done"
	};
	char left[16], right[16];
	const char *values[4];

	sprintf(left, "%d", cs_config_clamp_dead_zone(left_dead_zone));
	sprintf(right, "%d", cs_config_clamp_dead_zone(right_dead_zone));
	values[0] = left;
	values[1] = right;
	values[2] = invert_look ? "true" : "false";
	values[3] = "true";
	return cs_config_set(path, keys, values, 4);
}

int cs_config_save_skip(const char *path)
{
	static const char *const keys[1] = { "input.gamepad_setup_done" };
	static const char *const values[1] = { "true" };

	return cs_config_set(path, keys, values, 1);
}
