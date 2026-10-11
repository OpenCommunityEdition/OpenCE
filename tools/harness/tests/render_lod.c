#include "harness.h"
#include <strings.h>
#include <math.h>
static unsigned long revision;
static int configured, reads, tag_reads;
static char const *model_name = "weapons\\needler\\needler";
static unsigned long config_changes(void) { return revision; }
static int config_boolean(char const *name)
{
	CHECK(!strcmp(name, "display.disable_lod"), "reads its own option");
	reads++;
	return configured;
}
static char const *tag_get_name(long index)
{
	CHECK(index == 123, "valid model index passed through");
	tag_reads++;
	return model_name;
}
#define csstrcasecmp strcasecmp
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
#define PIN(v,l,h) ((v)<(l)?(l):((v)>(h)?(h):(v)))
struct model {real detail_cutoff_pixels[5];};
#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	CASE("default") {
		CHECK(!render_lod_disabled(), "default unchanged");
		CHECK(!render_lod_model_disabled(123), "stock model detail");
		CHECK(!tag_reads, "no tag lookup while off");
		CHECK(!render_lod_particle_visible(2, 2), "stock strict cutoff");
		CHECK(render_lod_particle_visible(3, 2), "stock visible particle");
		CHECK(reads == 1, "config cached across rendering calls");
	}
	CASE("live") {
		CHECK(!render_lod_disabled(), "starts off");
		configured = 1; revision++;
		CHECK(render_lod_model_disabled(123), "model switches live");
		CHECK(render_lod_particle_visible(.001f, 10), "tiny particle retained");
		CHECK(reads == 2, "one read per revision");
		configured = 0; revision++;
		CHECK(!render_lod_model_disabled(123), "normal detail restored");
		CHECK(!render_lod_particle_visible(.001f, 10), "cutoff restored");
	}
	CASE("cryotube") {
		configured = 1;
		model_name = "levels\\a10\\devices\\cryotube\\cryotube";
		CHECK(!render_lod_model_disabled(123), "keep cryotube cinematic mesh hidden");
		model_name = "LEVELS\\A10\\DEVICES\\CRYOTUBE\\CRYOTUBE";
		CHECK(!render_lod_model_disabled(123), "tag names case insensitive");
		model_name = "levels\\a10\\devices\\doors\\door cryo chamber\\door cryo chamber";
		CHECK(render_lod_model_disabled(123), "only cryotube is exempt");
		model_name = "characters\\cyborg\\cyborg";
		CHECK(render_lod_model_disabled(123), "reused tag index after map switch");
	}
	CASE("projection") {
		configured = 1;
		CHECK(!render_lod_particle_visible(0, 2), "zero cannot divide by zero");
		CHECK(!render_lod_particle_visible(-1, 2), "behind camera");
		CHECK(!render_lod_particle_visible(NAN, 2), "invalid projection");
		CHECK(render_lod_particle_visible(.001f, 2), "positive projection retained");
	}
	CASE("geometry") {
		struct model model = {{1, 10, 20, 40, 80}};
		short level;
		for (level = 0; level < 5; level++) {
			real pixels = model.detail_cutoff_pixels[level];
			CHECK(render_lod_geometry_level(&model, pixels, NONE, FALSE) == level, "authored threshold boundary");
			CHECK(render_lod_geometry_level(&model, pixels, NONE, TRUE) == 4, "all distances keep maximum detail");
		}
		CHECK(render_lod_geometry_level(&model, 79.99f, NONE, FALSE) == 3, "below boundary");
		CHECK(render_lod_geometry_level(&model, 0, 2, FALSE) == 2, "stock debug override");
		CHECK(render_lod_geometry_level(&model, 0, 2, TRUE) == 4, "setting forces maximum");
		CHECK(render_lod_geometry_level(&model, 0, 99, FALSE) == 4, "debug clamped");
		configured = 1;
		model.detail_cutoff_pixels[4] = 3000;
		model_name = "levels\\a10\\devices\\cryotube\\cryotube";
		CHECK(render_lod_geometry_level(&model, 200, NONE, render_lod_model_disabled(123)) == 3, "cryotube keeps normal mesh");
		model_name = "vehicles\\warthog\\warthog";
		CHECK(render_lod_geometry_level(&model, 200, NONE, render_lod_model_disabled(123)) == 4, "other models use highest mesh");
	}
	return 0;
}
