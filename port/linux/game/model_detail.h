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

/* the rules for a model drawn this frame, its size in the lines of the
480-line screen given (the level of detail's pixels, as the Xbox measured) */
short model_detail_rules(struct model const *model, real stock_pixels);
/* whether a model that has levels of its inside is as large as the Xbox drew
the first of them at (the camera inside it): FALSE for any other model */
boolean model_detail_inside_reached(struct model const *model, real stock_pixels);
/* the geometry a permutation draws for the model's level, NONE for none */
short model_detail_geometry_index(struct model const *model,
	struct model_region_permutation const *permutation, short level, short rules);

#endif
