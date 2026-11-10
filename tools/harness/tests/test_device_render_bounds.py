"""The real device_render_bounds_tags_loaded (device_render_bounds.c): a device's render sphere grown to hold every
vertex of its model wherever its position and power animations carry it, as a30's beam emitter's beam rises 1089 world
units out of a sphere of 50; it only grows, around the bounding offset as the root carries it, and a device that does
not move out of its sphere, or whose model or animations cannot be read, is left as the map has it."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["beam", "grows only", "not moving", "both nodes", "power", "root moves", "not interpolated", "unreadable",
         "uncompressed"]
FUNCTIONS = ("device_render_bounds_animation", "device_render_bounds_pose", "device_render_bounds_radius",
             "device_render_bounds_tags_loaded")

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the model at rest only, its animation not followed
    "rest-only": (("\t\t\t\toverlay_animation_apply_continuous(position, frame_index, orientations);\n", "\t\t\t\t;\n"),
                  "beam"),
    # the end of the travel not reached
    "end-not-reached": (("position_step <= DEVICE_RENDER_BOUNDS_STEPS;", "position_step < DEVICE_RENDER_BOUNDS_STEPS;"),
                        "beam"),
    # a vertex taken at its first node only
    "one-node": (("for (node = 0; node < 2; node++)", "for (node = 0; node < 1; node++)"), "both nodes"),
    # the power animation left out
    "no-power": (("\t\t\tif (power)\n\t\t\t{", "\t\t\tif (0)\n\t\t\t{"), "power"),
    # the sphere's middle where it is at rest, wherever the root goes
    "center-at-rest": (("matrix4x3_transform_point(&matrices[0], &device->object.bounding_offset, &center);",
                        "center = device->object.bounding_offset;"), "root moves"),
    # a sphere shrunk to its model
    "shrinks": (("radius > device->object.render_bounding_radius)", "radius != device->object.render_bounding_radius)"),
                "grows only"),
    # the graph's nodes asked to be the model's (a30's beam emitter's graph names none)
    "graph-nodes": (("\t\t!model->nodes.address ||\n",
                     "\t\t!model->nodes.address || graph->nodes.count != model->nodes.count ||\n"), "beam"),
    # an uncompressed vertex's nodes read as a compressed one's
    "nodes-tripled": (("nodes[0] = ((struct model_vertex_uncompressed const *)vertex)->nodes[0];",
                       "nodes[0] = ((struct model_vertex_uncompressed const *)vertex)->nodes[0] / 3;"), "uncompressed"),
}


def generated(fault=None):
    source = read("port/linux/game/device_render_bounds.c").replace("\r\n", "\n")
    model_types = read("source/rasterizer/rasterizer_model_types.h").replace("\r\n", "\n")
    types = "\n\n".join([
        enum_with(read("source/rasterizer/rasterizer_geometry.h").replace("\r\n", "\n"),
                  "_rasterizer_vertex_type_model_compressed"),
        enum_with(read("source/devices/device_definitions.h").replace("\r\n", "\n"), "_device_position_loops_bit"),
        structure(model_types, "model_vertex_uncompressed"),
        structure(model_types, "model_vertex_compressed"),
        enum_with(source, "_model_geometry_part_stripped_bit"),
        structure(source, "model_geometry"),
        structure(source, "model_geometry_part"),
        structure(source, "device_render_bounds_animations"),
    ] + re.findall(r"^typedef char verify_\w+\[[^;]*;", source, re.M | re.S)) + "\n"
    code = "\n\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test_types.inc", types), ("under_test.inc", code))


def test_tags_loaded_grow_them():
    """An Xbox map's tags, as they load, have the devices' render spheres grown, before the mods' tags play over them."""
    loader = function(read("source/cache/cache_files.c").replace("\r\n", "\n"), "scenario_tags_load")
    bounds = loader.index("powerup_render_bounds_tags_loaded();")
    devices = loader.index("device_render_bounds_tags_loaded();")
    assert bounds < devices < loader.index("mods_tags_loaded(stripped_scenario_name);", bounds)
    assert loader.count("device_render_bounds_tags_loaded();") == 1


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("device_render_bounds", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("device_render_bounds", generated(fault)), case)
    assert status == CHECK_FAILED, f"device_render_bounds with a fault ({control}) passed '{case}'"
