# R13a: sort_statistic_buffer in the /Od 0x5b6e00 single-exit shape
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, '\tstruct statistic_buffer const *entry0 = entry0_pointer;\n\tstruct statistic_buffer const *entry1 = entry1_pointer;\n'
        '\tlong result = 0;\n\tlong entry0_value;\n\tlong entry1_value;\n\n\tentry0_value = entry0->score;\n\tentry1_value = entry1->score;\n'
        '\tif (entry1_value < entry0_value)\n\t\treturn -1;\n\tif (entry1_value > entry0_value)\n\t\tresult = 1;\n\n\treturn result;\n',
        '\tstruct statistic_buffer const *entry0 = entry0_pointer;\n\tstruct statistic_buffer const *entry1 = entry1_pointer;\n'
        '\tlong result = 0;\n\n\tif (entry0->score > entry1->score)\n\t\tresult = -1;\n\telse if (entry1->score > entry0->score)\n'
        '\t\tresult = 1;\n\n\treturn result;\n')
