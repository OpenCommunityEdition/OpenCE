#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef float real;
typedef int boolean;
typedef unsigned char byte;
typedef unsigned short word;
typedef union { struct { real i,j,k; }; struct { real x,y,z; }; real n[3]; } real_vector3d;
typedef real_vector3d real_point3d;
typedef struct { real i,j; } real_vector2d;
typedef struct { real i,j,k,w; } real_quaternion;
typedef struct { real data[13]; } real_matrix4x3;
typedef struct { real_vector3d n; real d; } real_plane3d;
struct tag_block { long count; void *address; void *definition; };
#include "types.inc"

struct object_state {
    real_point3d position;
    real_vector3d translational_velocity, angular_velocity, forward, up;
    struct location location;
    unsigned long flags;
};
struct object_datum { struct object_state object; long definition_index; };
struct object_definition { struct { struct { long index; } physics; } object; };
struct vehicle_datum {
    struct object_state object;
    struct { real speed, turn; real_vector3d collision_force, collision_torque; } vehicle;
    long definition_index;
};
struct vehicle_definition { struct { struct { struct { long index; } physics; } object; } unit; };
struct material_definition {
    real physics_ground_friction_scale, physics_ground_friction_normal_k1_scale;
    real physics_ground_friction_normal_k0_scale, physics_ground_depth_scale;
    real physics_ground_damp_fraction_scale;
};
struct collision_result { real t; real_plane3d plane; };
static struct vehicle_datum test_vehicle;
static struct object_datum test_object;
static struct object_definition test_object_definition;
static struct vehicle_definition test_definition;
static struct physics_definition test_physics;
static struct material_definition test_material;
static struct powered_mass_point_definition powered_definitions[MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS];
static struct mass_point_definition mass_definitions[3];
static real_vector3d up = {{0,0,1}}, down = {{0,0,-1}};
static real_vector3d *global_up3d = &up, *global_down3d = &down;
static real global_gravity = 0.0035651792f;
static real_vector3d total_force, total_torque;
static int loaded, calls, warned_about_physics_mass_points;
static struct powered_mass_point_datum *received;
static int mcc_cache_tags_loaded(void) { return loaded; }

#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define FLAG(b) (1UL << (b))
#define TEST_FLAG(f,b) (((f) & FLAG(b)) != 0)
#define SET_FLAG(f,b,v) ((v) ? ((f) |= FLAG(b)) : ((f) &= ~FLAG(b)))
#define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)(b)->address + (i))
#define vehicle_datum_get(i) (&test_vehicle)
#define object_get(i) (&test_object)
#define object_definition_get(i) (&test_object_definition)
#define vehicle_specific_definition_get(i) (&test_definition)
#define physics_definition_get(i) (&test_physics)
#define scenario_material_definition_get(i) (&test_material)
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CEILING(a,b) ((a)<(b)?(a):(b))
#define PIN(a,b,c) ((a)<(b)?(b):((a)>(c)?(c):(a)))
#define _pi 3.14159265358979323846
#define VEHICLE_ANGULAR_ACCELERATION 0.0139626344f
#define magnitude3d(v) 0.0f
#define normalize3d(v) ((void)(v), 0.0f)
#define sine(v) sinf(v)
#define cosine(v) cosf(v)
#define square_root(v) sqrtf(v)
#define vehicle_cross_product3d_target(a,b,c) ((void)0)
#define vehicle_triple_product3d_target(a,b,c) 0.0f
#define rotate_vector_about_axis(a,b,c,d) ((void)(c),(void)(d))
#define angle_between_vectors3d(a,b) 0.0f
#define vehicle_dot_product3d_target(a,b) 0.0f
#define error(...) ((void)0)
#define match_vassert(a,b,c,d) ((void)0)
#define match_assert_valid_real_vector3d(a,b,v) ((void)0)
#define match_assert_valid_real_vector3d_axes2(a,b,c,d) ((void)0)
#include "vectors.inc"

/* Boat orientation, world queries and transforms are fixed for an axis-aligned
 * fixture. Powered flags, friction/lift and force sums are extracted from
 * physics.c. This is not a complete vehicle simulation. */
#define matrix4x3_transform_point(m,p,out) (*(out) = *(p))
#define matrix4x3_transform_normal(m,v,out) (*(out) = *(v))
#define matrix4x3_multiply(a,b,out) memset(out, 0, sizeof(*(out)))
#define matrix4x3_from_point_and_vectors(m,p,f,u) memset(m, 0, sizeof(*(m)))
#define matrix4x3_rotation_from_quaternion(m,q) memset(m, 0, sizeof(*(m)))
#define matrix4x3_transpose(m) ((void)0)
#define scenario_location_from_point(l,p) ((void)0)
#define scenario_location_from_line(l,o,p,n) ((void)0)
#define object_translate(i,p,l) ((void)0)
#define physics_compute_unit_collisions(i) ((void)0)
#define scenario_location_water_depth(l,p) 1.0f
#define _collision_test_for_bipeds_dead_flags 0
#define _material_ice 1
enum { _object_at_rest_bit, _object_on_ground_bit, _object_on_media_bit,
       _object_partially_under_media_bit, _object_wholly_under_media_bit };
static void compute_ground_plane(long index, struct mass_point_datum *point,
                                 struct mass_point_definition const *definition)
{
    point->ground_depth = 1.0f;
    point->ground_plane.n = up;
}
static int collision_test_vector(int flags, real_point3d const *point,
                                real_vector3d const *vector, long index,
                                struct collision_result *collision)
{
    collision->t = 0.5f;
    collision->plane.n = up;
    return TRUE;
}
static void component_vectors_from_normal3d(real_vector3d const *vector,
                                           real_vector3d const *normal,
                                           real_vector3d *parallel,
                                           real_vector3d *perpendicular)
{
    scale_vector3d(normal, dot_product3d(vector, normal), parallel);
    set_real_vector3d(perpendicular, vector->i-parallel->i,
                     vector->j-parallel->j, vector->k-parallel->k);
}
static int physics_instance_new(struct physics_instance *instance, long index)
{
    memset(instance, 0, sizeof(*instance));
    instance->physics = &test_physics;
    return TRUE;
}
static void physics_update_new(struct physics_instance const *instance,
                               struct powered_mass_point_datum const *state,
                               struct mass_point_datum const *points,
                               real_vector3d const *force, real_vector3d const *torque)
{
    total_force = *force;
    total_torque = *torque;
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#include "consumer.inc"
#pragma clang diagnostic pop

static void physics_update(long index, struct powered_mass_point_datum *state,
                           struct mass_point_datum *points,
                           real_vector3d const *force, real_vector3d const *torque)
{
    received = state;
    ++calls;
    production_physics_update(index, state, points, force, torque);
}
#include "under_test.inc"

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"failed line %d: %s\n",__LINE__,#c); return 1; } } while(0)
#define FORCE_CHECK(c) do { if (!(c)) { fprintf(stderr,"incorrect force at line %d: %s\n",__LINE__,#c); return 2; } } while(0)
#define NEAR(a,b) (fabs((a)-(b)) < 0.000001f)

int main(int argc, char **argv)
{
    struct powered_mass_point_datum state[MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS+1];
    struct powered_mass_point_datum before[MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS+1];
    struct mass_point_datum points[3];
    unsigned long flags;
    int i, j;
    char const *case_name = argv[1];
    char const *count_name = strstr(case_name, "count");
    loaded = strncmp(case_name, "mcc_", 4) == 0;
    test_physics.powered_mass_points.count = count_name ? atoi(count_name+5) : 3;
    test_physics.powered_mass_points.address = powered_definitions;
    test_physics.mass_points.count = 3;
    test_physics.mass_points.address = mass_definitions;
    test_physics.mass = 12.0f;
    test_physics.radius = strstr(case_name, "_old") ? 1.0f : 0.0f;
    test_physics.ground_friction = 0.125f;
    test_physics.air_friction = 0.25f;
    test_physics.water_friction = 0.5f;
    test_physics.water_depth = 1.0f;
    test_physics.ground_depth = 1.0f;
    test_physics.ground_normal_k1 = 1.0f;
    test_vehicle.vehicle.speed = strstr(case_name, "reverse") ? -0.25f :
                                 strstr(case_name, "zero") ? 0.0f : 0.25f;
    test_object.object.translational_velocity.i = 1.0f;
    flags = strstr(case_name, "unpowered") ? 0 : strstr(case_name, "all") ?
            (FLAG(NUMBER_OF_POWERED_MASS_POINT_FLAGS)-1) :
            FLAG(_powered_mass_point_water_friction_bit) | FLAG(_powered_mass_point_water_lift_bit);
    for (i=0;i<3;++i) {
        powered_definitions[i].flags = flags;
        powered_definitions[i].antigrav_height = 1.0f;
        powered_definitions[i].antigrav_normal_k1 = 1.0f;
        powered_definitions[i].antigrav_strength = 1.0f;
        mass_definitions[i].powered_mass_point_index = i < test_physics.powered_mass_points.count ? (short)i : NONE;
        mass_definitions[i].mass = 4.0f;
        mass_definitions[i].forward.i = 1.0f;
        mass_definitions[i].up.k = 1.0f;
        mass_definitions[i].friction_type = _friction_type_point;
    }
    /* Both non-finite and finite stale controls must be irrelevant. */
    memset(state, 255, sizeof(state));
    if (strstr(case_name, "finite")) {
        for (i=0;i<MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS+1;++i) {
            state[i].ground_friction_velocity = 13.0f;
            state[i].water_friction_velocity = -7.0f;
            state[i].air_friction_velocity = 4.0f;
            state[i].water_lift_ratio = 9.0f;
            state[i].air_lift_ratio = 6.0f;
            state[i].thrust_fraction = 3.0f;
            state[i].antigrav_fraction = 5.0f;
        }
    }
    memcpy(before, state, sizeof(state));
    update_human_boat_physics(0, points, state);
    CHECK(calls == 1);
    CHECK(memcmp(state+3, before+3, sizeof(state)-3*sizeof(*state)) == 0);
    if (test_physics.powered_mass_points.count != 3) {
        CHECK(received == NULL && memcmp(state, before, sizeof(state)) == 0);
        return 0;
    }
    CHECK(received == state);
    /* These checks precede expected force failures: a regression in the active
     * channels or scratch bounds must not be hidden by a known legacy xfail. */
    CHECK(state[0].water_friction_velocity == test_vehicle.vehicle.speed);
    for (i=0;i<3;++i) {
        CHECK(state[i].water_lift_ratio == (i==2 ? 0.005f : 0.003f));
        for (j=0;j<4;++j) CHECK(state[i].rotation_values[j] == (j==3 ? 1.0f : 0.0f));
    }
    for (i=0;i<3;++i) {
        if (!isfinite(points[i].force.i) || !isfinite(points[i].force.j) || !isfinite(points[i].force.k)) {
            fprintf(stderr, "point %d must have finite force: %g %g %g\n", i,
                    points[i].force.i, points[i].force.j, points[i].force.k);
            return 2;
        }
        FORCE_CHECK(NEAR(points[i].water_friction.friction.i,
                   -2.0f * (1.0f - (flags && i==0 ? test_vehicle.vehicle.speed : 0.0f))));
        FORCE_CHECK(NEAR(points[i].ground_friction.friction.i, -0.5f));
        FORCE_CHECK(NEAR(points[i].air_friction.friction.i, -1.0f));
        FORCE_CHECK(NEAR(points[i].powered_force.k, flags ? (i==2 ? 0.060f : 0.036f) : 0.0f));
        FORCE_CHECK(NEAR(points[i].powered_force.i, 0.0f));
    }
    if (test_physics.radius > 0.0f) {
        FORCE_CHECK(NEAR(test_object.object.translational_velocity.i,
                   1.0f + (-10.5f + (flags ? 2.0f*test_vehicle.vehicle.speed : 0.0f))/12.0f));
    } else {
        FORCE_CHECK(isfinite(total_force.i) && isfinite(total_force.j) && isfinite(total_force.k));
        FORCE_CHECK(NEAR(total_force.i, -10.5f + (flags ? 2.0f*test_vehicle.vehicle.speed : 0.0f)));
        FORCE_CHECK(total_torque.i == 0.0f && total_torque.j == 0.0f && total_torque.k == 0.0f);
    }
    for (i=0;i<3;++i) {
        if (loaded && !strstr(case_name, "consumer_only")) {
            CHECK(i==0 || state[i].water_friction_velocity == 0.0f);
            CHECK(state[i].ground_friction_velocity == 0.0f && state[i].air_friction_velocity == 0.0f);
            CHECK(state[i].air_lift_ratio == 0.0f && state[i].thrust_fraction == 0.0f);
            CHECK(state[i].antigrav_fraction == 0.0f);
        }
    }
    return 0;
}
