"""Finite Infection reserves, legacy profile migration and wire bounds."""
from tools.test_infection import compile_run, production, read, function
import pytest


def test_ammo_profiles_and_bounds(tmp_path):
    source = production() + r'''
    int main(void) {
        struct game_variant_options o, got;
        struct playlist_profile_options_header h;
        byte block[512]={0}; unsigned long n; int i;
        memset(&o,0,sizeof(o)); variant_ammo_options_default(&o);
        for(i=0;i<4;i++) assert(o.ammo_reserves[i]==-1);
        o.ammo_reserves[0]=200; o.ammo_reserves[1]=120;
        o.ammo_reserves[2]=100; o.ammo_reserves[3]=100;
        playlist_profile_options_to_block(block,&o);
        assert(playlist_profile_options_from_block(block,&got));
        assert(!memcmp(&o,&got,sizeof(o)));
        h.magic=PLAYLIST_PROFILE_OPTIONS_MAGIC; h.version=3; h.size=0x28;
        n=sizeof(h)+h.size; memcpy(block+256,&h,sizeof(h));
        memcpy(block+256+sizeof(h),&o,h.size);
        saved_game_file_generate_checksum(block+256,n,(XCALCSIG_SIGNATURE *)(block+256+n));
        assert(playlist_profile_options_from_block(block,&got));
        for(i=0;i<4;i++) assert(got.ammo_reserves[i]==-1);
        o.ammo_reserves[0]=-32768; o.ammo_reserves[1]=32767;
        variant_ammo_options_validate(&o);
        assert(o.ammo_reserves[0]==-1 && o.ammo_reserves[1]==999);
        /* A valid v4 profile from protocol 31 used to allow 1000. */
        o.ammo_reserves[0]=1000; o.ammo_reserves[1]=0;
        o.ammo_reserves[2]=-1; o.ammo_reserves[3]=500;
        playlist_profile_options_to_block(block,&o);
        assert(playlist_profile_options_from_block(block,&got));
        assert(got.ammo_reserves[0]==999 && got.ammo_reserves[1]==0);
        assert(got.ammo_reserves[2]==-1 && got.ammo_reserves[3]==500);
        assert(sizeof(o)==0x30); return 0;
    }'''
    assert compile_run(tmp_path, source)==0


def test_per_weapon_limits_and_spawn(tmp_path):
    source = production() + r'''
    #define MAXIMUM_WEAPONS_PER_UNIT 4
    enum { _weapon_list_pistol=10, _weapon_list_shotgun, _weapon_list_assault_rifle, _weapon_list_sniper_rifle };
    struct unit_datum { struct { long weapon_object_indices[4]; } unit; } unit;
    struct weapon_datum { long definition_index; struct { struct { short rounds_total,rounds_loaded; } magazines[2]; } weapon; } weapons[4];
    struct weapon_definition { struct { struct { int count; } magazines; } weapon; } definition;
    struct game_variant_options options;
    int game_engine=1;
    static int weapon_definition_index_to_list_index(long i) { return i; }
    static struct game_variant_options *game_variant_options_get(void) { return &options; }
    static boolean infection_enabled(void) { return game_engine && options.infection_enabled; }
    static struct unit_datum *unit_try_and_get(long i) { return i==0 ? &unit : 0; }
    static struct weapon_datum *weapon_get(long i) { return &weapons[i]; }
    static struct weapon_definition *weapon_definition_get(long i) { return &definition; }
    ''' + '\n'.join(function(read('source/game/game_engine.c'), name) for name in
        ('game_engine_weapon_reserve_override', 'game_engine_weapon_reserve_limit', 'game_engine_apply_spawn_reserves')) + r'''
    int main(void) {
        int i,j; short values[]={200,120,100,100}; short matrix[]={-1,0,100,200,500,999};
        variant_ammo_options_default(&options); options.infection_enabled=1; definition.weapon.magazines.count=1;
        for(i=0;i<4;i++) { unit.unit.weapon_object_indices[i]=i; weapons[i].definition_index=10+i;
            weapons[i].weapon.magazines[0].rounds_total=24; weapons[i].weapon.magazines[0].rounds_loaded=8; }
        game_engine_apply_spawn_reserves(0);
        for(i=0;i<4;i++) assert(weapons[i].weapon.magazines[0].rounds_total==24);
        for(i=0;i<4;i++) options.ammo_reserves[i]=values[i];
        game_engine_apply_spawn_reserves(0);
        for(i=0;i<4;i++) {
            assert(weapons[i].weapon.magazines[0].rounds_total==values[i]);
            assert(weapons[i].weapon.magazines[0].rounds_loaded==8);
            assert(game_engine_weapon_reserve_limit(10+i,0,24)==values[i]);
            assert(game_engine_weapon_reserve_limit(10+i,1,24)==24);
        }
        assert(game_engine_weapon_reserve_limit(99,0,24)==24);
        assert(game_engine_weapon_reserve_limit(12,0,540)==100);
        for(j=0;j<6;j++) {
            for(i=0;i<4;i++) { options.ammo_reserves[i]=matrix[j]; weapons[i].weapon.magazines[0].rounds_total=24; }
            game_engine_apply_spawn_reserves(0);
            for(i=0;i<4;i++) {
                assert(weapons[i].weapon.magazines[0].rounds_total==(matrix[j]<0?24:matrix[j]));
                assert(weapons[i].weapon.magazines[0].rounds_loaded==8);
            }
        }
        game_engine=0; game_engine_apply_spawn_reserves(0);
        assert(game_engine_weapon_reserve_limit(10,0,24)==24);
        return 0;
    }'''
    assert compile_run(tmp_path, source)==0


def test_ammo_menu_generated():
    import re
    import xml.etree.ElementTree as ET
    from tools.port_settings import multiplayer_files, AMMO_EDIT
    name = AMMO_EDIT.replace('/', '.') + '.port.xml'
    actual = read('port/assets/menus/ce/' + name)
    assert actual == '\n'.join(multiplayer_files()[name])
    ET.fromstring(actual)
    menu = read('port/linux/game/menu_functions.c')
    for i in range(4):
        assert re.search(r'"ammo_reserve_' + str(i) + r'_spinner"[^\n]+\{ -1, 0, 50, 100, 120, 200, 300, 500, 999 \}', menu)
    assert 'mp profile set slayer rules' in actual


@pytest.mark.parametrize('enabled', [0, 1])
def test_ammo_menu_edit_save_reload(tmp_path, enabled):
    from tools.test_infection import menu_world
    source = menu_world('ammo_reserve_') + r'''
    int main(void) {
        struct widget_instance list={0}, parent={0}, parent_row={0}, rows[4]={0}, spinners[4]={0};
        byte block[512]={0}; int i; short want[]={200,120,100,100};
        infection_options_default(&options); variant_ammo_options_default(&options);
        options.infection_enabled=TEST_ENABLED;
        variant.game_engine_index=2; list.child=rows;
        for(i=0;i<4;i++) {
            memcpy(rows[i].name,"op_test",8); rows[i].child=&spinners[i]; rows[i].parent=&list;
            if(i<3) rows[i].next=&rows[i+1];
            spinners[i].type=2; spinners[i].parent=&rows[i];
            memcpy(spinners[i].name,gametype_options[i].spinner,strlen(gametype_options[i].spinner)+1);
        }
        assert(gametype_options_init(&list));
        for(i=0;i<4;i++) {
            assert(spinners[i].parameters.list.selected_index==0);
            spinners[i].parameters.list.selected_index=gametype_option_index(&gametype_options[i],want[i]);
        }
        assert(gametype_options_save(&list));
        assert(options.infection_enabled==TEST_ENABLED && variant.universal_variant.teams==TEST_ENABLED);
        memcpy(parent_row.name,"op_infection_enabled",21); parent.child=&parent_row;
        assert(gametype_options_save(&parent));
        for(i=0;i<4;i++) assert(options.ammo_reserves[i]==want[i]);
        playlist_profile_options_to_block(block,&options); memset(&options,0,sizeof(options));
        assert(playlist_profile_options_from_block(block,&options));
        assert(gametype_options_init(&list));
        for(i=0;i<4;i++) assert(gametype_options[i].values[spinners[i].parameters.list.selected_index]==want[i]);
        return 0;
    }'''
    assert compile_run(tmp_path, source.replace('TEST_ENABLED', str(enabled)))==0
