/* Build with engine_xdk_prefix.h, as a game translation unit is built. */
#include "engine_sockets.h"
typedef char mac_game_fdset_capacity[
    FD_SETSIZE == HALO_PORT_FD_SETSIZE ? 1 : -1];
typedef char mac_game_fdset_size[
    sizeof(struct fd_set) == sizeof(unsigned int) +
        HALO_PORT_FD_SETSIZE * sizeof(unsigned int) ? 1 : -1];
int mac_game_fdset_prefix_witness(void) { return (int)sizeof(struct fd_set); }
