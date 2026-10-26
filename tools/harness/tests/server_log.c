/*
SERVER_LOG.C (test)

The real log of a dedicated server (port/linux/src/server_log.c): its ring
of the last SERVER_LOG_RING_SIZE lines, numbered, read after a cursor, and
its lines on standard output as text or JSON. test_server_log.py takes the
whole file with its headers and the JSON writer (server_json.c), and
posix_server.c's UTC text; the clock is the test's.
*/

#include "harness.h"

#include <limits.h>
#include <unistd.h>

#include "posix_server.h"

static long long clock_milliseconds = 1000000000123LL;

long long posix_server_unix_milliseconds(void)
{
	return clock_milliseconds;
}

#include "under_test.inc"

static void log_lines(int count)
{
	int index;

	for (index = 0; index < count; index++)
		server_log_ring(_server_log_info, "game", "a line");
}

/* a line's text, as the ring keeps it: "line <its number>" */
static void log_numbered(int count)
{
	int index;

	for (index = 0; index < count; index++)
	{
		char text[32];

		snprintf(text, sizeof(text), "line %lu", server_log_latest() + 1);
		server_log_ring(_server_log_info, "game", text);
	}
}

/* lines read in order, each its own number, from first */
static int lines_from(struct server_log_line const *lines, int count, unsigned long first)
{
	int index;

	for (index = 0; index < count; index++)
	{
		char text[32];

		snprintf(text, sizeof(text), "line %lu", first + (unsigned long)index);
		if (lines[index].number != first + (unsigned long)index || strcmp(lines[index].text, text))
			return 0;
	}
	return 1;
}

static struct server_log_line heard;
static int heard_count;

static void listener(struct server_log_line const *line)
{
	heard = *line;
	heard_count++;
}

static struct server_log_line lines[SERVER_LOG_RING_SIZE + 8];

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	unsigned long next = 99, missed = 99;
	int count;

	/* lines numbered from 1; each read starts after the cursor */
	CASE("numbering")
	{
		CHECK(server_log_latest() == 0, "an empty log has a latest line");
		count = server_log_read(0, lines, 16, &next, &missed);
		CHECK(count == 0 && next == 0 && missed == 0, "an empty log read %d, next %lu, missed %lu", count, next, missed);
		log_numbered(3);
		CHECK(server_log_latest() == 3, "the latest is %lu, not 3", server_log_latest());
		count = server_log_read(0, lines, 16, &next, &missed);
		CHECK(count == 3 && lines_from(lines, count, 1) && next == 3 && missed == 0,
			"from the oldest read %d, next %lu, missed %lu", count, next, missed);
		count = server_log_read(2, lines, 16, &next, &missed);
		CHECK(count == 1 && lines_from(lines, count, 3) && next == 3, "after 2 read %d from %lu", count, lines[0].number);
		count = server_log_read(3, lines, 16, &next, &missed);
		CHECK(count == 0 && next == 3 && missed == 0, "after the latest read %d, next %lu", count, next);
		return 0;
	}
	/* no more than the most asked for; next is the last read */
	CASE("maximum")
	{
		log_numbered(10);
		count = server_log_read(0, lines, 4, &next, &missed);
		CHECK(count == 4 && lines_from(lines, count, 1) && next == 4, "the first four read %d, next %lu", count, next);
		count = server_log_read(next, lines, 4, &next, &missed);
		CHECK(count == 4 && lines_from(lines, count, 5) && next == 8, "the next four read %d, next %lu", count, next);
		count = server_log_read(next, lines, 4, &next, &missed);
		CHECK(count == 2 && lines_from(lines, count, 9) && next == 10, "the last two read %d, next %lu", count, next);
		count = server_log_read(6, lines, 0, &next, &missed);
		CHECK(count == 0 && next == 6, "none asked for read %d, next %lu", count, next);
		return 0;
	}
	/* past SERVER_LOG_RING_SIZE lines, the newest are kept */
	CASE("wrap")
	{
		log_numbered(SERVER_LOG_RING_SIZE + 100);
		count = server_log_read(0, lines, SERVER_LOG_RING_SIZE + 8, &next, &missed);
		CHECK(count == SERVER_LOG_RING_SIZE, "%d lines were read of a full ring", count);
		CHECK(lines_from(lines, count, 101), "the ring did not keep 101 to %d: it began at %lu",
			SERVER_LOG_RING_SIZE + 100, lines[0].number);
		CHECK(next == SERVER_LOG_RING_SIZE + 100 && missed == 100, "next %lu, missed %lu", next, missed);
		/* (the ring's last place, and its first after it) */
		log_numbered(SERVER_LOG_RING_SIZE - 100 - 1);
		count = server_log_read(2 * SERVER_LOG_RING_SIZE - 3, lines, 8, &next, &missed);
		CHECK(count == 2 && lines_from(lines, count, 2 * SERVER_LOG_RING_SIZE - 2) && next == 2 * SERVER_LOG_RING_SIZE - 1,
			"around the ring's end read %d from %lu, next %lu", count, lines[0].number, next);
		log_numbered(2);
		count = server_log_read(2 * SERVER_LOG_RING_SIZE - 2, lines, 8, &next, &missed);
		CHECK(count == 3 && lines_from(lines, count, 2 * SERVER_LOG_RING_SIZE - 1) && missed == 0,
			"across the ring's end read %d from %lu", count, lines[0].number);
		return 0;
	}
	/* a reader behind the ring's start is told how many it missed, and
	reads from the oldest kept */
	CASE("behind")
	{
		log_numbered(SERVER_LOG_RING_SIZE + 100);
		count = server_log_read(50, lines, 4, &next, &missed);
		CHECK(count == 4 && lines_from(lines, count, 101) && missed == 50 && next == 104,
			"after 50 read %d from %lu, missed %lu, next %lu", count, lines[0].number, missed, next);
		count = server_log_read(100, lines, 4, &next, &missed);
		CHECK(count == 4 && lines_from(lines, count, 101) && missed == 0, "after 100 read from %lu, missed %lu",
			lines[0].number, missed);
		count = server_log_read(next, lines, 4, &next, &missed);
		CHECK(count == 4 && lines_from(lines, count, 105) && missed == 0, "the next read from %lu", lines[0].number);
		return 0;
	}
	/* a cursor past the latest (the server restarted) reads nothing, and
	next is the latest */
	CASE("past-latest")
	{
		log_lines(20);
		count = server_log_read(5000, lines, 8, &next, &missed);
		CHECK(count == 0 && next == 20 && missed == 0, "after 5000 read %d, next %lu, missed %lu", count, next, missed);
		count = server_log_read(21, lines, 8, &next, &missed);
		CHECK(count == 0 && next == 20 && missed == 0, "after 21 read %d, next %lu, missed %lu", count, next, missed);
		return 0;
	}
	/* the largest cursor too */
	CASE("past-latest-largest")
	{
		log_lines(20);
		count = server_log_read(ULONG_MAX, lines, 8, &next, &missed);
		CHECK(count == 0 && next == 20 && missed == 0, "after %lu read %d, next %lu, missed %lu", ULONG_MAX, count, next,
			missed);
		return 0;
	}
	/* a line's text kept to printable characters and its size, its category
	to its size ("server" for none), its level and time */
	CASE("text")
	{
		char long_text[1000];

		server_log_ring(_server_log_warning, "chat", "a\nb\rc\td\x1b[31me\x7f" "f\xc3\xa9");
		count = server_log_read(0, lines, 8, &next, &missed);
		CHECK(count == 1 && !strcmp(lines[0].text, "a b c d [31me f\xc3\xa9"), "kept \"%s\"", lines[0].text);
		CHECK(!strcmp(lines[0].category, "chat") && lines[0].level == _server_log_warning &&
			lines[0].time == clock_milliseconds, "kept %s, level %d, time %lld", lines[0].category, lines[0].level,
			lines[0].time);
		memset(long_text, 'x', sizeof(long_text) - 1);
		long_text[sizeof(long_text) - 1] = 0;
		server_log_ring(_server_log_error, NULL, long_text);
		server_log_ring(_server_log_info, "a category too long to keep", "y");
		count = server_log_read(1, lines, 8, &next, &missed);
		CHECK(count == 2 && strlen(lines[0].text) == SERVER_LOG_TEXT_SIZE - 1, "a long text was kept as %d characters",
			(int)strlen(lines[0].text));
		CHECK(!strcmp(lines[0].category, "server"), "no category was kept as %s", lines[0].category);
		CHECK(strlen(lines[1].category) == SERVER_LOG_CATEGORY_SIZE - 1 && !strncmp(lines[1].category, "a category",
			10), "a long category was kept as %s", lines[1].category);
		/* (server_log's text is formatted, to the same size) */
		server_log(_server_log_info, "net", "%s %d", long_text, 5);
		count = server_log_read(3, lines, 8, &next, &missed);
		CHECK(count == 1 && strlen(lines[0].text) == SERVER_LOG_TEXT_SIZE - 1, "a long formatted text was kept as %d",
			(int)strlen(lines[0].text));
		CHECK(!strcmp(server_log_level_name(_server_log_info), "info") &&
			!strcmp(server_log_level_name(_server_log_warning), "warning") &&
			!strcmp(server_log_level_name(_server_log_error), "error"), "a level's name was wrong");
		return 0;
	}
	/* the listener hears every line, as kept, until it is taken away */
	CASE("listener")
	{
		server_log_set_listener(listener);
		server_log_ring(_server_log_info, "platform", "ring\none");
		CHECK(heard_count == 1 && heard.number == 1 && !strcmp(heard.text, "ring one") &&
			!strcmp(heard.category, "platform"), "the ring's line was heard as %d %lu %s", heard_count, heard.number,
			heard.text);
		server_log(_server_log_error, "server", "logged %d", 2);
		CHECK(heard_count == 2 && heard.number == 2 && !strcmp(heard.text, "logged 2") &&
			heard.level == _server_log_error, "a logged line was heard as %d %lu %s", heard_count, heard.number,
			heard.text);
		server_log_set_listener(NULL);
		server_log_ring(_server_log_info, "server", "unheard");
		CHECK(heard_count == 2, "a line was heard after the listener was taken away");
		return 0;
	}
	/* standard output's lines, as text and as JSON; the ring's alone are not
	there */
	CASE("stdout")
	{
		int ends[2];
		char output[2048];
		int length;

		CHECK(!pipe(ends) && dup2(ends[1], STDOUT_FILENO) >= 0, "standard output could not be read");
		server_log(_server_log_info, "lobby", "%s joined", "Chief");
		server_log(_server_log_warning, "net", "slow");
		server_log_ring(_server_log_info, "platform", "the ring's alone");
		server_log_set_json(1);
		server_log(_server_log_error, "chat", "say \"hi\"\tnow");
		server_log_set_json(0);
		server_log(_server_log_info, NULL, "back to text");
		close(ends[1]);
		close(STDOUT_FILENO);
		length = (int)read(ends[0], output, sizeof(output) - 1);
		CHECK(length > 0, "nothing was written");
		output[length] = 0;
		CHECK(!strcmp(output,
			"2001-09-09T01:46:40Z INFO lobby: Chief joined\n"
			"2001-09-09T01:46:40Z WARN net: slow\n"
			"{\"time\":\"2001-09-09T01:46:40Z\",\"level\":\"error\",\"cat\":\"chat\",\"msg\":\"say \\\"hi\\\" now\"}\n"
			"2001-09-09T01:46:40Z INFO server: back to text\n"), "wrote:\n%s", output);
		return 0;
	}
	/* a private line (a secret's) on standard output alone: not in the
	ring, not numbered, not heard */
	CASE("private")
	{
		int ends[2];
		char output[2048];
		int length;

		CHECK(!pipe(ends) && dup2(ends[1], STDOUT_FILENO) >= 0, "standard output could not be read");
		server_log_set_listener(listener);
		server_log(_server_log_info, "server", "the password was made");
		server_log_private(_server_log_info, "server", "the password: %s", "hunter22");
		close(ends[1]);
		close(STDOUT_FILENO);
		length = (int)read(ends[0], output, sizeof(output) - 1);
		CHECK(length > 0, "nothing was written");
		output[length] = 0;
		CHECK(!strcmp(output,
			"2001-09-09T01:46:40Z INFO server: the password was made\n"
			"2001-09-09T01:46:40Z INFO server: the password: hunter22\n"), "wrote:\n%s", output);
		CHECK(server_log_latest() == 1, "the latest is %lu, not 1: the private line was numbered", server_log_latest());
		count = server_log_read(0, lines, 16, &next, &missed);
		CHECK(count == 1 && !strcmp(lines[0].text, "the password was made") && next == 1,
			"the ring read %d lines, the first \"%s\"", count, count ? lines[0].text : "");
		CHECK(heard_count == 1 && !strcmp(heard.text, "the password was made"),
			"the listener heard %d lines, the last \"%s\"", heard_count, heard.text);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
