"""A live alpha can change role between grenade wind-up and hand creation."""
import pytest
from tools.harness import function, read, enum_with
from tools.test_infection import compile_run, production


@pytest.mark.parametrize('client', [0, 1])
@pytest.mark.parametrize('role,enabled', [(2, 1), (1, 1), (2, 0)])
@pytest.mark.parametrize('dead', [0, 1])
def test_conversion_during_grenade_windup(tmp_path, client, role, enabled, dead):
    world = production() + '\n' + enum_with(read('source/units/units.h'), '_unit_grenade_throw_idle') + r'''
    #define csmemset memset
    #define VALID_INDEX(i,n) ((i)>=0 && (i)<(n))
    #define DATUM_INDEX_TO_ABSOLUTE_INDEX(i) ((i)&65535)
    #define _object_dead_bit 0
    #define TEST_FLAG(f,b) ((f)&(1UL<<(b)))
    struct unit_datum { struct { short owner_team_index; unsigned long damage_flags; } object;
        struct { char grenade_counts[2]; int grenade_throw_state; } unit; } unit;
    struct player_datum { long team_index,unit_index; float speed_multiplier; int powerup_durations[2];
        struct { char team_index; long player_list_index; } network_player_data; } player;
    struct network_game_server { int unused; } server;
    struct network_game { struct { char team_index; } players[128]; } game;
    static struct game_variant_options options;
    static struct distributed_infection_player role;
    static int enabled, client;
    static int infection_enabled(void) { return enabled; }
    static struct distributed_infection_player *infection_player(long p) { return &role; }
    static struct player_datum *player_get(long p) { return &player; }
    static struct unit_datum *unit_try_and_get(long p) { return &unit; }
    static struct game_variant_options *game_variant_options_get(void) { return &options; }
    static void infection_color_unit(struct unit_datum *u,long p) {}
    /* Shield reconciliation is covered separately; this world isolates
       grenade wind-up and already-spent throws. */
    static void infection_apply_shields(long slot,long index,struct unit_datum *u,boolean off) {}
    static int network_game_distributed_client(void) { return client; }
    static struct network_game_server *global_network_game_server_get(void) { return &server; }
    static struct network_game *network_game_server_get_game(struct network_game_server *s) { return &game; }
    ''' + function(read('port/linux/game/infection.c'), 'infection_reconcile_player')
    case = '''int main(void) {
        enabled=%d; client=%d; role.role=%d; unit.object.damage_flags=%d;
        unit.unit.grenade_counts[0]=2; unit.unit.grenade_counts[1]=1;
        unit.unit.grenade_throw_state=_unit_grenade_throw_wind_up;
        infection_reconcile_player(0);
        if(enabled && role.role==2 && !unit.object.damage_flags) {
            assert(unit.unit.grenade_counts[0]==0 && unit.unit.grenade_counts[1]==0);
            assert(unit.unit.grenade_throw_state==_unit_grenade_throw_ending);
        } else {
            assert(unit.unit.grenade_counts[0]==2 && unit.unit.grenade_counts[1]==1);
            assert(unit.unit.grenade_throw_state==_unit_grenade_throw_wind_up);
        }
        /* A grenade already in the hand was spent before conversion; normal
           release/cleanup must still run rather than leaving it attached. */
        unit.unit.grenade_throw_state=_unit_grenade_throw_in_hand;
        infection_reconcile_player(0);
        assert(unit.unit.grenade_throw_state==_unit_grenade_throw_in_hand);
        return 0;
    }''' % (enabled, client, role, dead)
    assert compile_run(tmp_path, world + case)==0
