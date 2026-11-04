"""Windows loader ordering and deferred MCC reservation diagnostics, without maps."""
import io
import re
import shutil
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest
from harness import function
from test_mcc_memory import implementation

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import ninja_syntax, windows_build


def link_flags(monkeypatch, release):
    output = io.StringIO()
    settings = SimpleNamespace(build_dir=Path("build"), compiler_launcher=None,
                               port_release=release, port_profile=False,
                               port_lto="off", port_pgo="off")
    with monkeypatch.context() as setup:
        setup.chdir(ROOT)
        setup.setattr(windows_build.sys, "platform", "win32")
        setup.setattr(windows_build, "fetch_sdl", lambda: None)
        setup.setattr(windows_build, "inline_export_wrapper", lambda source: source)
        windows_build.generate_windows_build(ninja_syntax.Writer(output), settings)
    graph = re.sub(r" \$\n *", " ", output.getvalue())
    return re.search(r"\n  ldflags = ([^\n]+)", graph).group(1).split()


@pytest.mark.parametrize("release", [False, True])
def test_windows_graph_delays_sdl_without_disabling_aslr(monkeypatch, release):
    flags = link_flags(monkeypatch, release)
    assert "-Wl,/DELAYLOAD:SDL3.dll" in flags
    assert "-ldelayimp" in flags
    assert not any("dynamicbase:no" in flag.lower() for flag in flags)


@pytest.mark.skipif(sys.platform != "win32", reason="Windows DLL loader")
@pytest.mark.parametrize("mode", ["delayed", "eager", "late-reservation"])
def test_reservation_precedes_the_first_sdl_call(tmp_path, monkeypatch, mode):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")

    def build(arguments):
        result = subprocess.run([compiler, "--target=i686-pc-windows-msvc", "-fuse-ld=lld",
                                 *map(str, arguments)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr

    # Only this disposable stand-in has ASLR disabled, so eager loading puts
    # it in the MCC window deterministically. It remains relocatable when the
    # real reservation wins. No installed or shipped DLL is modified.
    dll_source = tmp_path / "sdl.c"
    dll_source.write_text("__declspec(dllexport) int SDL_GetVersion(void) { return 3004016; }\n")
    library = tmp_path / "SDL3.lib"
    build(["-shared", dll_source, "-o", tmp_path / "SDL3.dll",
           "-Wl,/BASE:0x50000000,/DYNAMICBASE:NO", f"-Wl,/IMPLIB:{library}"])

    posix = (ROOT / "port/windows/src/win32_posix.c").read_text()
    memory = implementation()
    if mode == "late-reservation":
        memory = memory.replace("constructor(101)", "constructor(103)")
    source = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <timeapi.h>
#define HALO_WINDOWS 1
static int log_calls;
static void platform_log(const char *format, ...) {
    va_list args;
    log_calls++;
    va_start(args,format);vfprintf(stderr,format,args);va_end(args);
    fputc('\n',stderr);
}
''' + '#include "' + (ROOT / "port/windows/include/posix/sys/mman.h").as_posix() + '"\n'
    source += "\n".join(function(posix, name) for name in
                        ["errno_from_windows_error", "windows_protection", "address_reserved", "mmap", "munmap"])
    source += memory + r'''
__declspec(dllimport) int SDL_GetVersion(void);
static int held_before_sdl, version, loaded_before_call;
/* Same priority as CE's config read; the real settings path uses SDL. */
__attribute__((constructor(102))) static void read_settings(void) {
    MEMORY_BASIC_INFORMATION info;
    loaded_before_call=GetModuleHandleA("SDL3.dll")!=NULL;
    held_before_sdl=mcc_window==(void *)(uintptr_t)0x50000000u &&
        VirtualQuery(mcc_window,&info,sizeof(info)) && info.State==MEM_RESERVE;
    version=SDL_GetVersion();
}
int crash_reporter_process(void) {return 0;}
/* Exercise the actual no-console log setup even under pytest's pipes. */
#define _fileno(stream) (-2)
''' + function(posix, "windows_startup") + r'''
#define CHECK(c) do{if(!(c)){return __LINE__;}}while(0)
int main(void) {
    MEMORY_BASIC_INFORMATION info;
    unsigned char *p;
    HMODULE module=GetModuleHandleA("SDL3.dll");
    CHECK(version==3004016 && module);
    CHECK(log_calls==0);
    errno=EDOM;
    windows_startup();
#if EXPECT_SUCCESS
    CHECK(held_before_sdl && !loaded_before_call);
    CHECK((uintptr_t)module<0x50000000u || (uintptr_t)module>=0x54000000u);
    CHECK(log_calls==0);
    p=mcc_memory_reserve(0x50000000u,0x4000000u);CHECK(p);
    p[0]=42;p[0x3ffffff]=43;
    mcc_memory_release(p,0x4000000u);
    CHECK(mcc_memory_reserve(0x50000000u,0x4000000u)==p);
    CHECK(p[0]==0 && p[0x3ffffff]==0);
    mcc_memory_release(p,0x4000000u);
#else
    (void)p;
    CHECK(!held_before_sdl && !mcc_window);
    CHECK((uintptr_t)module==0x50000000u);
    CHECK(VirtualQuery(module,&info,sizeof(info)) && info.Type==MEM_IMAGE);
    CHECK(log_calls==1);
    mcc_memory_log_startup();CHECK(log_calls==1);
    CHECK(!mcc_memory_reserve(0x50000000u,0x4000000u));
    CHECK(SDL_GetVersion()==3004016); /* conflict was not overwritten */
#endif
    return 0;
}
'''
    probe = tmp_path / "startup.c"
    probe.write_text(source)
    flags = link_flags(monkeypatch, True)
    if mode == "eager":
        flags.remove("-Wl,/DELAYLOAD:SDL3.dll")
    binary = tmp_path / "startup.exe"
    build([*flags, "-std=gnu89", "-D_CRT_SECURE_NO_WARNINGS",
           f"-DEXPECT_SUCCESS={int(mode == 'delayed')}", probe, library, "-lwinmm", "-o", binary])
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    assert result.returncode == 0, (mode, result.returncode, result.stderr)
    log = (tmp_path / "halo.log").read_text()
    if mode == "delayed":
        assert not log
    else:
        assert "early reservation at 50000000 (64 MiB) failed (Invalid argument)" in log
        assert "will retry at map load" in log
