# LAB C14: Q1 strip test - neither the in-loop alias nor the view cast
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, '\t\t\tstruct netgame_goal *goal = &global_goal[goal_index];\n\n', '')
    sub(G, '\t\t\t\tpositions[count].x = goal->position.x;\n\t\t\t\tpositions[count].y = goal->position.y;\n',
        '\t\t\t\tpositions[count].x = global_goal[goal_index].position.x;\n\t\t\t\tpositions[count].y = global_goal[goal_index].position.y;\n')
