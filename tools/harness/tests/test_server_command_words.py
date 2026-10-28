"""The words of a dedicated server's commands (port/linux/game/server_commands.c): the tokenizer (spaces, quotes, no
escapes, at most MAXIMUM_WORDS), the text after some words, a reason kept printable, ban durations, numbers in range,
bans.txt's fields, and a player found by slot or the start of their name, with the name as server_status.c reads it
and checks it against the web page's; and how long each role may ban for (a moderator a day at most)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["split-words", "rest-after-words", "printable-text", "durations", "integers", "ban-fields", "find-player",
         "player-matches", "ban-limits"]

SOURCES = {
    "source/game/players.c": ["player_name_character_ascii"],
    "port/linux/game/server_status.c": ["server_status_player_name", "server_status_player_matches"],
    "port/linux/game/server_commands.c": ["split_words", "rest_after_words", "printable_text", "parse_duration",
                                          "parse_integer", "name_begins_with", "find_player", "ban_field",
                                          "ban_duration_allowed"],
}

# a fault in the code, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "quotes-split": (("if (*cursor == '\"')", "if (0)"), "split_words", "split-words"),
    "words-unbounded": (("while (*cursor && count < maximum)", "while (*cursor)"), "split_words", "split-words"),
    "tabs-in-words": (("while (*cursor == ' ' || *cursor == '\\t')", "while (*cursor == ' ')"), "split_words",
                      "split-words"),
    "rest-ignores-quotes": (("if (*line == '\"')", "if (0)"), "rest_after_words", "rest-after-words"),
    "quotes-kept": (("if (last > first && source[first] == '\"' && source[last] == '\"')", "if (0)"),
                    "printable_text", "printable-text"),
    "control-characters-kept": (("if ((unsigned char)source[index] >= 0x20 && (unsigned char)source[index] < 0x7F)",
                                 "if (1)"),
                                "printable_text", "printable-text"),
    "room-overrun": (("length < size - 1", "length < size"), "printable_text", "printable-text"),
    "hours-as-minutes": (("case 'h': unit = 60 * 60; break;", "case 'h': unit = 60; break;"), "parse_duration",
                         "durations"),
    "suffix-unchecked": (("if (*end && end[1])\n\t\treturn -1;", ""), "parse_duration", "durations"),
    "no-ceiling": (("if ((double)number * unit > 10.0 * 365 * 24 * 60 * 60)", "if (0)"), "parse_duration",
                   "durations"),
    "forever-unknown": (('!csstrcasecmp(text, "forever") || ', ""), "parse_duration", "durations"),
    "range-unchecked": (("if (*end || number < minimum || number > maximum)", "if (*end)"), "parse_integer",
                        "integers"),
    "fields-by-their-end": (('snprintf(key, sizeof(key), "\\t%s=", name);', 'snprintf(key, sizeof(key), "%s=", name);'),
                            "ban_field", "ban-fields"),
    "field-runs-on": (('length = (long)strcspn(at, "\\t");', "length = (long)strlen(at);"), "ban_field",
                      "ban-fields"),
    "exact-name-not-first": (("if (exact_count == 1)", "if (exact_count == 1 && 0)"), "find_player", "find-player"),
    "ambiguous-taken": (("if (matches > 1)", "if (0)"), "find_player", "find-player"),
    "slots-unchecked": (("|| !network_player_is_valid(&game->players[number])", ""), "find_player", "find-player"),
    "names-by-case": (("tolower((unsigned char)name[index]) != tolower((unsigned char)text[index])",
                       "name[index] != text[index]"), "name_begins_with", "find-player"),
    "matches-any-case": (("return !strcmp(current, name);", "return !strcasecmp(current, name);"),
                         "server_status_player_matches", "player-matches"),
    "names-uncut": (("&&\n\t\tindex < size - 1; index++)", "; index++)"), "server_status_player_name",
                    "player-matches"),
    "moderator-forever": (("(seconds > 0 && seconds <= MODERATOR_BAN_SECONDS)", "(seconds <= MODERATOR_BAN_SECONDS)"),
                          "ban_duration_allowed", "ban-limits"),
    "moderator-unlimited": (("(seconds > 0 && seconds <= MODERATOR_BAN_SECONDS)", "(seconds > 0)"),
                            "ban_duration_allowed", "ban-limits"),
    "admin-limited": (("role >= _server_role_admin || ", ""), "ban_duration_allowed", "ban-limits"),
}


def generated(fault=None, faulty_function=None):
    commands = read("port/linux/game/server_commands.c")
    # (MAXIMUM_SLOTS is halo_port_limits.h's player count, which constant() does not follow into)
    with_limits = commands + read("port/linux/include/halo_port_limits.h")
    config = f"#define MAXIMUM_WORDS {constant(commands, 'MAXIMUM_WORDS')}\n"
    config += f"#define MAXIMUM_SLOTS {constant(with_limits, 'MAXIMUM_SLOTS')}\n"
    # (a product of numbers, which constant() does not take whole)
    config += f"#define MODERATOR_BAN_SECONDS ({re.search(r'MODERATOR_BAN_SECONDS = ([^,]+),', commands).group(1)})\n"
    config += enum_with(read("port/linux/src/server_console.h"), "_server_role_moderator") + "\n"
    text = ""
    for path, names in SOURCES.items():
        source = read(path)
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("server_command_words", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("server_command_words", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
