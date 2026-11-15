/*
CAMERA_HANDOVER.C (test)

The real render_interpolation_blended_camera (render_interpolation.c): a
player's camera is drawn between the observer's snapshots, taken at the
first frame drawn after each tick. b40's opening gives the player the view
back one frame into a tick, after that tick's snapshot was taken: the view
turned to the player's aim at once while its position stayed where the
cinematic's camera was for the rest of the tick. A change of the director's
perspective after the snapshot is a cut: the camera is drawn as the
observer has it, and blended from there from the next tick.
test_camera_handover.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"
#include <math.h>

#define MAXIMUM_LOCAL_PLAYERS 4
#define TICKS_PER_SECOND 30
#define REAL_MAX 3.4e38f

struct observer_result { real_point3d position; real_vector3d velocity; real_vector3d forward; real_vector3d up; real field_of_view; };
typedef struct real_matrix4x3 { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;

enum { _director_perspective_first_person, _director_perspective_scripted = 3 };

static real_vector3d const zero_vector = { 0.f, 0.f, 0.f };
static real_vector3d const *global_zero_vector3d = &zero_vector;
static boolean interpolation_rendering = TRUE;
static long interpolation_tick;
static long interpolation_cut_tick = NONE;
static real interpolation_fraction;
static short perspective = _director_perspective_scripted;

static short director_get_perspective(short local_player_index) { (void)local_player_index; return perspective; }
static long scripted_camera_object_relative_to(void) { return NONE; }
static boolean scripted_camera_object_is_first_person_camera(long object_index) { (void)object_index; return FALSE; }
static boolean scripted_camera_moving_to_point(void) { return FALSE; }
static void *object_try_and_get_and_verify_type(long object_index, long mask) { (void)object_index; (void)mask; return NULL; }
static real_matrix4x3 const *render_interpolation_object_node_matrices(long object_index) { (void)object_index; return NULL; }
struct object_datum { struct { struct { long size; } node_matrices; } object; };
static struct object_datum *object_get(long object_index) { (void)object_index; return NULL; }
static void *object_header_block_get(long object_index, void const *block) { (void)object_index; (void)block; return NULL; }
#define _object_mask_all 0

#include "under_test.inc"

static struct observer_result shot(real x, real yaw)
{
	struct observer_result result;

	memset(&result, 0, sizeof(result));
	result.position.x = x;
	result.forward.i = (real)cos(yaw);
	result.forward.j = (real)sin(yaw);
	result.up.k = 1.f;
	result.field_of_view = 1.2f;
	return result;
}

/* a frame drawn a fraction into a tick, the observer as given */
static struct observer_result const *frame(long tick, real fraction, struct observer_result const *observer)
{
	interpolation_tick = tick;
	interpolation_fraction = fraction;
	return render_interpolation_blended_camera(0, observer);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct observer_result cinematic = shot(0.f, 0.f);
	struct observer_result cinematic_on = shot(0.1f, 0.f);
	struct observer_result player = shot(2.f, 0.3f);
	struct observer_result player_on = shot(2.1f, 0.3f);
	struct observer_result const *drawn;

	frame(10, 0.1f, &cinematic);
	frame(11, 0.1f, &cinematic_on);
	/* the cinematic's camera moving: blended between its snapshots */
	CASE("blended")
	{
		drawn = frame(11, 0.5f, &cinematic_on);
		CHECK(drawn->position.x > 0.f && drawn->position.x < 0.1f, "not blended: %f", drawn->position.x);
		return 0;
	}
	/* the view given back to the player a frame into tick 12, after its
	snapshot, 2 world units and 17 degrees from the cinematic's camera (short
	of a cut by the move alone): drawn where the player's camera is, not
	where the cinematic's was */
	CASE("handover")
	{
		frame(12, 0.1f, &cinematic_on);
		perspective = _director_perspective_first_person;
		drawn = frame(12, 0.5f, &player);
		CHECK(fabs(drawn->position.x - 2.f) < 1e-4f, "drawn at %f, not the player's 2", drawn->position.x);
		drawn = frame(12, 0.9f, &player);
		CHECK(fabs(drawn->position.x - 2.f) < 1e-4f, "later in the tick at %f", drawn->position.x);
		/* and blended from there from the next tick */
		drawn = frame(13, 0.5f, &player_on);
		CHECK(drawn->position.x > 2.f && drawn->position.x < 2.1f, "the next tick not blended: %f",
			drawn->position.x);
		return 0;
	}

	return 2;
}
