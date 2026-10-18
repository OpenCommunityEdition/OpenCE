#!/usr/bin/env python3
"""Extract the engine's menu activity setter/getter for headless services.

This subset owns only the typed activity flag; do not link with ui_widget.c.
It does not supply widgets, menu loading, or menu navigation.
"""
import argparse
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('output', type=Path)
a = p.parse_args()
s = Path('source/interface/ui_widget.c').read_text()
start = s.index('void main_menu_active(\n')
end = s.index('void ui_widget_load_progress_widget(\n', start)
body = s[start:end]
if body.count('we_are_at_the_main_menu') != 2:
    raise SystemExit('Unexpected menu activity implementation')
if s.count('\tboolean we_are_at_the_main_menu;') != 1:
    raise SystemExit('Unexpected menu activity type')
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text('/* Generated actual engine menu-state accessors, scoped headless owner. */\n'
                    'static boolean we_are_at_the_main_menu;\n' + body)
