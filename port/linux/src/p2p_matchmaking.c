/*
P2P_MATCHMAKING.C

MATCHMAKING's signed records, queues and search.
*/

#include "p2p_matchmaking.h"
#include "halo_port_limits.h"

#include <string.h>

int mm_playlist_has_size(unsigned playlists, int size)
{
	return size != P2P_MM_FFA_6 || playlists != 1u << P2P_MM_CTF;
}
