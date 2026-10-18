/* See feedback_report.h. */
#include "feedback_report.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

const char *const fb_topics[FB_TOPICS] =
{
	"The game crashed or quit", "Something looked wrong", "It was too slow", "A controller didn't work",
	"Network play", "A suggestion", "Something else"
};

/* ---------- strings */

void fb_buf_free(struct fb_buf *b)
{
	free(b->data);
	memset(b, 0, sizeof(*b));
}

static void buf_add_n(struct fb_buf *b, const char *text, size_t n)
{
	if (b->failed)
		return;
	if (b->length + n + 1 > b->size)
	{
		size_t size = b->size ? b->size : 256;
		char *grown;

		while (size < b->length + n + 1)
			size *= 2;
		grown = (char *)realloc(b->data, size);
		if (!grown)
		{
			b->failed = 1;
			return;
		}
		b->data = grown;
		b->size = size;
	}
	memcpy(b->data + b->length, text, n);
	b->length += n;
	b->data[b->length] = 0;
}

void fb_buf_add(struct fb_buf *b, const char *text)
{
	buf_add_n(b, text, strlen(text));
}

static void add_unit(struct fb_buf *b, unsigned long u)
{
	char s[8];

	sprintf(s, "\\u%04lx", u);
	buf_add_n(b, s, 6);
}

/* the length of the UTF-8 character at s (end: the string's end), and its code point; 0 if it is not one */
static int utf8_char(const unsigned char *s, const unsigned char *end, unsigned long *code)
{
	unsigned long c = s[0];
	int n, i;

	if (c < 0x80)
	{
		*code = c;
		return 1;
	}
	if (c >= 0xC2 && c <= 0xDF)
	{
		n = 2;
		c &= 0x1F;
	}
	else if (c >= 0xE0 && c <= 0xEF)
	{
		n = 3;
		c &= 0x0F;
	}
	else if (c >= 0xF0 && c <= 0xF4)
	{
		n = 4;
		c &= 0x07;
	}
	else
		return 0;
	if (end - s < n)
		return 0;
	for (i = 1; i < n; ++i)
	{
		if ((s[i] & 0xC0) != 0x80)
			return 0;
		c = (c << 6) | (s[i] & 0x3F);
	}
	if ((n == 3 && c < 0x800) || (n == 4 && (c < 0x10000 || c > 0x10FFFF)) || (c >= 0xD800 && c <= 0xDFFF))
		return 0;
	*code = c;
	return n;
}

static void json_string_n(struct fb_buf *b, const char *text, size_t length)
{
	const unsigned char *s = (const unsigned char *)text, *end = s + length;

	buf_add_n(b, "\"", 1);
	while (s < end)
	{
		unsigned long code = 0;
		int n = utf8_char(s, end, &code);

		if (n == 0)
		{
			add_unit(b, 0xFFFD);
			++s;
			continue;
		}
		s += n;
		switch (code)
		{
		case '"': buf_add_n(b, "\\\"", 2); break;
		case '\\': buf_add_n(b, "\\\\", 2); break;
		case '\n': buf_add_n(b, "\\n", 2); break;
		case '\r': buf_add_n(b, "\\r", 2); break;
		case '\t': buf_add_n(b, "\\t", 2); break;
		default:
			if (code >= 0x20 && code < 0x7F)
			{
				char c = (char)code;

				buf_add_n(b, &c, 1);
			}
			else if (code >= 0x10000)
			{
				code -= 0x10000;
				add_unit(b, 0xD800 + (code >> 10));
				add_unit(b, 0xDC00 + (code & 0x3FF));
			}
			else
				add_unit(b, code);
		}
	}
	buf_add_n(b, "\"", 1);
}

void fb_buf_add_json_string(struct fb_buf *b, const char *text)
{
	json_string_n(b, text ? text : "", text ? strlen(text) : 0);
}

/* ---------- this Mac */

static int sysctl_text(const char *name, char *out, size_t size)
{
	size_t length = size - 1;

	out[0] = 0;
	if (sysctlbyname(name, out, &length, NULL, 0) != 0)
	{
		out[0] = 0;
		return 0;
	}
	out[size - 1] = 0;
	return out[0] != 0;
}

int fb_os_from_darwin(const char *darwin, char *out, size_t size)
{
	int major = 0, minor = 0, patch = 0;

	if (sscanf(darwin, "%d.%d.%d", &major, &minor, &patch) < 2 || major < 4 || major > 19)
		return 0;
	/* (Darwin 4 is 10.0; each release adds one) */
	snprintf(out, size, "10.%d.%d", major - 4, minor);
	return 1;
}

/* the PowerPC's name from its sub-type (mach/machine.h: 750, 7400, 7450, 970 ...) */
static const char *ppc_name(int subtype)
{
	switch (subtype)
	{
	case 9: return "PowerPC G3 (750)";
	case 10: return "PowerPC G4 (7400)";
	case 11: return "PowerPC G4 (7450)";
	case 100: return "PowerPC G5 (970)";
	}
	return "PowerPC";
}

void fb_system_info(struct fb_system *s)
{
	char text[128];
	unsigned long long memory = 0, hz = 0;
	size_t length;
	int subtype = 0;

	s->os[0] = s->arch[0] = s->model[0] = s->cpu[0] = 0;
	s->memory_mb = 0;
	if (!sysctl_text("kern.osproductversion", s->os, sizeof(s->os)) &&
		sysctl_text("kern.osrelease", text, sizeof(text)))
		fb_os_from_darwin(text, s->os, sizeof(s->os));
	sysctl_text("hw.machine", s->arch, sizeof(s->arch));
	sysctl_text("hw.model", s->model, sizeof(s->model));
	length = sizeof(memory);
	if (sysctlbyname("hw.memsize", &memory, &length, NULL, 0) != 0 || length != sizeof(memory))
	{
		unsigned int small = 0;

		length = sizeof(small);
		memory = sysctlbyname("hw.physmem", &small, &length, NULL, 0) == 0 ? small : 0;
	}
	s->memory_mb = (unsigned long)(memory / (1024 * 1024));
	length = sizeof(hz);
	if (sysctlbyname("hw.cpufrequency", &hz, &length, NULL, 0) != 0 || length != sizeof(hz))
	{
		unsigned int small = 0;

		length = sizeof(small);
		hz = sysctlbyname("hw.cpufrequency", &small, &length, NULL, 0) == 0 ? small : 0;
	}
	if (sysctl_text("machdep.cpu.brand_string", text, sizeof(text)))
	{
		/* (Intel pads the name with runs of spaces) */
		size_t from, to = 0;

		for (from = 0; text[from] && to < sizeof(s->cpu) - 1; ++from)
			if (text[from] != ' ' || (to > 0 && s->cpu[to - 1] != ' '))
				s->cpu[to++] = text[from];
		while (to > 0 && s->cpu[to - 1] == ' ')
			--to;
		s->cpu[to] = 0;
	}
	else
	{
		length = sizeof(subtype);
		if (sysctlbyname("hw.cpusubtype", &subtype, &length, NULL, 0) != 0)
			subtype = 0;
		snprintf(s->cpu, sizeof(s->cpu), "%s", strstr(s->arch, "Power") ? ppc_name(subtype) : s->arch);
	}
	if (hz && !strstr(s->cpu, "GHz"))
	{
		size_t used = strlen(s->cpu);

		if (hz >= 1000000000ULL)
			snprintf(s->cpu + used, sizeof(s->cpu) - used, " %lu.%02lu GHz", (unsigned long)(hz / 1000000000ULL),
				(unsigned long)(hz / 10000000ULL % 100));
		else
			snprintf(s->cpu + used, sizeof(s->cpu) - used, " %lu MHz", (unsigned long)(hz / 1000000ULL));
	}
}

/* ---------- the report */

void fb_new_id(char *out, size_t size)
{
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	char stamp[32];

	stamp[0] = 0;
	if (tm)
		strftime(stamp, sizeof(stamp), "%Y%m%dT%H%M%S", tm);
	snprintf(out, size, "%s-%08x%08x", stamp, (unsigned)arc4random(), (unsigned)arc4random());
}

static void add_field(struct fb_buf *b, const char *name, const char *value, int comma)
{
	if (comma)
		fb_buf_add(b, ",");
	fb_buf_add_json_string(b, name);
	fb_buf_add(b, ":");
	fb_buf_add_json_string(b, value);
}

void fb_report_json(const struct fb_report *r, struct fb_buf *out)
{
	char summary[96], number[32];
	size_t n = 0, message_length = r->message ? strlen(r->message) : 0;
	const struct fb_system *s = r->system;

	/* the message, cut on a character if it is too long */
	if (message_length > FB_MESSAGE_MAX)
	{
		message_length = FB_MESSAGE_MAX;
		while (message_length > 0 && ((unsigned char)r->message[message_length] & 0xC0) == 0x80)
			--message_length;
	}
	/* the summary: the message's first line, 90 bytes at most, on a character */
	while (n < message_length && r->message[n] != '\n' && r->message[n] != '\r' && n < 90)
		++n;
	if (n == 90 && n < message_length)
		while (n > 0 && ((unsigned char)r->message[n] & 0xC0) == 0x80)
			--n;
	memcpy(summary, r->message ? r->message : "", n);
	summary[n] = 0;

	fb_buf_add(out, "{");
	add_field(out, "id", r->id, 0);
	add_field(out, "app", r->app, 1);
	add_field(out, "version", r->version, 1);
	add_field(out, "build", r->build, 1);
	add_field(out, "topic", r->topic, 1);
	add_field(out, "summary", summary, 1);
	fb_buf_add(out, ",\"message\":");
	json_string_n(out, r->message ? r->message : "", message_length);
	add_field(out, "email", r->email ? r->email : "", 1);
	fb_buf_add(out, ",\"system\":{");
	add_field(out, "os", s->os, 0);
	add_field(out, "arch", s->arch, 1);
	add_field(out, "model", s->model, 1);
	add_field(out, "cpu", s->cpu, 1);
	sprintf(number, "%lu", s->memory_mb);
	fb_buf_add(out, ",\"memoryMB\":");
	fb_buf_add(out, number);
	add_field(out, "screen", s->screen, 1);
	fb_buf_add(out, "}");
	if (r->log && r->log[0])
		add_field(out, "log", r->log, 1);
	fb_buf_add(out, "}");
}

/* ---------- the log */

char *fb_read_log_tail(const char *dir, const char *home, size_t max)
{
	DIR *d = opendir(dir);
	struct dirent *e;
	char newest[NAME_MAX + 1], path[PATH_MAX], *raw, *out;
	FILE *f;
	long size;
	size_t start = 0, got, home_length = home ? strlen(home) : 0, used = 0, i;
	const char *note = "";

	if (!d)
		return NULL;
	newest[0] = 0;
	while ((e = readdir(d)) != NULL)
		if (strncmp(e->d_name, "OpenCE-", 7) == 0 && strstr(e->d_name, ".log") && strcmp(e->d_name, newest) > 0 &&
			strlen(e->d_name) <= NAME_MAX)
			strcpy(newest, e->d_name);
	closedir(d);
	if (!newest[0] || strlen(dir) + strlen(newest) + 2 > sizeof(path))
		return NULL;
	sprintf(path, "%s/%s", dir, newest);
	f = fopen(path, "rb");
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if (size <= 0)
	{
		fclose(f);
		return NULL;
	}
	if ((size_t)size > max)
	{
		start = (size_t)size - max;
		note = "[the log's start was left out]\n";
	}
	raw = (char *)malloc((size_t)size - start + 1);
	out = (char *)malloc(((size_t)size - start) * 1 + strlen(note) + 16);
	if (!raw || !out)
	{
		free(raw);
		free(out);
		fclose(f);
		return NULL;
	}
	fseek(f, (long)start, SEEK_SET);
	got = fread(raw, 1, (size_t)size - start, f);
	fclose(f);
	raw[got] = 0;
	i = 0;
	if (start)
	{
		/* (from the next line's start, so that no line begins in its middle) */
		while (i < got && raw[i] != '\n')
			++i;
		if (i < got)
			++i;
	}
	strcpy(out, note);
	used = strlen(note);
	/* the home folder's path would name the player: "~" instead (out has room: "~" is shorter) */
	for (; i < got; ++i)
	{
		if (home_length > 1 && strncmp(raw + i, home, home_length) == 0)
		{
			out[used++] = '~';
			i += home_length - 1;
		}
		else
			out[used++] = raw[i];
	}
	out[used] = 0;
	free(raw);
	return out;
}

/* ---------- the Outbox */

static int make_dir(const char *path)
{
	return mkdir(path, 0755) == 0 || errno == EEXIST ? 0 : -1;
}

int fb_outbox_dir(const char *home, char *out, size_t size, int create)
{
	char p[PATH_MAX];

	if (snprintf(out, size, "%s/Library/Application Support/OpenCE/Outbox", home) >= (int)size)
		return -1;
	if (!create)
		return 0;
	snprintf(p, sizeof(p), "%s/Library", home);
	make_dir(p);
	snprintf(p, sizeof(p), "%s/Library/Application Support", home);
	make_dir(p);
	snprintf(p, sizeof(p), "%s/Library/Application Support/OpenCE", home);
	make_dir(p);
	return make_dir(out);
}

int fb_outbox_store(const char *dir, const char *id, const char *json, size_t length)
{
	char temp[PATH_MAX], path[PATH_MAX];
	FILE *f;
	int ok;

	if (snprintf(temp, sizeof(temp), "%s/.%s.tmp", dir, id) >= (int)sizeof(temp) ||
		snprintf(path, sizeof(path), "%s/%s.json", dir, id) >= (int)sizeof(path))
		return -1;
	f = fopen(temp, "wb");
	if (!f)
		return -1;
	ok = fwrite(json, 1, length, f) == length;
	ok = (fclose(f) == 0) && ok;
	if (!ok || rename(temp, path) != 0)
	{
		unlink(temp);
		return -1;
	}
	return 0;
}
