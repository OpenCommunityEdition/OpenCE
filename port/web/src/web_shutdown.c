/* exit() on a proxied pthread normally runs exitRuntime on the browser
 * thread. That is too late for OPFS: its synchronous operations need a
 * worker, and destroying an open OPFSFile asserts rather than closing it.
 *
 * Run the normal C/C++ exit handlers on the caller first. The cleanup hook
 * registered before entering the game runs after its later-registered hooks
 * (network/profile shutdown), but before WasmFS's startup destructor.
 * __funcs_on_exit drains the list, so the real exit cannot run it twice.
 * Keep EXIT_RUNTIME enabled: the real exit still terminates the runtime.
 */
#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void __funcs_on_exit(void);
extern void __real_exit(int status) __attribute__((noreturn));
extern void debug_file_web_close(void);

static void web_close_files(void)
{
    /* WasmFS's file_table.h defines WASMFS_FD_MAX as 4096. sysconf's
     * POSIX limit need not cover its entire table. Keep stdio 0..2 alive. */
    enum { wasmfs_fd_max = 4096 };

    debug_file_web_close();
    /* Includes stdio streams not owned by the debug logger. Flush BEFORE
     * closing descriptors: musl's stdio exit flushes but does not close them.
     * WasmFS owns the descriptors behind Xbox HANDLEs and raw POSIX opens;
     * close each once, including duplicates, to release OPFS access handles.
     * Do not unlink/unmount /data: saves and maps must remain persistent. */
    fflush(NULL);
    for (int fd = 3; fd < wasmfs_fd_max; ++fd)
        close(fd);
}

void web_shutdown_initialize(void)
{
    if (atexit(web_close_files) != 0)
        abort();
}

void __wrap_exit(int status)
{
    /* All application exits (window, timed quit, dashboard and main return)
     * use this path, not just SDL's quit event. */
    emscripten_cancel_main_loop();
    __funcs_on_exit();
    __real_exit(status);
}
