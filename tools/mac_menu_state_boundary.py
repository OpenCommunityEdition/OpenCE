#!/usr/bin/env python
"""Extract the engine's menu activity setter/getter for headless services.

This subset owns only the typed activity flag; do not link with ui_widget.c.
It does not supply widgets, menu loading, or menu navigation.
Runs on Python 2.6 and 3: usage mac_menu_state_boundary.py OUTPUT
"""
import sys
from mac_gen_io import read_text, write_text

s = read_text('source/interface/ui_widget.c')
start = s.index('void main_menu_active(\n')
end = s.index('void ui_widget_load_progress_widget(\n', start)
body = s[start:end]
if body.count('we_are_at_the_main_menu') != 2:
    raise SystemExit('Unexpected menu activity implementation')
if s.count('\tboolean we_are_at_the_main_menu;') != 1:
    raise SystemExit('Unexpected menu activity type')
write_text(sys.argv[1], '/* Generated actual engine menu-state accessors, scoped headless owner. */\n'
           'static boolean we_are_at_the_main_menu;\n' + body)
