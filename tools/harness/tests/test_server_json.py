"""The real JSON of a dedicated server (port/linux/src/server_json.c): the writer's escaping, nesting and values, the
reader over the vendored jsmn with its depth and token limits, strings unescaped, and base64."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = [
    "escaping", "nesting", "values", "write-depth", "write-size", "read", "strings", "malformed", "primitives",
    "read-depth", "tokens", "base64",
    "utf8-invalid-leads",
    "utf8-overlong-surrogates",
    "numbers-json-only",
    "list-tokens",
    "nul-refused",
]

JSMN = '#include "../../third_party/jsmn/jsmn.h"'

# a fault in server_json.c and the case that must catch it
NEGATIVE_CONTROLS = {
    "grammar-unchecked": (("maximum_tokens <= 0 || !json_check(text, length, maximum_depth))", "maximum_tokens <= 0)"),
                          "malformed"),
    "quotes-unescaped": (("text[run] != '\"' && text[run] != '\\\\')", "text[run] != '\\\\')"), "escaping"),
    "controls-unescaped": (("(unsigned char)text[run] >= 0x20 && (unsigned char)text[run] < 0x80",
                            "(unsigned char)text[run] < 0x80"), "escaping"),
    "no-commas": (("\t\t\tjson_append(json, \",\", 1);\n", ""), "nesting"),
    "nan-written": (("if (value != value || value > 1e300", "if (value > 1e300"), "values"),
    "shallower-writer": (("if (json->depth >= SERVER_JSON_MAXIMUM_DEPTH)",
                          "if (json->depth >= SERVER_JSON_MAXIMUM_DEPTH - 1)"), "write-depth"),
    "no-size-limit": (("if (capacity > SERVER_JSON_MAXIMUM_SIZE || ", "if ("), "write-size"),
    "keys-by-prefix": (("current->type == _server_json_string_token && current->end - current->start == length &&",
                        "current->type == _server_json_string_token &&"), "read"),
    "any-primitive-a-number": (("\telse\n\t\treturn 0;\n\tif (check->at < check->length && check->text[check->at] == '.')",
                                "\tif (check->at < check->length && check->text[check->at] == '.')"), "primitives"),
    "infinite-numbers": (("return *end == 0 && *value - *value == 0.0;", "return *end == 0;"), "numbers-json-only"),
    "surrogates-written": (("!(character == 0xED && second > 0x9F)", "1"), "utf8-overlong-surrogates"),
    "no-depth-limit": (("\t\tif (depth > maximum_depth)\n", "\t\tif (0)\n"), "read-depth"),
    "pairs-split": (("low >= 0xDC00 && low < 0xE000", "low >= 0xDC00 && low < 0xDC00"), "strings"),
    "string-past-room": (("if (out + encoded_length >= size)", "if (out + encoded_length > size)"), "strings"),
    "fewer-tokens": (("maximum_depth, SERVER_JSON_MAXIMUM_TOKENS);", "maximum_depth, SERVER_JSON_MAXIMUM_TOKENS - 1);"),
                     "tokens"),
    "list-fewer-tokens": (("tokens, (unsigned int)maximum_tokens);", "tokens, (unsigned int)maximum_tokens - 1);"),
                          "list-tokens"),
    "body-takes-list-tokens": (("maximum_depth, SERVER_JSON_MAXIMUM_TOKENS);",
                                "maximum_depth, SERVER_JSON_MAXIMUM_LIST_TOKENS);"), "list-tokens"),
    "nul-read": (("if (!json_hex(source + index + 1, &code) || !code)", "if (!json_hex(source + index + 1, &code))"),
                 "nul-refused"),
    "padding-anywhere": (("if (!found || padding)", "if (!found)"), "base64"),
    "bytes-past-most": (("if (count + 3 - padding > maximum)", "if (count + 2 - padding > maximum)"), "base64"),
    "padding-miscounted": (("text[out++] = index + 2 < size ? base64_alphabet[group & 63] : '=';",
                            "text[out++] = index + 1 < size ? base64_alphabet[group & 63] : '=';"), "base64"),
}


def generated(fault=None):
    code = read("port/linux/src/server_json.c")
    if fault:
        code = mutated(code, *fault)
    code = mutated(code, JSMN, '#include "jsmn.h"')
    return (("server_json.h", read("port/linux/src/server_json.h")), ("jsmn.h", read("port/third_party/jsmn/jsmn.h")),
            ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("server_json", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("server_json", generated(fault)), case)
    assert status == CHECK_FAILED, f"server_json.c with a fault ({control}) passed '{case}': the test cannot see it"
