#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef float real;
typedef union { struct { real i,j,k; }; real n[3]; } real_vector3d;
typedef struct { real i,j; } real_vector2d;
typedef struct { real i,j,k,w; } real_quaternion;
typedef struct { real data[13]; } real_matrix4x3;
#include "state.inc"
struct mass_point_datum { int unused; };
struct vehicle_datum {
    struct { real_vector3d translational_velocity, angular_velocity, forward, up; } object;
    struct { real speed, turn; } vehicle;
    long definition_index;
};
struct vehicle_definition { struct { struct { struct { long index; } physics; } object; } unit; };
struct physics_definition { struct { long count; } powered_mass_points; real xx_moment; };
static struct vehicle_datum test_vehicle;
static struct vehicle_definition test_definition;
static struct physics_definition test_physics;
static real_vector3d up = {{0,0,1}};
static real_vector3d *global_up3d = &up;
static int loaded, calls;
static struct powered_mass_point_datum *received;
static int mcc_cache_tags_loaded(void) { return loaded; }
#define vehicle_datum_get(i) (&test_vehicle)
#define vehicle_specific_definition_get(i) (&test_definition)
#define physics_definition_get(i) (&test_physics)
#define CEILING(a,b) ((a)<(b)?(a):(b))
#define PIN(a,b,c) ((a)<(b)?(b):((a)>(c)?(c):(a)))
#define _pi 3.14159265358979323846
#define VEHICLE_ANGULAR_ACCELERATION 0.0139626344f
#define magnitude3d(v) 0.0f
#define normalize3d(v) 0.0f
#define sine(v) sinf(v)
#define cosine(v) cosf(v)
#define square_root(v) sqrtf(v)
#define vehicle_cross_product3d_target(a,b,c) ((void)0)
#define vehicle_triple_product3d_target(a,b,c) 0.0f
#define rotate_vector_about_axis(a,b,c,d) ((void)(c),(void)(d))
#define angle_between_vectors3d(a,b) 0.0f
#define dot_product3d(a,b) 0.0f
#define vehicle_dot_product3d_target(a,b) 0.0f
#define scale_vector3d(a,b,c) ((void)(b))
static void set_real_vector2d(real_vector2d *v, real a, real b) { v->i=a; v->j=b; }
static void physics_update(long index, struct powered_mass_point_datum *state,
    struct mass_point_datum *points, real_vector3d const *force, real_vector3d const *torque)
{ received=state; ++calls; }
#include "under_test.inc"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"failed line %d: %s\n",__LINE__,#c); return 1; } } while(0)
int main(int argc, char **argv)
{
    struct powered_mass_point_datum state[4], before[4];
    struct mass_point_datum points[1];
    int i;
    loaded = strcmp(argv[1],"legacy") != 0;
    test_physics.powered_mass_points.count = strcmp(argv[1],"other_count")==0 ? 2 : 3;
    test_vehicle.vehicle.speed=0.25f;
    /* Every untouched float is NaN, not conveniently zero stack storage. */
    memset(state,255,sizeof(state));
    memcpy(before,state,sizeof(state));
    update_human_boat_physics(0,points,state);
    CHECK(calls==1);
    CHECK(memcmp(state+3,before+3,sizeof(state[3]))==0);
    if (test_physics.powered_mass_points.count!=3) {
        CHECK(received==NULL && memcmp(state,before,sizeof(state))==0);
    } else {
        CHECK(received==state && state[0].water_friction_velocity==0.25f);
        CHECK(state[0].water_lift_ratio==0.003f && state[1].water_lift_ratio==0.003f);
        CHECK(state[2].water_lift_ratio==0.005f);
        for (i=0;i<3;++i) CHECK(state[i].rotation_values[3]==1.0f);
        if (loaded) {
            CHECK(state[1].water_friction_velocity==0.0f && state[2].water_friction_velocity==0.0f);
            for (i=0;i<3;++i) {
                CHECK(state[i].ground_friction_velocity==0.0f && state[i].air_friction_velocity==0.0f);
                CHECK(state[i].air_lift_ratio==0.0f && state[i].thrust_fraction==0.0f && state[i].antigrav_fraction==0.0f);
            }
        } else {
            CHECK(memcmp(&state[1].water_friction_velocity,&before[1].water_friction_velocity,sizeof(real))==0);
            CHECK(memcmp(&state[2].water_friction_velocity,&before[2].water_friction_velocity,sizeof(real))==0);
        }
    }
    return 0;
}
