/* Exact collision-input snapshots for stationary visibility fields. Store
   values, never borrowed object/tag pointers. Moving lights still trace at
   their existing cadence; any changed blocker invalidates stationary reuse. */
#include "cseries.h"
#include "objects/objects.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_models.h"
#include "scenario/scenario.h"
#include "plasma_collision.h"
#include <string.h>
#include <math.h>

extern boolean debug_collision_skip_vectors, debug_collision_skip_objects;
static int scene_append(struct plasma_collision_scene *scene, int *offset, const void *data, int size)
{
	int changed;
	if (*offset < 0 || size < 0 || size > PLASMA_COLLISION_SCENE_BYTES - *offset || (!data && size))
	{
		*offset = -1;
		return TRUE;
	}
	if (!size) return FALSE;
	changed = *offset + size > scene->size || memcmp(scene->bytes + *offset, data, size);
	if (changed) memcpy(scene->bytes + *offset, data, size);
	*offset += size;
	return changed;
}

unsigned long plasma_collision_scene_revision(struct plasma_collision_scene *scene,
	real_point3d const *point, float extent, int vehicles)
{
	struct object_iterator iterator;
	struct object_datum *object;
	unsigned long mask = _object_mask_scenery | _object_mask_machine;
	int offset = 0, changed = FALSE;
#define SNAPSHOT(value) changed |= scene_append(scene, &offset, &(value), sizeof(value))
	SNAPSHOT(global_scenario_index);
	SNAPSHOT(global_structure_bsp_index);
	SNAPSHOT(debug_collision_skip_vectors);
	SNAPSHOT(debug_collision_skip_objects);
	changed |= scene_append(scene, &offset, breakable_surface_flags_get(), MAXIMUM_BREAKABLE_SURFACES_PER_MAP / 8);
	if (vehicles) mask |= _object_mask_vehicle;
	object_iterator_new(&iterator, mask, 0);
	while ((object = object_iterator_next(&iterator)) != NULL && offset >= 0)
	{
		unsigned long eligibility = object->object.flags & (FLAG(_object_invisible_bit) | FLAG(_object_has_collision_model_bit));
		int connected = object->object.first_cluster_reference_index != NONE;
		struct collision_model_instance model;
		struct object_cluster_iterator clusters;
		short cluster;
		int has_model;
		real bound = extent + object->object.bounding_sphere_radius;
		real margin = 0.0001f * (1 + fabsf(point->x) + fabsf(point->y) + fabsf(point->z) + fabsf(bound));
		/* Every ray stays in this cube. Objects outside it cannot pass the
		   engine's sphere gate. Include a margin and all borderline/invalid
		   bounds; unrelated animated doors must not invalidate every light. */
		if (object->object.bounding_sphere_radius >= 0 && bound >= 0 && (fabsf(object->object.bounding_sphere_center.x - point->x) > bound + margin ||
			fabsf(object->object.bounding_sphere_center.y - point->y) > bound + margin ||
			fabsf(object->object.bounding_sphere_center.z - point->z) > bound + margin))
			continue;
		SNAPSHOT(iterator.index);
		SNAPSHOT(object->definition_index);
		SNAPSHOT(eligibility);
		SNAPSHOT(connected);
		SNAPSHOT(object->object.type);
		/* Compare actual cluster membership, not allocation handles. Include
		   its terminator so changed list lengths cannot alias following data. */
		cluster = object_get_first_cluster(&clusters, iterator.index);
		for (;;)
		{
			SNAPSHOT(cluster);
			if (cluster == NONE || offset < 0) break;
			cluster = object_get_next_cluster(&clusters, iterator.index);
		}
		SNAPSHOT(object->object.next_object_index);
		SNAPSHOT(object->object.first_child_object_index);
		SNAPSHOT(object->object.parent_object_index);
		SNAPSHOT(object->object.bounding_sphere_center);
		SNAPSHOT(object->object.bounding_sphere_radius);
		SNAPSHOT(object->object.region_permutations);
		has_model = collision_model_instance_new(&model, iterator.index);
		SNAPSHOT(has_model);
		if (has_model)
		{
			SNAPSHOT(model.node_count);
			/* Visual-only nodes cannot change collision. Use the exact node
			   count/poses accepted by the engine's collision instance. */
			changed |= scene_append(scene, &offset, model.matrices, model.node_count * sizeof(real_matrix4x3));
		}
	}
#undef SNAPSHOT
	/* Overflow takes the uncached path on every view. No incomplete snapshot
	   can certify unchanged geometry. Object insertion/removal changes size. */
	if (changed || offset != scene->size || offset < 0 || !scene->revision)
		scene->revision++;
	scene->size = MAX(offset, 0);
	return scene->revision;
}
