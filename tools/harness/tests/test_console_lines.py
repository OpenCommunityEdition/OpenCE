"""A dedicated server's command lines (port/linux/src/server_console.c): the init file's and standard input's, a line
too long to run left out rather than run cut short, and server.console_exit_on_eof's "quit" after the standard input's
last line, unterminated or not."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = ["eof-quit", "eof-quit-terminated", "eof-go-on", "input-limits", "init-limits"]

HEADERS = ("server_console.h", "server_mode.h", "server_log.h", "posix_server.h")

# a fault in server_console.c and the case that must catch it
NEGATIVE_CONTROLS = {
    "quit-lost-after-last-line": (("\t\t\tif (!server_console.quit_owed)\n\t\t\t\treturn 0;\n",
                                   "\t\t\treturn 0;\n"), "eof-quit"),
    "input-line-cut": (("else if (length >= size)", "else if (length > size)"), "input-limits"),
    "last-input-line-cut": (("if (last && server_console.input_length >= size)",
                             "if (last && server_console.input_length > size)"), "input-limits"),
    "init-line-cut": (("if (strlen(line) >= SERVER_COMMAND_LINE_SIZE)", "if (0)"), "init-limits"),
}


def generated(fault=None):
    console = read("port/linux/src/server_console.c")
    if fault:
        console = mutated(console, *fault)
    files = tuple((name, read(f"port/linux/src/{name}")) for name in HEADERS)
    stubs = (
        ("platform.h", "const char *platform_data_root(void);\n"),
        ("port_config.h", "int config_boolean(const char *name);\nconst char *config_string(const char *name);\n"),
    )
    return files + stubs + (("under_test.inc", console),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("console_lines", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("console_lines", generated(fault)), case)
    assert status == CHECK_FAILED, f"server_console.c with a fault ({control}) passed '{case}': the test cannot see it"
