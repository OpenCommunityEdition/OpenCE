"""The game's own rules for a unit's animation states, taken from the sources
and run over every pair of states, to record what the native engine's must
give.

    python3 tools/engine_unit_state_reference.py          # writes the reference
    python3 tools/engine_unit_state_reference.py --check  # fails if it is stale

The functions (`unit_animation_state_interruptable`, `..._loops`,
`..._can_be_entered_without_animation`, the interpolation frame count, the
aiming screen of each state and whether an overlay action loops) are
source/units/units.c's, found by name as tools/harness finds them, and
compiled for this machine against a small fake world. The table is written
to engine/crates/sim/tests/unit_state_reference.txt, which
engine/crates/sim/tests/unit_state_reference.rs reads.
"""

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from harness import enum_with, function, read  # noqa: E402

OUTPUT = ROOT / "engine" / "crates" / "sim" / "tests" / "unit_state_reference.txt"

FUNCTIONS = [
    "unit_animation_state_interruptable",
    "unit_animation_state_loops",
    "unit_animation_state_can_be_entered_without_animation",
    "unit_animation_compute_interpolation_frame_count",
    "unit_animation_state_get_aiming_screen_index",
    "unit_animation_overlay_action_loops",
]

FAKE_WORLD = r"""
#include <stdio.h>

/* the game's 32-bit long */
#define long int

typedef int boolean;

#define TRUE 1
#define FALSE 0
#define NONE (-1)

struct unit_animation { char state; };
"""

MAIN = r"""
int main(void)
{
	int current;
	int desired;

	/* a state of none (-1) is the state of a unit whose seat just changed */
	for (current = -1; current<NUMBER_OF_UNIT_STATES; current++)
	{
		struct unit_animation animation;

		animation.state = (char)current;
		printf("state %d %d %d %d %d\n", current,
			unit_animation_state_loops(&animation),
			current<0 ? 1 : unit_animation_state_can_be_entered_without_animation((short)current),
			unit_animation_overlay_action_loops(&animation),
			unit_animation_state_get_aiming_screen_index((short)current));
		for (desired = 0; desired<NUMBER_OF_UNIT_STATES; desired++)
		{
			printf("pair %d %d %d %d\n", current, desired,
				unit_animation_state_interruptable(&animation, (short)desired),
				unit_animation_compute_interpolation_frame_count((short)desired, (short)current));
		}
	}
	return 0;
}
"""


def source_file():
    """The fake world, the game's functions and the cases, as one C file."""
    units_h = read("source/units/units.h")
    units_c = read("source/units/units.c")
    enums = [enum_with(units_h, member) for member in ("_unit_state_idle", "_unit_weapon_class_animation_idle")]
    definitions = [function(units_c, name) for name in FUNCTIONS]
    return "\n\n".join([FAKE_WORLD, *enums, *definitions, MAIN])


def reference():
    """The reference, as the game's code writes it."""
    with tempfile.TemporaryDirectory(prefix="unit-state-reference-") as work:
        source = Path(work) / "unit_state_reference.c"
        executable = Path(work) / "unit_state_reference"
        source.write_text(source_file())
        compiler = os.environ.get("CC", "clang")
        command = [compiler, "-std=gnu99", "-O1", "-w", str(source), "-o", str(executable)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(f"the reference does not compile:\n{result.stderr}")
        output = subprocess.run([str(executable)], capture_output=True, text=True, check=True).stdout
    return (
        "# The game's unit animation state rules (tools/engine_unit_state_reference.py).\n"
        "# state <state> <loops> <entered without animation> <overlay action loops> <aiming screen>\n"
        "# pair <current> <desired> <interruptable> <interpolation frames>\n" + output
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the written reference is not the code's")
    arguments = parser.parse_args()
    text = reference()
    if arguments.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != text:
            raise SystemExit(f"{OUTPUT.relative_to(ROOT)} is stale: run tools/engine_unit_state_reference.py")
        print("the unit state reference is the game's code's")
        return
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(text)
    print(f"wrote {OUTPUT.relative_to(ROOT)} ({len(text.splitlines())} lines)")


if __name__ == "__main__":
    main()
