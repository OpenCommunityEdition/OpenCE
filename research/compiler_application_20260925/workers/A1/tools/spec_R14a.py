# R14a: post_rasterize_post_game's scalar-only block -> function-scope entry_index
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    t = read(G).replace('\r\n', '\n')
    start = t.index('\tentry_count = select_players_to_display(_postgame_statistic_ranking, NONE, entries, 12);\n\t{\n\t\tlong entry_index;\n\n')
    body_start = start + len('\tentry_count = select_players_to_display(_postgame_statistic_ranking, NONE, entries, 12);\n\t{\n\t\tlong entry_index;\n\n')
    # the block ends at the first line that is exactly "\t}\n" after body_start
    end = t.index('\n\t}\n', body_start) + 1
    body = t[body_start:end]
    # de-indent the block body by one tab
    body = '\n'.join(l[1:] if l.startswith('\t') else l for l in body.split('\n'))
    new = '\tentry_count = select_players_to_display(_postgame_statistic_ranking, NONE, entries, 12);\n' + body
    t = t[:start] + new + t[end + len('\t}\n'):]
    t = t.replace('\tlong font_index;\n\tlong entry_count;\n\n\tif (!game_engine)\n\t\treturn;\n',
                  '\tlong font_index;\n\tlong entry_count;\n\tlong entry_index;\n\n\tif (!game_engine)\n\t\treturn;\n', 1)
    write(G, t)
