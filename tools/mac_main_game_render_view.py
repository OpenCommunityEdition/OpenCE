#!/usr/bin/env python3
"""Expose the unmodified main_game_render body to native diagnostics."""
import sys
import os

source_file = open("source/main/main.c", "r")
source = source_file.read()
source_file.close()
prototype = "static void main_game_render(\n\tdouble time_delta_since_tick_sec);"
definition = "static void main_game_render(\n\tdouble time_delta_since_tick_sec)\n{"
if source.count(prototype) != 1 or source.count(definition) != 1:
    raise SystemExit("main_game_render source boundary changed")
if len(sys.argv) != 2:
    raise SystemExit("usage: mac_main_game_render_view.py OUTPUT.c")

# port: the session has one map and no title map (levels\\ui\\ui) to go back
# to, which a network game does when it ends or its host goes. In the
# automated network test the session says the game is over instead
# (port/mac/engine_network_test.c) and its loop ends.
ui_prototype = "void main_load_ui_scenario(\n\tboolean precache_resources)\n{\n\tstruct game_options options;\n"
if source.count(ui_prototype) != 1:
    raise SystemExit("main_load_ui_scenario source boundary changed")
source = source.replace(ui_prototype, "int mac_engine_network_test_leave_game(void) __attribute__((weak)); /* port: port/mac/engine_network_test.c */\n" +
    ui_prototype + "\n\tif (mac_engine_network_test_leave_game())\n\t\treturn;\n")

# Keep the original program entrypoint compiled but unreferenced in this
# diagnostics link, whose test driver already owns main().
# port: the campaign flow (a level won, the credits, a map chosen) is read by the
# session, which has no main loop: main_set_map_name counts the maps asked for,
# and the credits (main_roll_credits: the Xbox's Bink outro on the main menu's
# scenario) are noted for the session to play the Mac game's ending movie.
map_name_definition = "void main_set_map_name(\n\tchar const *map_name)\n{\n"
credits_definition = ("void main_roll_credits(\n\tvoid)\n{\n"
    "\terror(_error_silent, \"congratulations, you won the game!\");\n"
    "\tmain_menu_load();\n\tgame_end_credits_start();\n\treturn;\n}\n")
if source.count(map_name_definition) != 1 or source.count(credits_definition) != 1:
    raise SystemExit("main_set_map_name or main_roll_credits source boundary changed")
source = source.replace(map_name_definition, map_name_definition + "\tmac_map_name_serial++;\n")
source = source.replace(credits_definition,
    "void main_roll_credits(\n\tvoid)\n{\n"
    "\terror(_error_silent, \"congratulations, you won the game!\");\n"
    "\tmac_credits_pending = TRUE;\n\tmain_globals.want_to_be_at_main_menu = TRUE;\n\treturn;\n}\n")
view = "#define main mac_original_main_entry\nstatic long mac_map_name_serial;\nstatic int mac_credits_pending;\n" + source
view += "\n/* Expose the original, camera-owning render call for native integration. */\n"
view += "void mac_engine_main_game_render(double time_delta_since_tick_sec)\n"
view += "{\n\tmain_game_render(time_delta_since_tick_sec);\n}\n"
# port: the menus' requests (port/mac/engine_menu.c): the game main_goto_main_menu
# asked for, read and cleared here because main_globals is private to this file.
view += "\n/* Whether the game asked to go to the main menu (cleared if clear). */\n"
view += "int mac_engine_main_wants_menu(int clear)\n"
view += "{\n\tint wanted = main_globals.want_to_be_at_main_menu != 0;\n"
view += "\tif (clear)\n\t\tmain_globals.want_to_be_at_main_menu = FALSE;\n"
view += "\treturn wanted;\n}\n"
# port: a script's switch_bsp or a BSP switch trigger volume asks main for a
# structure bsp switch, which main's loop applies at the head of a tick
# (main_switch_to_structure_bsp_private; main_globals is private to this file).
view += "\n/* Apply a pending structure bsp switch, as main's loop does before a tick. */\n"
view += "void mac_engine_main_apply_bsp_switch(void)\n"
view += "{\n\tif (main_globals.switch_to_structure_bsp_index != NONE)\n"
view += "\t\tmain_switch_to_structure_bsp_private();\n}\n"
# port: the checkpoint and death lifecycle of main's loop (a script's
# game_save, a player's death -> game_lost -> revert to the checkpoint, a
# revert or reset the game asked for). main_globals and the steps are private
# to main.c. The head runs before a tick, the save step after it.
view += "\n/* The loop's head requests: a lost map, respawn, finished save, revert, skip, reset. */\n"
view += "void mac_engine_main_lifecycle_head(void)\n"
view += "{\n\tif (main_globals.lost_map)\n\t\tmain_lost_map_private();\n"
view += "\tif (main_globals.won_map)\n\t\tmain_won_map_private();\n"
view += "\tif (main_globals.respawn)\n\t\tmain_respawn_private();\n"
view += "\tif (main_globals.save_map_completed)\n\t\tmain_saving_map_private();\n"
view += "\tif (main_globals.revert_map)\n\t\tmain_revert_map_private();\n"
view += "\tif (main_globals.skip_cinematic)\n\t\tmain_skip_cinematic_private();\n"
view += "\tif (main_globals.reset_map)\n\t\tmain_reset_map_private();\n}\n"
view += "\n/* A requested save waiting for a safe moment, after a tick. */\n"
view += "void mac_engine_main_save_step(void)\n"
view += "{\n\tif (main_globals.saving_map)\n\t\tmain_save_map_private();\n}\n"
# port: the head's won map (main_won_map_private: the next level asked for with
# the same difficulty, else the credits), the maps asked for, and the end of
# main_new_map (its requests cleared, the saved game of the persistent storage
# taken when the map was not chosen as a next level).
view += "\n/* The number of maps asked for (main_set_map_name) so far. */\n"
view += "long mac_engine_main_map_serial(void)\n{\n\treturn mac_map_name_serial;\n}\n"
view += "\n/* port: whether a map change is deferred (the menus have chosen the game: main_menu_switch_to_single_player;\n"
view += "   main_defer_map_map_change clears it while the difficulty is still to be chosen). */\n"
view += "int mac_engine_main_map_change_deferred(void)\n{\n\treturn main_globals.defer_map_change != 0;\n}\n"
view += "\n/* Whether the game is over (credits to play); cleared if clear. */\n"
view += "int mac_engine_main_credits_pending(int clear)\n"
view += "{\n\tint pending = mac_credits_pending;\n\tif (clear)\n\t\tmac_credits_pending = FALSE;\n\treturn pending;\n}\n"
view += "\n/* A level won, asked for by a script's game_won or a test. */\n"
view += "int mac_engine_main_won_pending(void)\n{\n\treturn main_globals.won_map;\n}\n"
view += "\n/* The tail of main_new_map: the requests cleared, and the saved game taken.\n"
view += "   Returns whether the saved game was looked for (persistent storage allowed). */\n"
view += "int mac_engine_main_new_map_finish(void)\n"
view += "{\n\tint looked = main_globals.allow_persistent_storage != 0;\n"
view += "\tmain_globals.reset_map = FALSE;\n\tmain_globals.defer_map_change = FALSE;\n"
view += "\tmain_globals.revert_map = FALSE;\n\tmain_globals.skip_cinematic = FALSE;\n"
view += "\tmain_globals.saving_map = FALSE;\n\tmain_globals.won_map = FALSE;\n"
view += "\tmain_globals.lost_map = FALSE;\n\tmain_globals.respawn = FALSE;\n"
view += "\tmain_globals.save_core = FALSE;\n\tmain_globals.switch_to_structure_bsp_index = NONE;\n"
view += "\tif (looked)\n\t\tgame_state_try_and_load_from_persistent_storage();\n"
view += "\treturn looked;\n}\n"
# port: the automated network test (port/linux/game/network_test.c) sets a
# game up only once the main menu is loaded, which main.c tells it and which
# main_globals keeps private to this file.
view += "\n/* Whether the main menu's scenario is loaded. */\n"
view += "int mac_engine_main_menu_loaded(void)\n"
view += "{\n\treturn main_globals.main_menu_scenario_loaded != 0;\n}\n"
# port: the render graph's own link (tools/mac_main_game_render_checks.sh) has
# no session in it: a weak answer to the game-over hook, which the session's
# port/mac/engine_network_test.c replaces
view += "\n/* The session's port/mac/engine_network_test.c defines this; this one lets the render graph link alone. */\n"
view += "int mac_engine_network_test_leave_game(void)\n{\n\treturn 0;\n}\n"
output_file = open(sys.argv[1], "w")
output_file.write(view)
output_file.close()
