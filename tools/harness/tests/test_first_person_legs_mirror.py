"""The real first_person_legs_node_matrices and first_person_legs_mirror_node_matrices (first_person_legs.c): the
first-person legs are set back behind the eye, and a mirror, drawn before the window's own view with a camera of its
own, draws the whole body and what it carries set back as far, as the window's own view finds it, so that the legs
meet their reflection; any other object, or the body outside a mirror or with the legs off, is drawn as it is."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["meets", "carried", "carried alone", "perspective", "as is"]
FUNCTIONS = ("first_person_legs_enabled", "first_person_legs_wanted", "first_person_legs_name_has",
             "first_person_legs_node", "first_person_legs_waist", "first_person_legs_clear_of_eye",
             "first_person_legs_pose", "first_person_legs_node_matrices", "first_person_legs_mirror_view",
             "first_person_legs_mirror_body", "first_person_legs_mirror_node_matrices")
# (the legs kept clear of the eye, where the sources have it: fix-first-person-legs-knees)
OPTIONAL = ("first_person_legs_clear_of_eye",)

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the body in the mirror as the game draws it (before fix-mirror-first-person-legs)
    "not-set-back": (("if (first_person_legs_mirror_local_player_index == NONE || !object || !nodes)", "if (1)"),
                     "meets"),
    # set back as the mirror's own camera would set it back
    "mirror-camera": (("&first_person_legs_mirror_camera,\n\t\t\t&first_person_legs_mirror_frustum, FALSE,",
                       "&global_window_parameters.camera,\n\t\t\t&global_window_parameters.frustum, FALSE,"), "meets"),
    # the upper body folded away in the mirror too
    "folded": (("&first_person_legs_mirror_frustum, FALSE,", "&first_person_legs_mirror_frustum, TRUE,"), "meets"),
    # what it carries left where it was
    "carried-left": (("\t\tfirst_person_legs_mirror_matrices[index].position.x += first_person_legs_mirror_offset.i;\n",
                      ""), "carried"),
    # what it carries set back by another scene's body
    "any-scene": ((" || first_person_legs_mirror_scene_index != render.scene_index", ""), "carried alone"),
    # the window's player's body set back seen from behind too
    "perspective-ignored": (("(object_index == unit_index &&\n\t\t\tdirector_get_perspective("
                             "first_person_legs_mirror_local_player_index) == _director_perspective_first_person)",
                             "(object_index == unit_index)"), "perspective"),
}


def generated(fault=None):
    source = read("port/linux/game/first_person_legs.c").replace("\r\n", "\n")
    config = "\n".join([enum_with(source, "FIRST_PERSON_LEGS_MAXIMUM_NODES")] +
                       re.findall(r"^#define FIRST_PERSON_LEGS_\w+ .*$", source, re.M)) + "\n"
    header = read("port/linux/game/object_mesh.h").replace("\r\n", "\n")
    mesh = "\n".join([enum_with(header, "OBJECT_MESH_MAXIMUM_NODES"), structure(header, "object_mesh_vertex"),
                      structure(header, "object_mesh")]) + "\n"
    state = "\n".join(re.findall(r"^static [^(\n]*;$", source, re.M))
    names = [name for name in FUNCTIONS if name not in OPTIONAL or f"{name}(\n" in source]
    code = state + "\n\n" + "\n\n".join(function(source, name) for name in names) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("mesh.inc", mesh), ("under_test.inc", code))


def test_mirror_sees_the_window_view():
    """render.c gives the mirror the window's own view, the rasterizer's camera and frustum the window is drawn with
    after it, around the mirror's scene; render_objects.c draws a mirror's objects with the mirror's pose."""
    render = function(read("source/render/render.c").replace("\r\n", "\n"), "render_player_frame")
    begin = render.index("first_person_legs_mirror_view(window->local_player_index, &window->rasterizer_camera, "
                         "&rasterizer_frustum);")
    mirror = render.index("render_window(\n\t\t\t\tNONE,\n\t\t\t\t&mirror_camera,", begin)
    end = render.index("first_person_legs_mirror_view(NONE, NULL, NULL);", mirror)
    own = render.index("render_window(\n\t\twindow->local_player_index,\n\t\tcamera,\n\t\t&frustum,\n"
                       "\t\t&window->rasterizer_camera,\n\t\t&rasterizer_frustum,", end)
    assert begin < mirror < end < own
    objects = function(read("source/render/render_objects.c").replace("\r\n", "\n"), "render_object_list")
    assert ("!data->shadow && render.camera.mirrored ?\n\t\t\t\tfirst_person_legs_mirror_node_matrices(object_index, "
            "object_get_node_matrices(object_index)) :") in objects


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("first_person_legs_mirror", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("first_person_legs_mirror", generated(fault)), case)
    assert status == CHECK_FAILED, f"the mirror's legs with a fault ({control}) passed '{case}': the test cannot see it"
