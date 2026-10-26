"""Which command lines the dedicated server keeps out of its log (port/linux/src/server_console.c): sv_password, and
sv_set of a password's or the publisher key's setting, matched in any case as the commands are."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["secrets"]
NAMES = ["word_is", "holds_word", "next_word", "server_console_line_is_secret"]

# a fault in the code, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "case-matters": (("tolower((unsigned char)word[index]) != tolower((unsigned char)text[index])",
                      "word[index] != text[index]"), "word_is", "secrets"),
    "prefix-is-enough": (("return index == length && !text[index];", "return index == length;"), "word_is",
                         "secrets"),
    "publisher-key-logged": ((' || holds_word(setting, setting_length, "publisher_key")', ""),
                             "server_console_line_is_secret", "secrets"),
    "quotes-kept": (("\t\t*word = ++at;", "\t\t*word = at++;"), "next_word", "secrets"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/src/server_console.c")
    text = ""
    for name in NAMES:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("under_test.inc", text),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("console_secret", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("console_secret", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
