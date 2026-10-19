#include "harness.h"

#define LONG_BITS 32
#define LONG_BITS_BITS 5
struct animation
{
	long frame_size;
	short node_count;
	unsigned long nodes_with_translation_flags[2];
	unsigned long nodes_with_rotation_flags[2];
	unsigned int nodes_with_scale_flags[2];
	struct { void *address; long size; } data;
};
#include "under_test.inc"

/* the original's walk, one node and kind at a time for each byte (what the table replaces) */
static unsigned char original_byte(struct animation const *animation, long offset)
{
	byte const *base = (byte const *)animation->data.address;
	long frame_size = animation->frame_size;

	if (frame_size > 0 && offset >= 0 && offset < animation->data.size)
	{
		long position = offset % frame_size;
		long frame_start = offset - position;
		long cursor = 0;
		short node_index;

		for (node_index = 0; node_index < animation->node_count && node_index < 64; node_index++)
		{
			long kind;

			for (kind = 0; kind < 3; kind++)
			{
				unsigned long flags = kind == 0 ? animation->nodes_with_rotation_flags[node_index >> 5] :
					kind == 1 ? animation->nodes_with_translation_flags[node_index >> 5] :
					animation->nodes_with_scale_flags[node_index >> 5];
				long size = kind == 0 ? 8 : kind == 1 ? 12 : 4;
				long unit = kind == 0 ? 2 : 4;

				if (flags & (1UL << (node_index & 31)))
				{
					if (position < cursor + size)
					{
						long relative = position - cursor;
						long unit_start = relative - relative % unit;

						return base[frame_start + cursor + unit_start + (unit - 1 - (relative - unit_start))];
					}
					cursor += size;
				}
			}
		}
	}
	return offset >= 0 && offset < animation->data.size ? base[offset] : (unsigned char)0;
}

static long compare(unsigned long seed, long trials)
{
	long trial, differ = 0;

	srand((unsigned)seed);
	for (trial = 0; trial < trials; trial++)
	{
		struct animation a;
		long frames = 1 + rand() % 6, nodes = 1 + rand() % 30, i, total = 0, offset;
		unsigned char *data;

		memset(&a, 0, sizeof(a));
		a.node_count = (short)nodes;
		for (i = 0; i < nodes; i++)
		{
			if (rand() % 4) { a.nodes_with_rotation_flags[i >> 5] |= 1UL << (i & 31); total += 8; }
			if (rand() % 3 == 0) { a.nodes_with_translation_flags[i >> 5] |= 1UL << (i & 31); total += 12; }
			if (rand() % 5 == 0) { a.nodes_with_scale_flags[i >> 5] |= 1U << (i & 31); total += 4; }
		}
		a.frame_size = total ? total : 1;
		a.data.size = a.frame_size * frames;
		data = (unsigned char *)malloc((size_t)a.data.size + 16);
		for (i = 0; i < a.data.size + 16; i++) data[i] = (unsigned char)rand();
		a.data.address = data;
		for (offset = -3; offset < a.data.size + 3; offset++)
			if (original_byte(&a, offset) != aiming_data_little_endian_byte(&a, 0, offset)) differ++;
		free(data);
	}
	return differ;
}

int main(int argc, char **argv)
{
	CHECK(argc == 2, "a case");
	if (!strcmp(argv[1], "same_bytes"))
	{
		CHECK(compare(7, 3000) == 0, "a byte differs from the original walk");
	}
	else if (!strcmp(argv[1], "new_animation_at_an_old_address"))
	{
		/* the table of an animation is made again when another one is at its address */
		struct animation a;
		unsigned char data[64];
		long i;

		memset(&a, 0, sizeof(a));
		for (i = 0; i < 64; i++) data[i] = (unsigned char)(i * 7 + 3);
		a.data.address = data; a.data.size = 64; a.node_count = 2; a.frame_size = 8;
		a.nodes_with_rotation_flags[0] = 1;
		for (i = 0; i < 8; i++) CHECK(aiming_data_little_endian_byte(&a, 0, i) == original_byte(&a, i), "first animation");
		/* the same frame size and node count, other flags */
		a.nodes_with_rotation_flags[0] = 0; a.nodes_with_scale_flags[0] = 3;
		for (i = 0; i < 8; i++) CHECK(aiming_data_little_endian_byte(&a, 0, i) == original_byte(&a, i), "second animation at the address");
	}
	else if (!strcmp(argv[1], "beyond_the_data"))
	{
		/* a frame whose nodes ask for more than the frame holds reads nothing past the data */
		struct animation a;
		unsigned char data[16];

		memset(&a, 0, sizeof(a));
		memset(data, 2, sizeof(data));
		memset(data, 1, 9);
		a.data.address = data; a.data.size = 9; a.node_count = 2; a.frame_size = 9;
		a.nodes_with_rotation_flags[0] = 3; a.nodes_with_translation_flags[0] = 1;
		CHECK(aiming_data_little_endian_byte(&a, 0, 8) == 1, "a byte past the data");
	}
	else CHECK(0, "unknown case %s", argv[1]);
	return 0;
}
