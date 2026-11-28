#include "harness.h"
#include <math.h>

#include "constants.inc"

struct contrail_definition { unsigned long flags; };
static struct { struct { real_point3d position; } camera; } render;

#define transition_function_evaluate(function, value) (value)
#define square_root(value) ((real)sqrt(value))

#include "vectors.inc"
#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *mode = argv[1];
	struct contrail_definition definition = { 0 };
	real_vector3d normal = { 0.0f, 0.0f, 1.0f };
	real_point3d point = { 3.0f, -2.0f, 1.5f };
	real fade;

	if (!strcmp(mode, "at-camera"))
	{
		/* (a player's own projectile: its first point at the eye) */
		render.camera.position = point;
		fade = contrail_fade(&definition, 1, &point, &normal);
		CHECK(fade == fade, "a number, not NaN");
		CHECK(fade == 1.0f, "unfaded: %g", (double)fade);
		fade = contrail_fade(&definition, 2, &point, &normal);
		CHECK(fade == fade && fade == 0.0f, "the second mode's opposite: %g", (double)fade);
		return 0;
	}
	if (!strcmp(mode, "away"))
	{
		/* (as before: by the angle between the normal and the camera) */
		render.camera.position.x = point.x;
		render.camera.position.y = point.y + 3.0f;
		render.camera.position.z = point.z + 4.0f;
		fade = contrail_fade(&definition, 1, &point, &normal);
		CHECK(fabs(fade - 0.8f) < 0.0001f, "the cosine, 0.8: %g", (double)fade);
		fade = contrail_fade(&definition, 0, &point, &normal);
		CHECK(fade == 1.0f, "no fade mode, no fade: %g", (double)fade);
		return 0;
	}
	fprintf(stderr, "no case %s\n", mode);
	return 2;
}
