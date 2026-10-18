/*
HALO_ENDIAN.H

What goes on the network is explicit about its byte order, so a big-endian
host (the PowerPC Mac) sends the same bytes a little-endian one does.

Two orders are on the wire:

- the port's own messages (port/linux/game/network_*.c, map_share.c) are
  LITTLE-endian: halo_le16, halo_le32 and halo_wire_swap below;
- the original game's (message headers, data_encoding.c's messages, the
  addresses of the transport) are BIG-endian, as the Xbox's network order:
  halo_be16 and halo_be32.

Each is the identity where the host already has that order, and on a
little-endian host the little-endian ones are macros that compile to nothing,
so the builds for x86 and ARM are the code they were.

A message's structure is swapped in place by its layout, the string of its
fields from the first: a count and a size letter each, b (a byte, or padding),
s (16 bits) or l (32 bits, a real too). A layout is written as a macro of
X(size letter, count) items, so the string and the size it adds up to (checked
against sizeof the structure when it is compiled) are one thing:

	#define PICKUP_LAYOUT(X) X(b,2) X(s,1) X(l,1)
	static char const pickup_layout[] = PICKUP_LAYOUT(HALO_WIRE_STRING);
	typedef char pickup_layout_assert[HALO_WIRE_SIZE(PICKUP_LAYOUT) == sizeof(struct pickup) ? 1 : -1];
*/

#ifndef __HALO_ENDIAN_H
#define __HALO_ENDIAN_H

#if defined(__BIG_ENDIAN__) || defined(__ppc__) || defined(__POWERPC__) || \
	(defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define HALO_BIG_ENDIAN_HOST 1
#endif

/* ---------- byte swaps (any host) */

#define HALO_SWAP16(q) ((unsigned short)((((unsigned short)(q)) >> 8) | (((unsigned short)(q)) << 8)))
#define HALO_SWAP32(q) \
	((unsigned long)((((unsigned long)(q) & 0xFF000000UL) >> 24) | (((unsigned long)(q) & 0x00FF0000UL) >> 8) | \
		(((unsigned long)(q) & 0x0000FF00UL) << 8) | (((unsigned long)(q) & 0x000000FFUL) << 24)) & 0xFFFFFFFFUL)

/* ---------- little-endian (the port's messages) and big-endian (the original's) */

#ifdef HALO_BIG_ENDIAN_HOST

#define halo_le16(q) HALO_SWAP16(q)
#define halo_le32(q) HALO_SWAP32(q)
#define halo_be16(q) ((unsigned short)(q))
#define halo_be32(q) ((unsigned long)(q))

#else

#define halo_le16(q) ((unsigned short)(q))
#define halo_le32(q) ((unsigned long)(q))
#define halo_be16(q) HALO_SWAP16(q)
#define halo_be32(q) HALO_SWAP32(q)

#endif

/* ---------- wire layouts */

/* (a function that may be left unused in a file, in the C of the game's
builds, which makes __inline a macro of its own) */
#if defined(__GNUC__)
#define HALO_ENDIAN_FUNCTION static __attribute__((unused))
#else
#define HALO_ENDIAN_FUNCTION static
#endif

#define HALO_WIRE_STRING(size, count) HALO_WIRE_STRING_(size, count)
#define HALO_WIRE_STRING_(size, count) #count #size
#define HALO_WIRE_SIZE_b 1
#define HALO_WIRE_SIZE_s 2
#define HALO_WIRE_SIZE_l 4
#define HALO_WIRE_ITEM_SIZE(size, count) + (HALO_WIRE_SIZE_##size * (count))
/* the bytes a layout's fields add up to */
#define HALO_WIRE_SIZE(layout) (0 layout(HALO_WIRE_ITEM_SIZE))

/* the bytes of one structure of a layout */
HALO_ENDIAN_FUNCTION long halo_wire_layout_size(
	char const *layout)
{
	long size = 0;

	while (*layout)
	{
		long repeat = 0;

		while (*layout >= '0' && *layout <= '9')
			repeat = repeat * 10 + (*layout++ - '0');
		switch (*layout++)
		{
		case 'b': size += repeat; break;
		case 's': size += 2 * repeat; break;
		case 'l': size += 4 * repeat; break;
		default: return size;
		}
	}
	return size;
}

/* count structures of layout, in place, between the order of the host and the
wire's little-endian by swapping every field. (halo_wire_swap_le is this on a
big-endian host and nothing on a little-endian one; the tests call this on
any host, to check the layouts.) */
HALO_ENDIAN_FUNCTION void halo_wire_swap_always(
	void *data,
	char const *layout,
	long count)
{
	unsigned char *bytes = (unsigned char *)data;

	while (count-- > 0)
	{
		char const *item = layout;

		while (*item)
		{
			long repeat = 0;
			unsigned char temporary;

			while (*item >= '0' && *item <= '9')
				repeat = repeat * 10 + (*item++ - '0');
			switch (*item++)
			{
			case 'b':
				bytes += repeat;
				break;
			case 's':
				for (; repeat > 0; repeat--, bytes += 2)
				{
					temporary = bytes[0]; bytes[0] = bytes[1]; bytes[1] = temporary;
				}
				break;
			case 'l':
				for (; repeat > 0; repeat--, bytes += 4)
				{
					temporary = bytes[0]; bytes[0] = bytes[3]; bytes[3] = temporary;
					temporary = bytes[1]; bytes[1] = bytes[2]; bytes[2] = temporary;
				}
				break;
			default:
				return;
			}
		}
	}
}

/* size bytes of units of 2 or 4 bytes (1: none), in place */
HALO_ENDIAN_FUNCTION void halo_swap_units_always(
	void *data,
	long size,
	long unit)
{
	unsigned char *bytes = (unsigned char *)data;
	long offset;

	for (offset = 0; unit > 1 && offset + unit <= size; offset += unit)
	{
		long low;

		for (low = 0; low < unit / 2; low++)
		{
			unsigned char temporary = bytes[offset + low];

			bytes[offset + low] = bytes[offset + unit - 1 - low];
			bytes[offset + unit - 1 - low] = temporary;
		}
	}
}

#ifdef HALO_BIG_ENDIAN_HOST

#define halo_wire_swap_le(data, layout, count) halo_wire_swap_always(data, layout, count)
#define halo_swap_units(data, size, unit) halo_swap_units_always(data, size, unit)

#else

#define halo_wire_swap_le(data, layout, count) ((void)0)
#define halo_swap_units(data, size, unit) ((void)0)

#endif

#endif /* __HALO_ENDIAN_H */
