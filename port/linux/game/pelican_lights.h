/*
PELICAN_LIGHTS.H

The Pelican's running lights drawn whole over its hull: both drawn as its
decals, with a depth bias of their own (pelican_lights.c).
*/

#ifndef __PELICAN_LIGHTS_H
#define __PELICAN_LIGHTS_H

struct shader;

/* ---------- prototypes */

/* as an Xbox map's tags load (scenario_tags_load), map_name its header's:
the Pelican's light shaders noted and given the decal flag, but on a10; how
many were given it */
short pelican_lights_tags_loaded(
	char const *map_name);
/* as any map's tags unload: none noted */
void pelican_lights_tags_unloaded(
	void);
/* the depth bias a model's decal drawn with that shader takes: the
Pelican's lights' own, else zbias */
long pelican_lights_zbias(
	struct shader const *shader,
	long zbias);

#endif
