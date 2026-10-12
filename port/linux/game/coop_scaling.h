/* Shared per-player growth; the first player never adds growth. */
#ifndef __COOP_SCALING_H
#define __COOP_SCALING_H

enum { COOP_SCALING_MAXIMUM_GROWTH = 8 };

static inline long coop_scaling_growth(short players, short percent)
{
	if (players <= 1 || percent <= 0)
		return 0;
	return MIN((long)MIN(percent, 200) *
		(MIN(players, HALO_PORT_MAXIMUM_NETWORK_PLAYERS) - 1),
		(COOP_SCALING_MAXIMUM_GROWTH - 1) * 100);
}

#endif
