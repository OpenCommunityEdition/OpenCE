#!/usr/bin/env python3
"""Streamer mode's words (port/linux/game/text_filter_match.c): writes
port/linux/game/text_filter_terms.inc from Surge AI's rated list of
profanities and this project's review of it, and lists or tries them.

    python3 tools/text_filter_terms.py --surge .../profanity_en.csv
    python3 tools/text_filter_terms.py --list [slurs|swearing|insults]
    python3 tools/text_filter_terms.py --check "some text"
    python3 tools/text_filter_terms.py --fold

The list (port/assets/text_filter/README.md): Surge AI's profanity_en.csv
(MIT), each row a spelling with up to three base words, three categories
and a severity (Mild, Strong, Severe). A base word's group is its own row's
first category's (failing that, the first category most of its spellings
give): SLURS, SWEARING or INSULTS (GROUPS). Its severity is the worst of the
rows that name it alone (failing that, of all its rows). Its spellings are
the rows that name it alone, kept to their letters; those with digits or
symbols are left to the matcher's leet, and those another term already
finds are left out.

port/assets/text_filter/review.txt is the review of every base word, the
words ROT13'd (nothing in the repository spells them out):

    keep <group|-> <severity|-> <rule|-> <word> a base word kept: its group
                                                (- Surge's first category's),
                                                its severity (- Surge's),
                                                its rule (- worked out)
    drop <word>                                 a base word or spelling left out
    term <group> <severity> <rule|-> <word>     a word of the project's, or
                                                a spelling's own settings

A rule (where a term may be found: text_filter.h) worked out is "anywhere"
when no English word (tools/../port/third_party/english-words) has it but
its own forms (s, es, ed, er, ing, y), "start" when none starts with it but
those, and "whole" otherwise. port/assets/text_filter/allow.txt is the words
never hidden (Scunthorpe), in plain text.

A base word of the CSV that the review does not name stops the generator,
which then prints a line to review for each (--template prints them all).
The CSV is not in the repository: the .inc records its checksum, and
tools/test_text_filter_terms.py checks the .inc against the review without
it. --fold writes text_filter_fold.inc, the characters above ASCII that read
as letters, from Python's Unicode names (its version is recorded).
"""

import argparse
import codecs
import csv
import gzip
import hashlib
import re
import subprocess
import sys
import unicodedata
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "port/assets/text_filter"
REVIEW = ASSETS / "review.txt"
ALLOW = ASSETS / "allow.txt"
TERMS_INC = ROOT / "port/linux/game/text_filter_terms.inc"
FOLD_INC = ROOT / "port/linux/game/text_filter_fold.inc"
ENGLISH = ROOT / "port/third_party/english-words/words_alpha.txt.gz"
KEY = 0x5A

GROUPS = {
    "racial / ethnic slurs": "slurs", "sexual orientation / gender": "slurs", "mental disability": "slurs",
    "physical disability": "slurs", "political": "slurs",
    "sexual anatomy / sexual acts": "swearing", "bodily fluids / excrement": "swearing",
    "religious offense": "swearing",
    "other / general insult": "insults", "animal references": "insults", "physical attributes": "insults",
}
GROUP_NAMES = {"slurs": "_text_filter_slurs", "swearing": "_text_filter_swearing", "insults": "_text_filter_insults"}
SEVERITIES = {"Mild": 1, "Strong": 2, "Severe": 3}
SEVERITY_NAMES = {1: "mild", 2: "strong", 3: "severe"}
RULES = {"anywhere": "_text_filter_anywhere", "start": "_text_filter_start", "whole": "_text_filter_whole"}
FORMS = ("", "s", "es", "ed", "er", "ers", "ing", "y", "ies")


def rot13(text):
    return codecs.encode(text, "rot13")


def letters(text):
    """a term's key: its letters, lowercase (spaces, hyphens, apostrophes
    and stops dropped); None for one with digits or other symbols"""
    text = text.lower()
    if re.search(r"[^a-z \-'.]", text):
        return None
    return re.sub(r"[^a-z]", "", text)


def collapse(word):
    """a word with each run of one letter made one ("batter" "bater")"""
    return re.sub(r"(.)\1+", r"\1", word)


def read_review(path):
    review = {"keep": {}, "drop": set(), "term": {}}
    for number, line in enumerate(path.read_text().splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(None, 4 if line.startswith(("keep", "term")) else 1)
        kind = parts[0]
        if kind == "drop" and len(parts) == 2:
            review["drop"].add(rot13(parts[1]))
        elif kind in ("keep", "term") and len(parts) == 5:
            _, group, severity, rule, word = parts
            known_group = group in GROUP_NAMES or (kind == "keep" and group == "-")
            known_severity = severity == "-" or severity in SEVERITY_NAMES.values()
            if not known_group or not known_severity or (rule != "-" and rule not in RULES):
                sys.exit(f"{path}:{number}: a group, severity or rule this does not know")
            entry = (None if group == "-" else group, None if severity == "-" else
                     {name: value for value, name in SEVERITY_NAMES.items()}[severity], None if rule == "-" else rule)
            review[kind][rot13(word)] = entry
        else:
            sys.exit(f"{path}:{number}: not a review line")
    return review


def read_allowed(path):
    return [line.strip().lower() for line in path.read_text().splitlines()
            if line.strip() and not line.startswith("#")]


class English:
    """the English word list, for working out rules"""

    def __init__(self, path):
        with gzip.open(path, "rt") as file:
            self.words = file.read().split()
        self.text = "\n" + "\n".join(self.words) + "\n"
        self.collapsed = "\n" + "\n".join(collapse(word) for word in self.words) + "\n"

    @staticmethod
    def _find(text, key, own):
        found, at = [], 0
        while True:
            at = text.find(key, at)
            if at < 0:
                return found
            start = text.rindex("\n", 0, at) + 1
            end = text.index("\n", at)
            word = text[start:end]
            if word not in own:
                found.append(word)
            at = end

    def containing(self, key):
        """the words that the matcher would find the key in (the key, it with
        "ph" for "f" and "vv" for "w", or with any letter doubled), but its
        own forms: (word, whether it starts with the key)"""
        found = []
        for variant in {key, key.replace("f", "ph"), key.replace("w", "vv")}:
            own = {variant + form for form in FORMS}
            found += [(word, word.startswith(variant)) for word in self._find(self.text, variant, own)]
            short = collapse(variant)
            found += [(word, word.startswith(short)) for word in self._find(self.collapsed, short,
                                                                          {collapse(word) for word in own})]
        return found

    def rule(self, key):
        others = self.containing(key)
        if len(key) >= 4 and not others:
            return "anywhere"
        if not any(starts for _, starts in others):
            return "start"
        return "whole"


def build(csv_path, review, english):
    """the terms: (group, severity, rule, key), and the base words the review
    has not named"""
    rows = list(csv.DictReader(open(csv_path, encoding="utf-8")))
    bases = defaultdict(list)
    for row in rows:
        forms = [row[f"canonical_form_{index}"] for index in (1, 2, 3) if row[f"canonical_form_{index}"]]
        for form in forms:
            bases[form].append((row, len(forms) == 1))
    unreviewed, kept = [], {}
    for base, base_rows in sorted(bases.items()):
        if base in review["drop"]:
            continue
        if base not in review["keep"]:
            unreviewed.append(base)
            continue
        own = [row for row, _ in base_rows if row["text"].lower() == base]
        category = own[0]["category_1"] if own else \
            Counter(row["category_1"] for row, _ in base_rows).most_common(1)[0][0]
        alone = [row for row, single in base_rows if single] or [row for row, _ in base_rows]
        group, severity, rule = review["keep"][base]
        kept[base] = (group or GROUPS[category],
                      severity or max(SEVERITIES[row["severity_description"]] for row in alone),
                      rule, [row["text"] for row, single in base_rows if single])
    terms = {}
    dropped = {letters(word) for word in review["drop"]}
    for base, (group, severity, rule, spellings) in kept.items():
        key = letters(base)
        if key:
            terms[key] = (group, severity, rule or english.rule(key))
    for base, (group, severity, rule, spellings) in kept.items():
        for spelling in spellings:
            key = letters(spelling)
            if not key or key in terms or key in dropped:
                continue
            terms[key] = (group, severity, None)
    for word, (group, severity, rule) in review["term"].items():
        key = letters(word)
        if not key:
            sys.exit(f"a review term has digits or symbols: {rot13(word)}")
        old = terms.get(key)
        terms[key] = (group, severity or (old[1] if old else 3), rule or (old[2] if old else None))
    for key, (group, severity, rule) in list(terms.items()):
        if rule is None:
            terms[key] = (group, severity, english.rule(key))
    # (a spelling another term finds: left out)
    found = {key for key, (group, severity, rule) in terms.items()
             if any(other != key and other in key and terms[other][2] == "anywhere" and terms[other][0] == group and
                    terms[other][1] >= severity for other in terms)}
    result = sorted((group, severity, rule, key) for key, (group, severity, rule) in terms.items() if key not in found)
    return result, unreviewed, bases


def template_line(base, base_rows, english):
    own = [row for row, _ in base_rows if row["text"].lower() == base]
    category = own[0]["category_1"] if own else Counter(row["category_1"] for row, _ in base_rows).most_common(1)[0][0]
    key = letters(base)
    rule = english.rule(key) if key else "-"
    return f"keep {GROUPS[category]} - {rule} {rot13(base)}"


def c_string(key):
    return "".join(f"\\x{ord(character) ^ KEY:02X}" for character in key)


def write_terms(terms, allowed, csv_path):
    digest = hashlib.sha256(Path(csv_path).read_bytes()).hexdigest() if csv_path else None
    lines = ["/* port/linux/game/text_filter_terms.inc: streamer mode's words, written by",
             "tools/text_filter_terms.py from Surge AI's profanity_en.csv",
             f"(sha256 {digest}) and port/assets/text_filter/review.txt and allow.txt.",
             "Each term's letters are xor'd with TEXT_FILTER_TERM_KEY. Do not edit. */", "",
             f"#define TEXT_FILTER_TERM_KEY 0x{KEY:02X}", "",
             "static struct text_filter_term_definition const text_filter_term_definitions[] =", "{"]
    for group, severity, rule, key in terms:
        lines.append(f'\t{{ {GROUP_NAMES[group]}, {severity}, {RULES[rule]}, "{c_string(key)}" }},')
    lines += ["};", "", "static char const *const text_filter_allowed_words[] =", "{"]
    lines += [f'\t"{word}",' for word in sorted(set(allowed))]
    lines += ["};", ""]
    TERMS_INC.write_text("\n".join(lines))


def read_terms():
    """the committed .inc's terms: (group, severity, rule, key)"""
    text = TERMS_INC.read_text()
    groups = {name: group for group, name in GROUP_NAMES.items()}
    rules = {name: rule for rule, name in RULES.items()}
    terms = []
    for group, severity, rule, encoded in re.findall(r'\{ (\w+), (\d), (\w+), "([^"]*)" \}', text):
        key = "".join(chr(int(byte, 16) ^ KEY) for byte in re.findall(r"\\x([0-9A-F]{2})", encoded))
        terms.append((groups[group], int(severity), rules[rule], key))
    return terms


# ---------- characters above ASCII read as letters

# Greek and Cyrillic letters that look like Latin ones
LOOK_ALIKES = {
    "α": "a", "β": "b", "γ": "y", "ε": "e", "η": "n", "ι": "i", "κ": "k", "ν": "v", "ο": "o", "ρ": "p", "τ": "t",
    "υ": "u", "χ": "x", "ω": "w", "Α": "a", "Β": "b", "Ε": "e", "Ζ": "z", "Η": "h", "Ι": "i", "Κ": "k", "Μ": "m",
    "Ν": "n", "Ο": "o", "Ρ": "p", "Τ": "t", "Υ": "y", "Χ": "x",
    "а": "a", "в": "b", "е": "e", "ё": "e", "ѕ": "s", "і": "i", "ї": "i", "ј": "j", "к": "k", "м": "m", "н": "h",
    "о": "o", "п": "n", "р": "p", "с": "c", "т": "t", "у": "y", "х": "x", "ь": "b", "ԁ": "d", "ӏ": "l", "г": "r",
    "А": "a", "В": "b", "Е": "e", "Ё": "e", "Ѕ": "s", "І": "i", "Ї": "i", "Ј": "j", "К": "k", "М": "m", "Н": "h",
    "О": "o", "Р": "p", "С": "c", "Т": "t", "У": "y", "Х": "x", "Ь": "b", "Ԁ": "d", "Ӏ": "l",
    "µ": "u", "ß": "sb", "ſ": "sf", "ı": "i", "ȷ": "j", "ł": "l", "Ł": "l", "ø": "o", "Ø": "o", "đ": "d", "Đ": "d",
    "ħ": "h", "Ħ": "h", "ŧ": "t", "Ŧ": "t", "æ": "a", "Æ": "a", "œ": "o", "Œ": "o", "þ": "p", "Þ": "p",
}
NAME = re.compile(r"^(?:LATIN|CIRCLED LATIN|PARENTHESIZED LATIN|LATIN SUBSCRIPT|SUPERSCRIPT LATIN) "
                  r"(?:SMALL|CAPITAL) LETTER (?:SMALL CAPITAL )?([A-Z])(?: WITH .*| BAR| STROKE)?$")
OTHER_NAMES = [re.compile(r"^LATIN LETTER SMALL CAPITAL ([A-Z])$"),
               re.compile(r"^MODIFIER LETTER (?:SMALL|CAPITAL) ([A-Z])$")]


def fold_letters(code):
    character = chr(code)
    found = LOOK_ALIKES.get(character)
    if found is None:
        base = unicodedata.normalize("NFD", character)[0]
        found = LOOK_ALIKES.get(base) if base != character else None
    if found is None:
        name = unicodedata.name(character, "")
        for pattern in [NAME, *OTHER_NAMES]:
            match = pattern.match(name)
            if match:
                found = match.group(1).lower()
                break
    return found


def write_fold():
    lines = ["/* port/linux/game/text_filter_fold.inc: the characters above ASCII that",
             "streamer mode reads as letters (a-z bits), written by tools/text_filter_terms.py",
             f"--fold from Unicode {unicodedata.unidata_version}'s names and its look-alikes. Do not edit. */", "",
             "static struct text_filter_fold const text_filter_folds[] =", "{"]
    for code in range(0x80, 0x10000):
        if 0xD800 <= code <= 0xDFFF or 0xFF01 <= code <= 0xFF5E:
            continue
        found = fold_letters(code)
        if found:
            bits = " | ".join(f"LETTER('{letter}')" for letter in sorted(set(found)))
            lines.append(f"\t{{ 0x{code:04X}, {bits} }},")
    lines += ["};", ""]
    FOLD_INC.write_text("\n".join(lines))


def check(texts):
    """the matcher's censoring of each text (all groups at mild), built as
    the harness builds it"""
    sys.path.insert(0, str(ROOT / "tools"))
    sys.path.insert(0, str(ROOT / "tools" / "harness" / "tests"))
    from harness import build  # noqa: E402
    from test_text_filter import generated  # noqa: E402
    executable = build("text_filter", generated(real=True))
    result = subprocess.run([str(executable), "--censor", "1", "1", "1"], input="\n".join(texts) + "\n",
                            capture_output=True, text=True, check=True)
    return result.stdout.splitlines()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--surge", help="Surge AI's profanity_en.csv: write text_filter_terms.inc")
    parser.add_argument("--template", action="store_true", help="with --surge: a review line for each base word")
    parser.add_argument("--list", nargs="?", const="all", help="the terms, decoded (a group, or all)")
    parser.add_argument("--check", nargs="+", help="texts to try: what is hidden")
    parser.add_argument("--fold", action="store_true", help="write text_filter_fold.inc")
    arguments = parser.parse_args()
    if arguments.fold:
        write_fold()
    if arguments.surge:
        english = English(ENGLISH)
        review = read_review(REVIEW)
        terms, unreviewed, bases = build(arguments.surge, review, english)
        if arguments.template:
            for base in sorted(bases):
                print(template_line(base, bases[base], english))
            return
        if unreviewed:
            print("base words the review does not name (review.txt lines to start from):", file=sys.stderr)
            for base in unreviewed:
                print(template_line(base, bases[base], english), file=sys.stderr)
            sys.exit(1)
        write_terms(terms, read_allowed(ALLOW), arguments.surge)
        print(f"{len(terms)} terms: " + ", ".join(f"{count} {group}" for group, count in
                                                  sorted(Counter(term[0] for term in terms).items())))
    if arguments.list:
        for group, severity, rule, key in read_terms():
            if arguments.list in ("all", group):
                print(f"{group:9} {SEVERITY_NAMES[severity]:7} {rule:9} {key}")
    if arguments.check:
        for text, shown in zip(arguments.check, check(arguments.check)):
            print(f"{text!r} -> {shown!r}")


if __name__ == "__main__":
    main()
