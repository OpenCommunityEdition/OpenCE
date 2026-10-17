"""Initial zombies must be able to play, even when other slots cannot spawn."""
from tools.harness import function, read
from tools.test_infection import compile_run, inventory_world


def world():
    return inventory_world()+r'''
    #define TICKS_PER_SECOND 30
    struct data_iterator { long datum_index,next; };
    static long population=4, now=150, fixture=3;
    static int player_data;
    static struct game_variant_options options;
    static void data_iterator_new(struct data_iterator *it,int data) { it->next=0; }
    static void *data_iterator_next(struct data_iterator *it) {
        if(it->next>=population) return NULL;
        it->datum_index=ids[it->next]; return &players[it->next++];
    }
    static long game_time_get(void) { return now; }
    static struct game_variant_options *game_variant_options_get(void) { return &options; }
    static void infection_added(long i) { assert(0); }
    static void infection_finish(long result) { infection_state.phase=3; infection_state.winner=result; }
    static long network_test_infection_initial_player(void) { return ids[fixture]; }
    static float real_random_range(float a,float b) { return b; }
    static void platform_log(char const *fmt,...) {}
    ''' + function(read('port/linux/game/infection.c'),'infection_update') + r'''
    static void setup(void) {
        int i; reset_world();
        for(i=0;i<4;i++) infection_state.players[i].role=1;
        players[2].unit_index=NONE;
        units[3].object.damage_flags=1;
        options.infection_initial_zombies=3;
        options.infection_countdown_seconds=5;
        infection_state.phase=1; infection_state.phase_tick=0;
    }
    '''


def test_alpha_excludes_missing_and_dead_units(tmp_path):
    assert compile_run(tmp_path,world()+r'''
    int main(void) {
        setup(); infection_update();
        assert(infection_state.phase==2);
        assert(infection_state.players[0].role==1);
        assert(infection_state.players[1].role==2);
        assert(infection_state.players[2].role==1 && infection_state.players[3].role==1);
        return 0;
    }''')==0


def test_wait_for_two_spawned_players_without_changing_active_counts(tmp_path):
    assert compile_run(tmp_path,world()+r'''
    int main(void) {
        setup(); players[1].unit_index=NONE; infection_update();
        assert(infection_state.phase==0);
        players[1].unit_index=1; now++; infection_update();
        assert(infection_state.phase==1);
        now+=150; infection_update(); assert(infection_state.phase==2);
        /* Active rounds still count connected humans waiting for respawn. */
        players[0].unit_index=NONE;
        options.time_limit=5;
        infection_update(); assert(infection_state.phase==2);
        return 0;
    }''')==0
