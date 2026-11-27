"""Production checkpoint membership, host policy, save and death handlers."""
from . import function, read, structure


def generated():
    coop = read('port/linux/game/network_coop.c')
    state = read('source/saved games/game_state.c')
    main = read('source/main/main.c')
    types = structure(coop, 'coop_checkpoint_players')
    code = function(read('port/linux/src/p2p_lobby.c'), 'p2p_hosting_listed')
    for name in ('coop_host', 'network_coop_checkpoint_saved', 'checkpoint_players_match',
                 'network_coop_hosting_public', 'network_coop_wipe_reverts', 'network_coop_new_game'):
        code += function(coop, name)
    code += function(state, 'game_state_save')
    for name in ('main_lost_map', 'main_death_timer_expired', 'main_lost_map_private'):
        code += function(main, name)
    return (('types.inc', types), ('under_test.inc', code))
