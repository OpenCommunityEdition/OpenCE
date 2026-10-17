"""Production rider eligibility and host melee cone, with controlled collision results."""
from tools.harness import function, read
from tools.test_infection import compile_run


def test_rider_hit_respects_seat_weapon_and_geometry(tmp_path):
    source = r'''
    #include <assert.h>
    #include <math.h>
    typedef int boolean;
    typedef float real;
    #define TRUE 1
    #define FALSE 0
    #define NONE (-1)
    #define MAXIMUM_WEAPONS_PER_UNIT 4
    #define VALID_INDEX(x,n) ((x)>=0 && (x)<(n))
    #define TEST_FLAG(f,b) ((f)&(1U<<(b)))
    #define _object_mask_weapon 2
    #define _object_mask_vehicle 4
    #define _object_type_biped 0
    #define _object_dead_bit 0
    #define _unit_seat_invisible_bit 0
    #define _distributed_infection_active 2
    #define _collision_test_for_projectiles_flags 0x1000E9
    #define _collision_result_object 3
    #define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((b)->data[i]))
    typedef union real_vector3d { struct {float i,j,k;}; struct {float x,y,z;}; } real_vector3d;
    struct object_marker {struct {real_vector3d position;} matrix;};
    struct collision_result {int type; long object_index;};
    struct unit_seat {unsigned flags;};
    struct unit_definition {struct {struct {long count; struct unit_seat data[3];} seats;} unit;} def;
    struct object_datum {long definition_index;} weapon={42};
    struct unit_datum {long definition_index; struct {long type,damage_flags,parent_object_index;} object;
        struct {short current_weapon_index,parent_seat_index;long weapon_object_indices[4],player_index;} unit;} units[3];
    struct distributed_infection_player {int role;} role={1};
    struct {int phase;} infection_state={2};
    int enabled=1, zombie=1, blocked=0, calls=0, hit_target=1;
    float distance=.65f;
    static int infection_unit_is_zombie(long u) {return enabled && zombie && u==0;}
    static struct unit_datum *unit_get(long u) {return &units[u];}
    static struct unit_datum *unit_try_and_get(long u) {return u>=0 && u<3 ? &units[u] : 0;}
    static void *object_try_and_get_and_verify_type(long u,int mask) {
        return mask==2 && u==10 ? &weapon : mask==4 && u==2 ? (void*)&units[2] : 0;
    }
    static long get_ball_definition_index(void) {return 42;}
    static struct distributed_infection_player *infection_player(long p) {return p==1 ? &role : 0;}
    static struct unit_definition *unit_definition_get(long d) {return &def;}
    static float normalize3d(real_vector3d *v) {float n=sqrtf(v->i*v->i+v->j*v->j+v->k*v->k);
        if(n>0){v->i/=n;v->j/=n;v->k/=n;} return n;}
    static real_vector3d *perpendicular3d(real_vector3d const *a,real_vector3d *b) {b->i=0;b->j=1;b->k=0;return b;}
    static void cross_product3d(real_vector3d const *a,real_vector3d const *b,real_vector3d *c) {
        c->i=a->j*b->k-a->k*b->j;c->j=a->k*b->i-a->i*b->k;c->k=a->i*b->j-a->j*b->i;}
    static void object_get_marker_by_name(long u,char const *s,struct object_marker *m,int n) {
        m->matrix.position.i=10;m->matrix.position.j=20;m->matrix.position.k=30;
    }
    static int collision_test_vector(int flags,real_vector3d const *o,real_vector3d const *v,long ignore,struct collision_result *h) {
        assert(flags==0x1000E9 && ignore==0); assert(o->i==10 && o->j==20 && o->k==30);
        assert(v->i*v->i+v->j*v->j+v->k*v->k<=.721f); calls++;
        h->type=blocked ? 2 : 3; h->object_index=hit_target;
        return distance<=v->i;
    }
    '''
    rules = read('port/linux/game/infection.c')
    source += function(rules, 'infection_melee_rider_target') + function(rules, 'infection_melee_rider_visible')
    source += r'''
    int main(void) {
        real_vector3d aim={1,0,0};
        units[0].unit.weapon_object_indices[0]=10;
        units[1].unit.player_index=1; units[1].object.parent_object_index=2;
        def.unit.seats.count=3;
        assert(infection_melee_rider_target(0,1));
        assert(infection_melee_rider_visible(0,1,&aim));
        blocked=1; assert(!infection_melee_rider_visible(0,1,&aim)); blocked=0;
        distance=2; assert(!infection_melee_rider_visible(0,1,&aim)); distance=.65f;
        hit_target=2; assert(!infection_melee_rider_visible(0,1,&aim)); hit_target=1;
        def.unit.seats.data[0].flags=1; assert(!infection_melee_rider_target(0,1)); def.unit.seats.data[0].flags=0;
        enabled=0; assert(!infection_melee_rider_target(0,1)); enabled=1;
        zombie=0; assert(!infection_melee_rider_target(0,1)); zombie=1;
        weapon.definition_index=99; assert(!infection_melee_rider_target(0,1)); weapon.definition_index=42;
        units[0].unit.current_weapon_index=NONE; assert(!infection_melee_rider_target(0,1)); units[0].unit.current_weapon_index=0;
        role.role=2; assert(!infection_melee_rider_target(0,1)); role.role=1;
        infection_state.phase=1; assert(!infection_melee_rider_target(0,1)); infection_state.phase=2;
        units[1].object.damage_flags=1; assert(!infection_melee_rider_target(0,1)); units[1].object.damage_flags=0;
        units[1].unit.parent_seat_index=3; assert(!infection_melee_rider_target(0,1));
        return 0;
    }'''
    assert compile_run(tmp_path, source) == 0
