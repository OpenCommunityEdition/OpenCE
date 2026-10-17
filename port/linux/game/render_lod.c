#include "cseries.h"
#include "models/model_definitions.h"
#include "render_lod.h"

int config_boolean(char const *name);
unsigned long config_changes(void);

boolean render_lod_disabled(void)
{
	static boolean initialized, disabled;
	static unsigned long read_at;
	unsigned long changes = config_changes();

	if (!initialized || read_at != changes)
	{
		disabled = config_boolean("display.disable_lod") != 0;
		initialized = TRUE;
		read_at = changes;
	}
	return disabled;
}

boolean render_lod_inside_model(struct model const *model, real_point3d const *camera,
	real_point3d const *center, real radius)
{
	short level;
	for (level = 0; level < NUMBEROF(model->detail_cutoff_pixels); level++)
	{
		if (model->detail_cutoff_pixels[level] > 1000.f)
		{
			double x, y, z, r;
			if (!camera || !center || !(radius > 0.f && radius <= REAL_MAX))
				return FALSE;
			x = (double)camera->x - center->x;
			y = (double)camera->y - center->y;
			z = (double)camera->z - center->z;
			r = radius;
			return x*x + y*y + z*z < r*r;
		}
	}
	return FALSE;
}

boolean render_lod_particle_visible(real diameter, real cutoff)
{
	/* Cull only half-pixel specks on Halo's logical 480p scale, retaining
	the map's smaller cutoffs. Full detail must never hide a
	particle normal detail would show. Reject invalid projections before
	minimum-pixel scaling divides by the diameter. */
	if (!(diameter > 0.0f && diameter <= REAL_MAX))
		return FALSE;
	return diameter > (render_lod_disabled() ? MIN(cutoff, 0.5f) : cutoff);
}

short render_lod_geometry_level(struct model const *model, real pixels,
	short debug_level, boolean full_detail)
{
	short level = NUMBEROF(model->detail_cutoff_pixels) - 1;

	if (debug_level != NONE)
		return PIN(debug_level, 0, NUMBEROF(model->detail_cutoff_pixels) - 1);
	if (full_detail)
		return level;
	while (level > 0 && pixels < model->detail_cutoff_pixels[level])
		level--;
	return level;
}

short render_lod_permutation_level(struct model const *model,
	struct model_region_permutation const *permutation, short level,
	boolean full_detail, boolean inside_model)
{
	if (!full_detail)
		return level;
	/* Very high authored cutoffs mark interior detail (the cryotube uses
	   3000). Do not expose it from outside, even while filling missing LODs. */
	while (level >= 0)
	{
		if ((inside_model || model->detail_cutoff_pixels[level] <= 1000.f) &&
			permutation->geometry_indices[level] != NONE)
			return level;
		level--;
	}
	return NONE;
}
