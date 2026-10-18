/* Checks feedback_report.c on the Mac that builds the app (tools/mac_app_bundle.sh runs it): the JSON is valid
   and plain ASCII for any input, the version mapping, the log's tail and the Outbox. Usage: test_feedback_report DIR */
#include "feedback_report.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

static void json_of(const char *text, struct fb_buf *b)
{
	memset(b, 0, sizeof(*b));
	fb_buf_add_json_string(b, text);
}

int main(int argc, char **argv)
{
	struct fb_buf b;
	char dir[1024], path[1100], os[32], id[64], *tail;
	FILE *f;
	int i;
	struct fb_system sys;
	struct fb_report r;
	const char *home = argc > 1 ? argv[1] : "/tmp/feedback_test";

	json_of("a\"b\\c\n\t\x01 caf\xC3\xA9 \xE2\x80\x94 \xF0\x9F\x98\x80", &b);
	CHECK(strcmp(b.data, "\"a\\\"b\\\\c\\n\\t\\u0001 caf\\u00e9 \\u2014 \\ud83d\\ude00\"") == 0);
	fb_buf_free(&b);
	/* bytes that are not UTF-8: replaced, never passed on; a lone surrogate, an overlong form, a cut character */
	json_of("x\xFF\xC0\xAF\xED\xA0\x80\xE2\x80", &b);
	CHECK(strcmp(b.data, "\"x\\ufffd\\ufffd\\ufffd\\ufffd\\ufffd\\ufffd\\ufffd\\ufffd\"") == 0);
	for (i = 0; b.data[i]; ++i)
		CHECK((unsigned char)b.data[i] >= 0x20 && (unsigned char)b.data[i] < 0x7F);
	fb_buf_free(&b);

	CHECK(fb_os_from_darwin("10.8.0", os, sizeof(os)) && strcmp(os, "10.6.0") != 0 && strcmp(os, "10.6.8") == 0);
	CHECK(fb_os_from_darwin("8.11.1", os, sizeof(os)) && strcmp(os, "10.4.11") == 0);
	CHECK(fb_os_from_darwin("9.8.0", os, sizeof(os)) && strcmp(os, "10.5.8") == 0);
	CHECK(!fb_os_from_darwin("25.0.0", os, sizeof(os)));
	CHECK(!fb_os_from_darwin("junk", os, sizeof(os)));

	fb_new_id(id, sizeof(id));
	CHECK(strlen(id) == 15 + 1 + 16);

	memset(&sys, 0, sizeof(sys));
	fb_system_info(&sys);
	CHECK(sys.arch[0] && sys.model[0] && sys.cpu[0] && sys.memory_mb > 0);
	snprintf(sys.screen, sizeof(sys.screen), "1024x768");
	memset(&r, 0, sizeof(r));
	r.id = id;
	r.app = "PowerSpark-test";
	r.version = "0.1";
	r.build = "20261018";
	r.topic = fb_topics[0];
	r.message = "It crashed\nwhen I started a game \xE2\x80\x94 twice";
	r.email = "";
	r.log = "line 1\nline 2 \"quoted\"\n";
	r.system = &sys;
	memset(&b, 0, sizeof(b));
	fb_report_json(&r, &b);
	CHECK(b.data && strstr(b.data, "\"app\":\"PowerSpark-test\"") && strstr(b.data, "\"summary\":\"It crashed\"") &&
		strstr(b.data, "\"system\":{\"os\":") && strstr(b.data, "\"screen\":\"1024x768\"") &&
		strstr(b.data, "\"log\":\"line 1\\nline 2 \\\"quoted\\\"\\n\"") && b.data[b.length - 1] == '}');
	printf("%s\n", b.data);
	fb_buf_free(&b);

	/* a long message is cut on a character, and the summary too */
	{
		char *big = (char *)malloc(FB_MESSAGE_MAX + 64);

		for (i = 0; i < FB_MESSAGE_MAX + 40; ++i)
			big[i] = (i % 2) ? (char)0xA9 : (char)0xC3;	/* "é" over and over: odd cut falls inside one */
		big[FB_MESSAGE_MAX + 40] = 0;
		r.message = big;
		memset(&b, 0, sizeof(b));
		fb_report_json(&r, &b);
		CHECK(!b.failed && b.length < 4 * FB_MESSAGE_MAX && b.data[b.length - 1] == '}');
		CHECK(strstr(b.data, "\\ufffd") == NULL);
		fb_buf_free(&b);
		free(big);
	}

	/* the log: the newest OpenCE-*.log, its end from a line's start, the home written "~" */
	snprintf(dir, sizeof(dir), "%s/logs", home);
	snprintf(path, sizeof(path), "mkdir -p '%s'", dir);
	system(path);
	snprintf(path, sizeof(path), "%s/OpenCE-20260101-000000.log", dir);
	f = fopen(path, "wb");
	fputs("old\n", f);
	fclose(f);
	snprintf(path, sizeof(path), "%s/OpenCE-20260102-000000.log", dir);
	f = fopen(path, "wb");
	for (i = 0; i < 3000; ++i)
		fprintf(f, "line %04d reading %s/Library/x\n", i, home);
	fclose(f);
	tail = fb_read_log_tail(dir, home, 1000);
	CHECK(tail != NULL);
	if (tail)
	{
		CHECK(strncmp(tail, "[the log's start was left out]\nline ", 35) == 0);
		CHECK(strstr(tail, "line 2999 reading ~/Library/x\n") != NULL && strstr(tail, home) == NULL);
		CHECK(strstr(tail, "line 0000") == NULL);
		free(tail);
	}
	tail = fb_read_log_tail(dir, home, 1 << 20);
	CHECK(tail && strncmp(tail, "line 0000", 9) == 0);
	free(tail);
	CHECK(fb_read_log_tail("/nonexistent-folder", home, 100) == NULL);

	/* the Outbox */
	CHECK(fb_outbox_dir(home, dir, sizeof(dir), 1) == 0);
	CHECK(fb_outbox_store(dir, "t-1", "{\"a\":1}", 7) == 0);
	snprintf(path, sizeof(path), "%s/t-1.json", dir);
	f = fopen(path, "rb");
	CHECK(f != NULL);
	if (f)
	{
		char text[16] = "";

		CHECK(fread(text, 1, 15, f) == 7 && strcmp(text, "{\"a\":1}") == 0);
		fclose(f);
	}
	unlink(path);
	CHECK(fb_outbox_store("/nonexistent-folder", "x", "y", 1) != 0);

	printf(failures ? "feedback report: %d FAILED\n" : "feedback report: ok\n", failures);
	return failures != 0;
}
