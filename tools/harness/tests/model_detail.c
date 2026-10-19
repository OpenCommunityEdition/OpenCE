/* display.model_detail's level for each permutation (port/linux/game/model_detail.c),
from eltanschauung's #282 */
#include "harness.h"
#include <math.h>
#define REAL_MAX 3.4028235e38f
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
struct model { real detail_cutoff_pixels[5]; };
struct model_region_permutation { short geometry_indices[5]; };
static struct { short debug_model_lod; } rasterizer_debug_options = {NONE};
static struct { struct { real_point3d position; } camera; } render;
static int stock;
static int halo_model_detail_stock(void) { return stock; }
#include "under_test.inc"

static short level_of(struct model const *model, struct model_region_permutation const *permutation,
    short level, short rules)
{
    short geometry = model_detail_geometry_index(model, permutation, level, rules);
    return geometry == NONE ? NONE : geometry - 10;
}

int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    CASE("interior") {
        struct model model = {{0, 0, 0, 0, 3000}};
        struct model_region_permutation permutation = {{10, 11, 12, 13, 14}};
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_outside) == 3, "the cryotube from outside");
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_inside) == 4, "the cryotube from inside");
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_stock) == 4, "stock keeps the level chosen");
        model.detail_cutoff_pixels[3] = 2000;
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_outside) == 2, "several interior levels");
        model.detail_cutoff_pixels[4] = 500; permutation.geometry_indices[4] = NONE;
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_outside) == 2,
            "a missing level does not fall back into interior geometry");
        model.detail_cutoff_pixels[2] = 1000;
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_outside) == 2, "1000 is outside");
        model.detail_cutoff_pixels[2] = 1000.01f;
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_outside) == 1, "over 1000 is inside");
        model.detail_cutoff_pixels[0] = model.detail_cutoff_pixels[1] = 2000;
        CHECK(level_of(&model, &permutation, 4, _model_detail_rules_outside) == NONE, "no outside geometry draws nothing");
        CHECK(level_of(&model, &permutation, 3, _model_detail_rules_stock) == 3, "stock draws the model's level");
    }
    CASE("bounds") {
        struct model model = {{0, 0, 0, 0, 3000}};
        real_point3d center = {10, 20, 30}, camera = {10, 20, 30};
        CHECK(model_detail_camera_inside(&model, &camera, &center, 2), "a sphere away from the origin");
        camera.x = 11.999f; CHECK(model_detail_camera_inside(&model, &camera, &center, 2), "just inside");
        camera.x = 12; CHECK(!model_detail_camera_inside(&model, &camera, &center, 2), "the sphere's surface is outside");
        camera.x = 12.001f; CHECK(!model_detail_camera_inside(&model, &camera, &center, 2), "just outside");
        CHECK(model_detail_camera_inside(&model, &camera, &center, 4), "a larger radius");
        CHECK(!model_detail_camera_inside(&model, &camera, &center, 0), "zero radius");
        CHECK(!model_detail_camera_inside(&model, &camera, &center, -1), "negative radius");
        CHECK(!model_detail_camera_inside(&model, &camera, &center, INFINITY), "infinite radius");
        CHECK(!model_detail_camera_inside(&model, &camera, &center, NAN), "radius not a number");
        camera.x = NAN; CHECK(!model_detail_camera_inside(&model, &camera, &center, 2), "camera not a number");
        model.detail_cutoff_pixels[4] = 500;
        CHECK(!model_detail_camera_inside(&model, NULL, NULL, 2), "other models need no camera or sphere");
    }
    CASE("permutations") {
        struct model_region_permutation permutation;
        struct model model = {{0, 1, 10, 50, 200}};
        int mask, requested, level;
        for (mask = 0; mask < 32; mask++) {
            for (level = 0; level < 5; level++)
                permutation.geometry_indices[level] = mask & (1 << level) ? 10 + level : NONE;
            for (requested = 0; requested < 5; requested++) {
                short expected = requested;
                while (expected >= 0 && !(mask & (1 << expected))) expected--;
                CHECK(level_of(&model, &permutation, requested, _model_detail_rules_outside) == expected,
                    "the most detailed level with geometry at or below the model's (mask %d, level %d)", mask, requested);
                CHECK(level_of(&model, &permutation, requested, _model_detail_rules_stock) ==
                    (mask & (1 << requested) ? requested : NONE),
                    "stock draws nothing where the model's level has none (mask %d, level %d)", mask, requested);
            }
        }
    }
    CASE("rules") {
        struct model cryotube = {{0, 0, 0, 0, 3000}}, rock = {{0, 1, 10, 50, 200}};
        real_point3d center = {1, 2, 3};
        render.camera.position = center;
        stock = TRUE;
        CHECK(model_detail_rules(&cryotube, &center, 2) == _model_detail_rules_stock, "stock");
        stock = FALSE;
        CHECK(model_detail_rules(&cryotube, &center, 2) == _model_detail_rules_inside, "inside the cryotube");
        CHECK(model_detail_rules(&rock, &center, 2) == _model_detail_rules_outside, "a model without an inside");
        rasterizer_debug_options.debug_model_lod = 4;
        CHECK(model_detail_rules(&cryotube, &center, 2) == _model_detail_rules_stock,
            "rasterizer_debug_model_lod draws the level it names");
        rasterizer_debug_options.debug_model_lod = NONE;
        render.camera.position.z = 6;
        CHECK(model_detail_rules(&cryotube, &center, 2) == _model_detail_rules_outside, "outside the cryotube");
    }
    return 0;
}
