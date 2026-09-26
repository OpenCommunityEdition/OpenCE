# C11: the two scenario equipment layouts + their HCEX flag enums into scenario/scenario_definitions.h
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    H = 'source/scenario/scenario_definitions.h'
    sub(G, '/* scenario_starting_equipment.flags (HCEX names) */\nenum\n{\n\t_netgame_starting_equipment_flag_no_grenades_bit = 0,\n'
        '\t_netgame_starting_equipment_flag_plasma_greandes_bit, /* (sic) HCEX spelling */\n};\n\n', '')
    sub(G, 'struct scenario_netgame_equipment\n{\n\tlong flags;\n\tshort game_type[4];\n\tshort team_index;\n\tshort spawn_time;\n'
        '\tlong run_time_spawned_item_index;\n\tlong unused1[11];\n\treal_point3d position;\n\treal facing;\n'
        '\tstruct tag_reference item_collection;\n\tlong unused2[12];\n};\n\n'
        'struct scenario_starting_equipment\n{\n\tlong flags;\n\tshort game_type[4];\n\tlong unused1[12];\n'
        '\tstruct tag_reference item_collection[6];\n\tlong unused2[12];\n};\n\n', '')
    sub(G, 'enum\n{\n\t_equipment_created_at_rest_bit = 0,\n};\n\n', '')
    sub(H, 'enum\n{\n\t_weapon_created_at_rest_bit = 0,\n\t_weapon_obsolete_bit,\n\t_weapon_does_accelerate_bit,\n'
        '\tNUMBER_OF_SCENARIO_WEAPON_FLAGS,\n};\n',
        'enum\n{\n\t_weapon_created_at_rest_bit = 0,\n\t_weapon_obsolete_bit,\n\t_weapon_does_accelerate_bit,\n'
        '\tNUMBER_OF_SCENARIO_WEAPON_FLAGS,\n};\n\n'
        'enum\n{\n\t_equipment_created_at_rest_bit = 0,\n\t_equipment_obsolete_bit,\n\t_equipment_does_accelerate_bit,\n'
        '\tNUMBER_OF_SCENARIO_EQUIPMENT_FLAGS,\n};\n\n'
        'enum\n{\n\t_netgame_starting_equipment_flag_no_grenades_bit = 0,\n'
        '\t_netgame_starting_equipment_flag_plasma_greandes_bit, /* (sic) HCEX spelling */\n};\n')
    sub(H, 'typedef char scenario_starting_profile_size_assert[\n\tsizeof(struct scenario_starting_profile) == 0x68 ? 1 : -1];\n',
        'typedef char scenario_starting_profile_size_assert[\n\tsizeof(struct scenario_starting_profile) == 0x68 ? 1 : -1];\n\n'
        'struct scenario_netgame_equipment\n{\n\tlong flags;\n\tshort game_type[4];\n\tshort team_index;\n\tshort spawn_time;\n'
        '\tlong run_time_spawned_item_index;\n\tlong unused1[11];\n\treal_point3d position;\n\treal facing;\n'
        '\tstruct tag_reference item_collection;\n\tlong unused2[12];\n};\n\n'
        'struct scenario_starting_equipment\n{\n\tlong flags;\n\tshort game_type[4];\n\tlong unused1[12];\n'
        '\tstruct tag_reference item_collection[6];\n\tlong unused2[12];\n};\n')
