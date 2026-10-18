"""Streamer mode's words (port/linux/game/text_filter_terms.inc, tools/text_filter_terms.py): the table agrees with the
review (port/assets/text_filter/review.txt) and the allowed words, hides in the English word list just the words
expected (english.txt), and hides nothing of the game's own text but its two lines of swearing: the menus' (asset-free,
at the widest levels) and the maps' string lists and HUD messages (assets/maps, else skipped; at the default levels).
With HALO_SURGE_CSV naming Surge AI's profanity_en.csv, the table is made again from it and must be the one committed.

The words are ROT13'd here and in the files, as in review.txt; a failure prints them so."""

import codecs
import gzip
import hashlib
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "harness" / "tests"))
import text_filter_terms  # noqa: E402
from harness import build  # noqa: E402
from test_text_filter import censor, generated  # noqa: E402

ASSETS = ROOT / "port" / "assets" / "text_filter"
MAPS = ROOT / "assets" / "maps"
ALL = ("1", "1", "1")
# the settings' defaults (port_config.c): slurs mild and worse, swearing strong and worse, insults off
DEFAULTS = ("1", "2", "0")


def rot13(text):
    return codecs.encode(text, "rot13")


@pytest.fixture(scope="module")
def matcher():
    return build("text_filter", generated(real=True))


def hidden(matcher, texts, levels):
    """the texts something is hidden in"""
    texts = [text.replace("\n", " ").replace("\r", " ") for text in texts]
    return {text for text, shown in zip(texts, censor(matcher, texts, levels)) if shown != text}


def test_table_agrees_with_the_review():
    review = text_filter_terms.read_review(ASSETS / "review.txt")
    terms = text_filter_terms.read_terms()
    keys = {key for _, _, _, key in terms}
    allowed = re.findall(r'^\t"([a-z]+)",$', text_filter_terms.TERMS_INC.read_text(), re.M)
    assert allowed == sorted(set(text_filter_terms.read_allowed(ASSETS / "allow.txt")))
    dropped = {text_filter_terms.letters(word) for word in review["drop"]} - {None}
    assert not keys & dropped, f"dropped words in the table: {sorted(rot13(key) for key in keys & dropped)}"
    anywhere = [(group, severity, key) for group, severity, rule, key in terms if rule == "anywhere"]
    for word, (group, severity, rule) in review["keep"].items():
        key = text_filter_terms.letters(word)
        assert key in keys or any(other in key for _, _, other in anywhere), \
            f"a kept word is not in the table: {rot13(word)}"
    for word in review["term"]:
        assert text_filter_terms.letters(word) in keys, f"a review term is not in the table: {rot13(word)}"
    recorded = re.search(r"sha256 ([0-9a-f]{64})", text_filter_terms.TERMS_INC.read_text()).group(1)
    assert recorded in (ASSETS / "README.md").read_text(), "the README names another version of the list"


@pytest.mark.skipif(not os.environ.get("HALO_SURGE_CSV"), reason="HALO_SURGE_CSV (Surge AI's profanity_en.csv) unset")
def test_table_made_again():
    csv_path = os.environ["HALO_SURGE_CSV"]
    assert hashlib.sha256(Path(csv_path).read_bytes()).hexdigest() in text_filter_terms.TERMS_INC.read_text()
    english = text_filter_terms.English(text_filter_terms.ENGLISH)
    terms, unreviewed, _ = text_filter_terms.build(csv_path, text_filter_terms.read_review(ASSETS / "review.txt"),
                                                   english)
    assert not unreviewed
    assert terms == text_filter_terms.read_terms()


def test_english_words_hidden(matcher):
    with gzip.open(text_filter_terms.ENGLISH, "rt") as file:
        words = file.read().split()
    found = hidden(matcher, words, ALL)
    expected = {rot13(line) for line in (ASSETS / "english.txt").read_text().split()}
    assert found == expected, (f"hidden, not expected: {sorted(rot13(word) for word in found - expected)}; "
                               f"expected, not hidden: {sorted(rot13(word) for word in expected - found)} "
                               "(ROT13'd: if right, port/assets/text_filter/english.txt)")


def menu_texts():
    texts = []
    for path in sorted((ROOT / "port" / "assets" / "menus").rglob("*.xml")):
        for value in re.findall(r'\b(?:text|strings)="([^"]*)"', path.read_text(encoding="utf-8")):
            texts += [part.replace("\\n", " ") for part in value.split("|")]
    return [text for text in texts if text.strip()]


# the game's own text that has swearing in it (ROT13'd: a multiplayer profile name, and a tip of the loading
# screens the PC version's menus have too), hidden as other players' would be
PROFILE_NAME = rot13("Cevfba Ovgpu")
TIP = rot13("Lbh unir bar yvsr gb yvir. Ohg ng yrnfg lbh'er hfvat vg gb xvpx nff npebff gur tnynkl vafgrnq bs whfg "
            "fvggvat va n znafvba pelvat jvgu lbhe evpu fbpvnyvgr ybiref.")


def test_menus_hide_nothing(matcher):
    texts = menu_texts()
    assert len(texts) > 500
    found = hidden(matcher, texts, ALL)
    assert found == {TIP}, f"the menus' own text hidden: {sorted(rot13(text) for text in found)}"


def map_texts(path):
    from test_mods import TAG_BASE, MapTags
    tags = MapTags(path)
    texts = []
    for group, _, definition in tags.instances.values():
        if group == b"ustr":
            references = [data for _, data in tags.block(definition, 0, 20)]
        elif group == b"hmt ":
            references = [tags.tags[definition - TAG_BASE:][:20]]
        else:
            continue
        for reference in references:
            size, _, _, address, _ = struct.unpack("<iiiII", reference)
            if size > 0:
                text = tags.tags[address - TAG_BASE:address - TAG_BASE + size].decode("utf-16-le", "replace")
                texts += [part for part in text.split("\0") if part.strip()]
    return texts


@pytest.mark.skipif(not MAPS.is_dir() or not any(MAPS.glob("*.map")), reason="no maps (assets/maps)")
def test_maps_hide_nothing_by_default(matcher):
    texts = []
    for path in sorted(MAPS.glob("*.map")):
        try:
            texts += map_texts(path)
        except Exception:  # (a map this reader cannot read: a Custom Edition one)
            continue
    assert len(texts) > 1000
    found = {" ".join(text.split()) for text in hidden(matcher, sorted(set(texts)), DEFAULTS)}
    assert found == {PROFILE_NAME, TIP}, f"the maps' own text hidden: {sorted(rot13(text) for text in found)}"


def test_check_command():
    result = subprocess.run([sys.executable, str(ROOT / "tools" / "text_filter_terms.py"), "--check",
                             rot13("shpx guvf"), "Scunthorpe"], capture_output=True, text=True, check=True)
    assert "-> '**** this'" in result.stdout and "-> 'Scunthorpe'" in result.stdout
