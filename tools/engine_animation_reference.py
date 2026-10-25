"""The game's own animation code, taken from the sources and run on made-up
animations, to record what the native engine's animation must give.

    python3 tools/engine_animation_reference.py          # writes the reference
    python3 tools/engine_animation_reference.py --check  # fails if it is stale

The functions (`animation_get_node_orientations`, the replacement and
overlay animations, blending, the node matrices and the maths under them)
are the decompilation's, found by name as tools/harness finds them, and
compiled for this machine against a small fake world with fused
multiply-add off, as every build of the game is. The animations are made
up here, not a map's, so the reference is the project's own. It is written
to engine/crates/sim/tests/animation_reference.txt, which
engine/crates/sim/tests/animation_reference.rs reads.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from harness import function, inline, read, structure  # noqa: E402

OUTPUT = ROOT / "engine" / "crates" / "sim" / "tests" / "animation_reference.txt"

MATH_FUNCTIONS = [
    (
        "source/math/real_math.h",
        ["square_root", "reciprocal_square_root", "cross_product3d", "set_real_quaternion", "set_real_point3d"],
        True,
    ),
    (
        "source/math/real_math.c",
        [
            "quaternions_multiply",
            "quaternions_interpolate",
            "quaternion_normalize",
            "quaternions_interpolate_and_normalize",
            "points_interpolate",
            "scalars_interpolate",
        ],
        False,
    ),
    (
        "source/math/matrix_math.c",
        [
            "matrix4x3_rotation_from_quaternion",
            "matrix4x3_from_orientation",
            "matrix4x3_rotation_from_vectors",
            "matrix4x3_from_point_and_vectors",
            "matrix4x3_multiply",
        ],
        False,
    ),
]

ANIMATION_FUNCTIONS = [
    ("source/models/model_animation_definitions.c", ["animation_get_frame_data"]),
    ("source/models/models.c", ["model_get_node_orientations"]),
    (
        "source/models/model_animations.c",
        [
            "quaternion_decompress_8byte",
            "quaternion_decompress_6byte",
            "animation_is_compressed",
            "animation_keyframe_search",
            "animation_get_keyframe_rotation",
            "animation_get_keyframe_translation",
            "animation_get_keyframe_scale",
            "animation_frame_data_offset",
            "animation_data_contains",
            "animation_node_flag_count",
            "animation_frame_valid",
            "animation_data_error",
            "animation_set_rest_orientations",
            "animation_get_node_orientations",
            "replacement_animation_apply",
            "overlay_animation_apply",
            "overlay_animation_apply_scaled",
            "overlay_animation_apply_continuous",
            "aiming_screen_apply",
            "interpolate_node_orientations",
            "animation_graph_node_matrices_from_orientations",
        ],
    ),
]

FAKE_WORLD = r"""
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* the game's 32-bit long */
#define long int

typedef float real;
typedef unsigned short word;
typedef unsigned char byte;
typedef int boolean;

#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define SHORT_MAX 32767
#define UNSIGNED_SHORT_MAX 65535
#define LONG_BITS 32
#define LONG_BITS_BITS 5
#define TAG_STRING_LENGTH 31
#define FLAG(bit) (1u<<(bit))
#define TEST_FLAG(flags, bit) (((flags)&FLAG(bit))!=0)
#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define CEILING(n, ceiling) ((n)>(ceiling) ? (ceiling) : (n))
#define PIN(n, floor, ceiling) ((n)<(floor) ? (floor) : CEILING((n), (ceiling)))
#define VALID_INDEX(index, count) ((index)>=0 && (index)<(count))
#define match_assert(file, line, condition) ((void)0)
#define match_vassert(file, line, condition, message) ((void)0)
#define fast_ftol(value) ((long)(value))
#define model_data_report_once(definition) 0
#define error(...) ((void)0)
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address+(index))

struct tag_block { long count; void *address; void *definition; };
struct tag_data { long size; unsigned long pad; long file_offset; void *address; void *definition; };

static void *tag_data_get_pointer(struct tag_data const *data, long offset, long size)
{
	(void)size;
	return (byte *)data->address+offset;
}

enum { _animation_compressed_bit = 0 };
enum { _animation_base = 0, _animation_overlay, _animation_replacement };
enum { MAXIMUM_NODES_PER_ANIMATION = 64, MAXIMUM_NODES_PER_MODEL = 64 };
enum { COMPRESSED_QUATERNION_COMPONENT_MAXIMUM = 32767 };
enum { COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS = 12 };

static boolean hs_model_animation_compression_enabled = TRUE;
"""

MAIN = r"""
/* ---------- the made-up animations */

static unsigned int seed = 0x12345678;

static unsigned int next_random(void)
{
	seed = seed*1664525u+1013904223u;
	return seed>>8;
}

static real random_real(real lower, real upper)
{
	return lower+(upper-lower)*(real)(next_random()&0xFFFF)/65535.f;
}

static short random_short(void)
{
	return (short)(next_random()&0xFFFF);
}

#define NODE_COUNT 5
#define ROTATION_FLAGS 0x16u     /* nodes 1, 2, 4 */
#define TRANSLATION_FLAGS 0x0Bu  /* nodes 0, 1, 3 */
#define SCALE_FLAGS 0x04u        /* node 2 */
#define COMPRESSED_FRAMES 10
#define UNCOMPRESSED_FRAMES 4

static byte uncompressed_data[1024];
static byte uncompressed_defaults[256];
static byte compressed_data[2048];

static void put_short(byte **at, short value) { memcpy(*at, &value, 2); *at += 2; }
static void put_word(byte **at, word value) { memcpy(*at, &value, 2); *at += 2; }
static void put_long(byte **at, unsigned long value) { memcpy(*at, &value, 4); *at += 4; }
static void put_real(byte **at, real value) { memcpy(*at, &value, 4); *at += 4; }
static void put_quaternion_8byte(byte **at) { int index; for (index = 0; index<4; index++) put_short(at, random_short()); }
static void put_quaternion_6byte(byte **at) { int index; for (index = 0; index<3; index++) put_short(at, random_short()); }
static void put_point(byte **at) { int index; for (index = 0; index<3; index++) put_real(at, random_real(-2.f, 2.f)); }
static void align(byte **at, byte *base) { while ((*at-base)&3) *(*at)++ = 0xA5; }

static long make_uncompressed(struct animation *animation)
{
	byte *at = uncompressed_data;
	byte *defaults = uncompressed_defaults;
	int frame, node;

	memset(animation, 0, sizeof(*animation));
	animation->frame_count = UNCOMPRESSED_FRAMES;
	animation->node_count = NODE_COUNT;
	animation->nodes_with_rotation_flags[0] = ROTATION_FLAGS;
	animation->nodes_with_translation_flags[0] = TRANSLATION_FLAGS;
	animation->nodes_with_scale_flags[0] = SCALE_FLAGS;
	for (frame = 0; frame<UNCOMPRESSED_FRAMES; frame++)
	{
		for (node = 0; node<NODE_COUNT; node++)
		{
			if (TEST_FLAG(ROTATION_FLAGS, node)) put_quaternion_8byte(&at);
			if (TEST_FLAG(TRANSLATION_FLAGS, node)) put_point(&at);
			if (TEST_FLAG(SCALE_FLAGS, node)) put_real(&at, random_real(0.5f, 1.5f));
		}
		if (!frame) animation->frame_size = (short)(at-uncompressed_data);
	}
	for (node = 0; node<NODE_COUNT; node++)
	{
		if (!TEST_FLAG(ROTATION_FLAGS, node)) put_quaternion_8byte(&defaults);
		if (!TEST_FLAG(TRANSLATION_FLAGS, node)) put_point(&defaults);
		if (!TEST_FLAG(SCALE_FLAGS, node)) put_real(&defaults, random_real(0.5f, 1.5f));
	}
	animation->data.address = uncompressed_data;
	animation->data.size = (long)(at-uncompressed_data);
	animation->default_data.address = uncompressed_defaults;
	animation->default_data.size = (long)(defaults-uncompressed_defaults);
	return 0;
}

/* each channel's keyframes, by adjusted index: their frames (a count of 0
is none, the node's default) */
static int const rotation_keyframes[3][4] = { {3, 2, 5, 9}, {0}, {2, 4, 7} };
static int const translation_keyframes[3][4] = { {2, 1, 9}, {3, 3, 6, 9}, {0} };
static int const scale_keyframes[1][4] = { {2, 5, 9} };

static void put_channel(
	byte **at,
	byte *base,
	int const (*keyframes)[4],
	int adjusted_count,
	int default_count,
	int value_size,
	long *node_headers,
	long *frame_indices,
	long *defaults,
	long *values,
	byte *rotation_node_headers)
{
	int adjusted, keyframe, first;

	if (node_headers)
	{
		*node_headers = (long)(*at-base);
	}
	for (adjusted = 0, first = 0; adjusted<adjusted_count; adjusted++)
	{
		unsigned long header = ((unsigned long)first<<COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS)|keyframes[adjusted][0];
		byte *header_at = rotation_node_headers ? rotation_node_headers+adjusted*4 : *at;

		memcpy(header_at, &header, 4);
		if (!rotation_node_headers) *at += 4;
		first += keyframes[adjusted][0];
	}
	align(at, base);
	*frame_indices = (long)(*at-base);
	for (adjusted = 0; adjusted<adjusted_count; adjusted++)
	{
		for (keyframe = 0; keyframe<keyframes[adjusted][0]; keyframe++)
		{
			put_word(at, (word)keyframes[adjusted][1+keyframe]);
		}
	}
	align(at, base);
	*defaults = (long)(*at-base);
	for (adjusted = 0; adjusted<default_count; adjusted++)
	{
		if (value_size==6) put_quaternion_6byte(at);
		else if (value_size==12) put_point(at);
		else put_real(at, random_real(0.5f, 1.5f));
	}
	align(at, base);
	*values = (long)(*at-base);
	for (adjusted = 0; adjusted<adjusted_count; adjusted++)
	{
		for (keyframe = 0; keyframe<keyframes[adjusted][0]; keyframe++)
		{
			if (value_size==6) put_quaternion_6byte(at);
			else if (value_size==12) put_point(at);
			else put_real(at, random_real(0.5f, 1.5f));
		}
	}
	align(at, base);
}

static void make_compressed(struct animation *animation)
{
	byte *base = compressed_data+16;
	struct compressed_animation_header header;
	byte *at = base+offsetof(struct compressed_animation_header, rotation_node_headers)+3*4;

	memset(animation, 0, sizeof(*animation));
	memset(compressed_data, 0x5A, sizeof(compressed_data));
	animation->frame_count = COMPRESSED_FRAMES;
	animation->node_count = NODE_COUNT;
	animation->flags = FLAG(_animation_compressed_bit);
	animation->compressed_data_offset = 16;
	animation->nodes_with_rotation_flags[0] = ROTATION_FLAGS;
	animation->nodes_with_translation_flags[0] = TRANSLATION_FLAGS;
	animation->nodes_with_scale_flags[0] = SCALE_FLAGS;
	put_channel(&at, base, rotation_keyframes, 3, NODE_COUNT, 6, NULL,
		&header.rotation_keyframe_frame_indices_offset, &header.default_rotations_offset,
		&header.rotation_keyframes_offset, base+offsetof(struct compressed_animation_header, rotation_node_headers));
	put_channel(&at, base, translation_keyframes, 3, NODE_COUNT, 12, &header.translation_node_headers_offset,
		&header.translation_keyframe_frame_indices_offset, &header.default_translations_offset,
		&header.translation_keyframes_offset, NULL);
	put_channel(&at, base, scale_keyframes, 1, 1, 4, &header.scale_node_headers_offset,
		&header.scale_keyframe_frame_indices_offset, &header.default_scales_offset,
		&header.scale_keyframes_offset, NULL);
	memcpy(base, &header, offsetof(struct compressed_animation_header, rotation_node_headers));
	animation->data.address = compressed_data;
	animation->data.size = (long)(at-compressed_data);
}

/* ---------- what is written */

static void print_hex(char const *label, char const *name, byte const *bytes, long size)
{
	long index;

	printf("%s %s ", label, name);
	for (index = 0; index<size; index++) printf("%02x", bytes[index]);
	printf("\n");
}

static void print_animation(char const *name, struct animation const *animation)
{
	printf("animation %s %d %d %d %d %u %d %u %u %u\n", name, animation->type, animation->frame_count,
		animation->frame_size, animation->node_count, animation->flags, animation->compressed_data_offset,
		animation->nodes_with_rotation_flags[0], animation->nodes_with_translation_flags[0],
		animation->nodes_with_scale_flags[0]);
	print_hex("data", name, animation->data.address, animation->data.size);
	print_hex("default", name, animation->default_data.address, animation->default_data.size);
}

static void print_words(real const *values, int count)
{
	int index;

	for (index = 0; index<count; index++)
	{
		unsigned int bits;

		memcpy(&bits, &values[index], 4);
		printf(" %08x", bits);
	}
}

static void print_orientations(real_orientation const *orientations, int count)
{
	int node;

	for (node = 0; node<count; node++)
	{
		real values[8];

		values[0] = orientations[node].rotation.v.i;
		values[1] = orientations[node].rotation.v.j;
		values[2] = orientations[node].rotation.v.k;
		values[3] = orientations[node].rotation.w;
		values[4] = orientations[node].translation.x;
		values[5] = orientations[node].translation.y;
		values[6] = orientations[node].translation.z;
		values[7] = orientations[node].scale;
		print_words(values, 8);
	}
	printf("\n");
}

static void print_matrix(real_matrix4x3 const *matrix)
{
	real values[13];

	memcpy(values, matrix->n, sizeof(real)*12);
	values[12] = matrix->scale;
	print_words(values, 13);
}

static void make_pose(real_orientation *pose)
{
	int node;

	for (node = 0; node<NODE_COUNT; node++)
	{
		set_real_quaternion(&pose[node].rotation, random_real(-1.f, 1.f), random_real(-1.f, 1.f),
			random_real(-1.f, 1.f), random_real(-1.f, 1.f));
		quaternion_normalize(&pose[node].rotation);
		set_real_point3d(&pose[node].translation, random_real(-1.f, 1.f), random_real(-1.f, 1.f),
			random_real(-1.f, 1.f));
		pose[node].scale = random_real(0.75f, 1.25f);
	}
}

static void each_frame(char const *name, struct animation *animation, real_orientation const *pose, char const *pose_name)
{
	short frame;

	for (frame = -1; frame<=animation->frame_count; frame++)
	{
		real_orientation nodes[NODE_COUNT];

		memcpy(nodes, pose, sizeof(nodes));
		if (animation->type==_animation_base)
		{
			if (frame<0 || frame>=animation->frame_count) continue;
			animation_get_node_orientations(NULL, animation, frame, nodes);
			printf("base %s %d", name, frame);
		}
		else if (animation->type==_animation_replacement)
		{
			replacement_animation_apply(animation, frame, nodes);
			printf("replace %s %s %d", name, pose_name, frame);
		}
		else
		{
			overlay_animation_apply(animation, frame, nodes);
			printf("overlay %s %s %d", name, pose_name, frame);
		}
		print_orientations(nodes, NODE_COUNT);
	}
}

int main(void)
{
	struct animation uncompressed;
	struct animation compressed;
	real_orientation pose[NODE_COUNT];
	real_orientation other[NODE_COUNT];
	short frame;

	make_pose(pose);
	make_pose(other);
	printf("pose a");
	print_orientations(pose, NODE_COUNT);
	printf("pose b");
	print_orientations(other, NODE_COUNT);

	make_uncompressed(&uncompressed);
	make_compressed(&compressed);
	print_animation("uncompressed", &uncompressed);
	print_animation("compressed", &compressed);

	uncompressed.type = compressed.type = _animation_base;
	each_frame("uncompressed", &uncompressed, pose, "a");
	each_frame("compressed", &compressed, pose, "a");
	uncompressed.type = compressed.type = _animation_replacement;
	each_frame("uncompressed", &uncompressed, pose, "a");
	each_frame("compressed", &compressed, pose, "a");
	uncompressed.type = compressed.type = _animation_overlay;
	each_frame("uncompressed", &uncompressed, pose, "a");
	each_frame("compressed", &compressed, pose, "a");

	/* compressed data cut short: its header holds, some of what it points
	to is past its end */
	{
		struct animation truncated = compressed;
		short sizes[] = { 16+0x20, 16+0x40, 16+0x80, 16+0xC0 };
		int index;

		truncated.type = _animation_base;
		for (index = 0; index<(int)(sizeof(sizes)/sizeof(sizes[0])); index++)
		{
			char name[32];

			truncated.data.size = sizes[index];
			snprintf(name, sizeof(name), "truncated%d", sizes[index]);
			print_animation(name, &truncated);
			each_frame(name, &truncated, pose, "a");
		}
	}

	/* overlays at a weight, between frames, and over an aiming screen */
	{
		struct animation *both[2];
		char const *names[2] = { "uncompressed", "compressed" };
		real const scales[] = { 0.f, 0.25f, 0.5f, 1.f };
		real const frames[] = { -0.5f, 0.f, 0.25f, 1.5f, 2.75f, 3.f, 3.5f, 8.125f, 9.f, 9.5f, 10.f, 12.f };
		real const angles[] = { -1.2f, -0.45f, -0.3f, -0.1f, 0.f, 0.05f, 0.3f, 0.42f, 1.f };
		/* a 3 by 3 screen on the compressed animation's 10 frames, 2 by 2
		on the uncompressed one's 4 */
		struct animation_aiming_screen_bounds screens[2] = {
			{ 0.3f, 0.3f, 0, 1, 0.25f, 0.25f, 0, 1 },
			{ 0.3f, 0.4f, 1, 1, 0.2f, 0.35f, 1, 1 } };
		int which, index, other_index;

		both[0] = &uncompressed;
		both[1] = &compressed;
		for (which = 0; which<2; which++)
		{
			struct animation *animation = both[which];

			animation->type = _animation_overlay;
			for (frame = -1; frame<=animation->frame_count; frame++)
			{
				for (index = 0; index<(int)(sizeof(scales)/sizeof(scales[0])); index++)
				{
					real_orientation nodes[NODE_COUNT];

					memcpy(nodes, pose, sizeof(nodes));
					overlay_animation_apply_scaled(animation, frame, scales[index], nodes);
					printf("scaled %s a %d %08x", names[which], frame, *(unsigned int *)&scales[index]);
					print_orientations(nodes, NODE_COUNT);
				}
			}
			for (index = 0; index<(int)(sizeof(frames)/sizeof(frames[0])); index++)
			{
				real_orientation nodes[NODE_COUNT];

				memcpy(nodes, pose, sizeof(nodes));
				overlay_animation_apply_continuous(animation, frames[index], nodes);
				printf("continuous %s a %08x", names[which], *(unsigned int *)&frames[index]);
				print_orientations(nodes, NODE_COUNT);
			}
			for (index = 0; index<(int)(sizeof(angles)/sizeof(angles[0])); index++)
			{
				for (other_index = 0; other_index<(int)(sizeof(angles)/sizeof(angles[0])); other_index++)
				{
					struct animation_aiming_screen_bounds const *bounds = &screens[which];
					real_orientation nodes[NODE_COUNT];

					memcpy(nodes, pose, sizeof(nodes));
					aiming_screen_apply(animation, bounds, angles[index], angles[other_index], nodes);
					printf("aiming %s a %08x %08x %08x %08x %d %d %08x %08x %d %d", names[which],
						*(unsigned int *)&angles[index], *(unsigned int *)&angles[other_index],
						*(unsigned int *)&bounds->negative_yaw_delta, *(unsigned int *)&bounds->positive_yaw_delta,
						bounds->negative_yaw_frame_count, bounds->positive_yaw_frame_count,
						*(unsigned int *)&bounds->negative_pitch_delta, *(unsigned int *)&bounds->positive_pitch_delta,
						bounds->negative_pitch_frame_count, bounds->positive_pitch_frame_count);
					print_orientations(nodes, NODE_COUNT);
				}
			}
		}
	}

	for (frame = 0; frame<4; frame++)
	{
		real_orientation target[NODE_COUNT];

		memcpy(target, other, sizeof(target));
		interpolate_node_orientations(NODE_COUNT, pose, target, frame, 4);
		printf("blend a b %d 4", frame);
		print_orientations(target, NODE_COUNT);
	}

	/* the graph: 0 has 1 and 3, 1 has 2, 3 has 4, whose parent is one the
	graph doesn't have */
	{
		static struct animation_graph_node graph_nodes[NODE_COUNT];
		static short const links[NODE_COUNT][3] = {
			/* parent, first child, next sibling */
			{ NONE, 1, NONE }, { 0, 2, 3 }, { 1, NONE, NONE }, { 0, 4, NONE }, { 9, NONE, NONE } };
		real_matrix4x3 matrices[NODE_COUNT];
		real_point3d origin = { { 1.f, 2.f, 3.f } };
		real_vector3d forward = { { 0.6f, 0.8f, 0.f } };
		real_vector3d up = { { 0.f, 0.f, 1.f } };
		real_orientation nodes[NODE_COUNT];
		int node;

		for (node = 0; node<NODE_COUNT; node++)
		{
			graph_nodes[node].parent_node_index = links[node][0];
			graph_nodes[node].first_child_node_index = links[node][1];
			graph_nodes[node].next_sibling_node_index = links[node][2];
		}
		fake_graph.nodes.count = NODE_COUNT;
		fake_graph.nodes.address = graph_nodes;
		compressed.type = _animation_base;
		animation_get_node_orientations(NULL, &compressed, 3, nodes);
		animation_graph_node_matrices_from_orientations(0, matrices, nodes, &origin, &forward, &up);
		printf("graph g");
		for (node = 0; node<NODE_COUNT; node++) printf(" %d,%d,%d", links[node][0], links[node][1], links[node][2]);
		printf("\nmatrices g compressed 3 1 2 3 0.6 0.8 0 0 0 1");
		for (node = 0; node<NODE_COUNT; node++) print_matrix(&matrices[node]);
		printf("\n");
	}
	return 0;
}
"""


def _prototype(definition):
    return definition[: definition.index("{")].rstrip() + ";"


def source_file():
    """The fake world, the game's functions and the cases, as one C file."""
    real_math_h = read("source/math/real_math.h")
    types = [
        re.search(
            r"^union " + name + r"\s*\{.*?^\};\ntypedef union " + name + r" " + name + r";", real_math_h, re.M | re.S
        ).group(0)
        for name in ("real_vector2d", "real_point3d", "real_vector3d")
    ]
    types += [
        structure(real_math_h, name) + f"\ntypedef struct {name} {name};"
        for name in ("real_quaternion", "real_orientation", "real_matrix4x3")
    ]
    animations_c = read("source/models/model_animations.c")
    definitions_h = read("source/models/model_animation_definitions.h")
    model_definitions_h = read("source/models/model_definitions.h")
    types += [
        structure(animations_c, name)
        for name in (
            "compressed_animation_header",
            "compressed_quaternion_8byte",
            "compressed_quaternion_6byte",
            "animation_graph_node",
        )
    ]
    types += [
        structure(definitions_h, "animation"),
        structure(definitions_h, "animation_aiming_screen_bounds"),
        structure(model_definitions_h, "model_node"),
        structure(model_definitions_h, "model"),
    ]
    types.append(
        "struct animation_graph { struct tag_block nodes; };\n"
        "static struct animation_graph fake_graph;\n"
        "#define animation_graph_definition_get(index) (&fake_graph)\n"
        "#define animation_get_default_data(animation) ((animation)->default_data.address)\n"
        "static real_quaternion const identity_quaternion = { { { 0.f, 0.f, 0.f } }, 1.f };\n"
        "static real_point3d const origin3d = { { 0.f, 0.f, 0.f } };\n"
        "#define global_identity_quaternion (&identity_quaternion)\n"
        "#define global_origin3d (&origin3d)\n"
    )
    definitions = []
    for path, names, header in MATH_FUNCTIONS:
        text = read(path)
        definitions += [inline(text, name) if header else function(text, name) for name in names]
    for path, names in ANIMATION_FUNCTIONS:
        text = read(path)
        definitions += [function(text, name) for name in names]
    # (every one static, and declared before any is defined)
    definitions = [
        definition if definition.startswith("static ") else "static " + definition for definition in definitions
    ]
    prototypes = [_prototype(definition) for definition in definitions]
    return "\n\n".join([FAKE_WORLD, *types, *prototypes, *definitions, MAIN])


def reference():
    """The reference, as the game's code writes it."""
    with tempfile.TemporaryDirectory(prefix="animation-reference-") as work:
        source = Path(work) / "animation_reference.c"
        executable = Path(work) / "animation_reference"
        source.write_text(source_file())
        compiler = os.environ.get("CC", "clang")
        command = [
            compiler,
            "-std=gnu99",
            "-O1",
            "-ffp-contract=off",
            "-fno-fast-math",
            "-w",
            str(source),
            "-o",
            str(executable),
            "-lm",
        ]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(f"the reference does not compile:\n{result.stderr}")
        output = subprocess.run([str(executable)], capture_output=True, text=True, check=True).stdout
    return (
        "# The game's animation code on made-up animations (tools/engine_animation_reference.py).\n"
        "# Values are f32 bits; an orientation is i j k w x y z scale, a matrix forward left up position scale.\n"
        + output
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the written reference is not the code's")
    arguments = parser.parse_args()
    text = reference()
    if arguments.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != text:
            raise SystemExit(f"{OUTPUT.relative_to(ROOT)} is stale: run tools/engine_animation_reference.py")
        print("the animation reference is the game's code's")
        return
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(text)
    print(f"wrote {OUTPUT.relative_to(ROOT)} ({len(text.splitlines())} lines)")


if __name__ == "__main__":
    main()
