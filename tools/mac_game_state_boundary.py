#!/usr/bin/env python
"""Share the exact core initialization within a complete native game.c view."""
import os
import sys
with open('source/game/game.c', 'r') as source_file:
    source = source_file.read()
start = source.index('void game_initialize(\n')
body = source.index('{\n', start) + 2
end = source.index('\treal_math_reset_precision();', body)
core = source[body:end]
expected = '''\tgame_globals = game_state_malloc(
\t\t"game globals",
\t\tNULL,
\t\tsizeof(*game_globals));
\tcsmemset(game_globals, 0, sizeof(*game_globals));
\tcsmemset(&game_variant_global, 0, sizeof(game_variant_global));
'''
if core != expected:
    raise SystemExit('Game initialization prefix changed; review native lifecycle')
shared = 'static void mac_game_initialize_core(void)\n{\n' + core + '}\n\n'
result = source[:start] + shared + source[start:body] + '\tmac_game_initialize_core();\n\n' + source[end:]
output = sys.argv[1]
parent = os.path.dirname(output)
if parent and not os.path.isdir(parent):
    os.makedirs(parent)
with open(output, 'w') as output_file:
    output_file.write('/* Generated complete game.c; exact core initialization factored once. */\n' + result)
