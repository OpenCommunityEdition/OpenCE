/* Local rendering detail policy; never changes map tags or simulation. */
#ifndef __RENDER_LOD_H
#define __RENDER_LOD_H

boolean render_lod_disabled(void);
boolean render_lod_model_disabled(long model_index);
struct model;
short render_lod_geometry_level(struct model const *model, real pixels,
	short debug_level, boolean full_detail);
struct model_region_permutation;
short render_lod_permutation_level(struct model_region_permutation const *permutation,
	short level, boolean full_detail);
boolean render_lod_particle_visible(real diameter, real cutoff);

#endif
