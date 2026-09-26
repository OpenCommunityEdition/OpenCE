# Q1 option B: the /Od-attested 8-byte view copy instead of the in-loop alias (owner-gated either way)
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, '\t\tfor (goal_index = 0; goal_index < 32; goal_index++)\n\t\t{\n'
        '\t\t\tstruct netgame_goal *goal = &global_goal[goal_index];\n\n'
        '\t\t\tif (goal_matches_player(goal_index, player, player_index) && count < maximum_count)\n\t\t\t{\n'
        '\t\t\t\tgoal_indices[count] = goal_index;\n'
        '\t\t\t\tpositions[count].x = goal->position.x;\n\t\t\t\tpositions[count].y = goal->position.y;\n',
        '\t\tfor (goal_index = 0; goal_index < 32; goal_index++)\n\t\t{\n'
        '\t\t\tif (goal_matches_player(goal_index, player, player_index) && count < maximum_count)\n\t\t\t{\n'
        '\t\t\t\tgoal_indices[count] = goal_index;\n'
        '\t\t\t\t/* /Od 0x5a20e0: one 8-byte copy of the goal\'s x,y (the 3D point viewed as its 2D prefix) */\n'
        '\t\t\t\tpositions[count] = *(real_point2d *)&global_goal[goal_index].position;\n')
