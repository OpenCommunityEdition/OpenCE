/*
MODEL_DETAIL.H

The geometry each of a model's permutations draws at the level of detail
display.model_detail chose for the model (model_detail.c).
*/

#ifndef MODEL_DETAIL_H
#define MODEL_DETAIL_H

#include "math/real_math.h"

struct model;
struct model_region_permutation;

/* how a model's permutations take its level (model_detail_rules) */
enum
{
	/* each the model's level, as the Xbox did ("stock", or rasterizer_debug_model_lod) */
	_model_detail_rules_stock,
	/* each its most detailed level at or below the model's that has geometry,
	not one of the model's inside */
	_model_detail_rules_outside,
	/* the same, the camera being inside the model, whose inside it sees */
	_model_detail_rules_inside,
};

/* the rules for a model drawn this frame, its bounding sphere in the world */
short model_detail_rules(struct model const *model, real_point3d const *centroid, real radius);
/* whether the camera is inside a model that has levels of its inside only:
FALSE for any other model, and for a sphere that is not one */
boolean model_detail_camera_inside(struct model const *model, real_point3d const *camera,
	real_point3d const *centroid, real radius);
/* the geometry a permutation draws for the model's level, NONE for none */
short model_detail_geometry_index(struct model const *model,
	struct model_region_permutation const *permutation, short level, short rules);

#endif
