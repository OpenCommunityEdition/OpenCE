"""The real lightning_submit (lightning.c) over a fake renderer: a bolt keeps its shape through the frames of a tick,
following its markers where each frame draws them, and takes a new one with the next tick, as the Xbox drew it once a
tick; the frames after a tick's first leave the local random seed as they found it; each window's bolt is its own, and
a new lightning does not take the one before it's."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["same-shape-in-a-tick", "follows-its-markers", "seed-left-as-found", "windows-apart", "new-lightning",
         "tick-zero", "map-reset", "checkpoint-load", "multiple-lightnings", "failed-redraw", "failed-first-draw"]
FUNCTIONS = ("lightning_offset_marker_position", "lightnings_reset_render_state", "lightnings_initialize_for_new_map",
             "lightning_new", "lightning_submit")

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # a new shape each frame, as before
    "shape-each-frame": (("*local_seed = lightning_tick_seeds[seed_lightning][seed_window].seed;", ""),
                         "same-shape-in-a-tick"),
    # the later frames of a tick move the seed on as the first did
    "seed-moved-on": (("if (tick_drawn)\n\t\t\t*local_seed = frame_seed;", "if (tick_drawn)\n\t\t\tframe_seed = *local_seed;"),
                      "seed-left-as-found"),
    # one seed for every window
    "one-seed-for-the-windows": (("short seed_window = render.window_index;", "short seed_window = 0;"),
                                 "windows-apart"),
    # a new lightning draws the one before it's bolt for the rest of the tick
    "seed-of-the-one-before": (("sizeof(lightning_tick_seeds[0]));", "0);"), "new-lightning"),
    "reset-left-stale": (("csmemset(lightning_tick_seeds, 0, sizeof(lightning_tick_seeds));", ""), "map-reset"),
}


def generated(fault=None):
    source = read("source/objects/widgets/lightning.c")
    header = read("source/objects/widgets/lightning.h")
    seeds = re.search(r"^static struct\s*\{[^}]*\} lightning_tick_seeds\[[^;]*;", source, re.M)
    if not seeds:
        raise LookupError("lightning_tick_seeds not found in the sources")
    config = "\n".join([
        "enum { MAXIMUM_LIGHTNING_POINTS = %d, LIGHTNING_TICK_SEED_LIGHTNINGS = %d, LIGHTNING_TICK_SEED_WINDOWS = %d };"
        % (constant(source, "MAXIMUM_LIGHTNING_POINTS"), constant(source, "LIGHTNING_TICK_SEED_LIGHTNINGS"),
           constant(source, "LIGHTNING_TICK_SEED_WINDOWS")),
        enum_with(source, "_lightning_marker_not_connected_to_next_marker_bit"),
        structure(source, "lightning_marker_definition"),
        structure(source, "lightning_definition"),
        structure(source, "intermediate_lightning_point"),
        structure(source, "lightning_vertex"),
        structure(header, "lightning_globals"),
        structure(header, "lightning_datum"),
        seeds.group(0),
    ]) + "\n"
    code = "\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    # Execute the engine's actual post-load callback table and dispatcher;
    # only unrelated subsystems are stubbed in this renderer-only world.
    state = read("source/saved games/game_state.c")
    callbacks = re.search(r"static game_state_after_load_proc after_load_procs\[\] =\s*\{.*?\};", state, re.S).group(0)
    names = re.findall(r"^\s*(\w+)\s*,?\s*$", callbacks, re.M)
    after_load = "typedef void (*game_state_after_load_proc)(void);\n#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))\n"
    after_load += "\n".join(f"static void {name}(void) {{ }}" for name in names
                            if name != "lightnings_reset_render_state")
    after_load += "\n" + callbacks + "\n" + function(state, "game_state_call_after_load_procs")
    return (("config.inc", config), ("under_test.inc", code), ("after_load.inc", after_load))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("lightning_tick_shape", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("lightning_tick_shape", generated(fault)), case)
    assert status == CHECK_FAILED, f"the lightning with a fault ({control}) passed '{case}': the test cannot see it"


def test_checkpoint_hook_negative_control():
    includes = tuple((name, mutated(text, "\tlightnings_reset_render_state,", ""))
                     if name == "after_load.inc" else (name, text) for name, text in generated())
    status, output = run(build("lightning_tick_shape", includes), "checkpoint-load")
    assert status == CHECK_FAILED, output
