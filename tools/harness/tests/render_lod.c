#include "harness.h"
#include <math.h>
static unsigned long revision;
static int configured, reads;
static unsigned long config_changes(void) { return revision; }
static int config_boolean(char const *name)
{
	CHECK(!strcmp(name, "display.disable_lod"), "reads its own option");
	reads++;
	return configured;
}
#define REAL_MAX 3.4028235e38f
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PIN(v,l,h) ((v)<(l)?(l):((v)>(h)?(h):(v)))
struct model {real detail_cutoff_pixels[5];};
struct model_region_permutation {short geometry_indices[5];};
#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	CASE("default") {
		CHECK(!render_lod_disabled(), "default unchanged");
		CHECK(!render_lod_particle_visible(2, 2), "stock strict cutoff");
		CHECK(render_lod_particle_visible(3, 2), "stock visible particle");
		CHECK(reads == 1, "config cached across rendering calls");
	}
	CASE("live") {
		CHECK(!render_lod_disabled(), "starts off");
		configured = 1; revision++;
		CHECK(render_lod_disabled(), "model switches live");
		CHECK(!render_lod_particle_visible(.001f, 10), "negligible particle culled");
		CHECK(render_lod_particle_visible(.501f, 10), "visible small particle retained");
		CHECK(reads == 2, "one read per revision");
		configured = 0; revision++;
		CHECK(!render_lod_disabled(), "normal detail restored");
		CHECK(!render_lod_particle_visible(.001f, 10), "cutoff restored");
	}
	CASE("interior") {
		struct model model = {{0, 0, 0, 0, 3000}};
		struct model_region_permutation permutation = {{1, 1, 1, 1, 0}};
		CHECK(render_lod_permutation_level(&model, &permutation, 4, TRUE, FALSE) == 3, "stock cryotube outside");
		CHECK(render_lod_permutation_level(&model, &permutation, 4, TRUE, TRUE) == 4, "stock cryotube inside");
		model.detail_cutoff_pixels[3] = 2000;
		CHECK(render_lod_permutation_level(&model, &permutation, 4, TRUE, FALSE) == 2, "multiple interior levels");
		model.detail_cutoff_pixels[4] = 500; permutation.geometry_indices[4] = NONE;
		CHECK(render_lod_permutation_level(&model, &permutation, 4, TRUE, FALSE) == 2, "missing LOD cannot fall back into interior geometry");
		model.detail_cutoff_pixels[2] = 1000;
		CHECK(render_lod_permutation_level(&model, &permutation, 4, TRUE, FALSE) == 2, "1000 remains exterior");
		model.detail_cutoff_pixels[2] = 1000.01f;
		CHECK(render_lod_permutation_level(&model, &permutation, 4, TRUE, FALSE) == 1, "strictly over 1000 is interior");
		model.detail_cutoff_pixels[0] = model.detail_cutoff_pixels[1] = 2000;
		CHECK(render_lod_permutation_level(&model, &permutation, 4, TRUE, FALSE) == NONE, "no exterior geometry stays hidden");
		CHECK(render_lod_permutation_level(&model, &permutation, 3, FALSE, FALSE) == 3, "authored or debug selection remains explicit");
	}
	CASE("bounds") {
		struct model model = {{0, 0, 0, 0, 3000}};
		real_point3d center = {10,20,30}, camera = {10,20,30};
		CHECK(render_lod_inside_model(&model,&camera,&center,2), "translated world-space center");
		camera.x = 11.999f; CHECK(render_lod_inside_model(&model,&camera,&center,2), "just inside");
		camera.x = 12; CHECK(!render_lod_inside_model(&model,&camera,&center,2), "boundary is outside");
		camera.x = 12.001f; CHECK(!render_lod_inside_model(&model,&camera,&center,2), "just outside");
		CHECK(render_lod_inside_model(&model,&camera,&center,4), "scaled world-space radius");
		CHECK(!render_lod_inside_model(&model,&camera,&center,0), "zero radius");
		CHECK(!render_lod_inside_model(&model,&camera,&center,-1), "negative radius");
		CHECK(!render_lod_inside_model(&model,&camera,&center,INFINITY), "infinite radius");
		CHECK(!render_lod_inside_model(&model,&camera,&center,NAN), "invalid radius");
		camera.x = NAN; CHECK(!render_lod_inside_model(&model,&camera,&center,2), "invalid camera");
		model.detail_cutoff_pixels[4] = 500;
		CHECK(!render_lod_inside_model(&model,NULL,NULL,2), "ordinary models need no camera or sphere test");
	}
	CASE("projection") {
		configured = 1;
		CHECK(!render_lod_particle_visible(0, 2), "zero cannot divide by zero");
		CHECK(!render_lod_particle_visible(-1, 2), "behind camera");
		CHECK(!render_lod_particle_visible(NAN, 2), "invalid projection");
		CHECK(!render_lod_particle_visible(INFINITY, 2), "infinite projection rejected");
		CHECK(!render_lod_particle_visible(.001f, 2), "negligible projection culled");
		CHECK(render_lod_particle_visible(.001f, 0), "zero authored cutoff retains positive projection");
	}
	CASE("particle-cutoff") {
		configured = 1;
		CHECK(!render_lod_particle_visible(.5f, 4), "strict default boundary");
		CHECK(render_lod_particle_visible(.5001f, 4), "just above default boundary");
		CHECK(render_lod_particle_visible(.3f, .25f), "preserve a smaller authored cutoff");
		CHECK(render_lod_particle_visible(.001f, 0), "preserve authored zero cutoff");
		configured = 0; revision++;
		CHECK(!render_lod_particle_visible(3, 4), "off keeps authored cutoff");
		CHECK(render_lod_particle_visible(5, 4), "off keeps authored visibility");
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
		CHECK(render_lod_geometry_level(&model, 0, 2, TRUE) == 2, "explicit debug override wins");
		CHECK(render_lod_geometry_level(&model, 0, -5, TRUE) == 0, "negative debug clamped");
		CHECK(render_lod_geometry_level(&model, 0, 99, FALSE) == 4, "debug clamped");
	}
	CASE("permutations") {
		struct model_region_permutation permutation;
		struct model model = {{0, 0, 0, 0, 0}};
		int mask, requested, level;
		for (mask = 0; mask < 32; mask++) {
			for (level = 0; level < 5; level++)
				permutation.geometry_indices[level] = mask & (1 << level) ? 0 : NONE;
			for (requested = 0; requested < 5; requested++) {
				short expected = requested;
				while (expected >= 0 && !(mask & (1 << expected))) expected--;
				CHECK(render_lod_permutation_level(&model, &permutation, requested, TRUE, FALSE) == expected,
					"highest available geometry at or below the requested level");
				CHECK(render_lod_permutation_level(&model, &permutation, requested, FALSE, FALSE) == requested,
					"authored missing geometry stays hidden with override off");
			}
		}
	}
	return 0;
}
