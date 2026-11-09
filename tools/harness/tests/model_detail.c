/* display.model_detail's level for each permutation (port/linux/game/model_detail.c),
from eltanschauung's #282 */
#include "harness.h"
#include <math.h>
#define REAL_MAX 3.4028235e38f
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
struct model { real detail_cutoff_pixels[5]; };
struct model_region_permutation { short geometry_indices[5]; };
static struct { short debug_model_lod; } rasterizer_debug_options = {NONE};
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
    CASE("reached") {
        /* a10's cryotube: its top level, at 3000, the tube with the Master Chief's armor in it */
        struct model model = {{0, 0, 0, 0, 3000}}, rock = {{0, 1, 10, 50, 200}};
        CHECK(model_detail_inside_reached(&model, 3000), "at its cutoff");
        CHECK(model_detail_inside_reached(&model, 5520), "seated in it (its center 0.1 from the eyes)");
        CHECK(model_detail_inside_reached(&model, REAL_MAX), "a cinematic's movie star");
        CHECK(!model_detail_inside_reached(&model, 2999.9f), "just below its cutoff");
        CHECK(!model_detail_inside_reached(&model, 1100), "standing by it, as one climbed out");
        CHECK(!model_detail_inside_reached(&model, 0), "far off");
        CHECK(!model_detail_inside_reached(&model, NAN), "a size not a number");
        model.detail_cutoff_pixels[3] = 2000;
        CHECK(model_detail_inside_reached(&model, 2000), "the first of several inside levels");
        CHECK(!model_detail_inside_reached(&model, 1999), "below the first of several");
        CHECK(!model_detail_inside_reached(&rock, REAL_MAX), "a model without an inside");
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
        stock = TRUE;
        CHECK(model_detail_rules(&cryotube, 5520) == _model_detail_rules_stock, "stock");
        stock = FALSE;
        CHECK(model_detail_rules(&cryotube, 5520) == _model_detail_rules_inside, "inside the cryotube");
        CHECK(model_detail_rules(&cryotube, 1100) == _model_detail_rules_outside, "standing by the cryotube");
        CHECK(model_detail_rules(&rock, 5520) == _model_detail_rules_outside, "a model without an inside");
        rasterizer_debug_options.debug_model_lod = 4;
        CHECK(model_detail_rules(&cryotube, 1100) == _model_detail_rules_stock,
            "rasterizer_debug_model_lod draws the level it names");
        rasterizer_debug_options.debug_model_lod = NONE;
    }
    return 0;
}
