"""Production menu/profile regressions from the manual ammo and FF reports."""
import pytest
from tools.test_infection import compile_run, menu_world, function, read, production


@pytest.mark.parametrize('friendly', range(4))
def test_friendly_fire_menu_save_reload(tmp_path, friendly):
    source = '#define _CRT_SECURE_NO_WARNINGS\n' + menu_world('friendly_fire') + r'''
    int main(void) {
        struct widget_instance list={0}, rows[2]={{0}}, spinners[2]={{0}};
        byte block[512]={0}; int i;
        infection_options_default(&options); variant_ammo_options_default(&options);
        variant.game_engine_index=2; variant.universal_variant.teams=1;
        options.infection_enabled=1; options.friendly_fire=_friendly_fire_off;
        list.child=rows;
        for(i=0;i<2;i++) {
            strcpy(rows[i].name,"op_test"); rows[i].child=&spinners[i]; rows[i].parent=&list;
            if(!i) rows[i].next=&rows[1];
            spinners[i].type=2; spinners[i].parent=&rows[i];
            strcpy(spinners[i].name,gametype_options[i].spinner);
        }
        assert(gametype_options_init(&list));
        spinners[0].parameters.list.selected_index=gametype_option_index(&gametype_options[0],TEST_FRIENDLY);
        spinners[1].parameters.list.selected_index=gametype_option_index(&gametype_options[1],10);
        assert(gametype_options_save(&list));
        assert(options.infection_enabled && options.friendly_fire==TEST_FRIENDLY);
        assert(options.friendly_fire_penalty==10);
        playlist_profile_options_to_block(block,&options); memset(&options,0,sizeof(options));
        assert(playlist_profile_options_from_block(block,&options));
        infection_options_validate(&variant,&options);
        assert(options.infection_enabled && options.friendly_fire==TEST_FRIENDLY);
        assert(options.friendly_fire_penalty==10);
        assert(gametype_options_init(&list));
        assert(gametype_options[0].values[spinners[0].parameters.list.selected_index]==TEST_FRIENDLY);
        return 0;
    }'''
    assert compile_run(tmp_path, source.replace('TEST_FRIENDLY', str(friendly))) == 0


def test_ammo_child_does_not_activate_infection(tmp_path):
    source = '#define _CRT_SECURE_NO_WARNINGS\n' + menu_world(('infection_enabled_', 'ammo_reserve_')) + r'''
    int running=1;
    static int game_engine_running(void) { return running; }
    static struct game_variant *game_engine_get_variant(void) { return &variant; }
    static struct game_variant_options *game_variant_options_get(void) { return &options; }
    ''' + function(read('port/linux/game/infection.c'), 'infection_enabled') + r'''
    int main(void) {
        struct widget_instance parent={0}, toggle_row={0}, toggle={0}, ammo={0}, rows[4]={{0}}, spinners[4]={{0}};
        byte block[512]={0}; int enabled,i;
        variant.game_engine_index=2;
        infection_options_default(&options); variant_ammo_options_default(&options);
        parent.child=&toggle_row; strcpy(toggle_row.name,"op_infection_enabled");
        toggle_row.parent=&parent; toggle_row.child=&toggle; toggle.type=2;
        toggle.parent=&toggle_row; strcpy(toggle.name,"infection_enabled_spinner");
        ammo.child=rows;
        for(i=0;i<4;i++) {
            strcpy(rows[i].name,"op_ammo"); rows[i].child=&spinners[i]; rows[i].parent=&ammo;
            if(i<3) rows[i].next=&rows[i+1];
            spinners[i].type=2; spinners[i].parent=&rows[i];
            strcpy(spinners[i].name,gametype_options[i+1].spinner);
        }
        /* FFA OFF -> ON -> OFF: open parent, edit child, return and save. */
        for(enabled=0;enabled<3;enabled++) {
            assert(gametype_options_init(&parent));
            toggle.parameters.list.selected_index=gametype_option_index(&gametype_options[0],enabled==1);
            assert(gametype_options_init(&ammo));
            for(i=0;i<4;i++) spinners[i].parameters.list.selected_index=8;
            assert(gametype_options_save(&ammo));
            assert(gametype_options_save(&parent));
            assert(options.infection_enabled==(enabled==1));
            assert(infection_enabled()==(enabled==1));
            for(i=0;i<4;i++) assert(options.ammo_reserves[i]==999);
            playlist_profile_options_to_block(block,&options); memset(&options,0,sizeof(options));
            assert(playlist_profile_options_from_block(block,&options));
            infection_options_validate(&variant,&options);
            assert(infection_enabled()==(enabled==1));
        }
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0


def test_betrayal_penalty_survives_infection_respawn(tmp_path):
    killed = function(read('source/game/game_engine.c'), 'game_engine_player_killed')
    # Compile the production timer/penalty block without unrelated kill messaging.
    timers = killed[killed.index("/* port: the gametype's friendly fire penalty:"):]
    timers = timers[:timers.index('dead_player = player_get(dead_player_index);')]
    source = production() + r'''
    #define TICKS_PER_SECOND 30
    #define DATUM_INDEX_TO_ABSOLUTE_INDEX(x) (x)
    struct { long respawn_timer; } dead, *dead_player=&dead;
    struct game_variant global_variant;
    struct game_variant_options options;
    short game_engine_betrayal_penalty[128];
    int infected, friendly_fire=1, valid_players=1, same_player=0;
    long dead_player_index=1, killing_player_index=0;
    static struct game_variant_options *game_variant_options_get(void) { return &options; }
    static int infection_enabled(void) { return infected; }
    static void timers(void) {
    ''' + timers + r'''
    }
    int main(void) {
        int mode, seconds;
        global_variant.universal_variant.teams=1; options.friendly_fire_penalty=10;
        for(mode=0;mode<2;mode++) for(seconds=1;seconds<=5;seconds+=4) {
            infected=mode; options.infection_respawn_seconds=seconds;
            dead.respawn_timer=60; game_engine_betrayal_penalty[1]=300;
            timers();
            assert(dead.respawn_timer==(mode ? seconds*30+300 : 360));
            assert(!game_engine_betrayal_penalty[1] && game_engine_betrayal_penalty[0]==300);
            dead.respawn_timer=60; timers();
            assert(dead.respawn_timer==(mode ? seconds*30 : 90));
        }
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0
