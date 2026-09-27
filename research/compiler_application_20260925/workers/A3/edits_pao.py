"""A3 edit groups for source/ai/path_obstacle_avoidance.c (card PAO-3), applied on copies/pao12."""

EDITS = {}

EDITS['PAO3'] = [
    ('\t\tboolean blocks_goal= FALSE;\n'
     '\t\treal_vector2d direction;\n',
     '\t\tboolean blocks_goal= FALSE;\n'
     '\t\tboolean add_step= TRUE;\n'
     '\t\treal_vector2d direction;\n'),
    ('\t\t\t\tif (chain_step->obstacle_direction_index!=obstacle_direction_index)\n'
     '\t\t\t\t{\n'
     '\t\t\t\t\tgoto done;\n'
     '\t\t\t\t}\n',
     '\t\t\t\tif (chain_step->obstacle_direction_index!=obstacle_direction_index)\n'
     '\t\t\t\t{\n'
     '\t\t\t\t\tadd_step= FALSE;\n'
     '\t\t\t\t\tbreak;\n'
     '\t\t\t\t}\n'),
    ('\t\t\t\t\t\t{\n'
     '\t\t\t\t\t\t\tgoto done;\n'
     '\t\t\t\t\t\t}\n'
     '\t\t\t\t\t}\n'
     '\n'
     '\t\t\t\t\tif (chain_step->obstructed_goal_step_indices[obstacle_direction_index]==previous_step_index ||\n'
     '\t\t\t\t\t\tchain_step->obstructed_goal_step_indices[obstacle_direction_index]==NONE)\n',
     '\t\t\t\t\t\t{\n'
     '\t\t\t\t\t\t\tadd_step= FALSE;\n'
     '\t\t\t\t\t\t}\n'
     '\t\t\t\t\t}\n'
     '\n'
     '\t\t\t\t\tif (add_step &&\n'
     '\t\t\t\t\t\t(chain_step->obstructed_goal_step_indices[obstacle_direction_index]==previous_step_index ||\n'
     '\t\t\t\t\t\tchain_step->obstructed_goal_step_indices[obstacle_direction_index]==NONE))\n'),
    ('\t\tstep_index= path->step_count;\n'
     '\t\tpath->step_count= step_index+1;\n'
     '\t\tstep= path_get_step(path, step_index);\n'
     '\t\tstep->point= *point;\n'
     '\t\tstep->surface_index= surface_index;\n'
     '\t\tstep->direction= direction;\n'
     '\t\tdistance= normalize2d(&step->direction);\n'
     '\t\tstep->distance= distance;\n'
     '\t\tstep->total_distance= distance+previous_distance;\n'
     '\t\tstep->obstacle_index= obstacle_index;\n'
     '\t\tstep->obstacle_direction_index= obstacle_direction_index;\n'
     '\t\tstep->previous_step_index= previous_step_index;\n'
     '\t\tcsmemset(step->obstructed_goal_step_indices, NONE, sizeof(step->obstructed_goal_step_indices));\n'
     '\n'
     '\t\tif (blocks_goal && step->distance<path->best_goal_blocked_distance)\n'
     '\t\t{\n'
     '\t\t\tpath->best_goal_blocked_distance= step->distance;\n'
     '\t\t\tpath->best_goal_blocked_step_index= step_index;\n'
     '\t\t}\n'
     '\n'
     '\t\tmatch_vassert("c:\\\\halo\\\\SOURCE\\\\ai\\\\path_obstacle_avoidance.c", 420, heap_insert(path, step_index), NULL);\n'
     '\t}\n'
     '\n'
     'done:\n'
     '\treturn step_index;\n',
     '\t\tif (add_step)\n'
     '\t\t{\n'
     '\t\t\tstep_index= path->step_count;\n'
     '\t\t\tpath->step_count= step_index+1;\n'
     '\t\t\tstep= path_get_step(path, step_index);\n'
     '\t\t\tstep->point= *point;\n'
     '\t\t\tstep->surface_index= surface_index;\n'
     '\t\t\tstep->direction= direction;\n'
     '\t\t\tdistance= normalize2d(&step->direction);\n'
     '\t\t\tstep->distance= distance;\n'
     '\t\t\tstep->total_distance= distance+previous_distance;\n'
     '\t\t\tstep->obstacle_index= obstacle_index;\n'
     '\t\t\tstep->obstacle_direction_index= obstacle_direction_index;\n'
     '\t\t\tstep->previous_step_index= previous_step_index;\n'
     '\t\t\tcsmemset(step->obstructed_goal_step_indices, NONE, sizeof(step->obstructed_goal_step_indices));\n'
     '\n'
     '\t\t\tif (blocks_goal && step->distance<path->best_goal_blocked_distance)\n'
     '\t\t\t{\n'
     '\t\t\t\tpath->best_goal_blocked_distance= step->distance;\n'
     '\t\t\t\tpath->best_goal_blocked_step_index= step_index;\n'
     '\t\t\t}\n'
     '\n'
     '\t\t\tmatch_vassert("c:\\\\halo\\\\SOURCE\\\\ai\\\\path_obstacle_avoidance.c", 420, heap_insert(path, step_index), NULL);\n'
     '\t\t}\n'
     '\t}\n'
     '\n'
     '\treturn step_index;\n'),
]

# PAO-4: /Od RTC name desired_direction (path_add_step only)
EDITS['PAO4'] = [
    ('\t\treal_vector2d direction;\n', '\t\treal_vector2d desired_direction;\n'),
    ('\t\tvector_from_points2d(point, &path->goal, &direction);\n',
     '\t\tvector_from_points2d(point, &path->goal, &desired_direction);\n'),
    ('\t\t\t\t\t\tif (dot_product2d(&opposite_step->direction, &direction)>0.f &&\n'
     '\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &previous_step->direction)*\n'
     '\t\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &direction)<0.f)\n',
     '\t\t\t\t\t\tif (dot_product2d(&opposite_step->direction, &desired_direction)>0.f &&\n'
     '\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &previous_step->direction)*\n'
     '\t\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &desired_direction)<0.f)\n'),
    ('\t\t\tstep->direction= direction;\n', '\t\t\tstep->direction= desired_direction;\n'),
]

# PAO-5: /Od factor order of the cross-product test (on top of PAO-4)
EDITS['PAO5'] = [
    ('\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &previous_step->direction)*\n'
     '\t\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &desired_direction)<0.f)\n',
     '\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &desired_direction)*\n'
     '\t\t\t\t\t\t\t\tcross_product2d(&opposite_step->direction, &previous_step->direction)<0.f)\n'),
]
