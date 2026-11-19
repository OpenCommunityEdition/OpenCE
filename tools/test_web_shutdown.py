"""Asset-free worker exit ordering regression (no Emscripten SDK needed)."""
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]


def test_worker_exit_drains_handlers_and_closes_files(tmp_path):
    cc = shutil.which("cc")
    if not cc:
        pytest.skip("needs a C compiler")
    (tmp_path / "emscripten.h").write_text("void emscripten_cancel_main_loop(void);\n")
    driver = tmp_path / "driver.c"
    driver.write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
static void (*handlers[8])(void);
static int count, phase, closed, real_exits;
static jmp_buf done;
int test_atexit(void (*f)(void)) { handlers[count++] = f; return 0; }
void __funcs_on_exit(void) { while (count) handlers[--count](); }
void emscripten_cancel_main_loop(void) { assert(phase == 0); phase = 1; }
void debug_file_web_close(void) { assert(phase == 2); phase = 3; }
int test_fflush(FILE *f) { assert(!f && phase == 3); phase = 4; return 0; }
int test_close(int fd) { assert(phase == 4 && fd == closed + 3); ++closed; return 0; }
void __real_exit(int code) {
    assert(code == 23 && phase == 5 && closed == 4093);
    __funcs_on_exit(); /* runtime's second drain must do nothing */
    ++real_exits;
    longjmp(done, 1);
}
static void wasmfs_destructor(void) { assert(phase == 4); phase = 5; }
static void game_shutdown(void) { assert(phase == 1); phase = 2; }
void web_shutdown_initialize(void);
void __wrap_exit(int);
int main(void) {
    test_atexit(wasmfs_destructor); /* startup's global destructor */
    web_shutdown_initialize();
    test_atexit(game_shutdown); /* game's later-registered hooks */
    if (!setjmp(done)) __wrap_exit(23);
    assert(real_exits == 1 && count == 0 && phase == 5);
    return 0;
}
''')
    exe = tmp_path / "shutdown"
    result = subprocess.run([
        cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(tmp_path),
        "-Datexit=test_atexit", "-Dfflush=test_fflush", "-Dclose=test_close",
        str(ROOT / "port/web/src/web_shutdown.c"), str(driver), "-o", str(exe),
    ], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(exe)], check=True)


def test_web_shutdown_is_wired_for_every_exit():
    from tools.test_web_build import graph, link_flags

    assert "-Wl,--wrap=exit" in link_flags(graph()).split()
    main = (ROOT / "port/web/src/web_main.c").read_text()
    assert main.index("web_shutdown_initialize();") < main.index("web_memory_initialize();")
    assert "exit(halo_game_main());" in main
    source = (ROOT / "source/cseries/errors.c").read_text()
    assert "void debug_file_web_close(void)" in source
    assert "if (debug_file_web_closed)" in source
