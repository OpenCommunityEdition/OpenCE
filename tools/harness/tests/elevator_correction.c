/* Production reconciliation in a small world. Delays are 3/6 ticks at 30 Hz;
the host and client platforms may have different phases, including snaps.
The apply stub implements the production position tolerance, not collision. */
#include "harness.h"
#include <math.h>
typedef unsigned short word;
enum { _distributed_unit_predicted_bit = 0, MAXIMUM_LOCAL_PLAYERS = 1, OWN_POSITION_TICKS = 64,
    _object_mask_biped = 1, _object_mask_machine = 2, _biped_airborne_bit = 0 };
struct player_datum { short local_player_index; };
struct distributed_unit_state { real_point3d position; unsigned flags; word predicted_time; };
struct fake_object {
    long mask;
    struct { real_point3d position; } object;
    struct { unsigned flags; long support_surface_index, elevator_object_index; char elevator_ticks; } biped;
};
#define biped_datum fake_object
#define device_datum fake_object
#define object_datum fake_object
static struct fake_object objects[4];
static struct fake_object *object_get(long i) { return &objects[i]; }
static void *object_try_and_get_and_verify_type(long i, long mask) {
    return i > 0 && i < 4 && (objects[i].mask & mask) ? &objects[i] : NULL;
}
static long now;
static long game_time_get(void) { return now; }
static boolean network_coop_active(void) { return TRUE; }
static long scenario_leaf_index_from_point(real_point3d const *p) { return 0; }
#include "config.inc"
static struct distributed_own_position distributed_own_positions[1][64];
static real distributed_own_round_trip;
static boolean distributed_apply_state(long i, struct distributed_unit_state const *s,
    real_point3d const *p, real tolerance, real b, real speed, real fall) {
    real dx = p->x - objects[i].object.position.x, dy = p->y - objects[i].object.position.y,
        dz = p->z - objects[i].object.position.z;
    if (dx*dx + dy*dy + dz*dz > tolerance*tolerance) objects[i].object.position = *p;
    return TRUE;
}
#include "under_test.inc"
int main(int argc, char **argv) {
    char const *case_name = argc > 1 ? argv[1] : "";
    struct player_datum p = {0};
    struct distributed_unit_state s = {{0,0,10},1,100};
    struct distributed_own_position *own = &distributed_own_positions[0][100 & 63];
    boolean protected = !strncmp(case_name,"100ms",5) || !strncmp(case_name,"200ms",5);
    long delay = !strncmp(case_name,"100ms",5) ? 3 : 6;
    objects[1].mask = _object_mask_biped; objects[2].mask = objects[3].mask = _object_mask_machine;
    objects[1].biped.support_surface_index = 1; objects[1].biped.elevator_ticks = 60;
    objects[1].biped.elevator_object_index = 2; objects[1].object.position.z = 10;
    own->time = 100; own->unit_index = 1; own->position.z = 13.2f; own->elevator_index = 2;
    now = 100 + delay;
    if (strstr(case_name,"-up")) {own->position.z=10; s.position.z=13.2f;}
    if (!strcmp(case_name,"jump")) objects[1].biped.flags = FLAG(_biped_airborne_bit);
    if (!strcmp(case_name,"walk-off")) objects[1].biped.elevator_ticks = 59;
    if (!strcmp(case_name,"other-elevator")) objects[1].biped.elevator_object_index = 3;
    if (!strcmp(case_name,"on-foot")) own->elevator_index = NONE;
    if (!strcmp(case_name,"teleport")) s.position.x = 10;
    if (!strcmp(case_name,"unpredicted")) {s.flags = 0; s.position.z = 0;}
    if (!strcmp(case_name,"no-history")) {own->time = NONE; s.position.z = 0;}
    distributed_correct_own_unit(&p,1,&s);
    if (protected) CHECK(fabsf(objects[1].object.position.z-10) < .0001f,
        "%ld ms echo placed the rider at %f under its floor", delay*1000/30, objects[1].object.position.z);
    else CHECK(objects[1].object.position.z < 7, "%s wrongly suppressed an ordinary correction", case_name);
    return 0;
}
