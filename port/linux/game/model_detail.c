/*
MODEL_DETAIL.C

The geometry each of a model's permutations draws when display.model_detail
is "native" or "highest" (halo_model_detail_scale, port/linux/src/d3d8_gl.c),
which choose the model's level of detail at sizes the Xbox's 480 lines
rarely or never reached (render_model, source/models/models.c). "stock"
draws each permutation at the model's level, as the Xbox did.

Two kinds of a map's levels are not made for those sizes:
- a permutation may have no geometry at a level, and draw nothing there:
  "highest" would leave the part out at every distance, so it draws its
  most detailed level below that has some;
- a level whose cutoff is over 1000 pixels is the model's inside, made for
  a camera within it (a10's cryotube's top level, at 3000, is the inside of
  the tube without its outside), so it is drawn only while the camera is
  inside the model's bounding sphere, and the level below otherwise.
*/

#include "cseries.h"
#include "model_detail.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer_console_vars.h"
#include "render/render.h"
#include "render/render_cameras.h"

/* ---------- constants */

enum
{
	/* the cutoff over which a level is a model's inside */
	MODEL_DETAIL_INTERIOR_CUTOFF_PIXELS = 1000,
};

/* ---------- public code */

short model_detail_rules(struct model const *model, real_point3d const *centroid, real radius)
{
	if (halo_model_detail_stock() || rasterizer_debug_options.debug_model_lod!=NONE)
	{
		return _model_detail_rules_stock;
	}
	return model_detail_camera_inside(model, &render.camera.position, centroid, radius) ?
		_model_detail_rules_inside :
		_model_detail_rules_outside;
}

boolean model_detail_camera_inside(struct model const *model, real_point3d const *camera,
	real_point3d const *centroid, real radius)
{
	short level;

	for (level = 0; level<(short)NUMBEROF(model->detail_cutoff_pixels); level++)
	{
		if (model->detail_cutoff_pixels[level]>MODEL_DETAIL_INTERIOR_CUTOFF_PIXELS)
		{
			real x, y, z;

			/* (a sphere of no size, infinite or not a number holds nothing) */
			if (!(radius>0.f && radius<=REAL_MAX))
			{
				return FALSE;
			}
			x = camera->x-centroid->x;
			y = camera->y-centroid->y;
			z = camera->z-centroid->z;
			return x*x+y*y+z*z<radius*radius;
		}
	}
	return FALSE;
}

short model_detail_geometry_index(struct model const *model,
	struct model_region_permutation const *permutation, short level, short rules)
{
	if (rules==_model_detail_rules_stock)
	{
		return permutation->geometry_indices[level];
	}
	for (; level>=0; level--)
	{
		if (permutation->geometry_indices[level]!=NONE &&
			(rules==_model_detail_rules_inside ||
				model->detail_cutoff_pixels[level]<=MODEL_DETAIL_INTERIOR_CUTOFF_PIXELS))
		{
			return permutation->geometry_indices[level];
		}
	}
	return NONE;
}
