/* Real camera caching and cloth scheduling; graphics, rumble and cloth physics
   are stubbed. The post-load callback table is extracted from the engine. */
#include <math.h>
#include "harness.h"
typedef unsigned short word;
typedef struct { real red, green, blue; } real_rgb_color;
typedef struct { real alpha, red, green, blue; } real_argb_color;
typedef struct { real yaw, pitch, roll; } real_euler_angles3d;
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
#define TICKS_PER_SECOND 30
#define MAXIMUM_LOCAL_PLAYERS 4
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
#define NUMBER_OF_DAMAGE_INDICATORS 4
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define csmemset memset
#define match_vassert(file,line,condition,...) CHECK(condition,"assertion")
#define SET_FLAG(v,b,on) ((on) ? ((v) |= FLAG(b)) : ((v) &= ~FLAG(b)))
#define MAX(a,b) ((a)>(b) ? (a):(b))
#define PIN(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
#include "config.inc"
static struct player_effect_globals_definition globals;
static struct player_effect_globals_definition *player_effect_globals=&globals;
static real_matrix4x3 identity={1,{1,0,0},{0,1,0},{0,0,1},{0,0,0}};
static real_matrix4x3 const *global_identity4x3=&identity;
static real_vector3d up={0,0,1};
static real_vector3d const *global_up3d=&up;
static long tick=10;
static unsigned long seed=123;
static long game_time_get(void) { return tick; }
static short game_time_get_elapsed(void) { return 1; }
static unsigned long *get_global_local_random_seed_address(void) { return &seed; }
static real real_seed_random_range(unsigned long *s,real lo,real hi) { ++*s; return hi; }
static void rumble_player_set_scale(real scale) { }
static void rumble_player_continuous(short player,real a,real b) { }
static real effect_scale_value(short f,real zero,real elapsed,real duration) { return zero*(1-elapsed/duration); }
static real periodic_function_evaluate(short f,real t) { return 1; }
#define sine sinf
#define cosine cosf
static void cross_product3d(real_vector3d const *a,real_vector3d const *b,real_vector3d *out) { *out=*a; }
static void matrix4x3_rotation_from_axis_and_angle(real_matrix4x3 *out,real_vector3d const *axis,real s,real c) { *out=identity; }
static void matrix4x3_rotation_from_angles(real_matrix4x3 *out,real yaw,real pitch,real roll) { *out=identity; }
static void get_shake_matrix(real translation,real rotation,real_matrix4x3 *out) { out->position.x=translation; }
static void matrix4x3_multiply(real_matrix4x3 const *a,real_matrix4x3 const *b,real_matrix4x3 *out) { real_matrix4x3 copy=*a; copy.position.x+=b->position.x; *out=copy; }
struct flag_datum_prefix { long definition_index,object_index; short updates_since_last_render; };
struct flag_definition { int unused; };
static struct flag_datum_prefix cloth={0,1,0};
static struct flag_definition cloth_definition;
static int steps;
static real stepped_time;
static long data_next_index(struct data_array *data,long previous) { return previous==NONE?0:NONE; }
static struct flag_datum_prefix *flag_get(long index) { return &cloth; }
static struct flag_definition *flag_definition_get(long index) { return &cloth_definition; }
static void flag_update(struct flag_datum_prefix *flag,struct flag_definition *definition,real dt) { ++steps; stepped_time=dt; }
#define data_make_valid(d) ((void)0)
#define data_make_invalid(d) ((void)0)
#include "under_test.inc"
static void impulse(short player,real value)
{
    struct player_effect_datum *effect=player_effect_get(player);
    memset(effect,0,sizeof(*effect));
    effect->direction.i=1;
    effect->camera_impulse.temporary_translation=value;
    effect->camera_impulse.temporary_duration=10;
    effect->camera_impulse.temporary_zero_scale_factor=1;
    effect->camera_impulse_time_left=10;
    effect->flags=FLAG(_player_effect_camera_impulse_just_started_bit);
}
static real draw(short player)
{
    real_matrix4x3 matrix=identity;
    player_effect_get_camera_effect_matrix(player,&matrix);
    CHECK(seed==123,"camera restores random seed");
    return matrix.position.x;
}
static void cloth_frame(real dt) { cloth.updates_since_last_render=0; flags_update(dt); }
int main(int argc,char **argv)
{
    const char *case_name=argc>1?argv[1]:"";
    int i;
    player_effect_initialize_for_new_map();
    flags_initialize_for_new_map();
    CASE("camera-checkpoint")
    {
        impulse(0,1); CHECK(draw(0)==1,"initial impulse");
        impulse(0,3); game_state_call_after_load_procs();
        CHECK(draw(0)==3,"same-tick checkpoint uses restored camera state");
        CHECK(draw(0)==3,"later frames retain the restored impulse");
        ++tick; CHECK(draw(0)<3,"next tick updates impulse");
        return 0;
    }
    CASE("camera-maps")
    {
        impulse(0,1); draw(0);
        player_effect_dispose_from_old_map(); impulse(0,2);
        CHECK(draw(0)==2,"unload invalidates cached camera");
        player_effect_initialize_for_new_map(); impulse(0,4);
        CHECK(draw(0)==4,"new map at same tick has its own camera");
        return 0;
    }
    CASE("camera-windows")
    {
        for(i=0;i<4;++i) { impulse(i,(real)i+1); CHECK(draw(i)==i+1,"window impulse"); }
        for(i=0;i<4;++i) CHECK(draw(i)==i+1,"window caches are independent");
        return 0;
    }
    CASE("cloth-checkpoint")
    {
        cloth_frame(0.01f); cloth_frame(0.02f);
        CHECK(steps==1,"one cloth step in a tick");
        game_state_call_after_load_procs(); cloth_frame(0.005f);
        CHECK(steps==2 && fabsf(stepped_time-0.005f)<1e-6f,"same-tick restore discards old pending time");
        return 0;
    }
    CASE("cloth-maps")
    {
        cloth_frame(0.01f); cloth_frame(0.02f);
        flags_dispose_from_old_map(); cloth_frame(0.005f);
        CHECK(steps==2 && fabsf(stepped_time-0.005f)<1e-6f,"unload resets cloth clock");
        cloth_frame(0.02f); flags_initialize_for_new_map(); cloth_frame(0.007f);
        CHECK(steps==3 && fabsf(stepped_time-0.007f)<1e-6f,"new map resets cloth clock");
        return 0;
    }
    CASE("cloth-frames")
    {
        cloth_frame(0); for(i=0;i<7;++i) cloth_frame(1.0f/240);
        CHECK(steps==0,"no physics on frames between ticks");
        ++tick; cloth_frame(1.0f/240);
        CHECK(steps==1 && fabsf(stepped_time-1.0f/30)<1e-6f,"accumulate all eight render frames");
        tick+=2; cloth_frame(2.0f/30);
        CHECK(steps==2 && fabsf(stepped_time-2.0f/30)<1e-6f,"low-frame-rate elapsed time is retained");
        return 0;
    }
    return 2;
}
