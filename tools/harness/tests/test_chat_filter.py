"""Streamer mode's word filter (chat_filter.c): what it replaces, in which spellings, what it leaves alone
(Scunthorpe), the case and endings of what it writes, and its buffer's bounds."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["replaces", "spellings", "dodges", "leaves-alone", "compounds", "case", "endings", "formatted", "shown",
         "idempotent", "bounds"]
FUNCTIONS = ("chat_filter_letter", "chat_filter_word_character", "chat_filter_lower", "chat_filter_in",
             "chat_filter_character_matches", "chat_filter_match_from", "chat_filter_match", "chat_filter_separator",
             "chat_filter_match_spelled", "chat_filter_ending", "chat_filter_put",
             "chat_filter_replace", "chat_filter_text", "chat_filter_formatted", "chat_filter_cached", "chat_filter_shown",
             "chat_filter_shown_ascii")

# a fault in chat_filter.c, the function or table it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "no-stand-ins": (("if (chat_filter_lower(character) == (wchar_t)stand_ins[index][0]",
                      "if (0 && chat_filter_lower(character) == (wchar_t)stand_ins[index][0]"),
                     "chat_filter_character_matches", "spellings"),
    "no-repeats": (("last < length && chat_filter_lower(text[last]) == chat_filter_lower(text[at]); last++)",
                    "0; last++)"), "chat_filter_match_from", "spellings"),
    "repeats-of-stand-ins": (("chat_filter_lower(text[last]) == chat_filter_lower(text[at]); last++)",
                              "chat_filter_character_matches(text[last], word[index]); last++)"),
                             "chat_filter_match_from", "leaves-alone"),
    "no-backtracking": (("for (; last > at; last--)", "for (; last > at; last = at)"), "chat_filter_match_from",
                        "spellings"),
    "no-spelled-out": (("matched = chat_filter_match_spelled(source, at, source_length, word->word);", "matched = 0;"),
                       "chat_filter_text", "dodges"),
    "no-ending-repeats": (("while (text[index + 1] != text[index] && at < length", "while (0 && at < length"),
                          "chat_filter_ending", "dodges"),
    "anywhere-for-all": (("if (word->match != _chat_filter_anywhere && !word_start)", "if (0)"),
                         "chat_filter_text", "leaves-alone"),
    "any-ending": (("if ((word->match == _chat_filter_word || word->match == _chat_filter_word_endings) &&",
                    "if (0 &&"), "chat_filter_text", "leaves-alone"),
    "lower-case-only": (("if (all_capitals || (first_capital && index == 0))", "if (0)"), "chat_filter_replace",
                        "case"),
    "endings-unfitted": (("else if (last == 'e' && (next == 'e' || next == 'i' || next == 'y'))", "else if (0)"),
                         "chat_filter_replace", "endings"),
    "unbounded": (("if (*length < size - 1)", "if (1)"), "chat_filter_put", "bounds"),
    "codes-filtered": (("while (source[index] && source[index] != '|' && run_length", "while (source[index] && run_length"),
                       "chat_filter_formatted", "formatted"),
    # (the cache's lookup keyed by nothing: the hash, length and text all left out)
    "cache-ignores-key": (('chat_filter_cache[slot].hash != hash ||\n\t\t\tchat_filter_cache[slot].length != length)\n\t\t{\n\t\t\tcontinue;\n\t\t}\n\t\tfor (index = 0; index < length && chat_filter_cache[slot].source[index] == string[index]; index++)\n\t\t\t;\n\t\tif (index == length)',
                           "chat_filter_cache[slot].length < 0)\n\t\t{\n\t\t\tcontinue;\n\t\t}\n\t\tif (1)"),
                          "chat_filter_cached", "shown"),
    "long-text-cut": (("if (length >= CHAT_FILTER_CACHED_SIZE - 1)\n\t\t\treturn string;", "if (0)\n\t\t\treturn string;"),
                      "chat_filter_cached", "shown"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/chat_filter.c")
    text = enum_block(source, "_chat_filter_word,") + enum_block(source, "CHAT_FILTER_PLURAL_ENDINGS =")
    for name in ("struct chat_filter_word\n{", "static struct chat_filter_word const chat_filter_words[] =",
                 "static char const *const chat_filter_endings[] =", "static struct\n{\n\tboolean valid;"):
        start = source.index(name)
        end = source.index("}", start + len(name))
        end = source.index(";", end) + 1
        text += source[start:end] + "\n"
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("under_test.inc", text),)


def enum_block(source, member):
    at = source.index(member)
    start = source.rindex("enum", 0, at)
    return source[start:source.index("};", at) + 2] + "\n"


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("chat_filter", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("chat_filter", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"chat_filter.c with a fault ({control}) passed '{case}': the test cannot see it"


def test_every_word_is_lower_case():
    """The words are matched as written in lower case: one with a capital would never match."""
    source = read("port/linux/game/chat_filter.c")
    table = source[source.index("chat_filter_words[] ="):]
    table = table[:table.index("};")]
    words = re.findall(r'\{ "([^"]+)", "', table)
    assert words and all(word == word.lower() for word in words), words
