"""The game's own item motion (`item_update`, `item_accelerate`), taken from
the sources and run over a few falls, throws and pushes, to record what the
native engine's must give.

    python3 tools/engine_item_reference.py          # writes the reference
    python3 tools/engine_item_reference.py --check  # fails if it is stale

The functions are source/items/items.c's (with the rotation and quaternion
helpers of source/math/real_math.c and the seeded random number of
source/math/random_math.c), found by name as tools/harness finds them, and
compiled for this machine against a small fake world: one item on one plane
through the origin (the structure's one surface), whose ground point marker is the item's own frame. Each
case prints the item after every tick. The table is written to
engine/crates/sim/tests/item_reference.txt, which
engine/crates/sim/tests/item_reference.rs reads.
"""

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from harness import function, read  # noqa: E402

OUTPUT = ROOT / "engine" / "crates" / "sim" / "tests" / "item_reference.txt"

ITEM_FUNCTIONS = [
    "item_rested_surface_index",
    "item_rested_surface",
    "item_set_rested_surface",
    "item_adjust_for_angular_velocity_change",
    "item_accelerate",
    "item_align_to_normal_and_point",
    "item_update",
]

FAKE_WORLD = r"""
#include <math.h>
#include <stdio.h>
#include <string.h>

/* the game's 32-bit long */
#define long int

typedef int boolean;
typedef float real;

#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define FLAG(bit) (1u<<(bit))
#define TEST_FLAG(flags, bit) ((((unsigned)(flags))>>(bit))&1)
#define SET_FLAG(flags, bit, value) ((value) ? ((flags)|= FLAG(bit)) : ((flags)&= ~FLAG(bit)))
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address+(index))
#define match_assert(file, line, condition) ((void)0)
#define profile_enter(section) ((void)0)
#define profile_exit(section) ((void)0)
#define MAXIMUM_COLLISION_USER_STACK_DEPTH 16
#define ITEM_UPDATE_COLLISION_TEST_FLAGS 0
#define RANDOM_A 1664525u
#define RANDOM_C 1013904223u
#define _real_epsilon 0.0001f

typedef struct { real i, j, k; } real_vector3d;
typedef struct { real x, y, z; } real_point3d;
typedef struct { real_vector3d v; real w; } real_quaternion;
typedef struct { real_vector3d n; real d; } real_plane3d;
typedef struct
{
	real scale;
	real_vector3d forward, left, up;
	real_point3d position;
} real_matrix4x3;

struct profile_section { char const *name; long a; boolean b; };
struct tag_block { long count; void *address; };
struct tag_reference { long index; };
struct location { long cluster_index; };
struct collision_surface { long plane_designator; unsigned char flags; short breakable_surface_index; };
struct collision_bsp { long bsp3d; struct tag_block surfaces; };
struct object_marker { real_matrix4x3 matrix; };
struct sound_location
{
	real_point3d position;
	real_vector3d forward;
	real_vector3d translational_velocity;
	struct location game_location;
};

enum { _collision_user_items };
enum { _collision_result_none, _collision_result_structure, _collision_result_object };
enum { _object_mask_scenery= 1, _object_mask_device= 2, _object_mask_biped= 4 };
enum { _object_connected_to_map_bit, _object_at_rest_bit };
enum
{
	_item_definition_always_maintains_z_up_bit,
	_item_definition_destroyed_by_explosions_bit,
	_item_definition_antigravity_bit
};
enum
{
	_item_on_structure_bit,
	_item_on_object_bit,
	_item_has_nonzero_angular_velocity_bit,
	_item_does_not_accelerate_bit,
	_item_attached_to_unit_bit
};
enum { _collision_surface_breakable_bit= 1 };

struct collision_result
{
	short type;
	real_point3d point;
	real_plane3d plane;
	struct location location;
	short material_type;
	long surface_index;
	long object_index;
};

struct item_definition
{
	struct
	{
		unsigned long flags;
		struct tag_reference material_effects;
		struct tag_reference collision_sound;
		struct tag_reference detonation_effect;
	} item;
};

struct item_datum
{
	long definition_index;
	struct
	{
		unsigned long flags;
		long parent_object_index;
		long owner_player_index;
		real_point3d position;
		real_vector3d forward;
		real_vector3d up;
		real_vector3d translational_velocity;
		real_vector3d angular_velocity;
		struct location location;
	} object;
	struct
	{
		unsigned long flags;
		long ignore_object_index;
		short rested_surface_index;
		short bsp_index;
		long item_on_rest_object_index;
		real_point3d item_rest_object_offset;
		real_vector3d rotation_axis;
		real rotation_sine;
		real rotation_cosine;
		short detonation_ticks;
		long last_owned_time;
	} item;
};

static real_vector3d const the_up= { 0.f, 0.f, 1.f };
static real_vector3d const the_down= { 0.f, 0.f, -1.f };
static real_vector3d const the_forward= { 1.f, 0.f, 0.f };
static real_vector3d const the_zero= { 0.f, 0.f, 0.f };
#define global_up3d (&the_up)
#define global_down3d (&the_down)
#define global_forward3d (&the_forward)
#define global_zero_vector3d (&the_zero)

static real global_gravity= 0.0035651792f;
static long global_current_collision_user_depth;
static long global_current_collision_users[MAXIMUM_COLLISION_USER_STACK_DEPTH];
static unsigned long the_seed;
static struct item_datum the_item;
static struct item_definition the_definition;
static int impacts, detonations;

static real dot_product3d(real_vector3d const *a, real_vector3d const *b)
{
	return a->i*b->i + a->j*b->j + a->k*b->k;
}

static real_vector3d *cross_product3d(real_vector3d const *a, real_vector3d const *b, real_vector3d *result)
{
	real_vector3d c;
	c.i= a->j*b->k - a->k*b->j;
	c.j= a->k*b->i - a->i*b->k;
	c.k= a->i*b->j - a->j*b->i;
	*result= c;
	return result;
}

static real magnitude_squared3d(real_vector3d const *v) { return dot_product3d(v, v); }
static real square_root(real x) { return sqrtf(x); }
static real magnitude3d(real_vector3d const *v) { return square_root(magnitude_squared3d(v)); }
static real sine(real x) { return sinf(x); }
static real cosine(real x) { return cosf(x); }

static real_vector3d *scale_vector3d(real_vector3d const *v, real s, real_vector3d *result)
{
	result->i= v->i*s;
	result->j= v->j*s;
	result->k= v->k*s;
	return result;
}

static real_vector3d *add_vectors3d(real_vector3d const *a, real_vector3d const *b, real_vector3d *result)
{
	result->i= a->i + b->i;
	result->j= a->j + b->j;
	result->k= a->k + b->k;
	return result;
}

static real normalize3d(real_vector3d *v)
{
	real magnitude= magnitude3d(v);

	if (!(_real_epsilon>fabs(magnitude-0.f)))
	{
		scale_vector3d(v, 1.f / magnitude, v);
	}
	else
	{
		magnitude= 0.f;
	}
	return magnitude;
}

static real_point3d *point_from_line3d(real_point3d const *p, real_vector3d const *v, real t, real_point3d *result)
{
	result->x= p->x + v->i*t;
	result->y= p->y + v->j*t;
	result->z= p->z + v->k*t;
	return result;
}

static void matrix4x3_from_point_and_vectors(real_matrix4x3 *matrix, real_point3d const *point,
	real_vector3d const *forward, real_vector3d const *up)
{
	matrix->scale= 1.f;
	matrix->forward= *forward;
	matrix->up= *up;
	cross_product3d(up, forward, &matrix->left);
	matrix->position= *point;
}

static boolean valid_real_matrix4x3(real_matrix4x3 const *matrix) { return TRUE; }

void quaternion_transform_point(real_quaternion const *q, real_point3d const *p, real_point3d *result);
real_vector3d *rotate_vector_about_axis(real_vector3d *v, real_vector3d const *n, real sine, real cosine);
real real_seed_random(unsigned long *seed);
static unsigned long *get_global_random_seed_address(void) { return &the_seed; }
static real real_random_range(real lower_bound, real upper_bound)
{
	return lower_bound + (upper_bound - lower_bound)*real_seed_random(&the_seed);
}
static real_vector3d *seed_random_direction3d(unsigned long *seed, real_vector3d *direction)
{
	/* (not reached by the cases: every push has a horizontal part) */
	*direction= the_up;
	return direction;
}

static struct item_datum *item_get(long item_index) { return &the_item; }
static struct item_definition *item_definition_get(long definition_index) { return &the_definition; }
static void *object_get(long object_index) { return &the_item; }
static void *object_try_and_get(long object_index) { return NULL; }
static short object_get_type(long object_index) { return 0; }
static real_matrix4x3 const *object_get_node_matrix(long object_index, short node) { return NULL; }
static void matrix4x3_transform_point(real_matrix4x3 const *m, real_point3d const *p, real_point3d *r) { *r= *p; }
static void matrix4x3_inverse_transform_point(real_matrix4x3 const *m, real_point3d const *p, real_point3d *r) { *r= *p; }
static struct collision_surface the_surface;
static struct collision_bsp the_bsp= { 0, { 1, &the_surface } };
static struct collision_bsp *global_collision_bsp_get(void) { return &the_bsp; }
static short global_structure_bsp_index_get(void) { return 0; }
static real_vector3d the_plane;
static void bsp3d_get_plane_from_designator(long *bsp, long designator, real_plane3d *plane)
{
	plane->n= the_plane;
	plane->d= 0.f;
}
static real plane3d_distance_to_point(real_plane3d const *plane, real_point3d const *point)
{
	return plane->n.i*point->x + plane->n.j*point->y + plane->n.k*point->z - plane->d;
}
static boolean breakable_surface_extant(short index) { return TRUE; }
static boolean game_engine_running(void) { return FALSE; }
static void object_set_garbage(long object_index, boolean garbage) { }
static void item_detonate(long item_index) { }
static boolean material_effect_visible(real_point3d const *point) { return TRUE; }
static void material_effect_new(long index, long count, short material, real_point3d const *point,
	real_vector3d const *normal, struct location const *location, real scale)
{
	printf(" impact %.9g %.9g %.9g %.9g", point->x, point->y, point->z, scale);
	impacts++;
}
static void unattached_impulse_sound_new(long index, struct sound_location const *location, real scale)
{
	printf(" sound %.9g %.9g %.9g", location->position.x, location->position.y, location->position.z);
}
static void effect_new_from_object(long a, long b, long c, long d, real e, real f, void *g, void *h)
{
	printf(" detonated");
	detonations++;
}
static void object_delete(long object_index) { }
static long game_time_get(void) { return 0; }

/* the marker is the item's own frame */
static boolean object_get_marker_by_name(long object_index, char const *name, struct object_marker *marker, short count)
{
	matrix4x3_from_point_and_vectors(&marker->matrix, &the_item.object.position,
		&the_item.object.forward, &the_item.object.up);
	return TRUE;
}

static void object_align_marker_to_matrix(void *object, struct object_marker const *marker, real_matrix4x3 const *matrix)
{
	the_item.object.position= matrix->position;
	the_item.object.forward= matrix->forward;
	the_item.object.up= matrix->up;
}

static void object_translate(long object_index, real_point3d const *position, struct location const *location)
{
	the_item.object.position= *position;
}

static real plane_distance(real_point3d const *point)
{
	return the_plane.i*point->x + the_plane.j*point->y + the_plane.k*point->z;
}

/* the plane through the origin, which is the structure */
static boolean collision_test_line(long flags, real_point3d const *from, real_point3d const *to,
	long ignore, struct collision_result *collision)
{
	real d0= plane_distance(from);
	real d1= plane_distance(to);
	real t;

	if (!(d0>=0.f && d1<0.f))
	{
		return FALSE;
	}
	t= d0/(d0-d1);
	memset(collision, 0, sizeof(*collision));
	collision->type= _collision_result_structure;
	collision->point.x= from->x + (to->x-from->x)*t;
	collision->point.y= from->y + (to->y-from->y)*t;
	collision->point.z= from->z + (to->z-from->z)*t;
	collision->plane.n= the_plane;
	collision->surface_index= 0;
	return TRUE;
}

static boolean collision_test_point(long flags, real_point3d const *point, long ignore)
{
	return plane_distance(point)<0.f;
}
"""

MAIN = r"""
static void print_vector(real_vector3d const *v) { printf(" %.9g %.9g %.9g", v->i, v->j, v->k); }

static void print_item(void)
{
	printf(" |");
	printf(" %.9g %.9g %.9g", the_item.object.position.x, the_item.object.position.y, the_item.object.position.z);
	print_vector(&the_item.object.translational_velocity);
	print_vector(&the_item.object.forward);
	print_vector(&the_item.object.up);
	print_vector(&the_item.object.angular_velocity);
	printf(" %d %d\n", (int)TEST_FLAG(the_item.object.flags, _object_at_rest_bit), the_item.item.detonation_ticks);
}

static void start(char const *name, unsigned long flags, real_vector3d plane, real_point3d position,
	real_vector3d velocity, real_vector3d forward, real_vector3d up, real_vector3d spin, short detonation_ticks)
{
	memset(&the_item, 0, sizeof(the_item));
	memset(&the_definition, 0, sizeof(the_definition));
	the_definition.item.flags= flags;
	the_definition.item.material_effects.index= 0;
	the_definition.item.collision_sound.index= 0;
	the_definition.item.detonation_effect.index= 0;
	the_item.object.flags= FLAG(_object_connected_to_map_bit);
	the_item.object.parent_object_index= NONE;
	the_item.object.owner_player_index= NONE;
	the_item.object.position= position;
	the_item.object.translational_velocity= velocity;
	the_item.object.forward= forward;
	the_item.object.up= up;
	the_item.object.angular_velocity= spin;
	the_item.item.ignore_object_index= NONE;
	the_item.item.rested_surface_index= NONE;
	the_item.item.detonation_ticks= detonation_ticks;
	the_item.item.rotation_cosine= 1.f;
	item_adjust_for_angular_velocity_change(0);
	the_plane= plane;
	normalize3d(&the_plane);
	the_seed= 0xDEADBEEFu;
	global_current_collision_user_depth= 1;
	printf("case %s %lu %.9g %.9g %.9g", name, (unsigned long)flags, the_plane.i, the_plane.j, the_plane.k);
	print_item();
}

static void run(int ticks)
{
	int tick;

	for (tick= 0; tick<ticks; tick++)
	{
		printf("tick");
		item_update(0);
		print_item();
	}
}

static void push(real_vector3d acceleration)
{
	printf("push %.9g %.9g %.9g", acceleration.i, acceleration.j, acceleration.k);
	item_accelerate(0, &acceleration, FALSE);
	print_item();
}

int main(void)
{
	real_vector3d const flat= { 0.f, 0.f, 1.f };
	real_vector3d const gentle= { 0.f, -0.5f, 0.8660254f };
	real_vector3d const steep= { 0.f, -0.8660254f, 0.5f };
	real_vector3d const still= { 0.f, 0.f, 0.f };
	real_vector3d const x= { 1.f, 0.f, 0.f };
	real_vector3d const z= { 0.f, 0.f, 1.f };
	real_point3d const high= { 0.f, 0.f, 1.f };
	unsigned long const z_up= FLAG(_item_definition_always_maintains_z_up_bit);
	unsigned long const antigravity= FLAG(_item_definition_antigravity_bit);

	start("dropped", 0, flat, high, still, x, z, still, 0);
	run(90);

	{
		real_point3d const p= { 0.f, 0.f, 0.5f };
		real_vector3d const v= { 0.03f, 0.01f, 0.02f };
		real_vector3d const w= { 0.f, 0.2f, 0.05f };
		start("thrown", 0, flat, p, v, x, z, w, 0);
		run(120);
	}

	{
		real_point3d const p= { 0.f, 0.f, 0.5f };
		real_vector3d const v= { 0.f, 0.f, -0.3f };
		start("fast", 0, flat, p, v, x, z, still, 0);
		run(60);
	}

	{
		real_point3d const p= { 0.f, 0.f, 0.3f };
		real_vector3d const f= { 0.8f, 0.f, 0.6f };
		real_vector3d const u= { -0.6f, 0.f, 0.8f };
		real_vector3d const w= { 0.05f, 0.f, 0.f };
		start("tilted", 0, flat, p, still, f, u, w, 0);
		run(60);
		start("upright", z_up, flat, p, still, f, u, w, 0);
		run(60);
	}

	start("gentle", 0, gentle, high, still, x, z, still, 0);
	run(90);
	start("steep", 0, steep, high, still, x, z, still, 0);
	run(60);

	{
		real_vector3d const v= { 0.01f, 0.f, 0.f };
		real_vector3d const w= { 0.f, 0.f, 0.1f };
		start("floating", antigravity, flat, high, v, x, z, w, 5);
		run(10);
	}

	{
		real_vector3d const kick= { 0.02f, 0.01f, 0.03f };
		real_vector3d const shove= { -0.05f, 0.f, 0.f };
		start("pushed", 0, flat, high, still, x, z, still, 0);
		run(90);
		push(kick);
		run(90);
		push(still);
		run(20);
		push(shove);
		run(60);
	}
	return 0;
}
"""


def source_file():
    """The fake world, the game's functions and the cases, as one C file."""
    items_c = read("source/items/items.c")
    real_math_c = read("source/math/real_math.c")
    random_math_c = read("source/math/random_math.c")
    gravity = items_c[items_c.index("static real const item_maximum_impact_velocity") :]
    gravity = gravity[: gravity.index(";") + 1]
    helpers = [
        function(real_math_c, "rotate_vector_about_axis"),
        function(real_math_c, "quaternion_transform_point"),
        function(random_math_c, "real_seed_random"),
    ]
    prototypes = "\n".join(
        [
            "void item_accelerate(long item_index, real_vector3d const *acceleration, boolean detonates_explosives);",
            "static void item_align_to_normal_and_point(long item_index, real_vector3d const *normal,"
            " real_point3d const *position, real_point3d *new_position);",
            "static void item_adjust_for_angular_velocity_change(long item_index);",
            "static struct profile_section item_update_section;",
        ]
    )
    definitions = [function(items_c, name) for name in ITEM_FUNCTIONS]
    return "\n\n".join([FAKE_WORLD, *helpers, gravity, prototypes, *definitions, MAIN])


def reference():
    """The reference, as the game's code writes it."""
    with tempfile.TemporaryDirectory(prefix="item-reference-") as work:
        source = Path(work) / "item_reference.c"
        executable = Path(work) / "item_reference"
        source.write_text(source_file())
        compiler = os.environ.get("CC", "clang")
        command = [compiler, "-std=gnu99", "-O1", "-ffp-contract=off", "-w", str(source), "-o", str(executable), "-lm"]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(f"the reference does not compile:\n{result.stderr}")
        output = subprocess.run([str(executable)], capture_output=True, text=True, check=True).stdout
    item = "<position> <velocity> <forward> <up> <angular velocity> <at rest> <detonation ticks>"
    return (
        "# The game's item motion (tools/engine_item_reference.py): one item over the plane through the\n"
        "# origin, its ground point marker its own frame, the seed 0xdeadbeef. Each line ends with the item\n"
        f"# after it: | {item}\n"
        "# case <name> <definition flags> <plane normal> | ...\n"
        "# tick [impact <point> <scale>] [sound <position>] [detonated] | ...\n"
        "# push <acceleration> | ...\n" + output
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the written reference is not the code's")
    arguments = parser.parse_args()
    text = reference()
    if arguments.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != text:
            raise SystemExit(f"{OUTPUT.relative_to(ROOT)} is stale: run tools/engine_item_reference.py")
        print("the item reference is the game's code's")
        return
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(text)
    print(f"wrote {OUTPUT.relative_to(ROOT)} ({len(text.splitlines())} lines)")


if __name__ == "__main__":
    main()
