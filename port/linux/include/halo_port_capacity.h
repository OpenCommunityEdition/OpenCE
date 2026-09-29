/*
HALO_PORT_CAPACITY.H

Memory capacity of the native builds (Windows, Linux, Android), sized for the
session limits in halo_port_limits.h, which includes this file. Game sources
use these values only under #ifdef HALO_LINUX, so the byte-matching MSVC build
keeps the Xbox sizes (given in parentheses below).

Every machine in a session must be built with the same values: the game is
simulated in lockstep on every machine, and a pool that runs full changes the
simulation (an object or a deterministic effect is not created).
*/

#ifndef __HALO_PORT_CAPACITY_H
#define __HALO_PORT_CAPACITY_H

/* ---------- game state

The Xbox game state is 0x345000 bytes at 0x80061000 and ends where the tag
cache begins (0x803A6000). Cache files are linked to those addresses, so
the game state cannot grow in place, and the port moves them with the
window where it cannot be at 0x80000000 (port/linux/include/halo_port_window.h).
The native builds put a 16 MB game state above the tag cache (which ends at
0x819A6000), inside the Xbox memory window (port/linux/src/platform.h) and
below everything the window hands out top-down (texture and sound caches,
Direct3D resources).

The CPU part holds about 13.6 MB of pools at the sizes below (the Xbox pools
fill 3,165,260 of its 0x305000 bytes); the GPU part holds only the decal
vertices, as on the Xbox. */

/* the game's own address, which is where the port places the game state
when the window is where the game data expects it; on Android, where the
host puts the window, the port shifts it (halo_port_window.h) */
#define HALO_PORT_GAME_STATE_XBOX_ADDRESS 0x81A00000 /* (0x80061000) */
#define HALO_PORT_GAME_STATE_CPU_SIZE 0xFC0000 /* (0x305000) */
#define HALO_PORT_GAME_STATE_GPU_SIZE 0x40000 /* (0x40000) */
#define HALO_PORT_GAME_STATE_SIZE (HALO_PORT_GAME_STATE_CPU_SIZE+HALO_PORT_GAME_STATE_GPU_SIZE)

/* ---------- objects */

#define HALO_PORT_MAXIMUM_OBJECTS_PER_MAP 8192 /* (2048) */
#define HALO_PORT_OBJECT_MEMORY_POOL_SIZE 0x800000 /* (0x100000) */
/* each of the two reference lists of every cluster partition (collideable
objects, noncollideable objects, lights) */
#define HALO_PORT_MAXIMUM_CLUSTER_REFERENCES 8192 /* (2048) */
#define HALO_PORT_MAXIMUM_RENDERED_OBJECTS 1024 /* (256) */
#define HALO_PORT_MAXIMUM_CACHED_OBJECT_RENDER_STATES 1024 /* (256) */
/* objects one explosion can damage */
#define HALO_PORT_MAXIMUM_AREA_OF_EFFECT_OBJECTS 256 /* (64) */
/* object references shared by all script object lists */
#define HALO_PORT_MAXIMUM_LISTED_OBJECTS_PER_MAP 1024 /* (128) */

/* ---------- effects, particles, lights and sounds */

#define HALO_PORT_MAXIMUM_EFFECTS 2048 /* (256) */
#define HALO_PORT_MAXIMUM_EFFECT_LOCATIONS 4096 /* (512) */
#define HALO_PORT_MAXIMUM_PARTICLES 8192 /* (1024) */
#define HALO_PORT_MAXIMUM_PARTICLE_SYSTEMS 256 /* (64) */
#define HALO_PORT_MAXIMUM_SYSTEM_PARTICLES 4096 /* (512) */
#define HALO_PORT_MAXIMUM_CONTRAILS 1024 /* (256) */
#define HALO_PORT_MAXIMUM_CONTRAIL_POINTS 8192 /* (1024) */
#define HALO_PORT_MAXIMUM_LIGHTS_PER_MAP 4096 /* (896) */
#define HALO_PORT_MAXIMUM_GAME_LOOPING_SOUNDS 4096 /* (1024) */

#endif /* __HALO_PORT_CAPACITY_H */
