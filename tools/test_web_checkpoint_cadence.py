"""Compile the real save and frame-clock functions against a synchronous fake disk."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    match = re.search(r"^(?:static )?[\w *]+\b" + name + r"\(\s*[^;{]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(f"Function not found: {name}")
    start = source.index("{", match.start())
    depth, end = 1, start + 1
    # These functions contain no braces inside strings or comments.
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


PREFIX = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean;
typedef float real;
typedef struct { long long QuadPart; } LARGE_INTEGER;
#define TRUE 1
#define FALSE 0
#define CEILING(n,c) ((n)>(c)?(c):(n))
#define PIN(n,f,c) ((n)<(f)?(f):CEILING((n),(c)))
enum { _game_connection_local };
static struct {
    boolean halt_time_scale, restart_time, movie;
    int connection;
    real recording_dt, seconds_elapsed;
    unsigned long frame_start_milliseconds;
    long rasterizer_initial_index;
} main_globals;
static struct { long frame_and_vertical_blank_index; } rasterizer_globals;
static struct { boolean saved_game_valid; } game_state_globals;
static int debug_force_frame_rate_update;
static long long counter = 1000000;
static int write_result;
static char calls[16];
static void record(char call) { size_t n = strlen(calls); calls[n] = call; calls[n + 1] = 0; }
static void QueryPerformanceCounter(LARGE_INTEGER *value) { value->QuadPart = counter; }
static void QueryPerformanceFrequency(LARGE_INTEGER *value) { value->QuadPart = 1000000; }
static unsigned long system_milliseconds(void) { return (unsigned long)(counter / 1000); }
static void profile_seconds_elapsed(real value) { (void)value; }
static void game_state_call_before_save_procs(void) { record('B'); }
static int game_state_write_to_file(void) { record('W'); counter += 5000; return write_result; }
static void game_state_note_event(const char *event) {
    if (strcmp(event, "checkpoint saved")) abort();
    record('E');
}
'''
WRAPPERS = r'''
void main_stop_time(void) { record('S'); actual_main_stop_time(); }
void main_start_time(void) { record('R'); actual_main_start_time(); }
'''
MAIN = r'''
int main(int argc, char **argv)
{
    if (argc != 5) return 2;
    main_globals.halt_time_scale = atoi(argv[1]);
    main_globals.restart_time = atoi(argv[2]);
    write_result = atoi(argv[3]);
    main_update_time_unthrottled();
    counter += atoi(argv[4]);
    game_state_save();
    main_update_time_unthrottled();
    printf("%s %d %d %d %.3f %.3f\n", calls,
        main_globals.halt_time_scale, main_globals.restart_time,
        game_state_globals.saved_game_valid, main_globals.seconds_elapsed * 1000000.0,
        main_globals.halt_time_scale * main_globals.seconds_elapsed * 1000000.0);
    return 0;
}
'''


class CheckpointCadenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        main = (ROOT / "source/main/main.c").read_text()
        saves = (ROOT / "source/saved games/game_state.c").read_text()
        source = PREFIX
        for name in ("main_stop_time", "main_start_time"):
            source += function(main, name).replace(name + "(", "actual_" + name + "(", 1)
        source += WRAPPERS + function(saves, "game_state_save")
        source += function(main, "main_update_time_unthrottled") + MAIN
        path = Path(cls.temp.name) / "checkpoint.c"
        path.write_text(source)
        cls.binaries = {}
        for web in (False, True):
            binary = path.with_name("web" if web else "native")
            command = shlex.split(os.environ.get("CC", "cc"))
            command += ["-std=gnu89", "-Wall", "-Wextra", "-Werror"]
            if web:
                command += ["-DHALO_WEB"]
            subprocess.run(command + [str(path), "-o", str(binary)], check=True, capture_output=True)
            cls.binaries[web] = binary

    def run_save(self, web, running=1, restart=0, result=7, elapsed=16667):
        output = subprocess.check_output(
            [str(self.binaries[web]), str(running), str(restart), str(result), str(elapsed)], text=True
        ).split()
        return output[0], [int(x) for x in output[1:4]], [float(x) for x in output[4:]]

    def test_native_preserves_call_order_and_deferred_restart(self):
        calls, flags, timing = self.run_save(False)
        self.assertEqual((calls, flags), ("BSWRE", [0, 1, 1]))
        self.assertAlmostEqual(timing[0], 21667, delta=1)
        self.assertEqual(timing[1], 0)

    def test_web_preserves_all_prior_pause_and_restart_states(self):
        for running in (0, 1):
            for restart in (0, 1):
                with self.subTest(running=running, restart=restart):
                    calls, flags, _ = self.run_save(True, running, restart)
                    self.assertEqual((calls, flags), ("BWE", [running, restart, 1]))

    def test_save_failure_and_nonzero_success_set_validity_on_both_platforms(self):
        for web in (False, True):
            for result in (0, 1, 7):
                with self.subTest(web=web, result=result):
                    _, flags, _ = self.run_save(web, result=result)
                    self.assertEqual(flags[2], int(result != 0))

    def test_web_next_update_includes_frame_interval_and_completed_save(self):
        _, _, timing = self.run_save(True)
        self.assertAlmostEqual(timing[0], 21667, delta=1)
        self.assertAlmostEqual(timing[1], 21667, delta=1)

    def test_web_existing_clock_cap_still_bounds_a_long_save_interval(self):
        _, _, timing = self.run_save(True, elapsed=2000000)
        self.assertAlmostEqual(timing[0], 1000000 / 15, delta=1)
        self.assertAlmostEqual(timing[1], 1000000 / 15, delta=1)


if __name__ == "__main__":
    unittest.main()
