# R13b: sort_statistic_buffer_ranking in the /Od 0x5b6e60 flat single-exit chain
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    old = ('\tstruct statistic_buffer const *entry0 = entry0_pointer;\n\tstruct statistic_buffer const *entry1 = entry1_pointer;\n'
           '\tlong result = 0;\n\tlong entry0_value;\n\tlong entry1_value;\n\n'
           '\tentry0_value = entry0->custom;\n\tentry1_value = entry1->custom;\n'
           '\tif (entry1_value < entry0_value)\n\t\tresult = -1;\n\telse if (entry1_value > entry0_value)\n\t\tresult = 1;\n\telse\n\t{\n'
           '\t\tentry0_value = entry0->kills;\n\t\tentry1_value = entry1->kills;\n'
           '\t\tif (entry1_value < entry0_value)\n\t\t\tresult = -1;\n\t\telse if (entry1_value > entry0_value)\n\t\t\tresult = 1;\n\t\telse\n\t\t{\n'
           '\t\t\tentry0_value = entry0->deaths;\n\t\t\tentry1_value = entry1->deaths;\n'
           '\t\t\tif (entry1_value < entry0_value)\n\t\t\t\tresult = 1;\n\t\t\telse if (entry1_value > entry0_value)\n\t\t\t\tresult = -1;\n\t\t\telse\n\t\t\t{\n'
           '\t\t\t\tentry0_value = entry0->assists;\n\t\t\t\tentry1_value = entry1->assists;\n'
           '\t\t\t\tif (entry1_value < entry0_value)\n\t\t\t\t\tresult = -1;\n\t\t\t\telse if (entry1_value > entry0_value)\n\t\t\t\t\tresult = 1;\n'
           '\t\t\t}\n\t\t}\n\t}\n\n\treturn result;\n')
    new = ('\tstruct statistic_buffer const *entry0 = entry0_pointer;\n\tstruct statistic_buffer const *entry1 = entry1_pointer;\n'
           '\tlong result = 0;\n\n'
           '\tif (entry0->custom > entry1->custom)\n\t\tresult = -1;\n\telse if (entry1->custom > entry0->custom)\n\t\tresult = 1;\n'
           '\telse if (entry0->kills > entry1->kills)\n\t\tresult = -1;\n\telse if (entry1->kills > entry0->kills)\n\t\tresult = 1;\n'
           '\telse if (entry0->deaths > entry1->deaths)\n\t\tresult = 1;\n\telse if (entry1->deaths > entry0->deaths)\n\t\tresult = -1;\n'
           '\telse if (entry0->assists > entry1->assists)\n\t\tresult = -1;\n\telse if (entry1->assists > entry0->assists)\n\t\tresult = 1;\n\n'
           '\treturn result;\n')
    sub(G, old, new)
