"""MCC reservation ownership and Windows decommit/reload behavior."""
from pathlib import Path
import subprocess
import sys

import pytest
from harness import function
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def implementation():
    text = (ROOT / "port/linux/src/mcc_memory.c").read_text()
    return "\n".join(line for line in text.splitlines()
                     if not line.startswith("#include"))


@pytest.mark.parametrize("windows", [False, True])
def test_reservation_never_replaces_foreign_memory(tmp_path, windows):
    code = r'''
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#define MAP_FAILED ((void *)-1)
#define MAP_PRIVATE 1
#define MAP_ANONYMOUS 2
#define MAP_NORESERVE 4
#define MAP_FIXED_NOREPLACE 8
#define MAP_FIXED 16
#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define __attribute__(x)
#define platform_log(...) ((void)0)
static void *owned;
static int blocked, commit_fail, maps, releases, wrong_address;
static void *mmap(void *p, unsigned n, int prot, int flags, int fd, long offset) {
    (void)fd;(void)offset; maps++;
    if(n!=0x4000000u) {fprintf(stderr,"size\n");return MAP_FAILED;}
    if(flags & MAP_FIXED) {
        if(p!=owned || !owned) {fprintf(stderr,"foreign replacement\n");return MAP_FAILED;}
        if(prot && commit_fail) {errno=ENOMEM;return MAP_FAILED;}
        return p;
    }
    if(!(flags & MAP_FIXED_NOREPLACE) || blocked || owned) {errno=ENOMEM;return MAP_FAILED;}
    owned=wrong_address ? (void *)(uintptr_t)0x60000000u : p;
    return owned;
}
static int munmap(void *p, unsigned n) {
    if(p!=owned || !p || n!=0x4000000u) return -1;
    owned=NULL; releases++; return 0;
}
''' + ("\n#define HALO_WINDOWS 1\n" if windows else "") + implementation() + r'''
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"line %d\n",__LINE__);return 1;}}while(0)
int main(void) {
    void *p; int before;
    CHECK(!mcc_memory_reserve(0x40440000u,0x4000000u));
    CHECK(!mcc_memory_reserve(0x50000001u,0x4000000u));
    CHECK(!mcc_memory_reserve(0x50000000u,0x4000001u));
    CHECK(!mcc_memory_reserve(0x71000000u,0x4000000u));
    CHECK(maps==0);
    blocked=1;
#ifdef HALO_WINDOWS
    mcc_window_initialize(); CHECK(!mcc_window);
#endif
    CHECK(!mcc_memory_reserve(0x50000000u,0x4000000u));
    CHECK(!owned && !releases);
    blocked=0;
    wrong_address=1;
    CHECK(!mcc_memory_reserve(0x50000000u,0x4000000u));
    CHECK(!owned && releases==1);
    wrong_address=0;
#ifdef HALO_WINDOWS
    mcc_window_initialize(); CHECK(owned && !mcc_window_active);
    commit_fail=1;
    CHECK(!mcc_memory_reserve(0x50000000u,0x4000000u));
    CHECK(owned && !mcc_window_active); commit_fail=0;
#endif
    p=mcc_memory_reserve(0x50000000u,0x4000000u); CHECK(p);
    before=maps;
#ifdef HALO_WINDOWS
    CHECK(!mcc_memory_reserve(0x60000000u,0x4000000u)); CHECK(maps==before);
#else
    (void)before;
#endif
    mcc_memory_release(NULL,0);
    mcc_memory_release(p,0x4000000u);
#ifdef HALO_WINDOWS
    CHECK(owned==p && !mcc_window_active);
#else
    CHECK(!owned);
#endif
    CHECK(mcc_memory_reserve(0x50000000u,0x4000000u)==p);
    mcc_memory_release(p,0x4000000u);
    /* A nonstandard base overlapping the default window is still accepted. */
    p=mcc_memory_reserve(0x51000000u,0x4000000u); CHECK(p==(void *)(uintptr_t)0x51000000u);
    mcc_memory_release(p,0x4000000u);
#ifdef HALO_WINDOWS
    mcc_window_dispose();
#endif
    CHECK(!owned);return 0;
}
'''
    source = tmp_path / "memory.c"
    source.write_text(code)
    binary = compile_tool(tmp_path, [source])
    subprocess.run([str(binary)], check=True, capture_output=True, text=True)


@pytest.mark.skipif(sys.platform != "win32", reason="Windows reservation semantics")
def test_windows_pages_are_reserved_decommitted_and_zeroed_on_reload(tmp_path):
    posix = (ROOT / "port/windows/src/win32_posix.c").read_text()
    code = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define HALO_WINDOWS 1
#define platform_log(...) ((void)0)
static int errno_from_windows_error(DWORD e) {(void)e;return ENOMEM;}
''' + '#include "' + (ROOT / "port/windows/include/posix/sys/mman.h").as_posix() + '"\n' + "\n".join(function(posix, name) for name in
                 ["windows_protection", "address_reserved", "mmap", "munmap"]) + implementation() + r'''
#define CHECK(c) do{if(!(c)){fprintf(stderr,"line %d\n",__LINE__);return 1;}}while(0)
int main(void) {
    MEMORY_BASIC_INFORMATION info;
    unsigned char *p;
    CHECK(mcc_window==(void *)(uintptr_t)0x50000000u);
    CHECK(VirtualQuery(mcc_window,&info,sizeof(info)) && info.State==MEM_RESERVE);
    CHECK(!VirtualAlloc(mcc_window,65536,MEM_RESERVE,PAGE_NOACCESS));
    p=mcc_memory_reserve(0x50000000u,0x4000000u);CHECK(p);
    p[0]=42;p[0x3ffffff]=43;
    mcc_memory_release(p,0x4000000u);
    CHECK(VirtualQuery(p,&info,sizeof(info)) && info.State==MEM_RESERVE);
    CHECK(mcc_memory_reserve(0x50000000u,0x4000000u)==p);
    CHECK(p[0]==0 && p[0x3ffffff]==0);
    mcc_memory_release(p,0x4000000u);
    CHECK(mcc_memory_reserve(0x51000000u,0x4000000u));
    mcc_memory_release((void *)(uintptr_t)0x51000000u,0x4000000u);
    CHECK(mcc_memory_reserve(0x50000000u,0x4000000u)==p);
    mcc_memory_release(p,0x4000000u);
    return 0;
}
'''
    source = tmp_path / "memory.c"
    source.write_text(code)
    binary = compile_tool(tmp_path, [source])
    subprocess.run([str(binary)], check=True, capture_output=True, text=True)
