"""The Linux build's write tracking and crash report (port/linux/src/memory_watch.c): memory_watch.c compiled as
it is (with its mprotect calls counted), beside a platform.h of the test's (a watched window of the test's own
memory), with a main that installs other handlers as a library would, writes to watched pages and faults. Needs
a C compiler (clang, or CC), not the game.

    python -m pytest -q tools/test_memory_watch.py
"""

import os
import shutil
import signal
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "port/linux/src/memory_watch.c"

PLATFORM_H = r"""
#ifndef TEST_PLATFORM_H
#define TEST_PLATFORM_H
typedef int BOOL;
#define TRUE 1
#define FALSE 0
extern unsigned long test_window;
#define PLATFORM_CONTIGUOUS_BASE test_window
#define PLATFORM_CONTIGUOUS_SIZE 0x100000UL
BOOL platform_is_contiguous(const void *address);
#endif
"""

MAIN = r"""
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "platform.h"

void memory_watch_initialize(void);
void memory_watch_protect(unsigned long address, unsigned long size);
unsigned long memory_watch_generation(unsigned long address, unsigned long size);
void memory_watch_begin_frame(void);

unsigned long test_window;
static volatile int debug_lines, foreign_calls;
static sigjmp_buf foreign_jump;
static struct sigaction foreign_previous;

BOOL platform_is_contiguous(const void *address)
{
	return (unsigned long)address - test_window < PLATFORM_CONTIGUOUS_SIZE;
}

void write_to_error_file(char *string, unsigned char date)
{
	(void)string;
	(void)date;
	debug_lines++;
}

/* a library's handler that handles the faults itself (and jumps out) */
static void foreign_jumping(int signal_number, siginfo_t *information, void *context)
{
	(void)signal_number;
	(void)information;
	(void)context;
	foreign_calls++;
	siglongjmp(foreign_jump, 1);
}

/* ... one that hands the fault to the handler it found (this file's) */
static void foreign_chaining(int signal_number, siginfo_t *information, void *context)
{
	foreign_calls++;
	if (foreign_previous.sa_flags & SA_SIGINFO)
		foreign_previous.sa_sigaction(signal_number, information, context);
}

static void foreign_install(void (*handler)(int, siginfo_t *, void *))
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = handler;
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	sigemptyset(&action.sa_mask);
	sigaction(SIGSEGV, &action, &foreign_previous);
}

static int foreign_installed(void)
{
	struct sigaction current;

	sigaction(SIGSEGV, NULL, &current);
	return (current.sa_flags & SA_SIGINFO) &&
		(current.sa_sigaction == foreign_jumping || current.sa_sigaction == foreign_chaining);
}

/* (counted.c's: memory_watch.c's mprotect calls) */
extern int test_protect_calls;

/* a write to a page already watched: noticed, and made */
static int noticed_write(unsigned long offset)
{
	unsigned long before = memory_watch_generation(test_window + offset, 0x1000);

	*(volatile int *)(test_window + offset) = 9;
	return memory_watch_generation(test_window + offset, 0x1000) > before &&
		*(volatile int *)(test_window + offset) == 9;
}

static int watched_write(unsigned long offset)
{
	unsigned long before;

	memory_watch_protect(test_window + offset, 0x1000);
	before = memory_watch_generation(test_window + offset, 0x1000);
	*(volatile int *)(test_window + offset) = 7;
	return memory_watch_generation(test_window + offset, 0x1000) > before &&
		*(volatile int *)(test_window + offset) == 7;
}

int main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : "";
	void *window = mmap(NULL, PLATFORM_CONTIGUOUS_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (window == MAP_FAILED)
		return 100;
	test_window = (unsigned long)window;
	setvbuf(stdout, NULL, _IONBF, 0);
	if (!strcmp(mode, "reinstall"))
	{
		/* the constructor's handler, then a library's over it, then the
		device's initialize */
		foreign_install(foreign_jumping);
		memory_watch_initialize();
		printf("foreign_installed=%d\n", foreign_installed());
		printf("watched_write=%d foreign_calls=%d\n", watched_write(0), foreign_calls);
		/* a fault of nobody's watch: reported, and handed to the library's */
		if (!sigsetjmp(foreign_jump, 1))
			*(volatile int *)0x10 = 1;
		printf("unrelated foreign_calls=%d debug_lines=%d\n", foreign_calls, debug_lines > 0);
		/* (the library jumped out: the next initialize, or frame, puts
		this file's back) */
		memory_watch_begin_frame();
		printf("after_frame foreign_installed=%d watched_write=%d foreign_calls=%d\n", foreign_installed(),
			watched_write(0x2000), foreign_calls);
		return 0;
	}
	if (!strcmp(mode, "runs"))
	{
		int calls, page, noticed = 0;

		/* pages next to each other, none watched: one call */
		calls = test_protect_calls;
		memory_watch_protect(test_window, 8 * 0x1000);
		printf("fresh calls=%d\n", test_protect_calls - calls);
		/* one watched in the middle (13): a call either side of it */
		memory_watch_protect(test_window + 13 * 0x1000, 0x1000);
		calls = test_protect_calls;
		memory_watch_protect(test_window + 10 * 0x1000, 8 * 0x1000);
		printf("split calls=%d\n", test_protect_calls - calls);
		/* a range that ends on a watched page, and one that starts on one */
		calls = test_protect_calls;
		memory_watch_protect(test_window + 8 * 0x1000, 3 * 0x1000);
		printf("ends watched calls=%d\n", test_protect_calls - calls);
		/* all watched: none */
		calls = test_protect_calls;
		memory_watch_protect(test_window, 18 * 0x1000);
		printf("again calls=%d\n", test_protect_calls - calls);
		/* a range that is not page aligned: its pages */
		calls = test_protect_calls;
		memory_watch_protect(test_window + 20 * 0x1000 + 0x800, 0x1000);
		printf("unaligned calls=%d\n", test_protect_calls - calls);
		for (page = 0; page < 18; page++)
			noticed += noticed_write(page * 0x1000);
		noticed += noticed_write(20 * 0x1000);
		noticed += noticed_write(21 * 0x1000);
		printf("noticed=%d\n", noticed);
		/* written pages (made writable) watched again, as a run */
		calls = test_protect_calls;
		memory_watch_protect(test_window, 8 * 0x1000);
		calls = test_protect_calls - calls;
		printf("rewatched calls=%d noticed=%d\n", calls, noticed_write(0x3000));
		return 0;
	}
	if (!strcmp(mode, "hole"))
	{
		int calls, page, noticed = 0;

		/* a page in the range that is not mapped: the call for the run
		fails, and each page is protected on its own */
		munmap((void *)(test_window + 4 * 0x1000), 0x1000);
		calls = test_protect_calls;
		memory_watch_protect(test_window, 8 * 0x1000);
		printf("calls=%d\n", test_protect_calls - calls);
		for (page = 0; page < 8; page++)
		{
			if (page != 4)
				noticed += noticed_write(page * 0x1000);
		}
		printf("noticed=%d\n", noticed);
		return 0;
	}
	if (!strcmp(mode, "without"))
	{
		/* the library's over this file's and no initialize after: the
		watch's fault goes to the library (what the device's call fixes) */
		memory_watch_protect(test_window, 0x1000);
		foreign_install(foreign_jumping);
		if (!sigsetjmp(foreign_jump, 1))
			*(volatile int *)test_window = 1;
		printf("foreign_calls=%d\n", foreign_calls);
		return 0;
	}
	if (!strcmp(mode, "loop"))
	{
		/* a library's handler that chains back to this file's, which chains
		to it: the default action ends it, once reported */
		foreign_install(foreign_chaining);
		memory_watch_initialize();
		*(volatile int *)0x10 = 1;
		return 0;
	}
	if (!strcmp(mode, "segv"))
		*(volatile int *)0x10 = 1;
	else if (!strcmp(mode, "abort"))
		abort();
	else if (!strcmp(mode, "bus"))
		raise(SIGBUS);
	else if (!strcmp(mode, "fpe"))
		raise(SIGFPE);
	else if (!strcmp(mode, "ill"))
		raise(SIGILL);
	else if (!strcmp(mode, "ignored"))
	{
		/* a handler of the program's own before this file's (here: none
		but SIGFPE ignored), kept as what it chains to */
		signal(SIGFPE, SIG_IGN);
		memory_watch_initialize();
		raise(SIGFPE);
		printf("survived\n");
		return 0;
	}
	return 0;
}
"""


# memory_watch.c with its mprotect calls counted
COUNTED = r"""
#include <sys/mman.h>

int test_protect_calls;

static int counted_mprotect(void *address, size_t length, int protection)
{
	test_protect_calls++;
	return mprotect(address, length, protection);
}

#define mprotect counted_mprotect
#include "memory_watch.c"
"""


@pytest.fixture(scope="module")
def program(tmp_path_factory):
    compiler = os.environ.get("CC", "clang")
    if not shutil.which(compiler):
        pytest.skip(f"no {compiler}")
    work = tmp_path_factory.mktemp("memory_watch")
    # (memory_watch.c's "platform.h" is found beside it first: the source is
    # compiled from a folder that has the test's)
    (work / "platform.h").write_text(PLATFORM_H)
    (work / "memory_watch.c").write_text(SOURCE.read_text())
    (work / "counted.c").write_text(COUNTED)
    (work / "main.c").write_text(MAIN)
    executable = work / "watch"
    subprocess.run([compiler, "-std=gnu11", "-O1", "-g", "-Wall", "-Werror", "-Wno-unused-result", "-D_GNU_SOURCE", "-I", str(work),
                    str(work / "main.c"), str(work / "counted.c"), "-o", str(executable)], check=True)
    return executable


def run(program, mode):
    return subprocess.run([str(program), mode], capture_output=True, text=True, timeout=20)


def test_initialize_puts_the_handler_back_over_a_library(program):
    result = run(program, "reinstall")
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    # this file's handler again, the watch's write noted and let through,
    # and the library's handler not called for it
    assert lines[0] == "foreign_installed=0"
    assert lines[1] == "watched_write=1 foreign_calls=0"
    # a fault outside the watch: reported, then the library's
    assert lines[2] == "unrelated foreign_calls=1 debug_lines=1"
    assert "halo-linux: segmentation fault" in result.stderr
    assert lines[3] == "after_frame foreign_installed=0 watched_write=1 foreign_calls=1"


def test_without_initialize_a_library_takes_the_watch(program):
    # what the device's initialize prevents
    result = run(program, "without")
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == "foreign_calls=1"


def test_a_handler_chaining_back_does_not_loop(program):
    result = run(program, "loop")
    assert result.returncode == -signal.SIGSEGV
    assert result.stderr.count("halo-linux: segmentation fault") == 1


@pytest.mark.parametrize("mode, number, name", [
    ("segv", signal.SIGSEGV, "segmentation fault"),
    ("abort", signal.SIGABRT, "abort"),
    ("bus", signal.SIGBUS, "bus error"),
    ("fpe", signal.SIGFPE, "arithmetic exception"),
    ("ill", signal.SIGILL, "illegal instruction"),
])
def test_crash_reported_then_default(program, mode, number, name):
    result = run(program, mode)
    assert result.returncode == -number
    assert result.stderr.count(f"halo-linux: {name} (signal {int(number)},") == 1
    assert "halo-linux: called from" not in result.stderr  # (debug.txt's lines, not the terminal's)


def test_an_ignored_signal_stays_ignored(program):
    result = run(program, "ignored")
    assert result.returncode == 0
    assert result.stdout.strip() == "survived"


def test_pages_next_to_each_other_are_protected_in_one_call(program):
    result = run(program, "runs")
    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == [
        "fresh calls=1",
        # 10-12 and 14-17, page 13 already watched
        "split calls=2",
        # 8 and 9; 10 already watched
        "ends watched calls=1",
        "again calls=0",
        # the half pages at either end: pages 20 and 21
        "unaligned calls=1",
        # every page protected: each write noticed (and made)
        "noticed=20",
        "rewatched calls=1 noticed=1",
    ]


def test_a_run_that_cannot_be_protected_is_protected_a_page_at_a_time(program):
    result = run(program, "hole")
    assert result.returncode == 0, result.stderr
    # the failed call, then one for each page (the one not mapped fails too)
    assert result.stdout.splitlines() == ["calls=9", "noticed=7"]
