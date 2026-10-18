#!/usr/bin/env python3
"""Create a native view that gives real Xbox DirectSound state native storage."""

import os
import sys


DECLARATION = "extern struct dsound_globals dsound_globals;"
DEFINITION = "struct dsound_globals dsound_globals;"


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: mac_dsound_globals_view.py SOURCE OUTPUT")
    source_path, output_path = sys.argv[1:]

    source = open(source_path, "r")
    try:
        text = source.read()
    finally:
        source.close()
    if text.count(DECLARATION) != 1:
        raise SystemExit("expected exactly one dsound_globals storage declaration")
    view = text.replace(DECLARATION, DEFINITION)
    if view.count(DEFINITION) != 1 or DECLARATION in view:
        raise SystemExit("native dsound_globals view failed its storage check")
    output_directory = os.path.dirname(output_path)
    if output_directory and not os.path.isdir(output_directory):
        os.makedirs(output_directory)
    output = open(output_path, "w")
    try:
        output.write(view)
    finally:
        output.close()
    sys.stdout.write("generated typed native storage view from source/sound/sound_dsound_xbox.c\n")


if __name__ == "__main__":
    main()
