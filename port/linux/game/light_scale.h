/*
LIGHT_SCALE.H

With per-pixel lighting, a light on a scaled object scaled with it
(light_scale.c).
*/

#ifndef __LIGHT_SCALE_H
#define __LIGHT_SCALE_H

/* ---------- prototypes */

/* the radius of a light of that radius on the object (NONE: none): times
the object's scale with display.per_pixel_lighting, as it is without */
real light_scale_radius(
	long object_index,
	real radius);

#endif
