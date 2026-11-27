/* MCC particle-system view selection; no persistent state. */
#ifndef MCC_PARTICLES_H
#define MCC_PARTICLES_H
struct object_marker;
short mcc_particle_system_markers(long object_index, short attachment_index,
    unsigned long flags, struct object_marker *markers, short maximum, short world_count);
boolean mcc_particle_system_visible(long object_index, unsigned long flags, short viewer);
#endif
