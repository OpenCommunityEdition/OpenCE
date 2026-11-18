/*
SHADOW_MAP_REUSE.C (test)

The real batches and kept maps of the objects' shadows (render_objects.c:
render_object_shadows, render_object_shadow_port_reuse and what they call),
frame after frame over a crowd of objects. Without display.shadow_map_reuse,
every object's map is drawn each frame, as on the Xbox. With it, a small
shadow's map is drawn every other frame and a large one's each frame; a map
is drawn again when the light turns or the renderer freed the slots; the
kept maps stay in the slots their byte budget gives (halo_shadow_map_slots).
Every frame, each object's shadow is projected once, from a map of its own,
where the object is now, drawn this frame or the one before.
test_shadow_map_reuse.py takes the code (under_test.inc).
*/

#include "harness.h"

typedef unsigned short word;
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
typedef union real_rgb_color { struct { real red, green, blue; }; real n[3]; } real_rgb_color;
struct render_lighting;

#include "config.inc"

#define csmemset memset
#define profile_enter(section) ((void)0)
#define profile_exit(section) ((void)0)
static boolean render_shadows = TRUE;
static struct { long frame_index; } render;

static real dot_product3d(real_vector3d const *a, real_vector3d const *b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

/* ---------- the world */

#define OBJECTS 40
/* each object's size on the screen, in the 480-line screen's pixels */
static real object_pixels[OBJECTS];
static long object_count;
/* the light's direction, the shadows' matrices' up */
static real_vector3d light_up = { 0.0f, 0.0f, 1.0f };

/* the shadow's matrix where the object is this frame (an object's number
and the frame in the position, to know a projection's), as
render_object_shadow_begin works it out */
static void render_object_shadow_port_setup(struct object_render_data *data, real level_of_detail,
	union real_rgb_color *shadow_color_out, real *radius_out)
{
	memset(&data->shadow_matrix, 0, sizeof(data->shadow_matrix));
	data->shadow_matrix.up = light_up;
	data->shadow_matrix.forward.i = 1.0f;
	data->shadow_matrix.position.x = (real)data->object_index;
	data->shadow_matrix.position.y = (real)render.frame_index;
	shadow_color_out->red = shadow_color_out->green = shadow_color_out->blue = 0.5f;
	*radius_out = 1.0f;
}

/* ---------- the renderer */

static boolean reuse_setting;
/* the slots past the first a budget's bytes give: a slot of 64 KB (128x128),
or 2 MB (budget's case) */
static int slot_shift = 16;
static long slots_given(unsigned long bytes) { return (long)(bytes >> slot_shift); }
static unsigned long slots_freed;
static long halo_shadow_map_slots(unsigned long bytes) { return slots_given(bytes); }
static int halo_shadow_map_reuse(void) { return reuse_setting; }
static unsigned long halo_shadow_map_slots_freed(void) { return slots_freed; }
static long halo_shadow_map_scale(void) { return 1; }

static boolean convolution = TRUE;
static void rasterizer_environment_shadows_begin(void) {}
static void rasterizer_environment_shadows_end(void) {}
static void rasterizer_environment_shadow_port_batches_end(void) {}

#define MAXIMUM_SLOTS 512
/* each slot's map: the object drawn into it and the frame; the frame's maps
drawn, by object; the projections */
static long slot_object[MAXIMUM_SLOTS];
static long slot_frame[MAXIMUM_SLOTS];
static long highest_slot;
static long maps_drawn[OBJECTS];
static long projections[OBJECTS];
/* the matrix of the shadow begun (render_object_shadow_begin) */
static real_matrix4x3 begun_matrix;
static long begun_object;

struct rasterizer_shadow_port_saved;
static void rasterizer_environment_shadow_port_map_done(long slot, struct rasterizer_shadow_port_saved *saved);
static void rasterizer_environment_shadow_port_project_begin(struct rasterizer_shadow_port_saved const *saved);
static long rasterizer_environment_shadow_port_batch_size(void);
static void render_object_shadow_end(struct object_render_data *data);
static void process_rendered_objects(struct object_render_data *data);
/* the slot of the projection begun */
static long projecting_slot;

#include "under_test.inc"

static long rasterizer_environment_shadow_port_batch_size(void)
{
	return convolution ? RASTERIZER_SHADOW_PORT_BATCH : 1;
}

static void rasterizer_environment_shadow_port_map_done(long slot, struct rasterizer_shadow_port_saved *saved)
{
	CHECK(slot >= 0 && slot < MAXIMUM_SLOTS, "slot %ld", slot);
	saved->matrix = begun_matrix;
	saved->radius = 1.0f;
	saved->scale = 1;
	saved->slot = slot;
	slot_object[slot] = begun_object;
	slot_frame[slot] = render.frame_index;
	if (slot > highest_slot)
		highest_slot = slot;
	maps_drawn[begun_object]++;
}

static void rasterizer_environment_shadow_port_project_begin(struct rasterizer_shadow_port_saved const *saved)
{
	projecting_slot = saved->slot;
}

/* a projection: the object's own map, of this frame or the one before,
where the object is this frame */
static void render_object_shadow_end(struct object_render_data *data)
{
	long object = (long)data->shadow_matrix.position.x;

	CHECK(object >= 0 && object < object_count, "projected at %g", data->shadow_matrix.position.x);
	CHECK((long)data->shadow_matrix.position.y == render.frame_index,
		"object %ld projected where it was in frame %g, in frame %ld", object, data->shadow_matrix.position.y,
		render.frame_index);
	CHECK(slot_object[projecting_slot] == object, "object %ld projected from slot %ld, object %ld's map", object,
		projecting_slot, slot_object[projecting_slot]);
	CHECK(render.frame_index - slot_frame[projecting_slot] <= 1, "object %ld projected from a map %ld frames old",
		object, render.frame_index - slot_frame[projecting_slot]);
	projections[object]++;
}

/* render_object's shadow: projected from its kept map, or its map drawn */
static void process_rendered_objects(struct object_render_data *data)
{
	long object;

	for (object = 0; object < object_count; object++)
	{
		data->object_index = object;
		if (!render_object_shadow_port_reuse(data, object_pixels[object], 1.0f))
		{
			union real_rgb_color color;
			real radius;

			render_object_shadow_port_setup(data, 1.0f, &color, &radius);
			begun_matrix = data->shadow_matrix;
			begun_object = object;
			render_object_shadow_port_map_done(object);
		}
	}
}

/* ---------- the frames */

static long frame_maps;

static void frame(void)
{
	long object;

	render.frame_index++;
	memset(maps_drawn, 0, sizeof(maps_drawn));
	memset(projections, 0, sizeof(projections));
	render_object_shadows();
	frame_maps = 0;
	for (object = 0; object < object_count; object++)
	{
		CHECK(projections[object] == 1, "object %ld projected %ld times in frame %ld", object, projections[object],
			render.frame_index);
		CHECK(maps_drawn[object] <= 1, "object %ld's map drawn %ld times", object, maps_drawn[object]);
		frame_maps += maps_drawn[object];
	}
}

static void crowd(long count, real pixels)
{
	long object;

	object_count = count;
	for (object = 0; object < count; object++)
		object_pixels[object] = pixels;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long index;

	render.frame_index = 100;
	for (index = 0; index < MAXIMUM_SLOTS; index++)
		slot_object[index] = NONE;

	/* the setting off: every map drawn each frame */
	CASE("off")
	{
		crowd(OBJECTS, 40.0f);
		for (index = 0; index < 20; index++)
		{
			frame();
			CHECK(frame_maps == OBJECTS, "frame %ld drew %ld maps of %d without the setting", index, frame_maps,
				OBJECTS);
		}
		CHECK(highest_slot < RASTERIZER_SHADOW_PORT_BATCH, "slot %ld kept without the setting", highest_slot);
		return 0;
	}
	/* small shadows: each map every other frame, the objects taking turns */
	CASE("small-every-other-frame")
	{
		reuse_setting = TRUE;
		crowd(OBJECTS, 40.0f);
		frame();
		CHECK(frame_maps == OBJECTS, "the first frame drew %ld maps of %d", frame_maps, OBJECTS);
		for (index = 0; index < 20; index++)
		{
			frame();
			CHECK(frame_maps == OBJECTS / 2, "frame %ld drew %ld maps, not half of %d", index, frame_maps, OBJECTS);
		}
		return 0;
	}
	/* large shadows: each map each frame */
	CASE("large-every-frame")
	{
		reuse_setting = TRUE;
		crowd(OBJECTS, 200.0f);
		for (index = 0; index < 10; index++)
		{
			frame();
			CHECK(frame_maps == OBJECTS, "frame %ld drew %ld maps of %d large shadows", index, frame_maps, OBJECTS);
		}
		return 0;
	}
	/* the light turned: every map drawn again */
	CASE("light-turned")
	{
		reuse_setting = TRUE;
		crowd(OBJECTS, 40.0f);
		for (index = 0; index < 6; index++)
			frame();
		light_up.i = 0.6f;
		light_up.k = 0.8f;
		frame();
		CHECK(frame_maps == OBJECTS, "the light turned and %ld maps of %d were drawn", frame_maps, OBJECTS);
		return 0;
	}
	/* the renderer freed the slots: every map drawn again */
	CASE("slots-freed")
	{
		reuse_setting = TRUE;
		crowd(OBJECTS, 40.0f);
		for (index = 0; index < 6; index++)
			frame();
		slots_freed++;
		frame();
		CHECK(frame_maps == OBJECTS, "the slots were freed and %ld maps of %d were drawn", frame_maps, OBJECTS);
		return 0;
	}
	/* more objects than the budget's entries: the kept maps in the slots it
	gives, the others drawn each frame */
	CASE("budget")
	{
		long entries;

		slot_shift = 21;
		entries = slots_given(SHADOW_PORT_CACHE_BYTES);
		reuse_setting = TRUE;
		CHECK(entries < OBJECTS, "the budget gives %ld entries, for %d objects", entries, OBJECTS);
		crowd(OBJECTS, 40.0f);
		for (index = 0; index < 20; index++)
			frame();
		CHECK(highest_slot < RASTERIZER_SHADOW_PORT_BATCH + entries,
			"slot %ld used, past the budget's %ld entries after the batch's %d", highest_slot, entries,
			RASTERIZER_SHADOW_PORT_BATCH);
		CHECK(frame_maps == OBJECTS - entries / 2, "the last frame drew %ld maps, not %ld", frame_maps,
			OBJECTS - entries / 2);
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
