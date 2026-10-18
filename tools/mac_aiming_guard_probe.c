#include "engine_xdk_prefix.h"
#include "models/model_animation_definitions.h"
#include <stdio.h>
#include <string.h>

extern unsigned long mac_engine_service_errors;

static int checks, failures;

#define CHECK(expression) do { ++checks; if (!(expression)) { ++failures; \
	fprintf(stderr, "aiming guard line %d failed: %s\n", __LINE__, #expression); } } while (0)

static void initialize_animation(struct animation *animation, byte *frame)
{
	memset(animation, 0, sizeof(*animation));
	animation->type = _animation_overlay;
	animation->frame_count = 1;
	animation->frame_size = 1;
	animation->data.size = 1;
	animation->data.address = frame;
}

static void initialize_bounds(struct animation_aiming_screen_bounds *bounds)
{
	memset(bounds, 0, sizeof(*bounds));
	bounds->negative_yaw_delta = bounds->positive_yaw_delta = 1.0f;
	bounds->negative_pitch_delta = bounds->positive_pitch_delta = 1.0f;
}

int main(void)
{
	struct animation animation;
	struct animation_aiming_screen_bounds bounds;
	byte frame = 0;
	unsigned long errors;
	union { unsigned long bits; real value; } special;

	initialize_animation(&animation, &frame);
	initialize_bounds(&bounds);
	errors = mac_engine_service_errors;
	aiming_screen_apply(&animation, &bounds, 0.0f, 0.0f, NULL);
	CHECK(mac_engine_service_errors == errors);

	/* Two source short counts whose sum plus one overflows the old short grid. */
	bounds.positive_yaw_frame_count = 32767;
	aiming_screen_apply(&animation, &bounds, 0.0f, 0.0f, NULL);
	CHECK(mac_engine_service_errors == errors);

	/* A representable 2x2 grid still must fit the actual frame table. */
	initialize_bounds(&bounds);
	bounds.positive_yaw_frame_count = 1;
	bounds.positive_pitch_frame_count = 1;
	animation.frame_count = 3;
	aiming_screen_apply(&animation, &bounds, 0.0f, 0.0f, NULL);
	CHECK(mac_engine_service_errors == errors);

	/* Negative source counts are refused before they form frame indices. */
	initialize_bounds(&bounds);
	animation.frame_count = 1;
	bounds.negative_pitch_frame_count = -1;
	aiming_screen_apply(&animation, &bounds, 0.0f, 0.0f, NULL);
	CHECK(mac_engine_service_errors == errors);

	/* A valid single-cell grid clamps huge finite angles safely. */
	initialize_bounds(&bounds);
	special.bits = 0x7F7FFFFFUL;
	aiming_screen_apply(&animation, &bounds, special.value, -special.value, NULL);
	CHECK(mac_engine_service_errors == errors);

	/* NaN input must stop before float-to-short conversion. */
	special.bits = 0x7FC00000UL;
	aiming_screen_apply(&animation, &bounds, special.value, 0.0f, NULL);
	CHECK(mac_engine_service_errors == errors);

	printf("native aiming grid guard: %d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
