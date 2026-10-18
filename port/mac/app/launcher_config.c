/* See launcher_config.h. */
#include "launcher_config.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

enum { T_BOOL, T_INT, T_REAL, T_STRING };

static const struct
{
	const char *key;
	int type;
	const char *def;	/* the game's default for the Mac (port_config.c) */
}
settings[LC_COUNT] =
{
	{ "display.mode", T_STRING, "windowed" },
	{ "display.resolution", T_STRING, "native" },
	{ "display.window_size", T_STRING, "" },
	{ "display.resolution_scaling", T_STRING, "native" },
	{ "display.vsync", T_BOOL, "true" },
	{ "display.max_fps", T_INT, "0" },
	{ "display.interpolation", T_BOOL, "true" },
	{ "display.gamma", T_REAL, "1.0" },
	{ "display.high_res_hud", T_BOOL, "true" },
	{ "display.high_res_text", T_BOOL, "true" },
	{ "display.anti_aliasing", T_STRING, "off" },
	{ "display.shadow_resolution", T_INT, "128" },
	{ "display.per_pixel_lighting", T_BOOL, "false" },
	{ "mac.detail", T_STRING, "auto" },
	{ "mac.texture_skip_levels", T_INT, "-1" },
	{ "audio.enabled", T_BOOL, "true" },
	{ "audio.volume", T_REAL, "1.0" },
	{ "audio.reverb", T_BOOL, "true" },
	{ "game.movies", T_BOOL, "true" },
	{ "game.intro_movies", T_BOOL, "true" },
	{ "game.achievements", T_BOOL, "false" }
};

const char *lc_key(int id)
{
	return settings[id].key;
}

/* ---------- the text of a TOML file */

static int is_blank(char c)
{
	return c == ' ' || c == '\t';
}

/* one line [start, end) of text (end at the newline, before a carriage return) */
struct line
{
	size_t start, end, next;
};

static int next_line(const char *text, size_t length, size_t pos, struct line *l)
{
	size_t e;

	if (pos >= length)
		return 0;
	for (e = pos; e < length && text[e] != '\n'; ++e)
		;
	l->start = pos;
	l->next = e < length ? e + 1 : length;
	if (e > pos && text[e - 1] == '\r')
		--e;
	l->end = e;
	return 1;
}

/* a [section] header: its name in name; 1 for [name], 2 for [[array]] */
static int parse_header(const char *text, const struct line *l, char *name, size_t size)
{
	size_t i = l->start, n = 0;
	int kind = 1;

	while (i < l->end && is_blank(text[i]))
		++i;
	if (i >= l->end || text[i] != '[')
		return 0;
	++i;
	if (i < l->end && text[i] == '[')
	{
		kind = 2;
		++i;
	}
	while (i < l->end && is_blank(text[i]))
		++i;
	while (i < l->end && text[i] != ']' && n + 1 < size)
		name[n++] = text[i++];
	while (n > 0 && is_blank(name[n - 1]))
		--n;
	name[n] = 0;
	return kind;
}

/* a "key = value" line: the key's extent and the value's start; 0 if the line is not one */
static int parse_assignment(const char *text, const struct line *l, size_t *key_start, size_t *key_end,
	size_t *value_start)
{
	size_t i = l->start;

	while (i < l->end && is_blank(text[i]))
		++i;
	if (i >= l->end || text[i] == '#' || text[i] == '[')
		return 0;
	*key_start = i;
	while (i < l->end && !is_blank(text[i]) && text[i] != '=')
		++i;
	*key_end = i;
	while (i < l->end && is_blank(text[i]))
		++i;
	if (i >= l->end || text[i] != '=' || *key_end == *key_start)
		return 0;
	++i;
	while (i < l->end && is_blank(text[i]))
		++i;
	*value_start = i;
	return 1;
}

static void split_key(const char *key, char *section, size_t ssize, char *name, size_t nsize)
{
	const char *dot = strchr(key, '.');

	if (!dot)
	{
		section[0] = 0;
		snprintf(name, nsize, "%s", key);
		return;
	}
	snprintf(section, ssize, "%.*s", (int)(dot - key), key);
	snprintf(name, nsize, "%s", dot + 1);
}

int lc_text_get(const char *text, const char *key, char *out, size_t size)
{
	char section[32], name[64], current[64];
	struct line l;
	size_t pos = 0, ks, ke, vs, n = 0;
	size_t dotted_length;
	char dotted[96];

	if (!text || !size)
		return 0;
	split_key(key, section, sizeof(section), name, sizeof(name));
	snprintf(dotted, sizeof(dotted), "%s", key);
	dotted_length = strlen(dotted);
	current[0] = 0;
	while (next_line(text, strlen(text), pos, &l))
	{
		int kind;

		pos = l.next;
		kind = parse_header(text, &l, current, sizeof(current));
		if (kind)
		{
			if (kind == 2)
				strcpy(current, "[[");
			continue;
		}
		if (!parse_assignment(text, &l, &ks, &ke, &vs))
			continue;
		if (!((strcmp(current, section) == 0 && ke - ks == strlen(name) && strncmp(text + ks, name, ke - ks) == 0) ||
			(current[0] == 0 && ke - ks == dotted_length && strncmp(text + ks, dotted, ke - ks) == 0)))
			continue;
		if (text[vs] == '"')
		{
			++vs;
			while (vs < l.end && text[vs] != '"' && n + 1 < size)
				out[n++] = text[vs++];
		}
		else
			while (vs < l.end && !is_blank(text[vs]) && text[vs] != '#' && n + 1 < size)
				out[n++] = text[vs++];
		out[n] = 0;
		return 1;
	}
	return 0;
}

/* inserts s at offset at */
static int insert_text(char **text, size_t *length, size_t at, const char *s)
{
	size_t n = strlen(s);
	char *grown = (char *)realloc(*text, *length + n + 1);

	if (!grown)
		return -1;
	memmove(grown + at + n, grown + at, *length - at + 1);
	memcpy(grown + at, s, n);
	*text = grown;
	*length += n;
	return 0;
}

static int replace_range(char **text, size_t *length, size_t start, size_t end, const char *s)
{
	size_t n = strlen(s);
	char *grown;

	if (n > end - start)
	{
		grown = (char *)realloc(*text, *length + (n - (end - start)) + 1);
		if (!grown)
			return -1;
		*text = grown;
	}
	memmove(*text + start + n, *text + end, *length - end + 1);
	memcpy(*text + start, s, n);
	*length = *length - (end - start) + n;
	return 0;
}

int lc_text_set(char **text, size_t *length, const char *key, const char *raw)
{
	char section[32], name[64], current[64], line_text[160], dotted_prefix[40];
	const char *eol;
	struct line l;
	size_t pos = 0, ks, ke, vs;
	size_t after_header = (size_t)-1, after_last = (size_t)-1, after_dotted = (size_t)-1;
	int seen_section = 0, i, crlf = 0;

	if (!*text)
	{
		*text = (char *)calloc(1, 1);
		if (!*text)
			return -1;
		*length = 0;
	}
	split_key(key, section, sizeof(section), name, sizeof(name));
	snprintf(dotted_prefix, sizeof(dotted_prefix), "%s.", section);
	for (i = 0; (size_t)i + 1 < *length && i < 4096; ++i)
		if ((*text)[i] == '\r' && (*text)[i + 1] == '\n')
		{
			crlf = 1;
			break;
		}
		else if ((*text)[i] == '\n')
			break;
	eol = crlf ? "\r\n" : "\n";

	current[0] = 0;
	while (next_line(*text, *length, pos, &l))
	{
		int kind;

		pos = l.next;
		kind = parse_header(*text, &l, current, sizeof(current));
		if (kind)
		{
			if (kind == 2)
				strcpy(current, "[[");
			else if (strcmp(current, section) == 0 && !seen_section)
			{
				seen_section = 1;
				after_header = l.next;
				after_last = l.next;
			}
			continue;
		}
		if (!parse_assignment(*text, &l, &ks, &ke, &vs))
			continue;
		if (strcmp(current, section) == 0 && ke - ks == strlen(name) && strncmp(*text + ks, name, ke - ks) == 0)
		{
			snprintf(line_text, sizeof(line_text), "%.*s = %s", (int)(ke - ks), *text + ks, raw);
			return replace_range(text, length, l.start, l.end, line_text);
		}
		if (current[0] == 0 && ke - ks == strlen(key) && strncmp(*text + ks, key, ke - ks) == 0)
		{
			snprintf(line_text, sizeof(line_text), "%s = %s", key, raw);
			return replace_range(text, length, l.start, l.end, line_text);
		}
		if (seen_section && strcmp(current, section) == 0)
			after_last = l.next;
		if (current[0] == 0 && ke - ks > strlen(dotted_prefix) &&
			strncmp(*text + ks, dotted_prefix, strlen(dotted_prefix)) == 0)
			after_dotted = l.next;
	}

	(void)after_header;
	if (seen_section)
	{
		size_t at = after_last;

		snprintf(line_text, sizeof(line_text), "%s = %s%s", name, raw, eol);
		if (at > 0 && (*text)[at - 1] != '\n')
		{
			if (insert_text(text, length, at, eol) != 0)
				return -1;
			at += strlen(eol);
		}
		return insert_text(text, length, at, line_text);
	}
	if (after_dotted != (size_t)-1)
	{
		snprintf(line_text, sizeof(line_text), "%s = %s%s", key, raw, eol);
		if (after_dotted > 0 && (*text)[after_dotted - 1] != '\n')
		{
			if (insert_text(text, length, after_dotted, eol) != 0)
				return -1;
			after_dotted += strlen(eol);
		}
		return insert_text(text, length, after_dotted, line_text);
	}
	snprintf(line_text, sizeof(line_text), "%s[%s]%s%s = %s%s", *length ? eol : "", section, eol, name, raw, eol);
	if (*length > 0 && (*text)[*length - 1] != '\n')
		if (insert_text(text, length, *length, eol) != 0)
			return -1;
	return insert_text(text, length, *length, line_text);
}

/* ---------- the model */

static void put(struct lc_model *m, int id, const char *v)
{
	snprintf(m->value[id], LC_VALUE, "%s", v);
}

void lc_model_defaults(struct lc_model *m)
{
	int i;

	memset(m, 0, sizeof(*m));
	for (i = 0; i < LC_COUNT; ++i)
		put(m, i, settings[i].def);
	memcpy(m->original, m->value, sizeof(m->original));
	strcpy(m->data_source, "auto");
	strcpy(m->data_source_original, "auto");
}

void lc_model_load(struct lc_model *m, const char *text)
{
	int i;
	char v[LC_VALUE];

	lc_model_defaults(m);
	if (!text)
		return;
	for (i = 0; i < LC_COUNT; ++i)
		if (lc_text_get(text, settings[i].key, v, sizeof(v)))
			put(m, i, v);
	/* (the Mac's display.mode is a window unless it says otherwise; an empty one is the desktop builds' default) */
	if (!m->value[LC_MODE][0])
		put(m, LC_MODE, "windowed");
	memcpy(m->original, m->value, sizeof(m->original));
}

int lc_model_save(const struct lc_model *m, const char *path)
{
	char *text = NULL;
	size_t length = 0;
	int i, changed = 0, ok = 1;
	FILE *f = fopen(path, "rb");

	if (f)
	{
		long size;

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (size < 0 || size > 4 * 1024 * 1024)
		{
			fclose(f);
			return -1;
		}
		text = (char *)malloc((size_t)size + 1);
		if (!text || fread(text, 1, (size_t)size, f) != (size_t)size)
		{
			free(text);
			fclose(f);
			return -1;
		}
		text[size] = 0;
		length = (size_t)size;
		fclose(f);
	}
	for (i = 0; i < LC_COUNT && ok; ++i)
	{
		char raw[LC_VALUE + 4];

		if (strcmp(m->value[i], m->original[i]) == 0)
			continue;
		if (settings[i].type == T_STRING)
			snprintf(raw, sizeof(raw), "\"%s\"", m->value[i]);
		else
			snprintf(raw, sizeof(raw), "%s", m->value[i]);
		ok = lc_text_set(&text, &length, settings[i].key, raw) == 0;
		changed = 1;
	}
	if (ok && changed)
	{
		char tmp[1100];

		snprintf(tmp, sizeof(tmp), "%s.tmp", path);
		f = fopen(tmp, "wb");
		if (!f)
			ok = 0;
		else
		{
			ok = fwrite(text, 1, length, f) == length;
			if (fclose(f) != 0)
				ok = 0;
			if (ok && rename(tmp, path) != 0)
				ok = 0;
			if (!ok)
				remove(tmp);
		}
	}
	free(text);
	return ok ? 0 : -1;
}

void lc_model_dump(const struct lc_model *m, FILE *out)
{
	int i;

	for (i = 0; i < LC_COUNT; ++i)
		fprintf(out, "  %-28s %s%s\n", settings[i].key, m->value[i][0] ? m->value[i] : "(empty)",
			strcmp(m->value[i], m->original[i]) ? "  (changed)" : "");
	fprintf(out, "  %-28s %s\n", "data source", m->data_source);
	fprintf(out, "  %-28s %s\n", "don't show at startup", m->hide ? "yes" : "no");
}

int lc_bool(const char *value)
{
	return strcmp(value, "true") == 0 || strcmp(value, "1") == 0;
}

void lc_set_bool(struct lc_model *m, int id, int on)
{
	put(m, id, on ? "true" : "false");
}

int lc_tenths(const char *value, int min, int max)
{
	double v = atof(value);
	int t = (int)(v * 10.0 + (v < 0 ? -0.5 : 0.5));

	return t < min ? min : t > max ? max : t;
}

void lc_set_tenths(struct lc_model *m, int id, int tenths)
{
	char v[LC_VALUE];

	snprintf(v, sizeof(v), "%d.%d", tenths / 10, tenths % 10);
	put(m, id, v);
}
