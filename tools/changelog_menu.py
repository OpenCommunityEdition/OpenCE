#!/usr/bin/env python3
"""Generate the port's player notes screens without the PC tag archive.

    python tools/changelog_menu.py

Use --text-only to update notes while preserving the committed artwork.
Build navigation uses existing open/back events; scrolling belongs to each
screen instance and resets when another build is opened.
"""

import argparse
from datetime import date
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

import port_settings

ROOT = Path(__file__).resolve().parent.parent
MENUS = ROOT / "port/assets/menus"
FILE = "main_menu.changelog.port.xml"
ITEM = "main_menu/main_menu_item_changelog"
CORNER = "main_menu/changelog_corner"
BASE = "main_menu/changelog"
# Center beneath the original version's visible text (x454..544), rather
# than its wider text widget, whose center is farther to the right.
CHILD = f'<child widget="{CORNER}" x="447" y="438"/>'
CORNER_WIDTH = 104
CORNER_HEIGHT = 21
BUILD_LABEL = "main_menu/gulch_build_number"
BITMAP = "main_menu/menu_changelog"
TITLE = f"{BASE}/header_changelog"
NOTES = ROOT / "port/assets/changelog/notes.json"
NOTE_LINE_HEIGHT = 22
VISIBLE_LINES = 8
NOTE_COLUMNS = 44

# The handmade redraws' letter side bearings, fitted to their existing
# menu/header pairs in the same way as port_settings.MENU_ITEM_SPACES.
SPACING = {"CH": 3.19, "HA": 1.22, "AN": 1.33, "NG": 3.0,
           "GE": 2.35, "EL": 3.56, "LO": -0.11, "OG": 2.27}

def validate_notes(data):
    """Reject data that could introduce menu escapes or unsupported text."""
    if not isinstance(data, dict) or set(data) != {"schema_version", "head", "entries"}:
        raise ValueError("notes must contain schema_version, head and entries")
    if type(data["schema_version"]) is not int or data["schema_version"] != 2:
        raise ValueError("unsupported notes schema_version")
    sha = re.compile(r"[0-9a-f]{40}")
    if not isinstance(data["head"], str) or not sha.fullmatch(data["head"]):
        raise ValueError("notes head must be a full lowercase commit SHA")
    if not isinstance(data["entries"], list):
        raise ValueError("notes entries must be a list")
    total_notes, builds = 0, set()
    for index, entry in enumerate(data["entries"]):
        context = f"notes entry {index + 1}"
        if (not isinstance(entry, dict) or not {"build", "date", "commit", "notes"} <= set(entry)
                or set(entry) - {"build", "date", "commit", "notes", "categories"}):
            raise ValueError(f"{context} must contain build, date, commit and notes")
        if entry["build"] is not None and (type(entry["build"]) is not int or not 1 <= entry["build"] <= 2147483647):
            raise ValueError(f"{context} build must be a positive integer or null")
        if entry["build"] in builds:
            raise ValueError(f"{context} duplicates a build")
        builds.add(entry["build"])
        if not isinstance(entry["commit"], str) or not sha.fullmatch(entry["commit"]):
            raise ValueError(f"{context} commit must be a full lowercase SHA")
        try:
            if not isinstance(entry["date"], str) or date.fromisoformat(entry["date"]).isoformat() != entry["date"]:
                raise ValueError()
        except (ValueError, TypeError):
            raise ValueError(f"{context} date must be YYYY-MM-DD") from None
        if not isinstance(entry["notes"], list):
            raise ValueError(f"{context} notes must be a list")
        if "categories" in entry:
            categories = entry["categories"]
            if (not isinstance(categories, list) or len(categories) != len(entry["notes"])
                    or any(not isinstance(label, str) or label and
                           (len(label) > 24 or not re.fullmatch(r"[A-Z][A-Z0-9]*(?: [A-Z0-9]+)*", label))
                           for label in categories)):
                raise ValueError(f"{context} categories must match notes and contain uppercase labels up to 24 characters")
        total_notes += len(entry["notes"])
        if total_notes > 4096:
            raise ValueError("notes must contain at most 4096 player notes")
        for note in entry["notes"]:
            if (not isinstance(note, str) or not 1 <= len(note) <= 100 or note != note.strip()
                    or any(not 32 <= ord(char) <= 126 or char in "\\|" for char in note)):
                raise ValueError(f"{context} notes must be printable ASCII, 1-100 characters, without backslash or pipe")
    return data


def load_notes(path=NOTES):
    return validate_notes(json.loads(Path(path).read_text(encoding="utf-8")))


def note_lines(note):
    # At small_ui's 10px capital height, this budget fits the 420px column,
    # including digit runs. Capitals and wide lowercase glyphs occupy two
    # columns. Split long words as well as wrapping at word boundaries.
    def columns(text):
        return sum(2 if char.isupper() or char in "mw@%&#" else 1 for char in text)

    lines, line, width = [], "- ", 2
    for word in note.split(" "):
        separator = "" if line in ("- ", "  ") else " "
        if width + len(separator) + columns(word) > NOTE_COLUMNS and line not in ("- ", "  "):
            lines.append(line)
            line, separator = "  ", ""
            width = 2
        line += separator
        width += len(separator)
        for char in word:
            char_width = columns(char)
            if width + char_width > NOTE_COLUMNS:
                lines.append(line)
                line = "  "
                width = 2
            line += char
            width += char_width
    lines.append(line)
    return lines


def reading_rows(entry):
    groups = {}
    categories = entry.get("categories", [""] * len(entry["notes"])) if entry else []
    for category, note in zip(categories, entry["notes"] if entry else []):
        groups.setdefault(category or "GENERAL", []).append(note)
    rows = []
    for category, notes in groups.items():
        if rows:
            rows.append(("", ""))
        rows.append(("", category))
        for note in notes:
            rows.extend((line, "") for line in note_lines(note))
    return rows or [("No player-facing changes recorded.", "")]


def reading_lines(entry):
    return [body for body, heading in reading_rows(entry)]


def _text(name, text, x, y, width, height=24, font="small", color="#FFCCCCCC", align="left"):
    return port_settings._widget(name, [
        ("type", "text"), ("left", x), ("top", y), ("width", width), ("height", height),
        ("text", text), ("font", f"ui\\{font}_ui"), ("color", color), ("align", align),
    ], [])


def corner_frames(redraws):
    """Uniformly fit the existing design beneath the visible version text."""
    frames = []
    for index, (caption, _, _) in enumerate(port_settings.menu_item_frames("CHANGELOG", redraws, SPACING)):
        border = (redraws / f"shell/bitmaps/text_button_background__{index}.svg").read_text(encoding="utf-8")
        inner = lambda svg: svg[svg.index(">") + 1:svg.rindex("</svg>")]
        svg = '\n'.join([
            '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
            f'width="{CORNER_WIDTH}" height="{CORNER_HEIGHT}" viewBox="0 0 160 32" '
            'preserveAspectRatio="xMinYMin meet">',
            '  <g transform="translate(0,3) scale(1.25,1)">', inner(border), '  </g>',
            '  <g transform="translate(2,4) scale(0.6)">', inner(caption), '  </g>', '</svg>', ''])
        frames.append((svg, CORNER_WIDTH, CORNER_HEIGHT))
    return frames


def widget_lines(data=None):
    """One scrolling screen per build, with one invoking-menu history entry.

    ui_widget_load_by_name_or_tag (source/interface/ui_widget.c) checks
    the invoking root's no_history flag before saving it. Each screen must
    carry the flag, while the main menu retains its existing history flags.
    """
    data = load_notes() if data is None else validate_notes(data)
    entries = data["entries"] or [None]
    lines = port_settings._widget(ITEM, [
        ("type", "text"), ("width", CORNER_WIDTH), ("height", CORNER_HEIGHT), ("bitmap", BITMAP),
    ], [f'<on event="a start" open="{BASE}/build_1"/>',
        '<on event="left_mouse" run="mouse emit accept event"/>'])
    lines += port_settings._widget(CORNER, [
        ("type", "column_list"), ("width", CORNER_WIDTH), ("height", CORNER_HEIGHT),
        ("flags", "pass_unhandled_to_focused_child"),
    ], [f'<child widget="{ITEM}"/>'])
    lines += port_settings._header(TITLE, TITLE)
    lines += port_settings._widget(BUILD_LABEL, [
        ("type", "text"), ("controller", 1), ("width", 150), ("height", 20),
        ("font", "ui\\small_ui"), ("color", "#FFFF8000"), ("align", "center"),
    ], ['<on event="created" run="port gulch build label"/>'])
    for index, entry in enumerate(entries, 1):
        page = f"{BASE}/build_{index}"
        contents = reading_rows(entry)
        if len(contents) + VISIBLE_LINES > 32767:
            raise ValueError(f"notes entry {index} exceeds the menu's 32767-line string index limit")
        lines += port_settings._strings(f"{page}/lines", [body for body, heading in contents] + [""] * VISIBLE_LINES)
        lines += port_settings._strings(f"{page}/headings", [heading for body, heading in contents] + [""] * VISIBLE_LINES)
        children = [f'<child widget="{TITLE}"/>',
                    f'<child widget="{page}/build_number"/>',
                    f'<child widget="{page}/label"/>',
                    f'<child widget="{page}/heading"/>',
                    f'<child widget="{page}/date"/>']
        children += [f'<child widget="{page}/row_{row}"/>' for row in range(VISIBLE_LINES)]
        children += [f'<child widget="{page}/heading_row_{row}"/>' for row in range(VISIBLE_LINES)]
        children += [f'<child widget="{page}/scroll_position"/>',
                     f'<child widget="{page}/instructions"/>',
                     f'<child widget="{page}/buttons" y="414"/>',
                     f'<child widget="{page}/scroll_controls" x="480" y="314"/>']
        events = ['<on event="b back start" back="true"/>',
                  '<on event="created" run="port changelog init"/>',
                  '<on event="up stick_up" run="port changelog up"/>',
                  '<on event="down stick_down" run="port changelog down"/>']
        if index < len(entries):
            events.append(f'<on event="left left_trigger" open="{BASE}/build_{index + 1}"/>')
        if index > 1:
            events.append(f'<on event="right right_trigger" open="{BASE}/build_{index - 1}"/>')
        lines += port_settings._widget(page, [
            ("width", 640), ("height", 480), ("flags", "pass_unhandled_to_focused_child pause_game no_history"),
            ("bitmap", "bitmaps/gradient"),
        ], events + ['<data input="port changelog focus update"/>'] + children)
        lines += _text(f"{page}/build_number", f"{index} / {len(entries)}", 430, 44, 170,
                       color="#FF2896FF", align="right")
        lines += _text(f"{page}/label", "PLAYER NOTES", 55, 90, 530, color="#FFFFB020")
        heading = (f"BUILD {entry['build']}" if entry["build"] is not None else "UNRELEASED") if entry else "NO PLAYER NOTES"
        lines += _text(f"{page}/heading", heading, 55, 118, 530, 34, "large", "#FFFFFFFF")
        lines += _text(f"{page}/date", entry["date"] if entry else "", 55, 158, 530)
        for row in range(VISIBLE_LINES):
            for name, table, color in (("row", "lines", "#FFCCCCCC"), ("heading_row", "headings", "#FF2896FF")):
                lines += port_settings._widget(f"{page}/{name}_{row}", [
                    ("type", "text"), ("left", 55), ("top", 188 + row * NOTE_LINE_HEIGHT),
                    ("width", 420), ("height", NOTE_LINE_HEIGHT), ("string_list", f"{page}/{table}"),
                    ("string_index", row), ("font", "ui\\small_ui"), ("color", color),
                ], [])
        lines += port_settings._widget(f"{page}/scroll_position", [
            ("type", "text"), ("left", 55), ("top", 382), ("width", 250), ("height", 24),
            ("font", "ui\\small_ui"), ("color", "#FF999999"),
        ], [])
        lines += _text(f"{page}/instructions", "Scroll: Up/Down or mouse wheel", 300, 382, 310,
                       color="#FF999999", align="right")
        for direction, label in (("up", "UP"), ("down", "DOWN")):
            lines += port_settings._text_button(f"{page}/scroll_{direction}", label,
                [f'<on event="a" run="port changelog {direction}"/>'])
        # Mouse targets must belong to a list (or a parent that tabs its
        # children). Root-level text buttons cannot receive pointer focus.
        lines += port_settings._widget(f"{page}/scroll_controls", [
            ("type", "column_list"), ("width", 128), ("height", 52),
            ("flags", "pass_unhandled_to_focused_child"),
        ], [f'<child widget="{page}/scroll_up"/>',
            f'<child widget="{page}/scroll_down" y="28"/>'])
        buttons = []
        for label, target, x in (("PREVIOUS", index + 1, 80), ("NEXT", index - 1, 250)):
            if not 1 <= target <= len(entries):
                continue
            name = f"{page}/button_{label.lower()}"
            buttons.append(f'<child widget="{name}" x="{x}" y="1"/>')
            lines += port_settings._text_button(name, label, [
                f'<on event="a" open="{BASE}/build_{target}"/>'])
        name = f"{page}/button_back"
        buttons.append(f'<child widget="{name}" x="510" y="1"/>')
        lines += port_settings._text_button(name, "BACK", ['<on event="a" back="true"/>'])
        lines += port_settings._widget(f"{page}/buttons", [
            ("type", "column_list"), ("width", 640), ("height", 28),
            ("flags", "pass_unhandled_to_focused_child"),
        ], buttons)
    return lines


def document(bitmaps, data=None):
    return '\n'.join(['<?xml version="1.0" encoding="UTF-8"?>',
                      '<!-- Player notes (tools/changelog_menu.py) -->',
                      '<menus>', *bitmaps, *widget_lines(data), '</menus>', ''])


def generate(art, data=None):
    """Write the same feature file and art in focused and full generation."""
    bitmaps = [art.port_picture(BITMAP, corner_frames(art.redraws)),
               art.title(TITLE, "CHANGELOG")]
    (MENUS / "ce" / FILE).write_text(document(bitmaps, data), encoding="utf-8")


def regenerate_text(data=None):
    """Keep bitmap XML verbatim; this path uses only the standard library."""
    path = MENUS / "ce" / FILE
    text = path.read_text(encoding="utf-8")
    root = ET.fromstring(text)
    bitmaps = re.findall(r'\t<bitmap\b[^>]*>.*?</bitmap>', text, re.DOTALL)
    if len(bitmaps) != 2 or {node.get("name") for node in root.findall("bitmap")} != {BITMAP, TITLE}:
        raise ValueError("expected existing changelog item and title bitmap definitions")
    path.write_text(document(bitmaps, data), encoding="utf-8")
    main = MENUS / "ce/main_menu.xml"
    if main.exists():
        main.write_text(insert_item(main.read_text(encoding="utf-8")), encoding="utf-8")


def insert_item(text):
    """Add only the feature child; retain every legacy attribute/position."""
    for name in (ITEM, CORNER, BUILD_LABEL):
        text, count = re.subn(r'\t\t<child widget="' + re.escape(name) + r'"[^\n]*/>\n', '', text)
        if count > 1:
            raise ValueError(f"expected at most one child for {name}")
    for owned in ('<on event="right stick_right" run="port changelog corner focus"/>',
                  '<on event="left stick_left" run="port changelog main focus"/>',
                  '<data input="port changelog corner update"/>',
                  '<data input="port changelog focus update"/>'):
        text = text.replace('\t\t' + owned + '\n', '')
    pattern = r'(\t<widget name="main_menu/main_menu"[^\n]+\n)'
    updated, count = re.subn(pattern, r'\1' + '\t\t<on event="right stick_right" run="port changelog corner focus"/>\n'
                           '\t\t<on event="left stick_left" run="port changelog main focus"/>\n'
                           '\t\t<data input="port changelog focus update"/>\n', text)
    if count != 1:
        raise ValueError("expected one main menu root")
    pattern = r'(\t\t<child widget="main_menu/build_number"[^\n]+)'
    updated, count = re.subn(pattern, r'\1' + f'\n\t\t<child widget="{BUILD_LABEL}" x="424" y="420"/>'
                           + '\n\t\t' + CHILD, updated)
    if count != 1:
        raise ValueError("expected one main menu build-number child")
    return updated


def regenerate(data=None):
    # Art is shared with the full generator; no proprietary tags are read.
    import ce_menus
    art = ce_menus.Art.__new__(ce_menus.Art)
    art.pngs = []
    art.redraws = MENUS / "svg"
    generate(art, data)
    main = MENUS / "ce/main_menu.xml"
    main.write_text(insert_item(main.read_text(encoding="utf-8")), encoding="utf-8")
    manifest = MENUS / "menus.json"
    data = json.loads(manifest.read_text(encoding="utf-8"))
    files = set(data["files"]) | set(art.pngs) | {f"ce/{FILE}"}
    data["files"] = sorted(name for name in files if name.endswith(".xml")) + sorted(
        name for name in files if not name.endswith(".xml"))
    manifest.write_text(json.dumps(data, indent=1) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--notes", type=Path, default=NOTES, help="validated player notes JSON")
    parser.add_argument("--text-only", action="store_true", help="preserve artwork and update only widget XML")
    args = parser.parse_args()
    try:
        data = load_notes(args.notes)
        (regenerate_text if args.text_only else regenerate)(data)
    except (OSError, ValueError) as error:
        parser.exit(1, f"changelog_menu: {error}\n")
