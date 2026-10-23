"""The game's own rules for a first-person weapon's states, taken from the
sources and run over every state, message and weapon, to record what the
native engine's must give.

    python3 tools/engine_first_person_reference.py          # writes the reference
    python3 tools/engine_first_person_reference.py --check  # fails if it is stale

The functions (`first_person_weapon_set_state`, `..._message`,
`..._next_state`, `..._start_interpolation` and the two tables that map a
message to a state and a state to an animation) are
source/interface/first_person_weapons.c's, found by name as tools/harness
finds them, and compiled for this machine against a small fake world: one
weapon whose type, overheated flag, magazine and animation graph each case
sets. The table is written to
engine/crates/sim/tests/first_person_reference.txt, which
engine/crates/sim/tests/first_person_reference.rs reads.
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

OUTPUT = ROOT / "engine" / "crates" / "sim" / "tests" / "first_person_reference.txt"

FUNCTIONS = [
    "first_person_weapon_state_from_weapon_message", "first_person_animation_type_from_weapon_state",
    "first_person_weapon_start_interpolation", "first_person_weapon_set_state",
    "first_person_weapon_message", "first_person_weapon_next_state",
]

FAKE_WORLD = r"""
#include <stdio.h>
#include <string.h>

/* the game's 32-bit long */
#define long int

typedef int boolean;
typedef float real;
typedef unsigned short word;

#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define PIN(x, lo, hi) ((x)<(lo) ? (lo) : ((x)>(hi) ? (hi) : (x)))
#define TEST_FLAG(flags, bit) (((flags)>>(bit))&1)
#define MAXIMUM_NODES_PER_ANIMATION 64
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address+(index))
#define csmemcpy memcpy
#define match_assert(file, line, condition) ((void)0)

typedef struct { real i, j, k, w; } real_orientation;

struct tag_block { long count; void *address; };
struct tag_reference { long index; };
struct animation_state { short index; short frame_index; };
struct animation_graph_animation_index { short animation_index; };

struct animation_graph_first_person_weapon_animations { struct tag_block animations; };
struct animation_graph
{
	struct tag_block first_person_weapon_animations;
	struct tag_block nodes;
};

struct weapon_magazine_definition { short rounds_loaded_maximum; };
struct weapon_magazine { short state; short rounds_loaded; short rounds_total; };
struct weapon_datum
{
	long definition_index;
	struct { long flags; struct weapon_magazine magazines[1]; } weapon;
};
struct weapon_definition
{
	struct
	{
		short weapon_type;
		struct tag_block magazines;
		struct { struct { long index; } first_person_animations; } interface_definition;
	} weapon;
};

struct first_person_weapon
{
	long unit_index;
	long weapon_index;
	short state;
	struct animation_state state_animation;
	real firing_push_back_velocity;
	short interpolation_frame_index;
	short interpolation_frame_count;
	real_orientation node_orientations[MAXIMUM_NODES_PER_ANIMATION];
	real_orientation original_node_orientations[MAXIMUM_NODES_PER_ANIMATION];
	boolean shotgun_empty;
	short shotgun_shells_to_reload;
	short shotgun_reload_type;
	long current_sound_index;
	short current_sound_state;
};

static struct first_person_weapon the_first_person_weapon;
static struct weapon_datum the_weapon;
static struct weapon_magazine_definition the_magazine_definition;
static struct weapon_definition the_weapon_definition;
static struct animation_graph_animation_index the_animations[32];
static struct animation_graph_first_person_weapon_animations the_first_person_animations;
static struct animation_graph the_graph;
static int sounds_stopped, weapons_forgotten, weapons_switched, unzooms;

static struct first_person_weapon *first_person_weapon_get(short local_player_index) { return &the_first_person_weapon; }
static struct weapon_datum *weapon_get(long weapon_index) { return &the_weapon; }
static struct weapon_definition *weapon_definition_get(long definition_index) { return &the_weapon_definition; }
static struct animation_graph *animation_graph_definition_get(long graph_index) { return &the_graph; }
static struct animation_graph_animation_index *animation_graph_animation_index_get(struct tag_block *block)
{
	return (struct animation_graph_animation_index *)block->address;
}
static void sound_stop_impulse(long sound_index) { sounds_stopped++; }
static void first_person_weapon_forget_weapon(short local_player_index) { weapons_forgotten++; }
static void first_person_weapon_switch_weapons(short local_player_index) { weapons_switched++; }
static void player_control_unzoom(long unit_index) { unzooms++; }
"""

MAIN = r"""
#define NUMBER_OF_ANIMATIONS NUMBER_OF_FIRST_PERSON_WEAPON_ANIMATIONS

/* a graph of every animation, of the first 17 (the older weapons'), or of every one but posing */
static int const graph_counts[]= { NUMBER_OF_ANIMATIONS, 17, NUMBER_OF_ANIMATIONS };

static void world(int weapon_type, int overheated, int graph, int state)
{
	int animation;

	memset(&the_first_person_weapon, 0, sizeof(the_first_person_weapon));
	the_first_person_weapon.unit_index= 1;
	the_first_person_weapon.weapon_index= 2;
	the_first_person_weapon.state= (short)state;
	the_first_person_weapon.state_animation.index= NONE;
	the_first_person_weapon.state_animation.frame_index= 5;
	the_first_person_weapon.current_sound_index= 7;
	the_first_person_weapon.current_sound_state= NONE;
	the_first_person_weapon.shotgun_reload_type= NONE;
	the_weapon.definition_index= 3;
	the_weapon.weapon.flags= overheated ? 1<<_weapon_overheated_bit : 0;
	the_weapon_definition.weapon.weapon_type= (short)weapon_type;
	the_weapon_definition.weapon.magazines.count= 1;
	the_weapon_definition.weapon.magazines.address= &the_magazine_definition;
	for (animation= 0; animation<NUMBER_OF_ANIMATIONS; animation++)
	{
		the_animations[animation].animation_index= (short)(100+animation);
	}
	if (graph==2)
	{
		the_animations[_first_person_weapon_animation_posing].animation_index= NONE;
	}
	the_first_person_animations.animations.count= graph_counts[graph];
	the_first_person_animations.animations.address= the_animations;
	the_graph.first_person_weapon_animations.count= 1;
	the_graph.first_person_weapon_animations.address= &the_first_person_animations;
	the_graph.nodes.count= 0;
	sounds_stopped= weapons_forgotten= weapons_switched= unzooms= 0;
}

static void print_weapon(void)
{
	printf(" %d %d %d %d %d %d %d %d\n",
		the_first_person_weapon.state,
		the_first_person_weapon.state_animation.index,
		the_first_person_weapon.state_animation.frame_index,
		the_first_person_weapon.interpolation_frame_count,
		sounds_stopped,
		the_first_person_weapon.shotgun_reload_type,
		the_first_person_weapon.shotgun_shells_to_reload,
		the_first_person_weapon.shotgun_empty);
}

/* the shotgun's magazines: rounds loaded, rounds in reserve, its capacity and its state */
static int const magazines[][4]= { {0, 10, 6, 0}, {5, 10, 6, 0}, {3, 0, 6, 0}, {5, 1, 6, 0}, {2, 10, 6, 1}, {0, 1, 6, 0} };

/* the weapon types these functions tell apart: the needler and plasma rifle act as an undefined type does */
static int const weapon_types[]= { _weapon_type_undefined, _weapon_type_shotgun, _weapon_type_plasma_pistol };
#define NUMBER_OF_CASE_WEAPON_TYPES ((int)(sizeof(weapon_types)/sizeof(weapon_types[0])))

int main(void)
{
	int type, weapon_type, overheated, graph, state, new_state, message, magazine, reload_type, empty;

	for (type= 0; type<NUMBER_OF_CASE_WEAPON_TYPES && (weapon_type= weapon_types[type], 1); type++)
	for (overheated= 0; overheated<2; overheated++)
	for (graph= 0; graph<3; graph++)
	for (state= -1; state<NUMBER_OF_FIRST_PERSON_WEAPON_STATES; state++)
	for (new_state= 0; new_state<NUMBER_OF_FIRST_PERSON_WEAPON_STATES; new_state++)
	{
		world(weapon_type, overheated, graph, state);
		first_person_weapon_set_state(0, (short)new_state, TRUE);
		printf("set %d %d %d %d %d", weapon_type, overheated, graph, state, new_state);
		print_weapon();
	}

	for (type= 0; type<NUMBER_OF_CASE_WEAPON_TYPES && (weapon_type= weapon_types[type], 1); type++)
	for (overheated= 0; overheated<2; overheated++)
	for (state= 0; state<NUMBER_OF_FIRST_PERSON_WEAPON_STATES; state++)
	for (reload_type= -1; reload_type<3; reload_type++)
	for (empty= 0; empty<2; empty++)
	{
		world(weapon_type, overheated, 0, state);
		the_first_person_weapon.shotgun_reload_type= (short)reload_type;
		the_first_person_weapon.shotgun_empty= empty;
		first_person_weapon_next_state(0);
		printf("next %d %d %d %d %d", weapon_type, overheated, state, reload_type, empty);
		print_weapon();
	}

	for (type= 0; type<NUMBER_OF_CASE_WEAPON_TYPES && (weapon_type= weapon_types[type], 1); type++)
	for (overheated= 0; overheated<2; overheated++)
	for (magazine= 0; magazine<(int)(sizeof(magazines)/sizeof(magazines[0])); magazine++)
	for (state= -1; state<NUMBER_OF_FIRST_PERSON_WEAPON_STATES; state++)
	for (message= 0; message<NUMBER_OF_FIRST_PERSON_WEAPON_MESSAGES; message++)
	{
		if (weapon_type!=_weapon_type_shotgun && magazine)
		{
			continue;
		}
		world(weapon_type, overheated, 0, state);
		the_weapon.weapon.magazines[0].rounds_loaded= (short)magazines[magazine][0];
		the_weapon.weapon.magazines[0].rounds_total= (short)magazines[magazine][1];
		the_magazine_definition.rounds_loaded_maximum= (short)magazines[magazine][2];
		the_weapon.weapon.magazines[0].state= (short)magazines[magazine][3];
		the_first_person_weapon.interpolation_frame_count= 4;
		the_first_person_weapon.interpolation_frame_index= 2;
		first_person_weapon_message(0, (short)message);
		printf("message %d %d %d %d %d %d %d %d %d %d %d",
			weapon_type, overheated, magazines[magazine][0], magazines[magazine][1], magazines[magazine][2],
			magazines[magazine][3], state, message, weapons_forgotten, weapons_switched, unzooms);
		printf(" %d", the_first_person_weapon.firing_push_back_velocity>0.0f);
		print_weapon();
	}
	return 0;
}
"""


def source_file():
    """The fake world, the game's functions and the cases, as one C file."""
    weapons_h = read("source/items/weapons.h")
    flags_h = read("source/items/weapon_datum_flags.h")
    first_person_c = read("source/interface/first_person_weapons.c")
    enums = [enum_with(weapons_h, member) for member in
             ("_first_person_weapon_message_primary_fire", "_weapon_type_undefined",
              "_shotgun_reload_type_first_round", "_first_person_weapon_animation_idle")]
    enums += [enum_with(flags_h, "_weapon_overheated_bit"),
              enum_with(first_person_c, "_first_person_weapon_state_idle")]
    push_back = next(line for line in first_person_c.splitlines()
                     if line.startswith("#define FIRST_PERSON_WEAPON_FIRING_PUSH_BACK_VELOCITY"))
    definitions = [function(first_person_c, name) for name in FUNCTIONS]
    return "\n\n".join([*enums, push_back, FAKE_WORLD, *definitions, MAIN])


def reference():
    """The reference, as the game's code writes it."""
    with tempfile.TemporaryDirectory(prefix="first-person-reference-") as work:
        source = Path(work) / "first_person_reference.c"
        executable = Path(work) / "first_person_reference"
        source.write_text(source_file())
        compiler = os.environ.get("CC", "clang")
        command = [compiler, "-std=gnu99", "-O1", "-w", str(source), "-o", str(executable)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(f"the reference does not compile:\n{result.stderr}")
        output = subprocess.run([str(executable)], capture_output=True, text=True, check=True).stdout
    weapon = ("<state> <animation> <frame> <interpolation frames> <sounds stopped>"
              " <shotgun reload type> <shells to reload> <shotgun empty>")
    return ("# The game's first-person weapon state rules (tools/engine_first_person_reference.py).\n"
            "# Animation n of the weapon's first-person block is the graph's animation 100+n; graph 1 has\n"
            "# only the first 17, graph 2 has every one but posing. Each line ends with the weapon after it:\n"
            f"# {weapon}\n"
            "# set <weapon type> <overheated> <graph> <state> <new state> ...\n"
            "# next <weapon type> <overheated> <state> <shotgun reload type> <shotgun empty> ...\n"
            "# message <weapon type> <overheated> <rounds loaded> <rounds total> <rounds maximum>"
            " <magazine state> <state> <message> <forgot> <switched> <unzoomed> <pushed back> ...\n"
            + output)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the written reference is not the code's")
    arguments = parser.parse_args()
    text = reference()
    if arguments.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != text:
            raise SystemExit(f"{OUTPUT.relative_to(ROOT)} is stale: run tools/engine_first_person_reference.py")
        print("the first-person reference is the game's code's")
        return
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(text)
    print(f"wrote {OUTPUT.relative_to(ROOT)} ({len(text.splitlines())} lines)")


if __name__ == "__main__":
    main()
