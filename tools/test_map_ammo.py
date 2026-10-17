"""Execute the original scenario respawn loop: reserves belong to new objects."""
from tools.test_infection import compile_run, production, function, read


def test_scenario_creation_reserves(tmp_path):
    engine = read('source/game/game_engine.c')
    source = production() + r'''
    #define TICKS_PER_SECOND 30
    #define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
    #define TEST_FLAG(f,b) ((f)&(1u<<(b)))
    #define SET_FLAG(f,b,v) ((f)=(v)?((f)|(1u<<(b))):((f)&~(1u<<(b))))
    #define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)((b)->data))
    enum { _weapon_list_pistol=10, _weapon_list_shotgun, _weapon_list_assault_rifle, _weapon_list_sniper_rifle,
        _object_type_weapon=1, _equipment_created_at_rest_bit=0, _object_at_rest_bit=0 };
    struct block { int count; void *data; };
    struct point { float x,y,z; };
    struct object_placement_data { struct point position,forward; long definition; };
    struct scenario_netgame_equipment { short game_type[4],spawn_time; struct { long index; } item_collection; struct point position; float facing; int flags; } equipment;
    struct scenario { struct block netgame_equipment; } scenario;
    struct item_collection_definition { short spawn_time; } collection;
    struct item_datum { long definition_index; struct { int flags,type; } object; struct { long last_owned_time; } item; struct { struct { short rounds_total,rounds_loaded; } magazines[2]; } weapon; } items[32];
    #define weapon_datum item_datum
    struct weapon_definition { struct { struct block magazines; } weapon; } definition;
    struct game_variant_options options;
    int game_engine=1, client=0, tick=0, count=0, current=12;
    static int weapon_definition_index_to_list_index(long i) { return i; }
    static struct game_variant_options *game_variant_options_get(void) { return &options; }
    static boolean infection_enabled(void) { return game_engine && options.infection_enabled; }
    static struct scenario *global_scenario_get(void) { return &scenario; }
    static boolean network_game_distributed_client(void) { return client; }
    static int game_engine_get_type(void) { return 2; }
    static int match_game_type(int a,int b,short *c) { return 1; }
    static long game_time_get(void) { return tick; }
    static struct item_collection_definition *item_collection_definition_get(long i) { return &collection; }
    static long random_item(long i) { return current; }
    static struct item_datum *object_definition_get(long i) { return &items[31]; }
    static void object_placement_data_new(struct object_placement_data *p,long i,long n) { p->definition=i; }
    static boolean custom_edition_cache_tags_loaded(void) { return FALSE; }
    static void vector3d_from_angle(struct point *p,float a) { }
    static long object_new(struct object_placement_data *p) {
        long i=count++; items[i].definition_index=p->definition;
        items[i].weapon.magazines[0].rounds_total=180; items[i].weapon.magazines[0].rounds_loaded=60; return i;
    }
    static struct item_datum *item_get(long i) { return &items[i]; }
    static struct item_datum *weapon_get(long i) { return &items[i]; }
    static struct item_datum *weapon_try_and_get(long i) { return i>=0 && i<count && items[i].definition_index>=10 ? &items[i] : 0; }
    static struct weapon_definition *weapon_definition_get(long i) { return &definition; }
    static void object_set_garbage(long i,int g) { }
    '''
    source += function(engine, 'game_engine_weapon_reserve_override')
    if 'void game_engine_apply_map_reserves(' in engine:
        source += function(engine, 'game_engine_apply_map_reserves')
    source += function(engine, 'game_engine_update_item_spawn') + r'''
    int main(void) {
        short values[]={-1,0,100,200,500,999}; int i;
        variant_ammo_options_default(&options); options.infection_enabled=1; scenario.netgame_equipment.count=1;
        scenario.netgame_equipment.data=&equipment; equipment.spawn_time=30;
        definition.weapon.magazines.count=1; items[31].object.type=_object_type_weapon;
        for(i=0;i<6;i++) {
            options.ammo_reserves[2]=values[i]; tick=i*900;
            game_engine_update_item_spawn(); assert(count==i+1);
            assert(items[i].weapon.magazines[0].rounds_total==(values[i]<0?180:values[i]));
            assert(items[i].weapon.magazines[0].rounds_loaded==60);
            /* A carried/dropped object stays spent across subsequent respawns. */
            items[i].weapon.magazines[0].rounds_total=7;
            tick++; game_engine_update_item_spawn(); assert(count==i+1);
            assert(items[i].weapon.magazines[0].rounds_total==7);
        }
        for(i=0;i<6;i++) assert(items[i].weapon.magazines[0].rounds_total==7);
        client=1; tick=9000; game_engine_update_item_spawn(); assert(count==6);
        client=0; current=99; game_engine_update_item_spawn(); assert(count==7);
        assert(items[6].weapon.magazines[0].rounds_total==180); /* unconfigured/plasma untouched */
        options.no_map_weapons=1; game_engine_update_item_spawn(); assert(count==7);
        options.no_map_weapons=0; options.infection_enabled=0; current=12;
        game_engine_update_item_spawn(); assert(count==8);
        assert(items[7].weapon.magazines[0].rounds_total==180); /* OFF ignores saved reserves */
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0
