"""A dedicated server's gametype files (port/linux/src/server_toml.c): the TOML, read by the vendored tomlc17, into a
base, a name and the rules of server_rotation.c's table, each in its own table and range, with the reason and line of
what is refused (unknown keys and tables, two engines, values of the wrong kind, malformed TOML, duplicates, a key
outside a table); and the names a gametype file may have."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["example", "base-name", "sections", "values", "comments", "duplicates", "malformed", "size", "every-rule",
         "name-valid"]

FUNCTIONS = ("server_gametype_file_name_valid", "gametype_value_text", "gametype_error", "server_gametype_file_parse")

# a fault in server_toml.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "aliases-taken": (("if (!rule || strcmp(rule->key, rule_key))", "if (!rule)"), "server_gametype_file_parse",
                      "sections"),
    "tables-unchecked": (("if (strcmp(rule->table, key))", "if (0)"), "server_gametype_file_parse", "sections"),
    "two-engines": (("if (file->engine_table[0] && strcmp(file->engine_table, key))", "if (0)"),
                    "server_gametype_file_parse", "sections"),
    "unknown-tables": (('if (strcmp(key, "universal") && strcmp(key, "options") &&', "if (0 &&"),
                       "server_gametype_file_parse", "sections"),
    "rule-line-of-table": (('rule_value.lineno, "%s goes in %s"', 'value.lineno, "%s goes in %s"'),
                           "server_gametype_file_parse", "sections"),
    "long-name-cut": (("if (strlen(value.u.s) >= sizeof(file->name))", "if (0)"), "server_gametype_file_parse",
                      "base-name"),
    "control-characters-named": (("(unsigned char)value.u.s[index] < 0x20 ||", ""), "server_gametype_file_parse",
                                 "base-name"),
    "base-case-kept": (("file->base[index] = (char)tolower((unsigned char)file->base[index]);", ";"),
                       "server_gametype_file_parse", "base-name"),
    "no-base-taken": (("if (ok && !file->base[0])", "if (0)"), "server_gametype_file_parse", "base-name"),
    "other-values-zero": (("default:\n\t\treturn 0;", 'default:\n\t\tsnprintf(text, (size_t)size, "0");\n\t\treturn 1;'),
                          "gametype_value_text", "values"),
    "size-unlimited": (("if (length > SERVER_GAMETYPE_FILE_MAXIMUM_SIZE)", "if (0)"), "server_gametype_file_parse",
                       "size"),
    "toml-line-lost": (('gametype_error(file, line, "%s%s", result.errmsg, "");',
                        'gametype_error(file, 0, "%s%s", result.errmsg, "");'), "server_gametype_file_parse",
                       "malformed"),
    "paths-named": (("character == '_' || character == '-') ||",
                     "character == '_' || character == '-' || character == '.' || character == '/') ||"),
                    "server_gametype_file_name_valid", "name-valid"),
    "names-unbounded": (("|| index >= SERVER_GAMETYPE_SIZE - 6", ""), "server_gametype_file_name_valid", "name-valid"),
    "empty-name": (("return index > 0;", "return 1;"), "server_gametype_file_name_valid", "name-valid"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/src/server_toml.c")
    text = re.search(r"^static const char \*const engine_tables\[\] = \{[^;]*\};", source, re.M).group(0) + "\n"
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("tomlc17.h", read("port/third_party/tomlc17/tomlc17.h")),
            ("toml.inc", read("port/third_party/tomlc17/tomlc17.c")),
            ("server_rotation.h", read("port/linux/src/server_rotation.h")),
            ("rotation.inc", read("port/linux/src/server_rotation.c")),
            ("server_toml.h", read("port/linux/src/server_toml.h")),
            ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("server_toml", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("server_toml", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"server_toml.c with a fault ({control}) passed '{case}': the test cannot see it"
