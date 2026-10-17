"""The rules of network_chat.c's text chat: a line's text (another machine's is untrusted), the lines the host takes
from a machine, who hears a team's line, and how long a line shows."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure  # noqa: E402

CASES = ["text-printable", "text-control-characters", "text-bounds", "text-empty", "append", "rate-burst",
         "rate-refill", "rate-wrap", "hears", "alpha"]
FUNCTIONS = ("chat_text_clean", "chat_text_append", "chat_rate_take", "chat_hears", "chat_line_alpha")

# a fault in network_chat.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "control-characters-kept": (("if (character < ' ' || character > '~')", "if (character < 0)"),
                                "chat_text_clean", "text-control-characters"),
    "read-past-size": (("index < source_size && source[index]", "source[index]"), "chat_text_clean", "text-bounds"),
    "no-length-limit": (("&& length < CHAT_TEXT_SIZE - 1; index++", "; index++"), "chat_text_clean", "text-bounds"),
    "trailing-spaces-kept": (("while (length > 0 && destination[length - 1] == ' ')", "while (0)"),
                             "chat_text_clean", "text-printable"),
    "append-unchecked": (("if (*typed >= ' ' && *typed <= '~')", "if (*typed)"), "chat_text_append", "append"),
    "no-burst-limit": (("if (rate->tokens <= 0)\n\t\treturn FALSE;", ""), "chat_rate_take", "rate-burst"),
    "refill-unbounded": (("MIN((unsigned long)CHAT_BURST, rate->tokens + gained)", "(rate->tokens + gained)"),
                         "chat_rate_take", "rate-refill"),
    "team-line-to-all": (("|| sender_team == listener_team", "|| TRUE"), "chat_hears", "hears"),
    "never-fades": (("if (typing || age < CHAT_LINE_MILLISECONDS)", "if (TRUE)"), "chat_line_alpha", "alpha"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/network_chat.c")
    config = ""
    for name in ("CHAT_TEXT_SIZE", "CHAT_BURST", "CHAT_REFILL_MILLISECONDS", "CHAT_LINE_MILLISECONDS",
                 "CHAT_FADE_MILLISECONDS"):
        config += f"#define {name} {constant(source, name)}\n"
    config += structure(source, "chat_rate") + "\n"
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("chat_logic", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("chat_logic", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"network_chat.c with a fault ({control}) passed '{case}': the test cannot see it"
