/*
LONGSWORD_EXHAUST.H

The Longswords' exhaust glow, seen from behind the ship only
(longsword_exhaust.c).
*/

#ifndef __LONGSWORD_EXHAUST_H
#define __LONGSWORD_EXHAUST_H

/* ---------- prototypes */

/* as an Xbox map's tags load (scenario_tags_load), and as any map's unload */
void longsword_exhaust_tags_loaded(
	void);
void longsword_exhaust_tags_unloaded(
	void);
/* the direction the flare of that definition, on the object, shines in:
the ship's way back, for the Longswords' exhaust; whether it is that */
boolean longsword_exhaust_flare_direction(
	long lens_flare_definition_index,
	long object_index,
	union real_vector3d *direction);

#endif
