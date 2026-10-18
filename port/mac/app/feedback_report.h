/* The feedback report PowerSpark sends: its JSON, the Mac it describes, the log it may carry and the
   Outbox that keeps a report until it is sent. Plain C (no Carbon), so that the launcher and the checks
   share it; gnu89, Mac OS X 10.4 and later. */
#ifndef FEEDBACK_REPORT_H
#define FEEDBACK_REPORT_H

#include <stddef.h>

#define FB_TOPICS 7
#define FB_MESSAGE_MAX 20000	/* the endpoint keeps this many bytes of the message */
#define FB_LOG_MAX 49152	/* the log's last bytes that a report carries */

extern const char *const fb_topics[FB_TOPICS];

/* a growing string (malloc); failed is set when memory ran out */
struct fb_buf
{
	char *data;
	size_t length, size;
	int failed;
};

void fb_buf_free(struct fb_buf *b);
void fb_buf_add(struct fb_buf *b, const char *text);
/* text as a JSON string with its quotes: UTF-8 as \uXXXX (surrogate pairs above U+FFFF), bytes that are not
   UTF-8 as U+FFFD, control characters escaped, so the result is plain ASCII and always valid JSON */
void fb_buf_add_json_string(struct fb_buf *b, const char *text);

struct fb_system
{
	char os[32];		/* "10.5.8" */
	char arch[32];		/* hw.machine: "Power Macintosh", "i386" */
	char model[64];		/* hw.model: "PowerBook6,5" */
	char cpu[96];		/* "PowerPC G4 (7450) 1.33 GHz", "Intel(R) Core(TM)2 Duo CPU ..." */
	char screen[32];	/* "1280x800", from the caller (the display is not plain C) */
	unsigned long memory_mb;
};

/* fills in what this Mac is (screen is left as the caller set it) */
void fb_system_info(struct fb_system *s);

/* the Mac OS X version that a Darwin kernel version belongs to ("10.8.0" is "10.6.8", "8.11.1" is "10.4.11");
   0 if it is not one of 10.0 to 10.15 */
int fb_os_from_darwin(const char *darwin, char *out, size_t size);

struct fb_report
{
	const char *id, *app, *version, *build, *topic, *message, *email;
	const char *log;	/* the log's end, or NULL */
	const struct fb_system *system;
};

/* the JSON the endpoint takes (the message is cut at FB_MESSAGE_MAX bytes, on a character) */
void fb_report_json(const struct fb_report *r, struct fb_buf *out);

/* a new report id: the time and 64 unguessable bits (the picture or log kept under it is served by it) */
void fb_new_id(char *out, size_t size);

/* The newest OpenCE-*.log of dir, its last max bytes from a line's start (with a note when cut), the home
   folder's path written as "~". Returns a malloc'ed string, or NULL if there is no log. */
char *fb_read_log_tail(const char *dir, const char *home, size_t max);

/* ~/Library/Application Support/OpenCE/Outbox (made if needed); 0 on success */
int fb_outbox_dir(const char *home, char *out, size_t size, int create);
/* writes the report as dir/ID.json through a temporary file; 0 on success */
int fb_outbox_store(const char *dir, const char *id, const char *json, size_t length);

#endif
