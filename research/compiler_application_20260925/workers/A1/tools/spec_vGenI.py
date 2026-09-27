# C10: HCEX itmc layout into its genuine owner header items/item_definitions.h
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    H = 'source/items/item_definitions.h'
    sub(G, '#define item_collection_definition_get(index) ((struct item_collection_definition *)tag_get(ITEM_COLLECTION_DEFINITION_TAG, index))\n\n', '')
    sub(G, "/* item collection ('itmc') tag layout, as recorded by the HCEX PDB */\n\n"
        'struct item_permutation_definition\n{\n\tlong unused1[8];\n\treal weight;\n\tstruct tag_reference item;\n\tlong unused2[8];\n};\n\n'
        'struct item_collection_definition\n{\n\tstruct tag_block permutations;\n\tshort spawn_time;\n\tshort pad;\n\tlong unused[19];\n};\n\n', '')
    sub(H, '#define item_definition_get(index) ((struct item_definition *)tag_get(ITEM_DEFINITION_TAG, index))\n',
        '#define item_definition_get(index) ((struct item_definition *)tag_get(ITEM_DEFINITION_TAG, index))\n'
        '#define item_collection_definition_get(index) ((struct item_collection_definition *)tag_get(ITEM_COLLECTION_DEFINITION_TAG, index))\n')
    sub(H, 'struct item_definition\n{\n\tstruct _object_definition object;\n\tstruct _item_definition item;\n};\n',
        'struct item_definition\n{\n\tstruct _object_definition object;\n\tstruct _item_definition item;\n};\n\n'
        'struct item_permutation_definition\n{\n\tlong unused1[8];\n\treal weight;\n\tstruct tag_reference item;\n\tlong unused2[8];\n};\n\n'
        'struct item_collection_definition\n{\n\tstruct tag_block permutations;\n\tshort spawn_time;\n\tshort pad;\n\tlong unused[19];\n};\n')
