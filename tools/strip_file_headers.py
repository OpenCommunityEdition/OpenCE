#!/usr/bin/env python3
"""Take the decompilation's file headers out of the game sources.

Each file under source/ opened with a comment holding its own name in
capitals ("PLAYERS.C") and, in most of them, a note the decompilation wrote
about matching the sources against the original build:

    symbols in this file:            the original build's link map for the
                                     file: an address, a size and a name for
                                     each symbol, one or two lines apiece
    no symbols in this file.         the same, for a file whose original
                                     object held none
    header included in hcex build.   a header that was found in another
                                     build and used as evidence

None of it is about the code, and nothing reads it: not the build, the tools
or the tests. This takes out the name and the notes. A comment that held nothing else goes, with the
blank lines after it, and the file starts at its code; one that also says
something else (what a header is for) keeps that. A file with nothing but
the comment keeps its name, so that it is not left empty.

Only the first comment of a file and the blank lines after it change; every
other byte of the file is kept, line endings included. A line counts as the
name only if it is the file's own name in capitals, so the third-party trees
(zlib's "infblock.h -- header to use infblock.c") are left as they are.

A branch made before the headers went can run this on its own files before
merging, so that both sides make the same edit and git sees no conflict.

Usage: strip_file_headers.py [--check] [path ...]   (default: source)
  --check   change nothing; list the files that still have a header, and
            fail if there are any
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Iterator, List, Optional

SUFFIXES = (".c", ".h", ".inc", ".cpp")
NOTES = ("no symbols in this file.", "header included in hcex build.")
LIST_HEADING = "symbols in this file:"
# a link map entry: "000A9470 0070:" with the name on the next line, or on
# the same line ("00008690 0080: _action_uncover_setup_target"); a name line,
# "\t_players_initialize (0000)" (its offset in the symbol, or the name the
# decompilation gave an address-named one)
LIST_ENTRY = re.compile(r"[0-9A-Fa-f]{8} [0-9A-Fa-f]+:(?: \S+)?|\t\S+ \(\w+\)")


def first_comment(text: str) -> Optional[range]:
    """the span of the comment the file opens with (only white space may
    come before it), from its /* to the end of its */, or None"""
    start = len(text) - len(text.lstrip())
    if not text.startswith("/*", start):
        return None
    end = text.find("*/", start + 2)
    return range(start, end + 2) if end >= 0 else None


def strip_header(text: str, file_name: str) -> Optional[str]:
    """the file's text without the name and notes in its first comment, or
    None when that comment has neither"""
    span = first_comment(text)
    if span is None:
        return None
    inner = text[span.start + 2:span.stop - 2]
    newline = "\r\n" if "\r\n" in inner else "\n"
    lines = inner.split(newline)
    code_follows = bool(text[span.stop:].strip())
    name = file_name.upper()
    kept: List[str] = []
    in_list = False
    seen_text = False
    for line in lines:
        bare = line.strip()
        if bare == LIST_HEADING:
            in_list = True
            continue
        if in_list and LIST_ENTRY.fullmatch(line.rstrip()):
            continue
        in_list = False
        if bare in NOTES:
            continue
        # the name is the comment's first line of text
        if bare and not seen_text:
            seen_text = True
            if bare == name and code_follows:
                continue
        kept.append(line)
    if len(kept) == len(lines):
        return None
    if not any(line.strip() for line in kept):
        # nothing left in the comment: it goes, and the blank lines after it
        rest = text[span.stop:]
        while True:
            line_end = rest.find("\n")
            if line_end < 0 or rest[:line_end].strip():
                break
            rest = rest[line_end + 1:]
        return text[:span.start] + rest
    # what follows /* on its line and what comes before */ on its line stay;
    # between them, no blank line at either end and none doubled
    body: List[str] = []
    for line in kept[1:-1]:
        if line.strip() or (body and body[-1].strip()):
            body.append(line)
    while body and not body[-1].strip():
        body.pop()
    lines = [kept[0], *body, kept[-1]] if len(kept) > 1 else kept
    comment = "/*" + newline.join(lines) + "*/"
    assert "*/" not in comment[2:-2]
    return text[:span.start] + comment + text[span.stop:]


def source_files(paths: List[Path]) -> Iterator[Path]:
    for path in paths:
        if path.is_file():
            yield path
        else:
            yield from sorted(p for p in path.rglob("*") if p.suffix in SUFFIXES and p.is_file())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--check", action="store_true", help="change nothing; fail if any file has a header")
    parser.add_argument("paths", nargs="*", type=Path, default=[Path("source")])
    args = parser.parse_args()

    found = 0
    for path in source_files(args.paths):
        # Latin-1 maps every byte to one character and back: the file is
        # written as it was read, but for its header
        text = path.read_bytes().decode("latin-1")
        stripped = strip_header(text, path.name)
        if stripped is None:
            continue
        found += 1
        if args.check:
            print(path.as_posix())
        else:
            path.write_bytes(stripped.encode("latin-1"))
    if args.check:
        if found:
            print(f"{found} files have the decompilation's header: run tools/strip_file_headers.py", file=sys.stderr)
        return 1 if found else 0
    print(f"{found} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
