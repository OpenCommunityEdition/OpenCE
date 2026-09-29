/*
HALO_PORT_WINDOW.H

Where the game's memory window is, for the ports that cannot put it at
0x80000000.

The Xbox maps physical memory at virtual 0x80000000 + P, and the game data
is written for that: a map file's tag cache sits at 0x803a6000, every tag
instance in it names where its data and its name are, and those values are
addresses in the window. Android's Java runtime has usually mapped its
large object space across 0x80000000 by the time the app runs, and that
space cannot be taken from it, so the host puts the window somewhere free
and the guest is told where (halo_guest_boot.contiguous_base, read into
platform_contiguous_base before any constructor runs).

The window stays a whole number of 256 MB and larger than the 128 MB the
game uses, so the arithmetic that turns a physical address into a virtual
one (an offset added to the base, or an address masked with its
complement) still gives the right answer wherever it lands.

Game sources include this where they name a fixed place in the window, and
translate it; the byte-matching MSVC build does not include it and compiles
the original constants. port/linux/src/platform.h owns the window itself.
*/

#ifndef __HALO_PORT_WINDOW_H
#define __HALO_PORT_WINDOW_H

/* where the window is; game sources do not include platform.h, so the base
is named here (the desktop ports have always had it at 0x80000000) */
#ifdef HALO_ANDROID
extern unsigned long platform_contiguous_base;
#define PORT_WINDOW_BASE (platform_contiguous_base)
#else
#define PORT_WINDOW_BASE 0x80000000UL
#endif

/* the address of a fixed place in the window, in this process */
#define PORT_WINDOW_ADDRESS(xbox_address) \
	((unsigned long)(xbox_address) - 0x80000000UL + PORT_WINDOW_BASE)

/* the offset of a fixed place inside the window, which is what XPhysicalAlloc
takes: the game's addresses have bit 31 clear */
#define PORT_WINDOW_PHYSICAL_ADDRESS(xbox_address) \
	((unsigned long)(xbox_address) & 0x7fffffffUL)

/* how far the window sits from where the game data was linked */
#define PORT_WINDOW_SHIFT (PORT_WINDOW_BASE - 0x80000000UL)

/* whether a value read out of the game data is one of its addresses, as
opposed to something already moved (the move is done where the value is
used, which can happen more than once for the same field) */
#define PORT_WINDOW_IS_LINKED(address) \
	((unsigned long)(address) >= 0x80000000UL && \
	 (unsigned long)(address) < 0x80000000UL + 0x08000000UL)

/* an address read out of the game data, moved to where it is now */
#define PORT_WINDOW_MOVE(address) \
	((void *)((byte *)(address) + PORT_WINDOW_SHIFT))

/* an address the game data was written with, moved to where the window
actually is. A value that is not one (already moved, or a count) is left
alone, so this can be done where the value is used even if that happens more
than once for the same field */
#define PORT_WINDOW_REBASE(address) \
	(PORT_WINDOW_IS_LINKED(address) ? PORT_WINDOW_MOVE(address) : (void *)(address))

#endif /* __HALO_PORT_WINDOW_H */
