/* The model-major phase of render-only ray batches. The stock query transforms
   the same origin and looks up the same node/permutation for each ray. Here
   one node serves all eligible rays before its borrowed data is discarded. */
#include "cseries.h"
#include "plasma_collision.h"
#include "physics/collisions.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/collision_models.h"
#include "objects/object_definitions.h"
#include <string.h>

void plasma_collision_model_depths(struct collision_model_instance const *instance,
	real_point3d const *point, real_vector3d const *vectors, int count,
	unsigned int const *pending, float *depths)
{
	real closest[PLASMA_COLLISION_RAYS];
	struct collision_bsp_vector_cache cache;
	unsigned long flags = FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit);
	int ray;
	short node_index;
	/* Invalid regions keep the original diagnostics and query behavior.
	   Validate before changing any output, so fallback cannot mix results. */
	if (count < 1 || count > PLASMA_COLLISION_RAYS) goto fallback;
	for (node_index = 0; node_index < instance->node_count; node_index++)
	{
		struct collision_node const *node = TAG_BLOCK_GET_ELEMENT(&instance->model->nodes, node_index, struct collision_node);
		if (node->region_index != NONE && (node->region_index < 0 || node->region_index >= MAXIMUM_REGIONS_PER_OBJECT)) goto fallback;
	}
	for (ray = 0; ray < count; ray++) closest[ray] = REAL_MAX;
	for (node_index = 0; node_index < instance->node_count; node_index++)
	{
		struct collision_node const *node = TAG_BLOCK_GET_ELEMENT(&instance->model->nodes, node_index, struct collision_node);
		short permutation;
		struct collision_bsp const *bsp;
		real_matrix4x3 inverse;
		real_point3d origin;
		if (node->region_index == NONE) continue;
		permutation = instance->region_permutation_indices[node->region_index];
		if (permutation == NONE || node->bsps.count <= 0) continue;
		permutation = PIN(permutation, 0, node->bsps.count - 1);
		bsp = TAG_BLOCK_GET_ELEMENT(&node->bsps, permutation, struct collision_bsp);
		if (bsp->bsp3d.nodes.count <= 0) continue;
		matrix4x3_inverse(&instance->matrices[node_index], &inverse);
		matrix4x3_transform_point(&inverse, point, &origin);
		memset(&cache, 0, sizeof(cache));
		for (ray = 0; ray < count; ray++)
			if (pending[ray / 32] & (1U << (ray % 32)))
			{
				real_vector3d vector;
				struct collision_bsp_test_vector_result result;
				matrix4x3_transform_vector(&inverse, &vectors[ray], &vector);
				/* Same node order and nearest-node limit as the original model
				   query. Cache only the fixed BSP/origin inside this node. */
				if (collision_bsp_test_vector_from_node(flags, bsp, 0, NULL, &origin, &vector,
					closest[ray], &result, 0, 0, &cache)) closest[ray] = result.t;
			}
	}
	for (ray = 0; ray < count; ray++)
		if (closest[ray] < 1.0f && closest[ray] < depths[ray]) depths[ray] = closest[ray];
	return;
fallback:
	for (ray = 0; ray < count; ray++)
		if (pending[ray / 32] & (1U << (ray % 32)))
		{
			struct collision_model_test_vector_result result;
			if (collision_model_test_vector(instance, flags, point, &vectors[ray], &result) &&
				result.bsp_result.t < 1.0f && result.bsp_result.t < depths[ray]) depths[ray] = result.bsp_result.t;
		}
}
