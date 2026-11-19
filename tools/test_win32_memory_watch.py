"""The Windows build's write tracking (port/windows/src/win32_memory_watch.c): compiled as it is, with its
VirtualProtect calls counted, beside a main that reserves part of the Xbox window (0x80000000), commits pages of
it, protects ranges and writes to them. Windows only; needs clang (or CC), not the game.

    python -m pytest -q tools/test_win32_memory_watch.py
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "port/windows/src/win32_memory_watch.c"

# win32_memory_watch.c with its VirtualProtect calls counted
COUNTED = r"""
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

int test_protect_calls;

static BOOL WINAPI counted_VirtualProtect(LPVOID address, SIZE_T size, DWORD protection, PDWORD previous)
{
	test_protect_calls++;
	return VirtualProtect(address, size, protection, previous);
}

void platform_log(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vprintf(format, arguments);
	va_end(arguments);
}

#define VirtualProtect counted_VirtualProtect
#include "win32_memory_watch.c"
"""

MAIN = r"""
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define BASE 0x80000000UL
#define PAGE 0x1000UL

void memory_watch_initialize(void);
void memory_watch_protect(unsigned long address, unsigned long size);
unsigned long memory_watch_generation(unsigned long address, unsigned long size);
extern int test_protect_calls;

/* a write to a page already watched: noticed, and made */
static int noticed_write(unsigned long page)
{
	unsigned long address = BASE + page * PAGE;
	unsigned long before = memory_watch_generation(address, PAGE);

	*(volatile int *)(ULONG_PTR)address = 9;
	return memory_watch_generation(address, PAGE) > before && *(volatile int *)(ULONG_PTR)address == 9;
}

static int read_only(unsigned long page)
{
	MEMORY_BASIC_INFORMATION information;

	VirtualQuery((void *)(ULONG_PTR)(BASE + page * PAGE), &information, sizeof(information));
	return information.Protect == PAGE_READONLY;
}

int main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : "";
	int calls, page, noticed = 0, protected_pages = 0;

	setvbuf(stdout, NULL, _IONBF, 0);
	/* the window reserved as one, its pages committed as the game's are
	(port/windows/src/win32_posix.c's mmap) */
	if (VirtualAlloc((void *)(ULONG_PTR)BASE, 64 * PAGE, MEM_RESERVE, PAGE_NOACCESS) != (void *)(ULONG_PTR)BASE)
	{
		printf("no window\n");
		return 100;
	}
	memory_watch_initialize();
	if (!strcmp(mode, "runs"))
	{
		VirtualAlloc((void *)(ULONG_PTR)BASE, 32 * PAGE, MEM_COMMIT, PAGE_READWRITE);
		/* pages next to each other, none watched: one call */
		calls = test_protect_calls;
		memory_watch_protect(BASE, 8 * PAGE);
		printf("fresh calls=%d\n", test_protect_calls - calls);
		/* one watched in the middle (13): a call either side of it */
		memory_watch_protect(BASE + 13 * PAGE, PAGE);
		calls = test_protect_calls;
		memory_watch_protect(BASE + 10 * PAGE, 8 * PAGE);
		printf("split calls=%d\n", test_protect_calls - calls);
		/* a range that ends on a watched page */
		calls = test_protect_calls;
		memory_watch_protect(BASE + 8 * PAGE, 3 * PAGE);
		printf("ends watched calls=%d\n", test_protect_calls - calls);
		/* all watched: none */
		calls = test_protect_calls;
		memory_watch_protect(BASE, 18 * PAGE);
		printf("again calls=%d\n", test_protect_calls - calls);
		/* a range that is not page aligned: its pages */
		calls = test_protect_calls;
		memory_watch_protect(BASE + 20 * PAGE + 0x800, PAGE);
		printf("unaligned calls=%d\n", test_protect_calls - calls);
		for (page = 0; page < 18; page++)
			protected_pages += read_only(page);
		protected_pages += read_only(20) + read_only(21);
		printf("read_only=%d next=%d\n", protected_pages, read_only(18) + read_only(19) + read_only(22));
		for (page = 0; page < 18; page++)
			noticed += noticed_write(page);
		noticed += noticed_write(20) + noticed_write(21);
		printf("noticed=%d\n", noticed);
		/* written pages (made writable) watched again, as a run */
		calls = test_protect_calls;
		memory_watch_protect(BASE, 8 * PAGE);
		calls = test_protect_calls - calls;
		printf("rewatched calls=%d noticed=%d\n", calls, noticed_write(3));
		return 0;
	}
	if (!strcmp(mode, "hole"))
	{
		/* page 4 reserved but not committed: the call for the run fails,
		and each page is protected on its own */
		VirtualAlloc((void *)(ULONG_PTR)BASE, 4 * PAGE, MEM_COMMIT, PAGE_READWRITE);
		VirtualAlloc((void *)(ULONG_PTR)(BASE + 5 * PAGE), 3 * PAGE, MEM_COMMIT, PAGE_READWRITE);
		calls = test_protect_calls;
		memory_watch_protect(BASE, 8 * PAGE);
		printf("calls=%d\n", test_protect_calls - calls);
		for (page = 0; page < 8; page++)
		{
			if (page != 4)
				noticed += noticed_write(page);
		}
		printf("noticed=%d\n", noticed);
		return 0;
	}
	return 0;
}
"""


@pytest.fixture(scope="module")
def program(tmp_path_factory):
    if sys.platform != "win32":
        pytest.skip("Windows only")
    compiler = os.environ.get("CC", "clang")
    if not shutil.which(compiler):
        pytest.skip(f"no {compiler}")
    work = tmp_path_factory.mktemp("win32_memory_watch")
    (work / "win32_memory_watch.c").write_text(SOURCE.read_text())
    (work / "counted.c").write_text(COUNTED)
    (work / "main.c").write_text(MAIN)
    executable = work / "watch.exe"
    subprocess.run([compiler, "-m32", "-O1", "-g", "-Wall", "-Werror", "-Wno-deprecated-declarations", "-I", str(work),
                    str(work / "main.c"), str(work / "counted.c"), "-o", str(executable), "-Wl,/LARGEADDRESSAWARE"], check=True)
    return executable


def run(program, mode):
    result = subprocess.run([str(program), mode], capture_output=True, text=True, timeout=20)
    if result.returncode == 100:
        pytest.skip("the Xbox window's address is taken in this process")
    return result


def test_pages_next_to_each_other_are_protected_in_one_call(program):
    result = run(program, "runs")
    assert result.returncode == 0, result.stdout + result.stderr
    assert result.stdout.splitlines() == [
        "fresh calls=1",
        # 10-12 and 14-17, page 13 already watched
        "split calls=2",
        # 8 and 9; 10 already watched
        "ends watched calls=1",
        "again calls=0",
        # the half pages at either end: pages 20 and 21
        "unaligned calls=1",
        # the range's pages read-only, and none past them
        "read_only=20 next=0",
        # every page protected: each write noticed (and made)
        "noticed=20",
        "rewatched calls=1 noticed=1",
    ]


def test_a_run_that_cannot_be_protected_is_protected_a_page_at_a_time(program):
    result = run(program, "hole")
    assert result.returncode == 0, result.stdout + result.stderr
    # the failed call, then one for each page (the one not committed fails too)
    assert result.stdout.splitlines() == ["calls=9", "noticed=7"]
