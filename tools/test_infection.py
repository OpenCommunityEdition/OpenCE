"""Focused production Infection rules, inventory, menu and save-layout checks."""
import os
from pathlib import Path
import shutil
import subprocess
import re

import pytest

from tools.harness import read, function, structure, enum_with


def inventory_world():
    """Real player trigger branch and unit drop functions; fake object storage only."""
    units = read('source/units/units.c')
    rules = read('port/linux/game/infection.c')
    players = read('source/game/players.c')
    trigger = players[players.index('if (weapon_index != NONE && weapon_must_be_readied(weapon_index))'):]
    trigger = trigger[:trigger.index('control_data.control_flags =')]
    source = production() + r'''
    #define MAXIMUM_WEAPONS_PER_UNIT 4
    #define DATUM_INDEX_TO_ABSOLUTE_INDEX(i) ((i)&65535)
    #define VALID_INDEX(i,n) ((i)>=0 && (i)<(n))
    #define TEST_FLAG(f,b) ((f)&(1UL<<(b)))
    #define _object_dead_bit 0
    #define _object_invisible_bit 1
    #define _object_type_weapon 2
    #define _object_mask_weapon (1UL<<2)
    #define _weapon_doesnt_count_toward_maximum_bit 3
    #define _unit_control_weapon_primary_trigger_bit 0
    #define _unit_control_weapon_secondary_trigger_bit 1
    struct unit_datum { long definition_index; struct { long damage_flags,parent_object_index; } object;
        struct { long player_index; short current_weapon_index,desired_weapon_index;
        long weapon_object_indices[4],equipment_object_index; byte grenade_counts[2]; } unit; } units[128];
    struct player_datum { long unit_index; boolean quit_out_of_game; } players[128];
    struct object_datum { long definition_index; struct { long flags,type; } object; } objects[512];
    struct unit_definition { int unused; } unit_def;
    struct weapon_definition { struct { long flags; } weapon; } weapon_def;
    struct distributed_infection_state infection_state;
    long ids[128], identities[128], participant[128], skull[128], equipped_unit[128], killed_unit[128];
    int enabled=1, drops, deletes, alive[512], equipping, client, missing_skull, allocations, ball=42;
    #define csmemset memset
    struct object_placement_data { long definition_index,owner_team_index; };
    static boolean infection_enabled(void) { return enabled; }
    static boolean network_game_distributed_client(void) { return client; }
    static struct unit_datum *unit_get(long i) { return &units[i]; }
    static struct unit_datum *unit_try_and_get(long i) { return i>=0 && i<128 ? &units[i] : NULL; }
    static struct player_datum *player_try_and_get(long i) { long s=i&65535; return s<128 && ids[s]==i ? &players[s] : NULL; }
    static struct player_datum *player_get(long i) { assert(player_try_and_get(i)); return &players[i&65535]; }
    static unsigned long infection_player_identity(long i) { return identities[i&65535]; }
    static struct object_datum *object_get(long i) { assert(i>=0 && i<512); return &objects[i]; }
    static struct object_datum *object_try_and_get(long i) { return i>=0 && i<512 && alive[i] ? &objects[i] : NULL; }
    static struct object_datum *object_try_and_get_and_verify_type(long i,long mask) {
        struct object_datum *o=object_try_and_get(i); return o && (mask&(1UL<<o->object.type)) ? o : NULL;
    }
    static struct object_datum *weapon_get(long i) { return object_get(i); }
    static struct unit_definition *unit_definition_get(long i) { return &unit_def; }
    static struct weapon_definition *weapon_definition_get(long i) { return &weapon_def; }
    static long get_ball_definition_index(void) { return ball; }
    static long unit_inventory_get_weapon(long u,short s) { return s>=0 && s<4 ? units[u].unit.weapon_object_indices[s] : NONE; }
    static long unit_get_current_weapon_index(long u) { return unit_inventory_get_weapon(u,units[u].unit.current_weapon_index); }
    static short unit_weapon_next_index(long u,short s,short d) { return NONE; }
    static short unit_inventory_next_weapon(long u,short s,short d) {
        int n; for(n=0;n<4;n++) if(units[u].unit.weapon_object_indices[n]!=NONE) return n; return NONE;
    }
    static boolean weapon_put_away(long w,boolean immediate) { return TRUE; }
    static boolean weapon_can_be_fired(long w) { return TRUE; }
    static boolean weapon_must_be_readied(long w) { return objects[w].definition_index==42; }
    static void first_person_weapon_message_from_unit(long u,int m) { }
    static void unit_drop_item(long u,long w) { drops++; }
    static void object_delete(long w) { assert(alive[w]); alive[w]=0; deletes++; }
    static void unit_exit_seat_end(long u) { units[u].object.parent_object_index=NONE; }
    static void object_placement_data_new(struct object_placement_data *p,long d,long owner) { p->definition_index=d; }
    static long object_new(struct object_placement_data *p) {
        long w=256+allocations++; assert(w<512); alive[w]=1; objects[w].definition_index=p->definition_index;
        objects[w].object.type=2; return w;
    }
    static boolean infection_pickup(long u,long w);
    static const char *unit_get_seat_label(long u) { return "stand"; }
    static const char *weapon_get_label(long w) { return "ball"; }
    static boolean unit_set_or_test_seat_and_weapon_label(long u,const char *s,const char *w,boolean set) { return TRUE; }
    static boolean game_engine_allow_pick_up(long u,long w) { return enabled ? infection_pickup(u,w) : TRUE; }
    static boolean unit_add_weapon_to_inventory(long u,long w,int mode) {
        assert(infection_pickup(u,w)); units[u].unit.weapon_object_indices[0]=w;
        units[u].unit.desired_weapon_index=0; return TRUE;
    }
    static void game_engine_default_postspawn_player_update(long p) { }
    static void infection_reconcile_player(long p) { }
    static void reset_world(void) {
        int i; memset(units,0,sizeof(units)); memset(players,0,sizeof(players)); memset(objects,0,sizeof(objects));
        memset(&infection_state,0,sizeof(infection_state)); memset(alive,0,sizeof(alive));
        for(i=0;i<128;i++) { ids[i]=0x10000+i; identities[i]=100+i; participant[i]=ids[i];
            players[i].unit_index=i; units[i].unit.player_index=ids[i]; equipped_unit[i]=i; killed_unit[i]=NONE;
            units[i].object.parent_object_index=units[i].unit.equipment_object_index=NONE;
            memset(units[i].unit.weapon_object_indices,255,sizeof(units[i].unit.weapon_object_indices));
            units[i].unit.current_weapon_index=units[i].unit.desired_weapon_index=0;
            units[i].unit.weapon_object_indices[0]=i+128; skull[i]=i+128;
            alive[i+128]=1; objects[i+128].definition_index=42; objects[i+128].object.type=2;
            infection_state.players[i].identity=identities[i]; infection_state.players[i].role=2;
        }
        infection_state.phase=2; enabled=1; equipping=0; drops=deletes=client=missing_skull=allocations=0; ball=42;
    }
    '''
    source += function(rules, 'infection_player') + function(rules, 'infection_unit_is_zombie')
    if 'boolean infection_allow_weapon_drop(' in rules:
        source += function(rules, 'infection_allow_weapon_drop')
    source += function(units, 'unit_drop_current_weapon') + function(units, 'unit_drop_selected_weapon')
    for name in ('infection_pickup', 'infection_delete_weapon', 'infection_delete_skull',
                 'infection_strip_weapons', 'infection_equip', 'infection_tick_player'):
        source += function(rules, name)
    source += function(units, 'unit_can_use_weapon')
    source += '''
    static void player_trigger(long u,int flags) {
        struct { long control_flags; short desired_weapon_index; } input, *action=&input;
        struct player_datum *player=player_get(units[u].unit.player_index);
        struct unit_datum *unit=unit_get(u); long weapon_index=unit_get_current_weapon_index(u);
        input.control_flags=flags;
    ''' + trigger + '\n}\n'
    return source


def test_voluntary_skull_drop(tmp_path):
    assert compile_run(tmp_path, inventory_world() + '''
    int main(void) {
        reset_world(); player_trigger(0,1); assert(!drops && units[0].unit.weapon_object_indices[0]==128);
        player_trigger(0,2); assert(!drops);
        assert(!unit_drop_selected_weapon(0) && !drops);
        /* A selected non-counting weapon falls back to dropping current.
           Check the weapon actually removed, even with incompatible state. */
        units[0].unit.desired_weapon_index=1; units[0].unit.weapon_object_indices[1]=400;
        objects[400].definition_index=10; objects[400].object.type=2; alive[400]=1;
        weapon_def.weapon.flags=1UL<<_weapon_doesnt_count_toward_maximum_bit;
        assert(!unit_drop_selected_weapon(0) && !drops);
        return 0;
    }''') == 0


def test_skull_drop_cleanup_and_other_modes(tmp_path):
    assert compile_run(tmp_path, inventory_world() + '''
    int main(void) {
        int i;
        reset_world();
        for(i=0;i<128;i++) { player_trigger(i,3); assert(!unit_drop_selected_weapon(i)); }
        assert(!drops && !allocations);
        enabled=0; player_trigger(0,1); assert(drops==1); /* normal objective drop */
        reset_world(); infection_state.phase=3; player_trigger(0,1); assert(drops==1);
        reset_world(); equipping=1; assert(unit_drop_selected_weapon(0));
        reset_world(); players[0].quit_out_of_game=1; assert(unit_drop_selected_weapon(0));
        reset_world(); units[0].object.damage_flags=1; assert(unit_drop_current_weapon(0,TRUE));
        infection_tick_player(ids[0]); assert(deletes==1 && skull[0]==NONE);
        infection_tick_player(ids[0]); assert(deletes==1);
        reset_world(); ids[0]+=0x10000; assert(infection_allow_weapon_drop(0,128)); /* stale datum */
        reset_world(); identities[0]++; assert(infection_allow_weapon_drop(0,128)); /* reused identity */
        reset_world(); client=1; player_trigger(0,1); assert(!drops); /* local prediction */
        return 0;
    }''') == 0


def test_skull_inventory_recovery(tmp_path):
    assert compile_run(tmp_path, inventory_world() + '''
    int main(void) {
        long issued; int i;
        reset_world();
        for(i=0;i<128;i++) { infection_equip(ids[i]); assert(skull[i]==i+128); }
        assert(!allocations && !deletes); /* no churn, including slots above 16 */
        units[0].unit.current_weapon_index=NONE; units[0].unit.desired_weapon_index=NONE;
        infection_equip(ids[0]); assert(units[0].unit.desired_weapon_index==0 && !allocations);
        units[0].unit.weapon_object_indices[1]=128; /* duplicate skull slot */
        units[0].unit.weapon_object_indices[2]=units[0].unit.weapon_object_indices[3]=400;
        objects[400].definition_index=10; objects[400].object.type=2; alive[400]=1;
        infection_equip(ids[0]); assert(!allocations && deletes==1);
        for(i=1;i<4;i++) assert(units[0].unit.weapon_object_indices[i]==NONE);
        units[0].unit.weapon_object_indices[0]=NONE; /* involuntary loss: repair once */
        infection_equip(ids[0]); issued=skull[0]; assert(issued==256 && deletes==2 && allocations==1);
        infection_equip(ids[0]); assert(skull[0]==issued && allocations==1);
        reset_world(); infection_state.players[0].role=1;
        assert(!infection_pickup(0,128)); /* red Spartan must not equip a skull */
        assert(!unit_can_use_weapon(0,128)); /* actual readying gate, despite compatible animation labels */
        assert(unit_can_use_weapon(1,129)); /* the zombie can ready its own skull */
        enabled=0; assert(unit_can_use_weapon(0,128)); enabled=1; /* normal Oddball */
        alive[400]=1; objects[400].definition_index=10; objects[400].object.type=2;
        units[0].unit.weapon_object_indices[1]=400;
        infection_equip(ids[0]); assert(skull[0]==NONE && deletes==1 && !allocations);
        assert(units[0].unit.weapon_object_indices[1]==400 && units[0].unit.desired_weapon_index==1);
        reset_world(); equipped_unit[0]=NONE; client=1;
        infection_equip(ids[0]); assert(!deletes && !allocations);
        reset_world(); ball=NONE; equipped_unit[0]=NONE;
        infection_equip(ids[0]); assert(missing_skull && !allocations);
        infection_equip(ids[0]); assert(!allocations); /* cancel, no per-tick allocation loop */
        return 0;
    }''') == 0


def test_human_native_spawn_inventory(tmp_path):
    # The native postspawn callback may intentionally leave the weapons from
    # player_spawn alone (generic equipment / no map override). It must not
    # have to recreate weapons deleted by Infection.
    assert compile_run(tmp_path, inventory_world() + '''
    int main(void) {
        reset_world(); infection_state.players[0].role=1;
        equipped_unit[0]=NONE; skull[0]=NONE;
        objects[128].definition_index=10; /* existing native spawn weapon */
        infection_equip(ids[0]);
        assert(alive[128] && units[0].unit.weapon_object_indices[0]==128);
        assert(!deletes && !allocations);
        infection_equip(ids[0]); /* no repeated loadout reset on player ticks */
        assert(alive[128] && units[0].unit.weapon_object_indices[0]==128);
        return 0;
    }''') == 0


def production():
    header = read("source/game/game_engine.h")
    network = read("port/linux/game/network_infection.h")
    options = read("port/linux/game/infection_options.c")
    profile = read("source/saved games/playlist_profile.c")
    return "\n".join([
        '#include <assert.h>\n#include <string.h>\n#include <stddef.h>\n#include <stdio.h>',
        'typedef unsigned char byte, boolean; typedef unsigned short word; typedef float real;',
        '#define TRUE 1\n#define FALSE 0\n#define NONE (-1)\n#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 128',
        '#define MIN(a,b) ((a)<(b)?(a):(b))\n#define MAX(a,b) ((a)>(b)?(a):(b))\n#define PIN(x,a,b) MIN(MAX(x,a),b)',
        '#define csmemcpy memcpy\n#define csmemcmp memcmp',
        '#define NUMBER_OF_VARIANT_VEHICLES 6\n#define PLAYLIST_PROFILE_OPTIONS_VERSION 1',
        '#define PLAYLIST_PROFILE_OPTIONS_MAGIC 0x4F565047\n#define PLAYLIST_PROFILE_OPTIONS_OFFSET 0x100',
        'enum { game_engine_slayer=2 };',
        *(enum_with(header, name) for name in ('_friendly_fire_on', '_radar_players_none', '_loadout_category', '_loadout_weapon_none')),
        *(structure(header, name) for name in ('universal_variant', 'ctf_variant', 'infection_options', 'slayer_variant', 'king_variant', 'oddball_variant', 'race_variant')),
        re.search(r'union game_engine_variant\s*\{[^}]+\};', header).group(),
        structure(header, 'game_variant'), structure(header, 'game_variant_options'),
        structure(network, 'distributed_infection_player'), structure(network, 'distributed_infection_state'),
        enum_with(network, '_distributed_infection_waiting'),
        structure(profile, 'playlist_profile_options_header'),
        'typedef struct { unsigned long value; } XCALCSIG_SIGNATURE;',
        'static void saved_game_file_generate_checksum(byte *p,unsigned long n,XCALCSIG_SIGNATURE *sum) { unsigned long hash=2166136261UL; while(n--) hash=(hash^*p++)*16777619UL; sum->value=hash; }',
        '\n'.join(line for line in options.splitlines() if not line.startswith('#include')),
        function(profile, 'playlist_profile_options_from_block'), function(profile, 'playlist_profile_options_to_block'),
        function(read('port/linux/game/infection.c'), 'infection_round_result'),
        function(read('port/linux/game/network_infection.c'), 'infection_snapshot_valid'),
    ])



CASES = {
    'victory': r'''
assert(infection_round_result(0,17,9000,9000)==1);
        assert(infection_round_result(1,17,9000,9000)==0);
        assert(infection_round_result(1,17,900000,0)==NONE);
        assert(infection_round_result(1,0,1,9000)==0);
        assert(infection_round_result(0,0,1,9000)==2);
        assert(infection_round_result(1,1,8999,9000)==NONE);
    ''',
    'options': r'''
struct game_variant v={0}, saved; struct infection_options *o; byte bytes[104];
 assert(sizeof(v)==104 && sizeof(struct game_variant_options)==28 && sizeof(union game_engine_variant)==24);
 v.game_engine_index=2; v.universal_variant.teams=1;
 assert(!infection_options_get(&v)->infection_enabled);
 o=infection_options_edit(&v); assert(o && o->infection_speed_percent==180 && o->infection_respawn_seconds==5 && o->infection_zombie_shields);
 o->infection_enabled=1; o->infection_speed_percent=190;
 memcpy(bytes,&v,sizeof(v)); memcpy(&saved,bytes,sizeof(v));
 assert(infection_options_get(&saved)->infection_enabled && infection_options_get(&saved)->infection_speed_percent==190);
 o->infection_speed_percent=99; assert(!infection_options_get(&v)->infection_enabled);
 o->infection_speed_percent=200; o->reserved[0]=1; assert(!infection_options_get(&v)->infection_enabled);
 o->reserved[0]=0; assert(infection_options_get(&v)->infection_enabled);
 v.game_engine_index=3; assert(!infection_options_get(&v)->infection_enabled && !infection_options_edit(&v));
 assert(!infection_options_get(NULL)->infection_enabled);
    ''',
    'snapshot': r'''
struct distributed_infection_state s; int i;
        memset(&s,0,sizeof(s)); s.game_number=7; s.revision=4; s.winner=NONE; s.phase=2;
        for(i=0;i<128;i++) { s.players[i].role=i<17?2:1; s.players[i].identity=i+1; }
        assert(infection_snapshot_valid(&s,7,3)); assert(!infection_snapshot_valid(&s,8,3));
        assert(!infection_snapshot_valid(&s,7,5)); s.players[127].role=3;
        assert(!infection_snapshot_valid(&s,7,3)); s.players[127].role=2; s.players[127].reserved[2]=1;
        assert(!infection_snapshot_valid(&s,7,3)); s.players[127].reserved[2]=0; s.winner=1;
        assert(!infection_snapshot_valid(&s,7,3)); s.phase=3; assert(infection_snapshot_valid(&s,7,3));
        s.players[127].score=-1; assert(!infection_snapshot_valid(&s,7,3));
    ''',
    'migration': r'''
byte block[512]={0}; struct game_variant_options old={0},got={0}; struct playlist_profile_options_header h;
 old.time_limit=15; old.friendly_fire=2; old.vehicle_counts[1][5]=3; old.primary_weapon=4;
 playlist_profile_options_to_block(block,&old); memcpy(&h,block+256,sizeof(h));
 assert(h.version==1 && h.size==28); assert(playlist_profile_options_from_block(block,&got));
 assert(!memcmp(&old,&got,sizeof(old))); block[268]^=1; assert(!playlist_profile_options_from_block(block,&got));
    ''',
}


def compile_run(tmp_path, source):
    compiler = os.environ.get('CC') or shutil.which('clang')
    if not compiler and os.name == 'nt':
        candidate = Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/bin/clang.exe')
        if candidate.exists(): compiler = str(candidate)
    if not compiler: pytest.skip('requires clang and a 32-bit C runtime')
    c = tmp_path / 'test.c'
    exe = tmp_path / ('test.exe' if os.name == 'nt' else 'test')
    c.write_text(source)
    args = [compiler, '-m32', '-std=gnu89', '-Werror', '-Wno-unused-function', str(c), '-o', str(exe)]
    if os.name != 'nt': args += ['-fshort-wchar', '-lm']
    result = subprocess.run(args, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return subprocess.run([str(exe)], capture_output=True, timeout=10).returncode


def test_alpha_armour_channels(tmp_path):
    source = read('port/linux/game/infection.c')
    if 'static void infection_color_unit(' in source:
        coloring = function(source, 'infection_color_unit')
    else:
        # Execute the original recoloring block as a regression baseline.
        block = source[source.index('game_engine_player_get_change_color(&unit->object.base_change_colors[0]'):]
        block = block[:block.index('if (team)')]
        coloring = 'static void infection_color_unit(struct unit_datum *unit, long index) {' + block + '}'
    world = r'''
    #include <assert.h>
    typedef struct { float red, green, blue; } real_rgb_color;
    #define NUMBER_OF_OBJECT_CHANGE_COLORS 4
    struct unit_datum { long definition_index; struct {
        real_rgb_color base_change_colors[4], outgoing_change_colors[4];
    } object; } unit;
    struct object_change_color_definition { short scaled_by, darken_by; } channels[4];
    struct object_definition { struct { struct { int count; } change_colors; } object; } definition;
    #define TAG_BLOCK_GET_ELEMENT(block,index,type) (&channels[index])
    static struct object_definition *object_definition_get(long index) { return &definition; }
    static void game_engine_player_get_change_color(real_rgb_color *color, long index) {
        color->red=0; color->green=0; color->blue=1;
    }
    '''
    case = r'''
    int main(void) {
        int i;
        definition.object.change_colors.count=4;
        channels[0].scaled_by=1; channels[3].darken_by=2;
        for(i=0;i<4;i++) {
            unit.object.base_change_colors[i].red=1;
            unit.object.outgoing_change_colors[i].red=0.25f;
        }
        infection_color_unit(&unit,0);
        assert(unit.object.outgoing_change_colors[1].blue==1);
        assert(unit.object.outgoing_change_colors[2].blue==1);
        for(i=0;i<4;i++) assert(unit.object.base_change_colors[i].blue==1);
        assert(unit.object.outgoing_change_colors[0].red==0.25f);
        assert(unit.object.outgoing_change_colors[3].red==0.25f);
        definition.object.change_colors.count=0;
        infection_color_unit(&unit,0);
        for(i=0;i<4;i++) assert(unit.object.outgoing_change_colors[i].blue==1);
        return 0;
    }
    '''
    assert compile_run(tmp_path, world + coloring + case) == 0


@pytest.mark.parametrize('case', CASES)
def test_production_rules(tmp_path, case):
    assert compile_run(tmp_path, production() + '\nint main(void) {\n' + CASES[case] + '\nreturn 0; }') == 0


@pytest.mark.parametrize('before,after,case', [
    ('if (!humans) return 1;', 'if (!humans) return 0;', 'victory'),
    ('state->game_number != game_number', '0', 'snapshot'),
    ('header.version != PLAYLIST_PROFILE_OPTIONS_VERSION', 'header.version == PLAYLIST_PROFILE_OPTIONS_VERSION', 'migration'),
])
def test_negative_controls(tmp_path, before, after, case):
    source = production()
    assert source.count(before) == 1
    assert compile_run(tmp_path, source.replace(before, after) + '\nint main(void) {\n' + CASES[case] + '\nreturn 0; }') != 0


def test_generated_menus(tmp_path):
    from tools import port_settings as menus
    import xml.etree.ElementTree as ET
    filename = menus.INFECTION_EDIT.replace('/', '.') + '.port.xml'
    generated = '\n'.join(menus.multiplayer_files()[filename])
    assert generated == (Path(__file__).resolve().parents[1] / 'port/assets/menus/ce' / filename).read_text()
    root = ET.fromstring(generated)
    names = [node.attrib['name'] for node in root.findall('widget')]
    assert len(names) == len(set(names))
    assert sum(name.endswith('_spinner') for name in names) == 8
    assert menus.INFECTION_CHILD in menus.WIDGET_PATCHES[f'{menus.SLAYER_EDIT}/slayer_options_menu']['children']


def death_world():
    return production() + '''
    struct player_datum { long unit_index, dead_unit_index; boolean quit_out_of_game; } world[128];
    struct distributed_infection_state infection_state;
    long killed_unit[128], equipped_unit[128]; int client, reconciled;
    #define DATUM_INDEX_TO_ABSOLUTE_INDEX(i) ((i)&65535)
    static boolean network_game_distributed_client(void) { return client; }
    static struct player_datum *player_get(long i) { return &world[i]; }
    static struct distributed_infection_player *infection_player(long i) {
        return i<0 || i>=128 || !infection_state.players[i].role ? NULL : &infection_state.players[i];
    }
    static void platform_log(char const *s, ...) { }
    static void infection_reconcile_player(long i) { reconciled++; }
    ''' + function(read('port/linux/game/infection.c'), 'infection_killed')


def test_death_authority_and_lifecycle(tmp_path):
    source = death_world() + '''
    int main(void) {
        int i;
        memset(killed_unit,255,sizeof(killed_unit));
        for(i=0;i<128;i++) { world[i].unit_index=1000+i; world[i].dead_unit_index=NONE; infection_state.players[i].role=1; }
        infection_state.players[0].role=2;
        infection_state.phase=1; infection_killed(0,NONE,1,FALSE);
        assert(infection_state.players[1].role==1 && infection_state.players[0].score==0);
        infection_state.phase=2; client=1; infection_killed(0,NONE,1,FALSE);
        assert(infection_state.players[1].role==1);
        client=0; infection_killed(0,NONE,1,FALSE);
        assert(infection_state.players[1].role==2 && infection_state.players[0].score==1);
        infection_killed(0,NONE,1,FALSE); assert(infection_state.players[0].score==1);
        infection_killed(2,NONE,0,FALSE); assert(infection_state.players[2].score==1);
        assert(infection_state.players[0].role==2);
        infection_killed(3,NONE,3,FALSE); assert(infection_state.players[3].role==2 && !infection_state.players[3].score);
        infection_killed(NONE,NONE,127,FALSE); assert(infection_state.players[127].role==2);
        /* A betrayal keeps the killed human human, with no infection score. */
        infection_killed(6,NONE,7,TRUE);
        assert(infection_state.players[7].role==1 && infection_state.players[6].score==0 && infection_state.phase==2);
        infection_killed(6,NONE,7,TRUE); assert(infection_state.players[6].score==0);
        world[4].quit_out_of_game=TRUE; infection_killed(0,NONE,4,FALSE); assert(infection_state.players[4].role==1);
        infection_state.phase=3; infection_killed(0,NONE,5,FALSE); assert(infection_state.players[5].role==1);
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0
    assert compile_run(tmp_path, source.replace('network_game_distributed_client() ||', 'FALSE ||', 1)) != 0


def test_human_loadout_preserved(tmp_path):
    assert compile_run(tmp_path, production() + '''
    int main(void) {
        struct game_variant v={0}; struct game_variant_options o;
        memset(&o,0,sizeof(o)); v.game_engine_index=2; infection_options_edit(&v)->infection_enabled=1;
        o.loadout=_loadout_custom; o.primary_weapon=_loadout_weapon_sniper_rifle; o.secondary_weapon=_loadout_weapon_pistol;
        (void)infection_options_get(&v);
        assert(o.primary_weapon==_loadout_weapon_sniper_rifle && o.secondary_weapon==_loadout_weapon_pistol);
        o.loadout=_loadout_category; (void)infection_options_get(&v); assert(o.loadout==_loadout_category);
        return 0;
    }''') == 0


def test_skull_damage_and_pickup_rules(tmp_path):
    rules = read('port/linux/game/infection.c')
    source = production() + '''
    #define MAXIMUM_WEAPONS_PER_UNIT 4
    #define VALID_INDEX(i,n) ((i)>=0 && (i)<(n))
    struct unit_datum { struct { long player_index; short current_weapon_index; long weapon_object_indices[4]; } unit; } units[3];
    struct object_datum { long definition_index; } objects[5];
    struct distributed_infection_state infection_state;
    struct game_variant variant;
    int enabled=1, equipping;
    static boolean infection_enabled(void) { return enabled; }
    static struct unit_datum *unit_try_and_get(long i) { return i>=0 && i<3 ? &units[i] : NULL; }
    static struct unit_datum *unit_get(long i) { return &units[i]; }
    static struct object_datum *object_get(long i) { return &objects[i]; }
    static long get_ball_definition_index(void) { return 42; }
    static struct game_variant *game_engine_get_variant(void) { return &variant; }
    static struct distributed_infection_player *infection_player(long i) { return i==NONE ? NULL : &infection_state.players[i]; }
    ''' + '\n'.join(function(rules, name) for name in (
        'infection_unit_is_zombie', 'infection_can_delete_skull',
        'infection_melee_multiplier', 'infection_pickup')) + '''
    int main(void) {
        units[0].unit.player_index=0; units[0].unit.current_weapon_index=0;
        units[0].unit.weapon_object_indices[0]=1; objects[1].definition_index=42;
        objects[2].definition_index=99; units[1].unit.player_index=1;
        infection_state.players[0].role=2; infection_state.players[1].role=1;
        infection_state.phase=2; variant.game_engine_index=2; infection_options_edit(&variant)->infection_damage_percent=200;
        assert(infection_melee_multiplier(0)==2.0f);
        assert(infection_melee_multiplier(1)==1.0f);
        infection_state.players[0].role=1; /* even a human holding a skull */
        assert(infection_melee_multiplier(0)==1.0f);
        infection_state.players[0].role=2;
        units[0].unit.current_weapon_index=NONE;
        assert(infection_melee_multiplier(0)==1.0f); /* no effective skull */
        units[0].unit.current_weapon_index=4;
        assert(infection_melee_multiplier(0)==1.0f);
        units[0].unit.current_weapon_index=0;
        assert(!infection_pickup(0,2) && !infection_pickup(1,1) && infection_pickup(1,2));
        assert(infection_pickup(0,1)); /* ready the skull already in our inventory */
        objects[3].definition_index=42; assert(!infection_pickup(0,3)); /* no ground skulls */
        equipping=1; assert(infection_pickup(0,1)); equipping=0;
        assert(infection_can_delete_skull(1) && !infection_can_delete_skull(2));
        units[0].unit.weapon_object_indices[0]=2;
        assert(infection_melee_multiplier(0)==1.0f);
        units[0].unit.weapon_object_indices[0]=1; infection_state.phase=1;
        assert(infection_melee_multiplier(0)==1.0f);
        enabled=0; infection_state.phase=2;
        assert(infection_melee_multiplier(0)==1.0f && !infection_can_delete_skull(1));
        assert(!infection_unit_is_zombie(NONE));
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0
    assert compile_run(tmp_path, source.replace('return !infection_unit_is_zombie(unit);', 'return TRUE;')) != 0


def menu_world(prefix='infection_'):
    menus = read('port/linux/game/menu_functions.c')
    prefixes = (prefix,) if isinstance(prefix, str) else prefix
    bindings = re.findall(r'^\s*\{ "(?:' + '|'.join(map(re.escape, prefixes)) +
                          r')[^"]*".*?\},', menus, re.M | re.S)
    assert bindings
    source = production()
    source += '''
    #define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
    #define FLAG(b) (1UL<<(b))
    #define _game_variant_draw_object_in_motion_sensor_bit 0
    #define WIDGET_NAME_LENGTH 31
    #define INFECTION_FIELD(field) (short)offsetof(struct infection_options, field)
    #define OPTIONS_FIELD(field) (short)offsetof(struct game_variant_options, field)
    ''' + enum_with(menus, '_option_option_byte') + structure(menus, 'gametype_option')
    # The common option walker also handles upstream's public-game chat lock.
    source += '\n' + '\n'.join(re.findall(r'^#define TEXT_CHAT_.*$', menus, re.M)) + '\n'
    source += 'static boolean p2p_hosting_listed(void) { return FALSE; }\n'
    source += function(menus, 'text_chat_locked')
    source += '\nstatic struct gametype_option const gametype_options[] = {\n' + '\n'.join(bindings) + '\n};\n'
    source += '\n'.join(function(menus, name) for name in (
        'gametype_option_named', 'gametype_option_value', 'gametype_option_value_set', 'gametype_option_index'))
    source += '''
    struct widget_instance { char name[32]; short type; struct widget_instance *child,*next,*parent;
        struct { struct { short selected_index; } list; } parameters; };
    struct game_variant variant; struct game_variant_options options;
    struct { int setup; } gametype_edit;
    static struct game_variant *edit_variant(void) { return &variant; }
    static struct game_variant_options *edit_options(void) { return &options; }
    static void visible_set(struct widget_instance *w,boolean v) { }
    static short gametype_option_setting_index(struct gametype_option const *o) { return 0; }
    static boolean setting_write(char const *a,char const *b) { return FALSE; }
    static void platform_log(char const *s,...) { }
    static boolean campaign_fail(void) { return FALSE; }
    static void vehicles_show(struct widget_instance *w) { }
    static void vehicles_keep(struct widget_instance *w) { }
    static struct widget_instance *named(struct widget_instance *w,char const *s,long n) {
        struct widget_instance *c,*found;
        if(!strncmp(w->name,s,31)) return w;
        for(c=w->child;c;c=c->next) { found=named(c,s,n); if(found) return found; }
        return NULL;
    }
    ''' + '\n'.join(function(menus, name) for name in (
        'gametype_options_each', 'gametype_options_list', 'gametype_options_init', 'gametype_options_save'))
    return source


def test_menu_bindings_and_saved_values(tmp_path):
    source = menu_world() + '''
    int main(void) {
        struct game_variant v={0}, saved; struct game_variant_options o={0}, restored;
        byte block[4096]={0}; char shortname[32]; int i;
        long wanted[]={1,3,190,250,5,3,0,10};
        struct widget_instance list={0}, rows[8]={0}, spinners[8]={0};
        v.game_engine_index=2; v.universal_variant.teams=1;
        for(i=0;i<8;i++) {
            struct gametype_option const *binding;
            int j; for(j=0;j<31 && gametype_options[i].spinner[j];j++) shortname[j]=gametype_options[i].spinner[j]; shortname[j]=0;
            binding=gametype_option_named(shortname); assert(binding==&gametype_options[i]);
            assert(binding->values[gametype_option_index(binding,wanted[i])]==wanted[i]);
            gametype_option_value_set(binding,wanted[i],&v,&o);
            assert(gametype_option_value(binding,&v,&o)==wanted[i]);
        }
        saved=v; playlist_profile_options_to_block(block,&o);
        assert(playlist_profile_options_from_block(block,&restored));
        for(i=0;i<8;i++) assert(gametype_option_value(&gametype_options[i],&saved,&restored)==wanted[i]);
        memset(&variant,0,sizeof(variant)); variant.game_engine_index=2; memset(&options,0,sizeof(options));
        list.child=rows;
        for(i=0;i<8;i++) {
            int j;
            memcpy(rows[i].name,"op_test",8); rows[i].child=&spinners[i]; rows[i].parent=&list;
            if(i<7) rows[i].next=&rows[i+1];
            spinners[i].type=2; spinners[i].parent=&rows[i];
            for(j=0;j<31 && gametype_options[i].spinner[j];j++) spinners[i].name[j]=gametype_options[i].spinner[j];
        }
        assert(gametype_options_init(&list));
        assert(spinners[2].parameters.list.selected_index==5); /* 180% */
        assert(spinners[3].parameters.list.selected_index==2); /* 200% */
        assert(spinners[6].parameters.list.selected_index==1); /* shields ON */
        assert(spinners[7].parameters.list.selected_index==3); /* default 5 min */
        for(i=0;i<8;i++) spinners[i].parameters.list.selected_index=gametype_option_index(&gametype_options[i],wanted[i]);
        assert(gametype_options_save(&list)); assert(variant.universal_variant.teams);
        for(i=0;i<8;i++) assert(gametype_option_value(&gametype_options[i],&variant,&options)==wanted[i]);
        playlist_profile_options_to_block(block,&options); memset(&options,0,sizeof(options));
        assert(playlist_profile_options_from_block(block,&options));
        assert(gametype_options_init(&list));
        for(i=0;i<8;i++) assert(gametype_options[i].values[spinners[i].parameters.list.selected_index]==wanted[i]);
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0
