/* Local rendering detail policy; never changes map tags or simulation. */
#ifndef __RENDER_LOD_H
#define __RENDER_LOD_H

#include "math/real_math.h"

boolean render_lod_disabled(void);
struct model;
boolean render_lod_inside_model(struct model const *model, real_point3d const *camera,
	real_point3d const *center, real radius);
short render_lod_geometry_level(struct model const *model, real pixels,
	short debug_level, boolean full_detail);
struct model_region_permutation;
short render_lod_permutation_level(struct model const *model,
	struct model_region_permutation const *permutation, short level,
	boolean full_detail, boolean inside_model);
boolean render_lod_particle_visible(real diameter, real cutoff);

#endif
