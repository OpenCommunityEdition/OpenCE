/*
AIMING_FRAME_BYTES.C (test)

aiming_screen_apply walks an uncompressed aiming animation's frames without skipping the scales of the nodes that animate
them (the original's bug, kept), so a later node's rotation is read from the scales' bytes. The x86 reads them as a
little-endian frame lays them out; the Mac's PowerPC finds the frame converted in the width of each field. The helpers
(aiming_decompress_8byte, aiming_read_point3d) are taken from model_animations.c with the big-endian branch on; the frame
is built little-endian and converted as the swap converts it, and what they read must be what the x86 reads from the
little-endian one. test_aiming_frame_bytes.py takes the helpers and the animation structure.
*/

#include "harness.h"

typedef unsigned short word;
#define LONG_BITS_BITS 5
#define LONG_BITS 32
#include "types.inc"
#include "under_test.inc"

/* 22 nodes as the carrier's aim-still: rotations of nodes 0-3, 6-8 and 16-17, scales of nodes 11-12 */
static void build_frames(unsigned char *le, unsigned char *be, long frames)
{
	static const unsigned rotation_nodes[] = { 0, 1, 2, 3, 6, 7, 8, 16, 17 };
	long frame, node, cursor = 0, size;
	long units[64], widths[64], count = 0, at, i;

	for (node = 0; node < 22; node++)
	{
		int r = 0;
		for (i = 0; i < 9; i++) r |= rotation_nodes[i] == (unsigned)node;
		if (r) { units[count] = 2; widths[count++] = 8; }
		if (node == 11 || node == 12) { units[count] = 4; widths[count++] = 4; }
	}
	for (i = 0, size = 0; i < count; i++) size += widths[i];
	CHECK(size == 80, "frame size %ld", size);
	for (frame = 0; frame < frames; frame++)
	{
		cursor = frame * size;
		for (i = 0; i < count; i++)
		{
			long k;
			for (k = 0; k < widths[i]; k++)
				le[cursor + k] = (unsigned char)(frame * 31 + i * 7 + k * 3 + 1);
			if (widths[i] == 4)
			{
				/* a scale: a float, 1.0 or 1.5 */
				unsigned long bits = (i % 2) ? 0x3FC00000UL : 0x3F800000UL;
				le[cursor] = (unsigned char)bits; le[cursor + 1] = (unsigned char)(bits >> 8);
				le[cursor + 2] = (unsigned char)(bits >> 16); le[cursor + 3] = (unsigned char)(bits >> 24);
			}
			for (at = 0; at < widths[i]; at += units[i])
				for (k = 0; k < units[i]; k++)
					be[cursor + at + k] = le[cursor + at + units[i] - 1 - k];
			cursor += widths[i];
		}
	}
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	static unsigned char le[80 * 9], be[80 * 9];
	struct animation animation;
	long offset;

	memset(&animation, 0, sizeof(animation));
	build_frames(le, be, 9);
	animation.frame_size = 80;
	animation.node_count = 22;
	animation.data.size = 80 * 9;
	animation.data.address = be;
	animation.nodes_with_rotation_flags[0] = 0x301cf;
	animation.nodes_with_scale_flags[0] = 0x1800;

	/* every offset the walk can start a rotation or a translation at, in every frame */
	CASE("rotations")
	{
		for (offset = 0; offset + 8 <= 80 * 9; offset += 2)
		{
			real_quaternion got, want;
			short components[4];
			long k;

			for (k = 0; k < 4; k++)
				components[k] = (short)(le[offset + k * 2] | (le[offset + k * 2 + 1] << 8));
			quaternion_decompress_8byte((struct compressed_quaternion_8byte const *)components, &want);
			aiming_decompress_8byte(&animation, be + offset, &got);
			CHECK(got.v.i == want.v.i && got.v.j == want.v.j && got.v.k == want.v.k && got.w == want.w, "rotation at %ld", offset);
		}
	}

	CASE("translations")
	{
		for (offset = 0; offset + 12 <= 80 * 9; offset += 4)
		{
			real_point3d got;
			unsigned long want[3];
			long k;

			for (k = 0; k < 3; k++)
				want[k] = (unsigned long)le[offset + k * 4] | ((unsigned long)le[offset + k * 4 + 1] << 8) |
					((unsigned long)le[offset + k * 4 + 2] << 16) | ((unsigned long)le[offset + k * 4 + 3] << 24);
			aiming_read_point3d(&animation, be + offset, &got);
			CHECK(!memcmp(&got, want, sizeof(got)), "translation at %ld", offset);
		}
	}

	return 0;
}
