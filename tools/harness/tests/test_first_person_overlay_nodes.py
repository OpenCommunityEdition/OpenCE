"""The real first_person_weapon_overlay_nodes (first_person_weapons.c): the nodes an overlay animation moves, by its
rotation, translation and scale flags, with their children wherever they come in the graph, within the animation's
and the graph's nodes. The first-person interpolation draws them as the tick has them when the ammunition's count
changes, so that a loaded round goes into the magazine, not through the gun."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure  # noqa: E402

CASES = ["moved", "children", "out of range"]
NEGATIVE_CONTROLS = {
    # the overlay's scaled nodes left out
    "no-scale": (("||\n\t\t\tBIT_VECTOR_TEST_FLAG(animation->nodes_with_scale_flags, node_index))", ")"), "moved"),
    # the nodes' children left out
    "no-children": (("\t\t\t\tBIT_VECTOR_SET_FLAG(nodes, node_index, TRUE);\n\t\t\t\tchanged= TRUE;\n", ""),
                    "children"),
    # one pass over the nodes: a child before its parent left out
    "one-pass": (("\t\t\t\tchanged= TRUE;\n", ""), "children"),
    # flags past the animation's nodes taken
    "past-the-animation": ((" && node_index<animation->node_count", ""), "out of range"),
}


def generated(fault=None):
    source = read("source/interface/first_person_weapons.c").replace("\r\n", "\n")
    cseries = read("source/cseries/cseries.h")
    config = "\n".join([
        "enum { " + ", ".join(f"{name} = {constant(cseries, name)}" for name in ["LONG_BITS", "LONG_BITS_BITS"]) + " };",
        f"#define TAG_STRING_LENGTH {constant(read('source/tag_files/tag_files.h'), 'TAG_STRING_LENGTH')}",
        "#define MAXIMUM_NODES_PER_ANIMATION "
        f"{constant(read('source/models/model_animation_definitions.h'), 'MAXIMUM_NODES_PER_ANIMATION')}",
        re.search(r"^#define SET_FLAG\(.*", cseries, re.M).group(),
        re.search(r"^#define BIT_VECTOR_SIZE_IN_LONGS\(.*", cseries, re.M).group(),
        re.search(r"^#define BIT_VECTOR_TEST_FLAG\(.*", cseries, re.M).group(),
        re.search(r"^#define BIT_VECTOR_SET_FLAG\(.*", cseries, re.M).group(),
        structure(source, "animation_graph_node"),
    ]) + "\n"
    code = function(source, "first_person_weapon_node_parents") + "\n" + \
        function(source, "first_person_weapon_overlay_nodes") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("first_person_overlay_nodes", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("first_person_overlay_nodes", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_wired():
    """The ammunition's overlay, where it shows the rounds loaded, passes its frame and nodes to the interpolation;
    every weapon passes its nodes' parents (for a node put elsewhere in a tick), the overlay's or the graph's."""
    source = read("source/interface/first_person_weapons.c").replace("\r\n", "\n")
    build_node_matrices = function(source, "first_person_weapon_build_node_matrices")
    assert "count_frame= weapon->weapon.magazines[0].rounds_loaded;" in build_node_matrices
    assert ("count_frame,\n\t\t\tcount_frame!=NONE ? count_nodes : NULL,\n"
            "\t\t\tcount_parents);") in build_node_matrices
    parents = build_node_matrices.index("if (count_frame==NONE)\n\t\t{\n\t\t\tfirst_person_weapon_node_parents(")
    assert parents < build_node_matrices.index("render_interpolation_first_person(")
