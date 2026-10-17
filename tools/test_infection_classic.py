"""Classic defaults, migration, and per-unit shield policy (production C)."""
from tools.test_infection import compile_run, function, production, read


def test_classic_options_and_v2_migration(tmp_path):
    source = production() + r'''
    int main(void) {
        struct game_variant v={2,{1}}; struct game_variant_options o, got;
        struct playlist_profile_options_header h;
        byte block[512]={0}; unsigned long n; int i;
        int speeds[]={100,115,150,180,190,200};
        memset(&o,0,sizeof(o)); infection_options_default(&o);
        assert(o.infection_speed_percent==180 && o.infection_respawn_seconds==5);
        assert(o.infection_zombie_shields==1);
        for(i=0;i<6;i++) {
            o.infection_speed_percent=(byte)speeds[i]; infection_options_validate(&v,&o);
            assert(o.infection_speed_percent==speeds[i]);
        }
        o.infection_speed_percent=255; infection_options_validate(&v,&o);
        assert(o.infection_speed_percent==200);
        o.infection_speed_percent=115; o.infection_respawn_seconds=3;
        o.infection_zombie_shields=0; o.infection_enabled=1;
        h.magic=PLAYLIST_PROFILE_OPTIONS_MAGIC; h.version=2; h.size=0x24;
        n=sizeof(h)+h.size;
        memcpy(block+256,&h,sizeof(h)); memcpy(block+256+sizeof(h),&o,h.size);
        saved_game_file_generate_checksum(block+256,n,(XCALCSIG_SIGNATURE *)(block+256+n));
        memset(&got,0xAA,sizeof(got)); assert(playlist_profile_options_from_block(block,&got));
        assert(got.infection_enabled && got.infection_speed_percent==115);
        assert(got.infection_respawn_seconds==3 && got.infection_zombie_shields==1);
        for(i=0;i<2;i++) {
            got.infection_zombie_shields=(byte)i;
            playlist_profile_options_to_block(block,&got);
            assert(playlist_profile_options_from_block(block,&o));
            assert(!memcmp(&o,&got,sizeof(o)));
        }
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0


def test_shields_are_per_unit_and_restore_without_healing(tmp_path):
    source = r'''
    #include <assert.h>
    typedef int boolean; typedef float real;
    #define NONE (-1)
    #define TRUE 1
    #define FALSE 0
    #define FLAG(b) (1UL<<(b))
    #define SET_FLAG(v,b,on) ((v) = (on) ? ((v)|FLAG(b)) : ((v)&~FLAG(b)))
    enum { _object_shield_charging_bit, _object_shield_over_charging_bit, _object_shield_depleted_bit };
    struct unit_datum { struct { real maximum_shield_vitality,shield_vitality,body_vitality;
        unsigned long damage_flags; } object; };
    static long shield_unit[2]={NONE,NONE};
    static real shield_maximum[2];
    ''' + function(read('port/linux/game/infection.c'), 'infection_apply_shields') + r'''
    int main(void) {
        struct unit_datum a={{75,0.6f,0.8f,3}},b={{100,1,1,0}};
        infection_apply_shields(0,123,&a,FALSE);
        assert(a.object.maximum_shield_vitality==75 && a.object.shield_vitality==0.6f);
        infection_apply_shields(0,123,&a,TRUE);
        assert(!a.object.maximum_shield_vitality && !a.object.shield_vitality);
        assert(a.object.body_vitality==0.8f && !(a.object.damage_flags&3));
        a.object.shield_vitality=1; infection_apply_shields(0,123,&a,TRUE);
        assert(!a.object.shield_vitality && shield_maximum[0]==75);
        infection_apply_shields(1,456,&b,FALSE);
        assert(b.object.maximum_shield_vitality==100 && b.object.shield_vitality==1);
        infection_apply_shields(0,123,&a,FALSE);
        assert(a.object.maximum_shield_vitality==75 && !a.object.shield_vitality);
        assert(a.object.body_vitality==0.8f);
        infection_apply_shields(0,123,&a,TRUE);
        infection_apply_shields(0,789,&b,TRUE); /* new respawn, reused player slot */
        infection_apply_shields(0,789,&b,FALSE);
        assert(b.object.maximum_shield_vitality==100 && !b.object.shield_vitality);
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0
