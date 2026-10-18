"""Streamer mode's matcher (port/linux/game/text_filter_match.c), taken whole, with a list of made-up words so that the
cases spell nothing offensive: how a word is found however it is dressed (case, accents, look-alikes, leet, repeats,
spaces and punctuation, the game's "|" codes), the rules of where it may be, the allowed words and the levels."""

import subprocess
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, mutated, read  # noqa: E402

# the made-up list: (group, severity, rule, word)
WORDS = [
    ("_text_filter_slurs", 3, "_text_filter_anywhere", "zorbul"),
    ("_text_filter_slurs", 2, "_text_filter_whole", "blik"),
    ("_text_filter_slurs", 1, "_text_filter_start", "quan"),
    ("_text_filter_swearing", 3, "_text_filter_anywhere", "frak"),
    ("_text_filter_swearing", 1, "_text_filter_whole", "darn"),
    ("_text_filter_swearing", 2, "_text_filter_anywhere", "bebo"),
    ("_text_filter_swearing", 2, "_text_filter_anywhere", "wug"),
    ("_text_filter_insults", 2, "_text_filter_whole", "gorp"),
    ("_text_filter_insults", 1, "_text_filter_anywhere", "trillo"),
    ("_text_filter_insults", 1, "_text_filter_whole", "zarch"),
]
ALLOWED = ["frakton"]

ALL = ("1", "1", "1")
# (text, levels, what is shown); narrow cases are the game's char strings
CASES = {
    "plain": [("you frak", ALL, "you ****"), ("FRAK off", ALL, "**** off"), ("zorbul", ALL, "******")],
    "inside-a-name": [("xXzorbulXx", ALL, "xX******Xx"), ("xxfrakxx", ALL, "xx****xx")],
    "accents": [("fr\u00e4k", ALL, "****"), ("FR\u00c2K", ALL, "****"), ("z\u00f8rbul", ALL, "******")],
    "look-alikes": [("fr\u0430k", ALL, "****"), ("\u0444rak", ALL, "\u0444rak"),
                    ("\uff46\uff52\uff41\uff4b", ALL, "****"),
                    ("\u1d00", ALL, "\u1d00"), ("fr\u1d00k", ALL, "****")],
    "leet": [("fr4k", ALL, "****"), ("fr@k", ALL, "****"), ("phrak", ALL, "*****"), ("z0rbu1", ALL, "******"),
             ("vvug", ALL, "****"), ("|-|", ALL, "|-|")],
    "repeats": [("frrraaak", ALL, "********"), ("ffrak", ALL, "*****"), ("zorrbuul", ALL, "********")],
    "doubled-in-the-word": [("trilo", ALL, "trilo"), ("trillo", ALL, "******"), ("trilllllo", ALL, "*********")],
    "punctuation": [("f.r.a.k", ALL, "*******"), ("f-r-a-k", ALL, "*******"), ("f_r_a_k!", ALL, "*******!")],
    "spaces": [("f r a k", ALL, "*******"), ("fr ak", ALL, "*****"), ("f  r  a  k", ALL, "**********"),
               ("zor bul", ALL, "*******"), ("mazor bulb", ALL, "mazor bulb"), ("f r\ra k", ALL, "*******")],
    "too-far-apart": [("f      rak", ALL, "f      rak")],
    "wildcards": [("fr*k", ALL, "****"), ("fr?k", ALL, "****"), ("f**k", ALL, "f**k"), ("*rak", ALL, "*rak"),
                  ("zo?bu?", ALL, "******"), ("****", ALL, "****")],
    "digits-alone": [("8380", ALL, "8380"), ("b3b0", ALL, "****")],
    "hidden-characters": [("fr\u200bak", ALL, "*****"), ("fr\u00adak", ALL, "*****"), ("fra\u0301k", ALL, "*****")],
    "breaks": [("fr\u4e2dak", ALL, "fr\u4e2dak")],
    "wide-bar": [("fr|xak", ALL, "******"), ("|nfrak", ALL, "|n****")],
    "whole": [("blik", ALL, "****"), ("bliks", ALL, "*****"), ("blikes", ALL, "blikes"), ("zarches", ALL, "*******"),
              ("bliker", ALL, "bliker"),
              ("ablik", ALL, "ablik"), ("a blik.", ALL, "a ****."), ("gorp!", ALL, "****!")],
    "short-words": [("blik", ALL, "****"), ("bliik", ALL, "bliik"), ("bliiik", ALL, "******"),
                    ("zarch", ALL, "*****")],
    "start": [("quanx", ALL, "****x"), ("aquan", ALL, "aquan"), ("quan", ALL, "****")],
    "allowed": [("frakton", ALL, "frakton"), ("FR4KTON", ALL, "FR4KTON"), ("frak frakton", ALL, "**** frakton")],
    "levels": [("darn", ("1", "2", "1"), "darn"), ("darn", ("1", "1", "1"), "****"), ("frak", ("1", "0", "1"), "frak"),
               ("frak", ("0", "3", "0"), "****"), ("blik", ("3", "1", "1"), "blik"), ("gorp", ("1", "1", "0"), "gorp")],
}
NARROW_CASES = {
    "narrow-style-codes": [("|bfrak", ALL, "|b****"), ("fr|bak", ALL, "******"), ("f|nr|na|nk", ALL, "**********")],
    "narrow-bar": [("|<", ALL, "|<")],
}

# a fault in text_filter_match.c, and the case that must catch it
NEGATIVE_CONTROLS = {
    "no-punctuation-between": (("if (term_index > 0 && gap < MAXIMUM_GAP &&", "if (0 &&"), "punctuation"),
    "no-repeats": (("if (repeated != end &&", "if (0 &&"), "repeats"),
    "short-words-stretched": (("attempt->length > 4 || repeated - end >= 2", "1"), "short-words"),
    "no-look-alikes": (("*letters = text_filter_fold_letters(code);", "*letters = 0;"), "look-alikes"),
    "nothing-allowed": (("if (text_filter_allowed(attempt))", "if (0)"), "allowed"),
    "wildcards-unlimited": (("if (path.wildcards && (", "if (0 && ("), "wildcards"),
    "digits-alone-enough": (("if (!path.real_letters || path.real_letters * 3 < attempt->length)", "if (0)"),
                            "digits-alone"),
    "spaces-anywhere": (("if (path.spaced && path.longest_piece > 2 && !(start_boundary && end_boundary))", "if (0)"),
                        "spaces"),
    "whole-anywhere": (("\tcase _text_filter_whole:\n\t\tif (!start_boundary)",
                        "\tcase _text_filter_whole:\n\t\tif (0)"),
                       "whole"),
    "levels-ignored": (("term->severity < levels[term->group]", "0"), "levels"),
}


def encoded(word):
    return "".join(f"\\x{ord(character) ^ 0x5A:02X}" for character in word)


def terms_include():
    lines = ["#define TEXT_FILTER_TERM_KEY 0x5A", "static struct text_filter_term_definition const "
             "text_filter_term_definitions[] = {"]
    lines += [f'\t{{ {group}, {severity}, {rule}, "{encoded(word)}" }},' for group, severity, rule, word in WORDS]
    lines += ["};", "static char const *const text_filter_allowed_words[] = {"]
    lines += [f'\t"{word}",' for word in ALLOWED]
    lines += ["};", ""]
    return "\n".join(lines)


def generated(real=False, fault=None):
    source = read("port/linux/game/text_filter_match.c")
    if fault:
        source = mutated(source, *fault)
    terms = read("port/linux/game/text_filter_terms.inc") if real else terms_include()
    return (("text_filter.h", read("port/linux/game/text_filter.h")),
            ("text_filter_terms.inc", terms),
            ("text_filter_fold.inc", read("port/linux/game/text_filter_fold.inc")),
            ("under_test.inc", source))


def censor(executable, texts, levels, narrow=False, extra=()):
    command = [str(executable), "--censor", *(["--narrow"] if narrow else []), *extra, *levels]
    data = ("\n".join(texts) + "\n").encode("latin-1" if narrow else "utf-8")
    result = subprocess.run(command, input=data, capture_output=True, check=True, timeout=60)
    return result.stdout.decode("latin-1" if narrow else "utf-8").split("\n")[:len(texts)]


def failures(executable, cases, narrow=False):
    found = []
    for text, levels, expected in cases:
        shown = censor(executable, [text], levels, narrow)[0]
        if shown != expected:
            found.append(f"{text!r} at {levels}: {shown!r}, not {expected!r}")
    return found


@pytest.mark.parametrize("case", sorted(CASES))
def test_case(case):
    assert not failures(build("text_filter", generated()), CASES[case])


@pytest.mark.parametrize("case", sorted(NARROW_CASES))
def test_narrow_case(case):
    assert not failures(build("text_filter", generated()), NARROW_CASES[case], narrow=True)


def test_players_words():
    executable = build("text_filter", generated())
    assert censor(executable, ["a plonk here", "zorbul"], ("0", "0", "0"), extra=("--add", "plonk")) == \
        ["a ***** here", "zorbul"]
    assert censor(executable, ["xzorbulite"], ALL, extra=("--allow", "zorbulite")) == ["xzorbulite"]


@pytest.mark.parametrize("control", sorted(NEGATIVE_CONTROLS))
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    assert failures(build("text_filter", generated(fault=fault)), CASES[case]), \
        f"{control}: the case {case} does not catch it"
