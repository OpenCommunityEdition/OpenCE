"""Six-plane and conservative facing tests from the production geometry code."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, inline, mutated, read, run, structure

CASES = ["outside-near", "outside-far", "outside-side", "crossing-kept",
         "facing-away", "facing-toward", "edge-on-kept", "degenerate-kept",
         "cone-never-more-aggressive", "cone-culls-something", "cone-camera-inside",
         "cone-wide-no-verdict", "box-outside", "box-partly", "box-inside",
         "split-partition", "split-limits", "split-room", "split-loose",
         "out-of-range-surfaces", "group-material", "missing-owner",
         "load-subclusters", "load-surface-lists", "load-lifetime", "load-allocation-failure",
         "load-small-group-scratch", "cluster-order", "cluster-order-per-call", "stale-structure",
         "walk-fallback", "walk-planes", "walk-facing", "walk-near-first", "set-tests", "set-tests-unloaded", "console-local", "walk-unit-forward",
         "source-fog-rejected", "source-fog-straddling", "debug-record", "dispose-empty", "mirror-far", "build-budget",
          "rejected-bucket-visits", "compact-allocation-failure", "overlay-truncation", "default-mask", "debug-letterbox",
          "walk-profile", "missing-subcluster-warning", "debug-stale-data"]
CONTROLS = {
    "text-under-letterbox": (("bounds.y0 = (render.camera.viewport_bounds.y1 - render.camera.viewport_bounds.y0) / 8 + 2;", "bounds.y0 = 0;"), "structure_culling_render_debug", "debug-letterbox"),
    "drop-default-bucket-facing": (("FLAG(_structure_culling_bucket_facing_bit)", "0"), "structure_culling_tests", "default-mask"),
    "overlay-no-cap": (("if (count >= STRUCTURE_CULLING_MAXIMUM_DEBUG_TRIANGLES)", "if (count >= LONG_MAX)"), "structure_culling_render_debug", "overlay-truncation"),
    "compact-with-realloc": (("allocation = malloc(final_size ? final_size : 1);", "allocation = realloc(structure_culling.allocation, final_size ? final_size : 1);"), "structure_culling_structure_bsp_loaded", "compact-allocation-failure"),
    "scan-rejected-bucket": (("if (bucket_reason && !structure_culling_debug_enabled)", "if (FALSE)"), "structure_culling_traverse", "rejected-bucket-visits"),
    "drop-build-budget": (("if (size >= STRUCTURE_CULLING_BUILD_BUDGET)", "if (FALSE)"), "structure_culling_structure_bsp_loaded", "build-budget"),
    "mirror-use-far": (("if (render.camera.z_near == 0.0f)", "if (FALSE)"), "structure_culling_traverse", "mirror-far"),
    "free-null-allocation": (("if (structure_culling.allocation)", "if (TRUE)"), "structure_culling_dispose", "dispose-empty"),
    "record-drawn": (("surface_index, keep);", "surface_index, keep || !reason);"), "structure_culling_record", "debug-record"),
    "record-while-off": (("if (!structure_culling_debug_enabled)", "if (FALSE)"), "structure_culling_record", "debug-record"),
    "clear-while-off": (("if (structure_culling_debug_enabled)", "if (TRUE)"), "structure_culling_traverse", "debug-record"),
    "per-window-cap": (("static long count;", "long count = 0;"), "structure_culling_render_debug", "debug-record"),
    "drop-near": (("if (plane3d_distance_to_point", "if (plane_index == 4) continue;\n\t\tif (plane3d_distance_to_point"), "structure_culling_triangle_outside", "outside-near"),
    "different-planes": (("return FALSE;", "return outside_points[0] && outside_points[1] && outside_points[2];"), "structure_culling_triangle_outside", "crossing-kept"),
    "ignore-radius": (("sin_alpha = cone->radius / length;", "sin_alpha = 0.0f;"), "structure_culling_cone_facing", "cone-never-more-aggressive"),
    "flip-facing": (("STRUCTURE_CULLING_FRONT_SIGN * dot_product3d(&normal, &view) < 0.0f", "STRUCTURE_CULLING_FRONT_SIGN * dot_product3d(&normal, &view) > 0.0f"), "structure_culling_triangle_faces_away", "facing-away"),
    "box-four-planes": (("plane_index < 6; ++plane_index", "plane_index < 4; ++plane_index"), "structure_culling_box_test", "box-outside"),
    "mix-eligibility": (("if (eligible_count && eligible_count < count)", "if (FALSE && eligible_count && eligible_count < count)"), "structure_culling_split_range", "split-limits"),
    "drop-split-tail": (("ends[(*bucket_count)++] = first + count;", "ends[(*bucket_count)++] = first + count - 1;"), "structure_culling_split_range", "split-partition"),
    "skip-facing-split": (("if (allow_facing && cutoff < STRUCTURE_CULLING_CONE_SPLIT_CUTOFF)", "if (FALSE)"), "structure_culling_split_range", "split-room"),
    "keep-invalid-vertices": (("if (from_surface_list && !testable)", "if (FALSE)"), "structure_culling_gather_pairings", "out-of-range-surfaces"),
    "use-owner-in-group": (("from_surface_list ? group_material : surface_materials[surface_index]", "surface_materials[surface_index]"), "structure_culling_gather_pairings", "group-material"),
    "drop-missing-owner": (("if (from_surface_list && !testable)", "if (material_index < 0 || (from_surface_list && !testable))"), "structure_culling_gather_pairings", "missing-owner"),
    "scratch-total-capacity": (("size += largest_group_count * scratch_unit_size;", "size += capacity * scratch_unit_size;"), "structure_culling_structure_bsp_loaded", "load-small-group-scratch"),
    "last-group-maximum": (("MAX(*maximum_group_count, count)", "count"), "structure_culling_build_lists", "load-small-group-scratch"),
    "sort-descending": (("distances[order[j - 1]] > distances[entry]", "distances[order[j - 1]] < distances[entry]"), "structure_culling_order_clusters", "cluster-order"),
    "drop-structure-check": (("structure_culling.structure == structure &&", "TRUE &&"), "structure_culling_data_matches", "stale-structure"),
    "ignore-zero-mask": (("!structure_culling_tests || ", ""), "structure_culling_traverse", "walk-fallback"),
    "use-fog-far": (("rasterizer_globals.far_clip_distance", "render.camera.z_far"), "structure_culling_traverse", "walk-planes"),
    "skip-forward-normalization": (("scale_vector3d(&render.camera.forward, 1.0f / square_root(magnitude_squared3d(&render.camera.forward)),\n\t\t&planes[_structure_culling_frustum_plane_far].n);", "planes[_structure_culling_frustum_plane_far].n = render.camera.forward;"), "structure_culling_traverse", "walk-unit-forward"),
    "drop-source-box": (("if (bucket->source_bounds && !render_frustum_cube_visible", "if (FALSE && bucket->source_bounds && !render_frustum_cube_visible"), "structure_culling_traverse", "source-fog-rejected"),
    "clamp-finer-box": (("render_frustum_cube_visible(frustum, bucket->source_bounds, FALSE)", "render_frustum_cube_visible(frustum, &bucket->bounds, FALSE)"), "structure_culling_traverse", "source-fog-straddling"),
    "skip-profile-enter": (("profile_enter((*section));", ""), "structure_culling_traverse", "walk-profile"),
    "skip-profile-exit": (("profile_exit((*section));", ""), "structure_culling_traverse", "walk-profile"),
    "draw-stale-counters": (("if (!structure_culling_data_matches(structure))", "if (FALSE)"), "structure_culling_render_debug", "debug-stale-data"),
    "skip-missing-warning": (("if (!warned_about_missing_subclusters)", "if (FALSE && !warned_about_missing_subclusters)"), "structure_visibility_compute", "missing-subcluster-warning"),
}


def generated(control=None):
    path = Path(__file__).resolve().parents[3] / "port/linux/game/structure_culling.c"
    source = path.read_text() if path.exists() else ""
    names = ["structure_culling_triangle_outside", "structure_culling_triangle_faces_away",
             "structure_culling_cone_build", "structure_culling_cone_facing", "structure_culling_box_test",
             "structure_culling_split_class", "structure_culling_split_compare", "structure_culling_split_range",
             "structure_culling_split", "structure_culling_gather_pairings",
             "structure_culling_dispose", "structure_culling_pairing_points", "structure_culling_bounds_add",
             "structure_culling_build_group", "structure_culling_build_lists", "structure_culling_structure_bsp_loaded"]
    names += ["structure_culling_order_clusters", "structure_culling_data_matches", "structure_culling_record",
              "structure_culling_traverse", "structure_culling_set_tests", "structure_culling_debug",
              "structure_culling_render_debug"]
    parts = {name: function(source, name) for name in names}
    parts["hs_expression_changes_no_game"] = function(read("source/hs/hs.c"), "hs_expression_changes_no_game")
    if control and control not in ("drop-default-bucket-facing", "skip-missing-warning"):
        (before, after), name, _ = CONTROLS[control]
        if control == "different-planes":
            # Each point outside a different plane wrongly becomes sufficient.
            parts[name] = mutated(parts[name], before, after)
            parts[name] = mutated(parts[name], "short plane_index;", "short plane_index; boolean outside_points[3] = {FALSE, FALSE, FALSE};")
            parts[name] = mutated(parts[name], "if (plane3d_distance_to_point", "outside_points[0] |= plane3d_distance_to_point(&planes[plane_index], point0) > 0.0f;\n\t\toutside_points[1] |= plane3d_distance_to_point(&planes[plane_index], point1) > 0.0f;\n\t\toutside_points[2] |= plane3d_distance_to_point(&planes[plane_index], point2) > 0.0f;\n\t\tif (plane3d_distance_to_point")
        else:
            parts[name] = mutated(parts[name], before, after)
    header = read("port/linux/game/structure_culling.h")
    types = structure(header, "structure_culling_cone") + "\n" + enum_with(header, "_structure_culling_facing_away")
    types += "\n" + structure(header, "structure_culling_split_surface") + "\n" + structure(header, "structure_culling_pairing")
    dependencies = [
        ("source/tag_files/tag_groups.h", ["tag_block", "tag_reference", "tag_data"]),
        ("source/render/render.h", ["render_distant_light", "render_lighting"]),
        ("source/structures/leaf_map.h", ["leaf_map"]),
        ("source/rasterizer/rasterizer_geometry.h", ["vertex_buffer"]),
        ("source/structures/structure_bsp_definitions.h", ["structure_surface", "structure_material", "structure_lightmap", "structure_bsp"]),
        ("source/structures/structure_visibility.c", ["structure_visibility_cluster", "structure_visibility_subcluster"]),
        ("source/shaders/shader_definitions.h", ["shader_radiosity_properties", "shader_physics_properties", "shader_base", "shader"]),
    ]
    for file, structs in dependencies:
        text = read(file)
        types += "\n" + "\n".join(structure(text, name) for name in structs)
    for name in ["structure_culling_material", "structure_culling_bucket", "structure_culling_cluster", "structure_culling_data", "structure_culling_workspace"]:
        types += "\n" + structure(source, name)
    types += "\n" + enum_with(header, "_structure_culling_triangle_frustum_bit")
    types += "\n" + enum_with(source, "COMPRESSED_STRUCTURE_VERTEX_SIZE")
    default = re.search(r"static long structure_culling_tests\s*=[^;]+;", source).group(0)
    if control == "drop-default-bucket-facing":
        default = mutated(default, *CONTROLS[control][0])
    types += "\nstatic struct structure_culling_data structure_culling;\n" + default + "\n"
    types += "static long structure_culling_submitted, structure_culling_outside, structure_culling_facing;\n"
    types += "static boolean structure_culling_debug_enabled;\nstatic unsigned long structure_culling_recorded[2][4096];\n"
    cameras = read("source/render/render_cameras.c")
    types += "\n" + enum_with(cameras, "_render_frustum_point_flags_left_bit")
    sign = re.search(r"^#define STRUCTURE_CULLING_FRONT_SIGN .+$", source, re.M).group(0)
    sign += "\n" + "\n".join(re.findall(r"^#define STRUCTURE_CULLING_(?:MAXIMUM_BUCKET_SURFACES|LOOSE_BUCKET_SIZE|CONE_SPLIT_CUTOFF|BUILD_BUDGET|MAXIMUM_DEBUG_TRIANGLES) .+$", source, re.M))
    sign += "\nstatic short structure_culling_split_axis;\n"
    math = read("source/math/real_math.h")
    helpers = "\n".join(inline(math, name) for name in ["vector_from_points3d", "dot_product3d", "cross_product3d", "magnitude_squared3d", "scale_vector3d", "plane3d_distance_to_point"])
    helpers += "\n" + function(cameras, "render_frustum_cube_visible")
    helpers += "\n" + function(read("source/structures/structure_visibility.c"), "structure_visibility_traverse_subclusters")
    surface_walk = function(read("source/structures/structure_visibility.c"), "structure_visibility_compute")
    if control == "skip-missing-warning":
        surface_walk = mutated(surface_walk, *CONTROLS[control][0])
    surface_walk = surface_walk.split("profile_exit(render_structure_visibility_portal_traversal);", 1)[1]
    surface_walk = """static void structure_visibility_traverse_surface_lists(struct structure_bsp *structure) {}
static void fixture_visibility_surface_walk(struct structure_bsp *structure)
{
    static boolean warned_about_missing_subclusters, warned_about_environment_surfaces;
""" + surface_walk
    return (("types.inc", types), ("under_test.inc", sign + "\n" + helpers + "\n" + "\n".join(parts.values()) + "\n" + surface_walk))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("structure_culling", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", CONTROLS)
def test_negative_control(control):
    status, output = run(build("structure_culling", generated(control)), CONTROLS[control][2])
    assert status == CHECK_FAILED and "check failed:" in output, f"{control} escaped its CHECK: {status}: {output}"
