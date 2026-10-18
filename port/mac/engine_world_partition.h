#ifndef HALO_MAC_ENGINE_WORLD_PARTITION_H
#define HALO_MAC_ENGINE_WORLD_PARTITION_H
struct cluster_partition;
/* Engine-thread native storage for real cluster_partition_* routines. Caller
   supplies a zeroed partition and disconnects every datum before disposal.
   Not registered with save-game partition tracking; diagnostic/integration
   bootstrap only until full game_state lifecycle is available. */
int mac_engine_partition_start(struct cluster_partition *);
int mac_engine_partition_stop(struct cluster_partition *);
#endif
