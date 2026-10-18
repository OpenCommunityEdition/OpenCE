"""The rules of network_chat.c's text chat: a line's text (another machine's is untrusted), the lines the host takes
from a machine, who hears a team's line and a vehicle's, how long a line shows, and the host's /mute: what it
reads from a typed line, the player a name means, and how long a mute lasts."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["text-printable", "text-control-characters", "text-bounds", "text-empty", "append", "rate-burst",
         "rate-refill", "rate-wrap", "hears", "hears-vehicle", "flags", "alpha", "command-mute", "command-unmute",
         "command-not", "command-usage", "name-match", "name-bounds", "mute-time", "mute-wrap", "mute-again",
         "mute-full"]
FUNCTIONS = ("chat_text_clean", "chat_text_append", "chat_rate_take", "chat_hears", "chat_channel_from_flags",
             "chat_channel_flags", "chat_line_alpha", "chat_lower", "chat_command_word", "chat_command_parse",
             "chat_name_match", "chat_mute_minutes_left", "chat_mute_find", "chat_mute_set", "chat_minutes_plural")

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
    "vehicle-line-to-all": (("return sender_vehicle != NONE && sender_vehicle == listener_vehicle;", "return TRUE;"),
                            "chat_hears", "hears-vehicle"),
    "vehicle-line-on-foot": (("return sender_vehicle != NONE && sender_vehicle", "return sender_vehicle"),
                             "chat_hears", "hears-vehicle"),
    "vehicle-flag-dropped": (("if (channel == _chat_channel_vehicle)", "if (0)"), "chat_channel_flags", "flags"),
    "never-fades": (("if (typing || age < CHAT_LINE_MILLISECONDS)", "if (TRUE)"), "chat_line_alpha", "alpha"),
    "command-word-prefix": (("if (text[index] && text[index] != ' ')\n\t\treturn NULL;", ""), "chat_command_word",
                            "command-not"),
    "command-case": (("if (chat_lower(text[index]) != word[index])", "if (text[index] != word[index])"),
                     "chat_command_word", "command-mute"),
    "minutes-unbounded": (("MIN(typed_minutes * 10 + (rest[index] - '0'), (long)CHAT_MUTE_MAXIMUM_MINUTES)",
                           "typed_minutes * 10 + (rest[index] - '0')"), "chat_command_parse", "command-mute"),
    "minutes-zero": (("if (typed_minutes == 0)\n\t\t\t\treturn _chat_command_usage;", ""), "chat_command_parse",
                     "command-usage"),
    "name-unbounded": (("index < length && index < name_size - 1", "index < length"), "chat_command_parse",
                       "command-mute"),
    "ambiguous-taken": (("if (exact_count > 1 || begins_count > 1)", "if (exact_count > 1 || begins_count > 99)"),
                        "chat_name_match", "name-match"),
    "exact-not-first": (("if (exact_count == 1)\n\t\treturn exact_index;", "if (0)\n\t\treturn exact_index;"),
                        "chat_name_match", "name-match"),
    "mute-never-ends": (("if (elapsed >= mute->milliseconds)", "if (0)"), "chat_mute_minutes_left", "mute-time"),
    "mute-wrap-broken": (("unsigned long elapsed = now - mute->started;",
                          "unsigned long elapsed = now > mute->started ? now - mute->started : 0;"),
                         "chat_mute_minutes_left", "mute-wrap"),
    "mute-doubled": (("struct chat_mute *mute = chat_mute_find(mutes, count, name, now);",
                      "struct chat_mute *mute = NULL;"), "chat_mute_set", "mute-again"),
    "mute-overwrites": (("if (!chat_mute_minutes_left(&mutes[index], now))", "if (TRUE)"), "chat_mute_set",
                        "mute-full"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/network_chat.c")
    config = ""
    for name in ("CHAT_TEXT_SIZE", "CHAT_BURST", "CHAT_REFILL_MILLISECONDS", "CHAT_LINE_MILLISECONDS",
                 "CHAT_FADE_MILLISECONDS", "CHAT_NAME_SIZE", "CHAT_MUTE_DEFAULT_MINUTES", "CHAT_MUTE_MAXIMUM_MINUTES",
                 "CHAT_MILLISECONDS_PER_MINUTE"):
        config += f"#define {name} {constant(source, name)}\n"
    config += f"#define CHAT_NAME_AMBIGUOUS ({re.search(r'CHAT_NAME_AMBIGUOUS = (-?[0-9]+)', source).group(1)})\n"
    config += structure(source, "chat_rate") + "\n"
    config += structure(source, "chat_mute") + "\n"
    config += enum_with(source, "_chat_line_team_bit,") + "\n"
    config += enum_with(source, "_chat_command_none,") + "\n"
    config += enum_with(read("port/linux/game/network_chat.h"), "_chat_channel_everyone,") + "\n"
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
