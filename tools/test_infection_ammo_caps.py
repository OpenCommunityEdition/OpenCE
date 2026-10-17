"""Run production reserve guards and native duplicate pickups without assets."""
from tools.test_infection import compile_run, production, read, function


def world():
    engine = read('source/game/game_engine.c')
    return production() + r'''
    #define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)(b)->data+(i))
    enum { _weapon_list_pistol=10, _weapon_list_shotgun, _weapon_list_assault_rifle, _weapon_list_sniper_rifle };
    struct block { int count; void *data; };
    struct weapon_magazine { short rounds_total,rounds_loaded; };
    struct weapon_datum { long definition_index; struct { struct weapon_magazine magazines[2]; } weapon; } weapons[2];
    #define item_datum weapon_datum
    struct weapon_ammunition_object { struct { long index; } object; short rounds; };
    struct weapon_magazine_definition { short rounds_total_maximum; struct block ammunition_objects; } magazine;
    struct weapon_definition { struct { struct block magazines; struct { long index; } pickup_sound; } weapon; } definition;
    struct game_variant variant={2,{1}};
    struct game_variant_options options;
    int game_engine=1, deleted;
    static int weapon_definition_index_to_list_index(long i) { return i; }
    static struct game_variant_options *game_variant_options_get(void) { return &options; }
    static struct game_variant *game_engine_get_variant(void) { return &variant; }
    static boolean game_engine_running(void) { return game_engine; }
    static struct weapon_datum *weapon_get(long i) { return &weapons[i]; }
    static struct weapon_datum *item_get(long i) { return &weapons[i]; }
    static struct weapon_definition *weapon_definition_get(long i) { return &definition; }
    static int weapon_magazine_count(struct weapon_definition *d) { return d->weapon.magazines.count; }
    static struct weapon_magazine *weapon_magazine_get(struct weapon_datum *w,int i) { return &w->weapon.magazines[i]; }
    static void object_delete(long i) { deleted++; }
    static void unspatialized_impulse_sound_new(long i,float v) { }
    static void equipment_definition_handle_pickup(long i) { }
    ''' + function(read('port/linux/game/infection.c'), 'infection_enabled') + '\n'.join(
        function(engine, n) for n in ('game_engine_weapon_reserve_override', 'game_engine_weapon_reserve_limit')) + function(
        read('source/items/weapons.c'), 'weapon_handle_potential_inventory_item')


def test_reserves_only_in_infection(tmp_path):
    source = world() + r'''
    int main(void) {
        int mode,teams,on,running,i;
        for(i=0;i<4;i++) options.ammo_reserves[i]=100;
        for(mode=0;mode<7;mode++) for(teams=0;teams<2;teams++)
        for(on=0;on<2;on++) for(running=0;running<2;running++) {
            variant.game_engine_index=mode; variant.universal_variant.teams=teams;
            options.infection_enabled=on; game_engine=running;
            for(i=0;i<4;i++) assert(game_engine_weapon_reserve_limit(10+i,0,600)==
                (mode==2 && teams && on && running ? 100 : 600));
        }
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0


def test_native_partial_pickups_respect_cap(tmp_path):
    source = world() + r'''
    int main(void) {
        short taken=0;
        options.infection_enabled=1; options.ammo_reserves[2]=100; options.ammo_reserves[3]=100;
        definition.weapon.magazines.count=1; definition.weapon.magazines.data=&magazine;
        definition.weapon.pickup_sound.index=NONE; magazine.rounds_total_maximum=600;
        weapons[0].definition_index=weapons[1].definition_index=12;
        weapons[0].weapon.magazines[0].rounds_total=100;
        weapons[1].weapon.magazines[0].rounds_total=100;
        weapon_handle_potential_inventory_item(0,1,NONE,&taken);
        assert(taken==0 && weapons[0].weapon.magazines[0].rounds_total==100);
        assert(weapons[1].weapon.magazines[0].rounds_total==100 && !deleted);
        /* AR and sniper: one missing round takes exactly one; donor survives. */
        for(weapons[0].definition_index=12;weapons[0].definition_index<=13;weapons[0].definition_index++) {
            weapons[1].definition_index=weapons[0].definition_index;
            magazine.rounds_total_maximum=weapons[0].definition_index==12?600:24;
            weapons[0].weapon.magazines[0].rounds_total=99;
            weapons[1].weapon.magazines[0].rounds_total=100;
            weapons[1].weapon.magazines[0].rounds_loaded=4;
            assert(weapon_handle_potential_inventory_item(0,1,NONE,&taken));
            assert(taken==1 && weapons[0].weapon.magazines[0].rounds_total==100);
            assert(weapons[1].weapon.magazines[0].rounds_total==99 && !deleted);
            assert(weapons[1].weapon.magazines[0].rounds_loaded==4);
        }
        /* OFF returns to the original 600-round AR capacity even with stale options. */
        options.infection_enabled=0; magazine.rounds_total_maximum=600;
        weapons[0].definition_index=weapons[1].definition_index=12;
        weapons[0].weapon.magazines[0].rounds_total=100;
        weapons[1].weapon.magazines[0].rounds_total=100;
        assert(weapon_handle_potential_inventory_item(0,1,NONE,&taken));
        assert(taken==100 && weapons[0].weapon.magazines[0].rounds_total==200 && deleted==1);
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0


def test_ammo_menu_belongs_to_infection():
    from tools.port_settings import multiplayer_files, AMMO_CHILD, INFECTION_EDIT
    generated = multiplayer_files()[INFECTION_EDIT.replace('/', '.') + '.port.xml']
    assert any(AMMO_CHILD in line for line in generated)
    assert '/open_ammo' not in read('port/assets/menus/ce/main_menu.settings_select.multiplayer_setup.item_options_edit.xml')
