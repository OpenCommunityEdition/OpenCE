/*
STAGEKIT.H

What StageKit's parts share (README.md): the tags being built (cache.c),
collision geometry (collision.c), what every map has (stage.c), the content
scenarios place (props.c), and a scenario itself (scenarios/).
*/

#ifndef __STAGEKIT_H
#define __STAGEKIT_H
#pragma once

/* ---------- headers */

#include <stdio.h>
#include <string.h>

#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "devices/device_machines.h"
#include "game/game_globals.h"
#include "interface/weapon_hud_interface_definition.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "sound/sound_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "text/font_group.h"
#include "units/biped_definitions.h"


/* (cseries.h and the port's stdio.h name their own; the C library's here) */
#undef printf
#undef fprintf
#undef snprintf
#undef sprintf
#undef fopen
#undef strlen
#undef strcpy
#undef strcmp
#undef memcpy
#undef memset

/* ---------- constants */

/* the chamber: a box from (-CHAMBER_HALF_WIDTH, -CHAMBER_HALF_WIDTH, 0) to
(CHAMBER_HALF_WIDTH, CHAMBER_HALF_WIDTH, CHAMBER_HEIGHT), in world units */
#define CHAMBER_HALF_WIDTH 500.f
#define CHAMBER_HEIGHT 100.f

#define FOURCC(a, b, c, d) (((unsigned long)(a) << 24) | ((unsigned long)(b) << 16) | ((c) << 8) | (d))

/* ---------- structures */

/* the map being built, as a scenario sees it */
struct stage
{
	struct scenario *scenario;
};

struct stagekit_scenario
{
	char const *name;
	char const *description;
	/* places what the scenario adds to the stage (stage.c) */
	void (*build)(struct stage *stage);
};

/* ---------- prototypes/CACHE.C */

/* the tag cache at the game's address for a Custom Edition map's tags; the
structure BSP's room at its top */
void cache_open(void);
void *cache_allocate(unsigned long size);
void *cache_block(struct tag_block *block, long count, unsigned long element_size);
/* one more element at the block's end, zeroed */
void *cache_block_add(struct tag_block *block, unsigned long element_size);
void *cache_bsp_allocate(unsigned long size);
void *cache_bsp_block(struct tag_block *block, long count, unsigned long element_size);
/* a new tag of the group (and the groups it inherits from), its data zeroed */
long cache_tag_new(unsigned long group, unsigned long parent, unsigned long grandparent, char const *name,
	unsigned long size, void **data);
void cache_tag_no_data(long handle);
void cache_reference(struct tag_reference *reference, long handle);
/* a reference to no tag, of the group it names */
void cache_no_reference(struct tag_reference *reference, unsigned long group);
void cache_set_scenario(long handle);
/* where the structure BSP is in the file, how big, and where in memory */
void cache_bsp_place(struct scenario_structure_bsp_reference *reference);
/* the map */
void cache_write(char const *path, long *tag_count);

/* ---------- prototypes/COLLISION.C */

/* a box's collision: hollow (walls facing in, open inside: the chamber) or
solid (faces out, open outside: an obstacle), its blocks made by block_new */
void box_build(struct collision_bsp *collision, void *(*block_new)(struct tag_block *, long, unsigned long),
	real_rectangle3d const *bounds, boolean solid);

/* ---------- prototypes/STAGE.C */

/* the scenario, the chamber, the globals and interface, the player's biped,
no scripts and a player start, with scenario's content in between */
void stage_build(struct stage *stage, struct stagekit_scenario const *scenario);

/* ---------- prototypes/PROPS.C */

/* a player start at position (on what stands there), facing along x */
void player_start(struct stage *stage, real_point3d const *position);

/* a model that draws nothing (one node, region and permutation), made once */
long prop_empty_model(void);

/* a scenery box standing at position (bounds in its own space), solid to anything */
void prop_obstacle(struct stage *stage, char const *name, real_rectangle3d const *bounds,
	real_point3d const *position);

/* a platform (bounds in its own space) at position that rises rise in seconds
when its position is set to 1 (device_set_position <name> 1) and comes down
when set to 0, as the game's lifts do; elevator: what stands on it rides it */
void prop_elevator(struct stage *stage, char const *name, real_rectangle3d const *platform,
	real_point3d const *position, real rise, real seconds);

#endif // __STAGEKIT_H
