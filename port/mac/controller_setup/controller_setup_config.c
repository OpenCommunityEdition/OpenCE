#include "controller_setup_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CS_MAX_KEYS 16

static int is_space(char c)
{
	return c == ' ' || c == '\t';
}

/* Finds the key of the line [line, line + length); returns its start and
   length, and the offset of the value, or 0 if the line is not an assignment. */
static int parse_line(const char *line, int length, int *key_start,
	int *key_length, int *value_start)
{
	int i = 0;
	int start;

	while (i < length && is_space(line[i]))
		i++;
	if (i >= length || line[i] == '#' || line[i] == '[' || line[i] == '\r' || line[i] == '\n')
		return 0;
	start = i;
	while (i < length && !is_space(line[i]) && line[i] != '=' && line[i] != '\r' && line[i] != '\n')
		i++;
	*key_start = start;
	*key_length = i - start;
	while (i < length && is_space(line[i]))
		i++;
	if (i >= length || line[i] != '=')
		return 0;
	*value_start = i + 1;
	return *key_length > 0;
}

static int line_is_section(const char *line, int length)
{
	int i = 0;

	while (i < length && is_space(line[i]))
		i++;
	return i < length && line[i] == '[';
}

static int key_index(const char *line, int length, const char *const *keys, int count)
{
	int ks, kl, vs, i;

	if (!parse_line(line, length, &ks, &kl, &vs))
		return -1;
	for (i = 0; i < count; i++)
		if ((int)strlen(keys[i]) == kl && memcmp(line + ks, keys[i], kl) == 0)
			return i;
	return -1;
}

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
	char *data = NULL;
	long size = 0;
	long pos, insert_at, i;
	int found[CS_MAX_KEYS];
	char *tmp_path;
	FILE *out;
	int k, eol_crlf = 0, ok = 0;
	long line_end;

	if (!path || count < 0 || count > CS_MAX_KEYS)
		return -1;
	for (k = 0; k < CS_MAX_KEYS; k++)
		found[k] = 0;
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
	for (i = 0; i + 1 < size; i++)
		if (data[i] == '\r' && data[i + 1] == '\n') {
			eol_crlf = 1;
			break;
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

	/* the insertion point: the first [section] line, else the end */
	insert_at = size;
	for (pos = 0; pos < size;) {
		for (line_end = pos; line_end < size && data[line_end] != '\n'; line_end++)
			;
		if (line_is_section(data + pos, (int)(line_end - pos))) {
			insert_at = pos;
			break;
		}
		pos = line_end < size ? line_end + 1 : size;
	}

	ok = 1;
	/* first pass: lines before the insertion point, then the new keys, then the rest.
	   Replacements apply everywhere. */
	for (pos = 0; pos <= size && ok;) {
		long next;

		if (pos == insert_at) {
			/* add the missing keys here, after making sure a line ends before */
			int first = 1;

			for (k = 0; k < count; k++) {
				long j;
				int dup = 0;

				if (found[k])
					continue;
				/* a later occurrence in the file still counts */
				for (j = insert_at; j < size && !dup;) {
					long e;

					for (e = j; e < size && data[e] != '\n'; e++)
						;
					if (key_index(data + j, (int)(e - j), keys, count) == k)
						dup = 1;
					j = e < size ? e + 1 : size;
				}
				if (dup)
					continue;
				if (first && pos > 0 && data[pos - 1] != '\n')
					if (fputs(eol_crlf ? "\r\n" : "\n", out) < 0)
						ok = 0;
				first = 0;
				if (fprintf(out, "%s = %s%s", keys[k], values[k], eol_crlf ? "\r\n" : "\n") < 0)
					ok = 0;
				found[k] = 1;
			}
		}
		if (pos >= size)
			break;
		for (line_end = pos; line_end < size && data[line_end] != '\n'; line_end++)
			;
		next = line_end < size ? line_end + 1 : size;
		{
			int len = (int)(line_end - pos);
			int idx = key_index(data + pos, len, keys, count);

			if (idx >= 0) {
				int has_cr = len > 0 && data[pos + len - 1] == '\r';
				int has_nl = line_end < size;

				if (fprintf(out, "%s = %s", keys[idx], values[idx]) < 0)
					ok = 0;
				found[idx] = 1;
				if (has_cr && fputc('\r', out) == EOF)
					ok = 0;
				if (has_nl && fputc('\n', out) == EOF)
					ok = 0;
			} else if (fwrite(data + pos, 1, (size_t)(next - pos), out) != (size_t)(next - pos))
				ok = 0;
		}
		pos = next;
		if (pos == size && insert_at == size)
			continue; /* loop once more so the insertion at the end runs */
	}
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
	long size, pos, line_end;
	char *data = read_file(path, &size);
	int result = 0;

	if (!data)
		return 0;
	for (pos = 0; pos < size;) {
		int ks, kl, vs, len;
		const char *line;

		for (line_end = pos; line_end < size && data[line_end] != '\n'; line_end++)
			;
		len = (int)(line_end - pos);
		line = data + pos;
		if (parse_line(line, len, &ks, &kl, &vs) && (int)strlen(key) == kl
			&& memcmp(line + ks, key, kl) == 0) {
			int ve = len;
			int n;

			while (vs < ve && is_space(line[vs]))
				vs++;
			while (ve > vs && (is_space(line[ve - 1]) || line[ve - 1] == '\r'))
				ve--;
			n = ve - vs;
			if (n >= out_size) {
				result = -1;
			} else {
				memcpy(out, line + vs, n);
				out[n] = 0;
				result = 1;
			}
			break;
		}
		pos = line_end < size ? line_end + 1 : size;
	}
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
