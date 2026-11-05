#include "harness.h"
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#include <string.h>

enum { _object_type_projectile = 5 };
enum { _point_light_dynamic_bit = 0 };
#define SET_FLAG(value, bit, enabled) ((value) = (enabled) ? ((value) | (1 << (bit))) : ((value) & ~(1 << (bit))))
struct object_datum { long definition_index; struct { short type; } object; };
static struct object_datum projectile = { 1, { _object_type_projectile } };
static const char *tag_name = "weapons\\plasma rifle\\bolt";
static int muzzle_on, emitter_on, shield_on;
static int enabled = 1, refreshes, present = 1, interpolation_rendering = 1;
static int rendered_pose;
static float connected_radius;
int lighting_config_boolean(const char *name) { return strcmp(name, "display.muzzle_lighting") ? enabled : muzzle_on; }
static struct object_datum *object_try_and_get(long index) { return present ? &projectile : NULL; }
static const char *tag_get_name(long index) { return tag_name; }
struct light_datum { long object_index, parent_light_index; int flags; float radius; long definition_index; real_point3d position; real_vector3d forward; };
static struct { int valid; } storage = { 1 }, *light_data = &storage;
static int stale;
static struct light_datum *datum_try_and_get(void *data, long index)
{
	static struct light_datum light;
	light.object_index = index + stale;
	light.parent_light_index = NONE;
	return &light;
}
enum { _light_definition_no_specular_bit = 1 };
#define _pi 3.14159265f
struct point_light_definition { real radius_modifier_upper_bound, radius, specular_radius_multiplier, lens_flare_radius, cutoff_angle, runtime_cosine_cutoff_angle, runtime_sine_cutoff_angle; int flags; };
static struct point_light_definition light_definition = {1,1,2,0,2,1,1,0};
static struct point_light_definition *light_definition_get(long i) { return &light_definition; }
static struct light_datum *light_get(long i) { return datum_try_and_get(light_data,i); }
float rocket_lighting_radius(long l,long o,float r) { return r; }
static void light_compute_bounding_sphere(long i,boolean m,boolean s,boolean f,real_point3d *p,real *r);
static void light_disconnect_from_map(long index) {}
static void light_reconnect_to_map(long index)
{
	refreshes++;
	rendered_pose = interpolation_rendering;
	{ real_point3d p; light_compute_bounding_sphere(index, TRUE, FALSE, TRUE, &p, &connected_radius); }
}
void plasma_occlusion_model_slot(long i, short s) {}
static int visibility_resets;
void plasma_occlusion_reset(void) { visibility_resets++; }
void muzzle_lighting_frame_end(void) {}
void muzzle_lighting_window_begin(void) {}
void muzzle_lighting_submitted(long l,long r) {}
int muzzle_lighting_selected(long i) { return muzzle_on && i == 20; }
float muzzle_lighting_radius(long l,float r) { return r; }
void rocket_lighting_frame_end(void) {}
void rocket_lighting_window_begin(void) {}
void rocket_lighting_submitted(long i,long r) {}
int rocket_lighting_selected(long i) { return 0; }
void plasma_occlusion_window(void) {}
void plasma_occlusion_submit(long i,long r) {}
void plasma_lighting_reset(void);
int plasma_lighting_selected(long index);
void plasma_falloff_dispose(void) {}
void plasma_falloff_end(void) {}
int covenant_emitter_lighting_enabled(void) { return emitter_on; }
void covenant_emitter_lighting_frame_end(void) {}
void covenant_emitter_lighting_window_begin(void) {}
void covenant_emitter_lighting_submitted(long i,long r) {}
int covenant_emitter_lighting_selected(long i) { return emitter_on && i == 22; }
float covenant_emitter_lighting_radius(long l,float r) { return r; }
void covenant_lighting_frame_end(void) {}
void covenant_lighting_window_begin(void) {}
void covenant_lighting_submitted(long i,long r) {}
int covenant_lighting_selected(long i) { return 0; }
int covenant_lighting_owns(long l,long o) { return l == 21; }
float covenant_lighting_radius(long l,long o,float r) { return r; }
int shield_glow_enabled(void) { return shield_on; }
int shield_glow_selected(long i) { return shield_on && i == 23; }
void shield_glow_frame_end(void) {}
void shield_glow_window_begin(void) {}
void shield_glow_submitted(long l,long r) {}
float shield_glow_radius(long l,float r) { return r; }
int authored_emitter_lighting_enabled(void) { return 0; }
int authored_emitter_lighting_profile(long o,long d,float *s) { return 0; }
int authored_emitter_lighting_category_enabled(int c) { return enabled; }
#include "under_test.inc"

int main(int argc, char **argv)
{
	int i;
	const char *test = argv[1];

	if (!strcmp(test, "shield"))
	{
		enabled = muzzle_on = emitter_on = 0; shield_on = 1;
		CHECK(plasma_lighting_selected(23), "shield-only selection");
		plasma_model_lighting_slot(23, 0);
		CHECK(plasma_model_lighting_mask() == 1, "shield-only model lighting");
		plasma_lighting_frame_end();
		CHECK(visibility_resets == 0, "shield-only cache retained");
		shield_on = 0;
		CHECK(!plasma_lighting_selected(23), "disabled shield not selected");
	}
	else if (!strcmp(test, "counter_cache"))
	{
		enabled = muzzle_on = 0;
		emitter_on = 1;
		plasma_lighting_frame_end();
		CHECK(visibility_resets == 0, "emitter-only frame retains directional textures");
		emitter_on = 0;
		plasma_lighting_frame_end();
		CHECK(visibility_resets == 1, "disabled lighting releases visibility cache");
	}
	else if (!strcmp(test, "covenant"))
 {
  const char *names[] = {"vehicles\\ghost\\ghost bolt", "vehicles\\banshee\\banshee bolt", "vehicles\\banshee\\banshee fuel rod", "vehicles\\c gun turret\\c gun turret", "weapons\\fuel rod gun\\fuel rod", "weapons\\needler\\needle", "vehicles\\wraith\\wraith mortar", "weapons\\plasma_cannon\\plasma_cannon"};
  float scales[] = {1.75f,1.75f,2.75f,2.5f,2.75f,1.5f,1.75f,2.75f};
  for(i=0;i<8;i++) {
   tag_name=names[i];plasma_lighting_prepare(1,1, NONE);
   CHECK(plasma_lighting_radius(1,1,1)==scales[i],"Covenant authored radius");
   plasma_lighting_frame_end();
   CHECK(plasma_lighting_radius(1,1,1)==1,"Covenant restore");
  }
  plasma_lighting_prepare(21,21, NONE);CHECK(plasma_lighting_count==0,"owned glow bypasses authored restore");
 }
 else if (!strcmp(test, "radius"))
	{
		const char *names[] = {"weapons\\plasma rifle\\bolt", "weapons\\plasma pistol\\bolt",
			"weapons\\plasma rifle\\charged bolt", "weapons\\rocket launcher\\rocket"};
		float scales[] = {1.75f, 2.25f, 2.75f, 1.75f};
		real_point3d position;
		real radius;
		for (i = 0; i < 4; i++)
		{
			tag_name = names[i]; enabled = 1;
			datum_try_and_get(light_data, 1)->radius = 0.5f;
			plasma_lighting_prepare(1, 1, NONE);
			CHECK(plasma_lighting_radius(1, 1, 1.0f) == scales[i], "per-projectile reach");
			CHECK(connected_radius == 2.0f * scales[i], "partition sees tuned radius before submission");
			datum_try_and_get(light_data, 1)->radius = scales[i];
			light_compute_bounding_sphere(1, FALSE, FALSE, FALSE, &position, &radius);
			CHECK(radius == scales[i], "actual bounds do not scale twice");
			light_compute_bounding_sphere(1, FALSE, TRUE, FALSE, &position, &radius);
			CHECK(radius == 2.0f * scales[i], "specular keeps authored multiplier");
			plasma_lighting_prepare(1, 1, NONE);
			CHECK(plasma_lighting_radius(1, 1, 1.0f) == scales[i], "split views do not compound radius");
			CHECK(plasma_lighting_radius(1, 2, 1.0f) == 1.0f, "wrong owner untouched");
			datum_try_and_get(light_data, 1)->radius = 9.0f;
			enabled = 0;
			CHECK(plasma_lighting_radius(1, 1, 1.0f) == 1.0f, "disabled policy is stock");
			plasma_lighting_frame_end();
			CHECK(datum_try_and_get(light_data, 1)->radius == 0.5f, "radius restored even when disabled");
			CHECK(connected_radius == 2.0f, "partition restored to stock");
		}
	}
	else if (!strcmp(test, "rocket"))
	{
		tag_name = "weapons\\rocket launcher\\rocket";
		plasma_lighting_prepare(1, 1, NONE);
		plasma_lighting_submitted(1, 3);
		CHECK(plasma_lighting_selected(3), "rocket flight attachment selected");
		render_interpolation_frame_end();
		CHECK(!plasma_lighting_selected(3), "rocket selection expires with frame");
		CHECK(!datum_try_and_get(light_data, 1)->flags, "rocket dynamic state restored");
		tag_name = "weapons\\rocket launcher\\explosion";
		plasma_lighting_prepare(1, 1, NONE);
		tag_name = "weapons\\rocket launcher\\effects\\rocket explosion";
		plasma_lighting_prepare(1, 1, NONE);
		CHECK(refreshes == 2, "explosion attachments stay stock");
	}
	else if (!strcmp(test, "pistol"))
	{
		const char *names[] = { "weapons\\plasma pistol\\bolt", "weapons\\plasma rifle\\charged bolt" };
		for (i = 0; i < 2; i++)
		{
			tag_name = names[i];
			interpolation_rendering = 1;
			datum_try_and_get(light_data, 1)->flags = i ? 3 : 0;
			plasma_lighting_prepare(1, 1, NONE);
			plasma_lighting_submitted(1, 3);
			CHECK(plasma_lighting_selected(3), "pistol projectile attachment selected");
			render_interpolation_frame_end();
			CHECK(!plasma_lighting_selected(3), "pistol light expires with render frame");
			CHECK(datum_try_and_get(light_data, 1)->flags == (i ? 3 : 0), "pistol authored flags restored");
		}
		tag_name = "weapons\\plasma pistol\\muzzle flash";
		plasma_lighting_prepare(1, 1, NONE);
		CHECK(refreshes == 4, "muzzle effects excluded");
	}
	else if (!strcmp(test, "model"))
	{
		plasma_lighting_prepare(1,1, NONE); plasma_lighting_submitted(1,3);
		plasma_model_lighting_slot(3,0); plasma_model_lighting_slot(7,1);
		CHECK(plasma_model_lighting_mask()==1,"only selected slot is softened");
		plasma_model_lighting_slot(3,1);
		CHECK(plasma_model_lighting_mask()==3,"both selected slots supported");
		plasma_model_lighting_slot(NONE,0);
		CHECK(plasma_model_lighting_mask()==2,"empty slot clears previous model");
		enabled=0; CHECK(!plasma_model_lighting_mask(),"disable immediately gates model state");
		enabled=1; plasma_lighting_window_begin();
		CHECK(!plasma_model_lighting_mask(),"next view clears slots");
		plasma_lighting_submitted(1,3); plasma_model_lighting_slot(3,1);
		plasma_lighting_frame_end();
		CHECK(!plasma_model_lighting_mask(),"frame cleanup clears slots");
	}
	else if (!strcmp(test, "muzzle"))
	{
		enabled=0; muzzle_on=1;
		CHECK(plasma_lighting_selected(20), "muzzle uses shared soft falloff independently");
		CHECK(!plasma_lighting_selected(3), "unrelated light stays stock");
		plasma_model_lighting_slot(20,0);
		CHECK(plasma_model_lighting_mask()==1,"muzzle model slot works without projectile lighting");
		muzzle_on=0; CHECK(!plasma_model_lighting_mask(),"both off restores stock model");
	}
	else if (!strcmp(test, "off"))
	{
		enabled = 0;
		plasma_lighting_prepare(1, 1, NONE);
		CHECK(!refreshes, "disabled changes no attachments");
	}
	else if (!strcmp(test, "filter"))
	{
		tag_name = "weapons\\rocket launcher\\explosion";
		plasma_lighting_prepare(1, 1, NONE);
		tag_name = "weapons\\plasma rifle\\bolt";
		projectile.object.type = 0;
		plasma_lighting_prepare(1, 1, NONE);
		CHECK(!refreshes, "other projectile tags and object types stay stock");
	}
	else if (!strcmp(test, "restore"))
	{
		plasma_lighting_prepare(1, 1, NONE);
		CHECK(rendered_pose && refreshes == 1, "draw uses render pose");
		CHECK(datum_try_and_get(light_data, 1)->flags == 1, "lens-flare attachment promoted only for rendering");
		enabled = 0;
		render_interpolation_frame_end();
		CHECK(!rendered_pose && refreshes == 2, "disable still restores simulation pose");
		CHECK(datum_try_and_get(light_data, 1)->flags == 0, "original dynamic flag restored");
		plasma_lighting_frame_end();
		CHECK(refreshes == 2, "restoration happens once");
	}
	else if (!strcmp(test, "split"))
	{
		plasma_lighting_prepare(1, 1, NONE);
		plasma_lighting_prepare(1, 1, NONE);
		CHECK(refreshes == 1, "two views share one attachment refresh");
	}
	else if (!strcmp(test, "capacity"))
	{
		for (i = 0; i < 130; i++) plasma_lighting_prepare(i, i, NONE);
		CHECK(refreshes == 128, "overflow leaves excess lights untouched");
		render_interpolation_frame_end();
		CHECK(refreshes == 256 && !rendered_pose, "every accepted light restored");
	}
	else if (!strcmp(test, "unload"))
	{
		plasma_lighting_prepare(1, 1, NONE);
		plasma_lighting_reset();
		render_interpolation_frame_end();
		CHECK(refreshes == 1, "old map handles never revisited");
	}
	else if (!strcmp(test, "missing"))
	{
		present = 0;
		plasma_lighting_prepare(1, 1, NONE);
		plasma_lighting_frame_end();
		CHECK(!refreshes, "deleted projectile cannot acquire a render light");
	}
	else if (!strcmp(test, "stale"))
	{
		plasma_lighting_prepare(1, 1, NONE);
		stale = 1;
		render_interpolation_frame_end();
		CHECK(refreshes == 1, "changed light owner is not restored");
		storage.valid = 0;
		plasma_lighting_prepare(2, 2, NONE);
		CHECK(refreshes == 1, "invalid light storage is untouched");
	}
	else if (!strcmp(test, "selection"))
	{
		plasma_lighting_prepare(1, 1, NONE);
		plasma_lighting_submitted(1, 3);
		CHECK(plasma_lighting_selected(3) && !plasma_lighting_selected(2), "only submitted plasma lights selected");
		CHECK(!plasma_lighting_selected(NONE), "failed submission excluded");
		plasma_lighting_window_begin();
		CHECK(!plasma_lighting_selected(3), "view indices cannot leak across split screen");
		plasma_lighting_submitted(1, 0);
		enabled = 0;
		CHECK(!plasma_lighting_selected(0), "disable immediately restores stock selection");
		emitter_on = 1;
		CHECK(plasma_lighting_selected(22) && !plasma_lighting_selected(0), "emitter-only option selects only emitter lights");
		plasma_model_lighting_reset();
		plasma_model_lighting_slot(22, 0);
		CHECK(plasma_model_lighting_mask() == 1, "emitter-only option reaches model lighting");
	}
	else if (!strcmp(test, "existing"))
	{
		datum_try_and_get(light_data, 1)->flags = 1;
		plasma_lighting_prepare(1, 1, NONE);
		render_interpolation_frame_end();
		CHECK(datum_try_and_get(light_data, 1)->flags == 1, "authored dynamic lighting stays dynamic");
	}
	return 0;
}
