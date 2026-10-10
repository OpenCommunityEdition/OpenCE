/*
HALO_PORT_CAPACITY.H

Memory capacity of the native builds (Windows, Linux, Android), sized for the
session limits in halo_port_limits.h, which includes this file. The Xbox
sizes are given in parentheses below.

Every machine in a session must be built with the same values: the
distributed netcode names objects and players by their datum index, the
same on every machine (port/linux/game/network_objects.c tracks
MAXIMUM_TRACKED_OBJECTS = HALO_PORT_MAXIMUM_OBJECTS_PER_MAP objects, and a
client's own objects take the upper half of the object array).
*/

#ifndef __HALO_PORT_CAPACITY_H
#define __HALO_PORT_CAPACITY_H

/* ---------- game state

The Xbox game state is 0x345000 bytes at 0x80061000 and ends where the tag
cache begins (0x803A6000). Cache files are linked to that tag cache address,
so the game state cannot grow in place. The native builds put a 16 MB game
state above the tag cache (which ends at 0x819A6000), inside the Xbox memory
window (0x80000000-0xA0000000, Android's to 0x88000000:
port/linux/src/platform.h) and below everything the window hands out
top-down (texture and sound caches, Direct3D resources).

The CPU part holds about 17.2 MB of pools at the sizes below (the Xbox pools
fill 3,165,260 of its 0x305000 bytes); the GPU part holds only the decal
vertices, as on the Xbox. A change to a pool's size changes the game state's
layout: saved games of builds before it no longer load. */

#define HALO_PORT_GAME_STATE_BASE_ADDRESS 0x81A00000 /* (0x80061000) */
#define HALO_PORT_GAME_STATE_CPU_SIZE 0x13C0000 /* (0x305000) */
#define HALO_PORT_GAME_STATE_GPU_SIZE 0x200000 /* (0x40000): decal vertices for twelve times the Xbox's decals (game.decals "insane") */
#define HALO_PORT_GAME_STATE_SIZE (HALO_PORT_GAME_STATE_CPU_SIZE+HALO_PORT_GAME_STATE_GPU_SIZE)

/* ---------- textures

The texture cache holds the textures being drawn in 16 KB pages, 22 MB of
them on the Xbox, which Xbox maps were made to fit. Halo Custom Edition maps
were made for Halo PC, which has no such bound: a texture that does not fit
is drawn as the default one ("YOU GOT STABBED" in debug.txt; Elite_Alpha_Siege
did at 22 MB), and a frame of bigass_v3 draws more than 64 MB (DamnationCE's
measurement). The desktop builds' cache is 256 MB, half their 512 MB memory
window (port/linux/src/platform.h), whose pages are backed as they are used.
Android's window is 128 MB, and its cache the Xbox's. */

#ifdef HALO_ARM64_GUEST
#define HALO_PORT_TEXTURE_CACHE_PAGE_COUNT 0x580 /* (0x580) */
#else
#define HALO_PORT_TEXTURE_CACHE_PAGE_COUNT 0x4000 /* (0x580) */
#endif
#define HALO_PORT_TEXTURE_CACHE_SIZE (HALO_PORT_TEXTURE_CACHE_PAGE_COUNT*0x4000) /* (0x1600000) */

/* ---------- sounds

The sound cache holds the sounds being played in 4 KB pages, 4 MB of them on
the Xbox. Halo Custom Edition maps' sounds, converted when they load
(port/linux/game/custom_edition_sounds.c), and those of sound tags loaded
over a map's (loose_sounds.c), are longer than the Xbox maps' and fill it (a
campaign map's dialogue and music did, "SOUND CACHE BLOWN" in debug.txt).
The desktop builds' cache is 16 MB; Android's window keeps the Xbox's. */

#ifdef HALO_ARM64_GUEST
#define HALO_PORT_SOUND_CACHE_SIZE 0x400000 /* (0x400000) */
#else
#define HALO_PORT_SOUND_CACHE_SIZE 0x1000000 /* (0x400000) */
#endif

/* ---------- AI

Network co-op adds enemies for its players (port/linux/game/coop_enemies.c):
the actors, and their knowledge of the units about them (props: with many
players, more each), have room for several times a level's own. */

#define HALO_PORT_MAXIMUM_ACTORS 1024 /* (256) */
#define HALO_PORT_MAXIMUM_PROPS 8192 /* (768) */
#define HALO_PORT_MAXIMUM_SWARMS 128 /* (32) */
#define HALO_PORT_MAXIMUM_SWARM_COMPONENTS 1024 /* (256) */

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
/* script object lists, and the object references they all share. The
lists of a tick's scripts are freed after it (object_list_gc): a Halo PC
map whose scripts test (players) in many places a tick took more than the
Xbox's 48 (coldsnap's), and its game halted */
#define HALO_PORT_MAXIMUM_OBJECT_LISTS_PER_MAP 1024 /* (48) */
#define HALO_PORT_MAXIMUM_LISTED_OBJECTS_PER_MAP 8192 /* (128) */
/* widgets (light volumes, antennas, flags, glows, lightning), each made with
its object and kept for its life: an assault rifle's flashlight beam, held or
dropped, and a plasma bolt's light volume; a full pool draws the object
without its widget */
#define HALO_PORT_MAXIMUM_WIDGETS 2048 /* (64) */
#define HALO_PORT_MAXIMUM_LIGHT_VOLUMES 2048 /* (256) */

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
