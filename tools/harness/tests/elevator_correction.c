/* Production reconciliation in a small world. Delays are 3/6 ticks at 30 Hz;
the host and client platforms may have different phases, including snaps.
The apply stub implements the production position tolerance, not collision. */
#include "harness.h"
#include <math.h>
typedef unsigned short word;
typedef struct { real i, j; } real_vector2d;
enum { _distributed_unit_predicted_bit = 0, OWN_POSITION_TICKS = 64,
    _object_mask_biped = 1, _object_mask_machine = 2, _biped_airborne_bit = 0 };
struct player_datum { short local_player_index; };
struct fake_object {
    long mask;
    struct { real_point3d position, bounding_sphere_center; real_vector3d translational_velocity; } object;
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
/* (the latency meter's timing of the host's echo: not under test) */
static void distributed_note_own_round_trip(long time) { (void)time; }
/* (the riding words of distributed_correct_own_unit: this fixture has no vehicle window, so none is taken) */
struct vehicle_riding_claim *vehicle_riding_player_claim(short player_index) { return NULL; }
static boolean distributed_own_episode_current(struct player_datum const *p, long u, struct distributed_unit_state const *s) { return FALSE; }
static boolean distributed_own_window_current(struct player_datum const *p, long u, struct distributed_unit_state const *s) { return FALSE; }
static boolean distributed_deck_whole(struct distributed_unit_state const *s) { return FALSE; }
static boolean distributed_correct_own_unechoed_rider(long u, struct distributed_unit_state const *s) { return FALSE; }
static void biped_riding_cabin_host(long u, struct vehicle_riding_deck const *d, boolean b) {}
static void biped_riding_air_verdict_host(long u, struct vehicle_riding_deck const *d, boolean b) {}
static boolean biped_riding_deck(long u, struct vehicle_riding_deck *d) { return FALSE; }
static boolean biped_riding_get_state(long u, struct vehicle_riding_state *s) { return FALSE; }
static short biped_riding_vehicle_node_count(long v) { return NONE; }
static void biped_riding_drop(long u) {}
static void biped_riding_rebase(long u) {}
static boolean biped_riding_take_deck(long u, struct vehicle_riding_deck const *d) { return FALSE; }
static boolean biped_riding_air_take(long u, struct vehicle_riding_deck const *d) { return FALSE; }
static boolean biped_riding_deck_position(long u, struct vehicle_riding_deck const *d, real_point3d *p) { return FALSE; }
static boolean biped_riding_frame_velocity(long u, struct vehicle_riding_deck const *d, real_point3d const *b, real_vector3d *v) { return FALSE; }
static boolean biped_riding_vehicle(long v, short n, struct vehicle_riding_vehicle *out) { return FALSE; }
static boolean biped_riding_air_correct(long u, real_vector3d const *d, real peak, short verdict) { return FALSE; }
static void biped_get_physics_pill(long u, real_point3d *base, real *h, real *w) {}
boolean vehicle_riding_deck_valid(struct vehicle_riding_deck const *d, short nodes) { return FALSE; }
boolean vehicle_riding_claim_grace(struct vehicle_riding_claim *c, long ov, short on, long hv, short hn, short g) { return FALSE; }
static long distributed_own_windows[1], distributed_own_air_episodes[1];
static long game_connection(void) { return 0; }
#define _game_connection_network_client 1
static boolean network_objects_reconcile(long i, real_point3d const *p, real_vector3d const *f, real_vector3d const *u,
    real_vector3d const *v, real_vector3d const *av, real blend, short kind) { return FALSE; }
#define RIDING_TICKS 60
static boolean distributed_apply_state(long i, struct distributed_unit_state const *s,
    real_point3d const *p, real_vector3d const *velocity, real tolerance, real b, real speed, real fall) {
    real dx = p->x - objects[i].object.position.x, dy = p->y - objects[i].object.position.y,
        dz = p->z - objects[i].object.position.z;
    if (dx*dx + dy*dy + dz*dz > tolerance*tolerance) objects[i].object.position = *p;
    return TRUE;
}
#include "under_test.inc"
int main(int argc, char **argv) {
    char const *case_name = argc > 1 ? argv[1] : "";
    struct player_datum p = {0};
    struct distributed_unit_state s;
    struct distributed_own_position *own = &distributed_own_positions[0][100 & 63];
    boolean protected = !strncmp(case_name,"100ms",5) || !strncmp(case_name,"200ms",5);
    long delay = !strncmp(case_name,"100ms",5) ? 3 : 6;
    memset(&s, 0, sizeof(s)); s.position.z = 10; s.flags = 1; s.predicted_time = 100;
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
    if (!strcmp(case_name,"far")) s.position.z = -10;
    if (!strcmp(case_name,"unpredicted")) {s.flags = 0; s.position.z = 0;}
    if (!strcmp(case_name,"no-history")) {own->time = NONE; s.position.z = 0;}
    distributed_correct_own_unit(&p,1,&s);
    if (protected) CHECK(fabsf(objects[1].object.position.z-10) < .0001f,
        "%ld ms echo placed the rider at %f under its floor", delay*1000/30, objects[1].object.position.z);
    else CHECK(objects[1].object.position.z < 7, "%s wrongly suppressed an ordinary correction", case_name);
    return 0;
}
