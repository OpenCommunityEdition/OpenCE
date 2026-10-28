"""The real log of a dedicated server (port/linux/src/server_log.c): its ring of the last 2048 lines, numbered and read
after a cursor (the API's, the remote console's, the web page's), and its lines on standard output as text or JSON, a
secret's there alone."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = [
    "numbering", "maximum", "wrap", "behind", "past-latest", "text", "listener", "stdout",
    "past-latest-largest", "private",
]

HEADERS = ("server_log.h", "server_mode.h", "server_json.h", "posix_server.h")

# a fault in server_log.c and the case that must catch it
NEGATIVE_CONTROLS = {
    "kept-a-place-early": (("lines[server_log_globals.latest % SERVER_LOG_RING_SIZE]",
                            "lines[(server_log_globals.latest - 1) % SERVER_LOG_RING_SIZE]"), "numbering"),
    "next-past-the-last": (("*next = number - 1;", "*next = number;"), "maximum"),
    "oldest-overwritten": (("server_log_globals.latest - SERVER_LOG_RING_SIZE + 1 : 1",
                            "server_log_globals.latest - SERVER_LOG_RING_SIZE : 1"), "wrap"),
    "missed-untold": (("*missed = after + 1 < oldest ? oldest - (after + 1) : 0;", "*missed = 0;"), "behind"),
    "restart-not-followed": (("\tif (after > server_log_globals.latest)\n\t\tafter = server_log_globals.latest;\n", ""),
                             "past-latest"),
    "controls-kept": (("character < 0x20 || character == 0x7F ? ' '", "character == 0x7F ? ' '"), "text"),
    "text-unbounded": (("text[index] && index < SERVER_LOG_TEXT_SIZE - 1;", "text[index] && index < SERVER_LOG_TEXT_SIZE;"),
                       "text"),
    "ring-unheard": (("\tlistener = server_log_globals.listener;\n\tpthread_mutex_unlock",
                      "\tlistener = NULL;\n\tpthread_mutex_unlock"), "listener"),
    "never-json": (("if (server_log_globals.json)", "if (0)"), "stdout"),
    "private-kept": (("server_log_write(level, category, text, 0);", "server_log_write(level, category, text, 1);"),
                     "private"),
}


def generated(fault=None):
    log = read("port/linux/src/server_log.c")
    if fault:
        log = mutated(log, *fault)
    json = mutated(read("port/linux/src/server_json.c"), '#include "../../third_party/jsmn/jsmn.h"', '#include "jsmn.h"')
    text = function(read("port/linux/src/posix_server.c"), "posix_server_utc_text") + "\n" + json + "\n" + log
    files = tuple((name, read(f"port/linux/src/{name}")) for name in HEADERS)
    return files + (("jsmn.h", read("port/third_party/jsmn/jsmn.h")), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("server_log", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("server_log", generated(fault)), case)
    assert status == CHECK_FAILED, f"server_log.c with a fault ({control}) passed '{case}': the test cannot see it"
