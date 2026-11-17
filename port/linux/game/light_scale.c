/*
LIGHT_SCALE.C

A light on an object (a vehicle's cabin light, an engine's) keeps the
radius its definition gives it however large or small the object is drawn
(object_set_scale). a10's escape (x30) flies the lifepod past a miniature
of the ring: the pod is drawn at a fiftieth of its size (lifeboat_approach,
0.25 then 0.02) and the ring at two or three times its own, while the
pod's red cabin light (vehicles\lifepod\light_overhead) still reached a
world unit from it, some fifty times the shrunken pod, over the ring's
hull beside it. Lit for each vertex, as on the Xbox, the ring's few and far
vertices were out of its reach and nothing showed; lit for each pixel
(display.per_pixel_lighting), the hull's edge was painted red.

With display.per_pixel_lighting, a light on an object reaches as far as
its definition says times the object's scale (lights_preprocess_scene):
the pod's light stays in the pod. Lit for each vertex, the radius is the
definition's, as on the Xbox. Only the drawing changes.
*/

/* ---------- headers */

#include "cseries.h"
#include "memory/data.h"
#include "objects/objects.h"

#include "light_scale.h"

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

/* ---------- private code */

/* display.per_pixel_lighting, read again when a setting changes */
static boolean light_scale_per_pixel(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.per_pixel_lighting") != 0;
	}

	return enabled;
}

/* ---------- public code */

real light_scale_radius(
	long object_index,
	real radius)
{
	struct object_datum *object;

	if (object_index == NONE || !light_scale_per_pixel())
	{
		return radius;
	}
	object = object_try_and_get_and_verify_type(object_index, _object_mask_all);
	if (!object || !(object->object.scale > 0.0f))
	{
		return radius;
	}

	return radius * object->object.scale;
}
