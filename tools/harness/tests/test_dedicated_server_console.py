"""What a dedicated server takes from its console and the network (dedicated_server.c): printable words in their
buffers, map names that are never paths, exact addresses, so many connections a minute from an address (an internet
play peer's by its real one), and no password in the log."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["printable", "split", "text", "map-names", "addresses", "join-limit", "join-limit-peers", "join-limit-full",
         "password-not-logged"]
FUNCTIONS = ("real_address", "address_text", "parse_address", "map_name_valid", "printable", "split", "rest_of_line",
             "unquoted", "log_command", "dedicated_server_active", "dedicated_server_join_allowed")

# a fault in dedicated_server.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "control-characters-kept": (("*source >= 32 && *source < 127", "*source != 0"), "printable", "printable"),
    "printable-overruns": (("length < size - 1", "length < size"), "printable", "printable"),
    "split-overruns": (("while (*cursor && count < maximum)", "while (*cursor)"), "split", "split"),
    "quotes-kept": (("kept[0] == '\"' && kept[length - 1] == '\"'", "0"), "unquoted", "text"),
    "dots-in-map-names": (("character == '_' || character == '-'", "character == '_' || character == '-' ||\n"
                           "\t\t\tcharacter == '.' || character == '/'"), "map_name_valid", "map-names"),
    "long-map-names": (("length < MAP_NAME_SIZE", "length < 2 * MAP_NAME_SIZE"), "map_name_valid", "map-names"),
    "address-parts-over-255": (("parts[index] > 255", "parts[index] > 999"), "parse_address", "addresses"),
    "address-tail-taken": (("if (*text)\n\t\treturn FALSE;", ""), "parse_address", "addresses"),
    "no-join-limit": (("slot->allowance--;", ""), "dedicated_server_join_allowed", "join-limit"),
    "allowance-never-back": (("slot->allowance++;", ""), "dedicated_server_join_allowed", "join-limit"),
    "stand-ins-counted-apart": (("address = real_address(address);", ""), "dedicated_server_join_allowed",
                                "join-limit-peers"),
    "full-table-refuses": (("slot->allowance = DEDICATED_JOINS_PER_MINUTE;", "slot->allowance = 0;"),
                           "dedicated_server_join_allowed", "join-limit-full"),
    "password-logged": (("if (stristr(name, \"password\"))", "if (0)"), "log_command", "password-not-logged"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/dedicated_server.c")
    console = read("port/linux/src/dedicated_console.h")
    config = f"#define DEDICATED_CONSOLE_LINE_SIZE {constant(console, 'DEDICATED_CONSOLE_LINE_SIZE')}\n"
    # (the constants' enum, whole: JOIN_REFILL_SECONDS is made of another)
    match = re.search(r"^/\* -+ constants \*/\n\nenum\n\{.*?^\};\n", source, re.M | re.S)
    assert match, "the constants' enum not found in dedicated_server.c"
    config += match.group(0).split("\n", 2)[2]
    match = re.search(r"^struct join_address\n\{.*?^\};\n", source, re.M | re.S)
    assert match, "struct join_address not found in dedicated_server.c"
    config += match.group(0)
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("dedicated_server_console", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("dedicated_server_console", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"dedicated_server.c with a fault ({control}) passed '{case}': the test cannot see it"
