/*
STRUCTURE_BSP_DEFINITIONS.C
*/

/* ---------- headers */

#include "cseries.h"
#include "structure_bsp_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* port: the pvs of a cluster (from the map) that is not one of the bsp's,
or whose pvs is not in the map's cluster data: no cluster is visible */
static unsigned long structure_bsp_empty_cluster_pvs[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_CLUSTERS_PER_STRUCTURE)];

/* ---------- public code */

/* port: whether a map's bsp fits what the engine holds of one, as the code
that traverses it trusts: it has a collision bsp and clusters, its clusters
and surfaces fit the engine's arrays and bit vectors of them (512 clusters,
0x20000 surfaces), and its clusters' pvs and sound data are all there.
Every retail bsp does. Cheap enough to check where the bsp is used */
boolean structure_bsp_port_verify(
	struct structure_bsp const *structure_bsp)
{
	long cluster_count = structure_bsp->clusters.count;

	return structure_bsp->collision_bsp.count > 0 &&
		cluster_count > 0 &&
		cluster_count <= MAXIMUM_CLUSTERS_PER_STRUCTURE &&
		structure_bsp->surfaces.count >= 0 &&
		structure_bsp->surfaces.count <= MAXIMUM_SURFACES_PER_STRUCTURE &&
		structure_bsp->cluster_data.size >= cluster_count * (long)BIT_VECTOR_SIZE_IN_BYTES(cluster_count) &&
		structure_bsp->sound_cluster_data.size >= cluster_count * (cluster_count - 1) / 2;
}

unsigned long *structure_bsp_get_cluster_pvs(
	struct structure_bsp *structure_bsp,
	short cluster_index)
{
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 36, cluster_index>=0 && cluster_index<structure_bsp->clusters.count);
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c",
		37,
		(cluster_index+1)*BIT_VECTOR_SIZE_IN_LONGS(structure_bsp->clusters.count)<=structure_bsp->cluster_data.size);

	if (cluster_index < 0 ||
		cluster_index >= structure_bsp->clusters.count ||
		structure_bsp->clusters.count > MAXIMUM_CLUSTERS_PER_STRUCTURE ||
		(cluster_index + 1) * (long)BIT_VECTOR_SIZE_IN_BYTES(structure_bsp->clusters.count) >
			structure_bsp->cluster_data.size)
	{
		return structure_bsp_empty_cluster_pvs;
	}

	// Get pointer to bitvector starting at the cluster index
	return (unsigned long *)(
		(byte *)structure_bsp->cluster_data.address +
		sizeof(unsigned long) * cluster_index *
		BIT_VECTOR_SIZE_IN_LONGS(structure_bsp->clusters.count));
}

/* port: FALSE (with neither index usable) if no lightmap's material has
the surface: the map's lightmaps and materials are searched as they are
given, and what a malformed map's search ends on is not used */
boolean structure_bsp_find_material_for_surface(
	struct structure_bsp *structure,
	long surface_index,
	short *lightmap_index,
	short *material_index)
{
	struct structure_lightmap *lightmap;
	struct structure_material *material;
	short lightmap_last_index;
	short material_last_index;
	short i;

	i =0;
	*lightmap_index = 0;
	*material_index = 0;
	lightmap_last_index = structure->lightmaps.count-1;

	if (structure->lightmaps.count <= 0)
	{
		return FALSE;
	}

	while (lightmap_last_index>i)
	{
		struct structure_lightmap *curr_lightmap;

		*lightmap_index = (lightmap_last_index-i) / 2+i;
		curr_lightmap = TAG_BLOCK_GET_ELEMENT(&structure->lightmaps, *lightmap_index, struct structure_lightmap);

		/* port: a lightmap searched has materials to compare (the retail
		maps' lightmaps without materials are never searched) */
		if (curr_lightmap->materials.count <= 0)
		{
			return FALSE;
		}

		if (surface_index<TAG_BLOCK_GET_ELEMENT(&curr_lightmap->materials, 0, struct structure_material)->first_surface_index)
		{
			lightmap_last_index = *lightmap_index-1;
			*lightmap_index = lightmap_last_index;
		}
		else
		{
			if (surface_index<
				TAG_BLOCK_GET_ELEMENT(&curr_lightmap->materials, curr_lightmap->materials.count-1, struct structure_material)->surface_count+
				TAG_BLOCK_GET_ELEMENT(&curr_lightmap->materials, curr_lightmap->materials.count-1, struct structure_material)->first_surface_index)
			{
				break;
			}

			i = *lightmap_index+1;
			*lightmap_index = i;
		}
	}

	/* port: the search ends before the first lightmap if the surface is
	before every lightmap's */
	if (*lightmap_index < 0 || *lightmap_index >= structure->lightmaps.count)
	{
		return FALSE;
	}

	lightmap = TAG_BLOCK_GET_ELEMENT(&structure->lightmaps, *lightmap_index, struct structure_lightmap);

	i =0;
	*material_index = 0;
	material_last_index = lightmap->materials.count;

	if (lightmap->materials.count <= 0)
	{
		return FALSE;
	}

	while (i<material_last_index)
	{
		const struct structure_material *curr_material;

		*material_index = (material_last_index-i) / 2+i;
		curr_material = TAG_BLOCK_GET_ELEMENT(&lightmap->materials, *material_index, struct structure_material);

		if (surface_index<curr_material->first_surface_index)
		{
			material_last_index = *material_index-1;
			*material_index = material_last_index;
		}
		else
		{
			if (surface_index<curr_material->first_surface_index+curr_material->surface_count)
			{
				break;
			}

			i = *material_index+1;
			*material_index = i;
		}
		
	}

	/* port: and before or after the lightmap's materials if the surface is
	in none of them */
	if (*material_index < 0 || *material_index >= lightmap->materials.count)
	{
		return FALSE;
	}

	material = TAG_BLOCK_GET_ELEMENT(&lightmap->materials, *material_index, struct structure_material);

	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 102, surface_index>=material->first_surface_index);
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 103, surface_index<material->first_surface_index+material->surface_count);

	return surface_index >= material->first_surface_index &&
		surface_index < material->first_surface_index + material->surface_count;
}

void vertex_type_from_shader_tag(
	unsigned long group_tag,
	short *vertex_type,
	short *lightmap_vertex_type,
	boolean compressed)
{
	if (compressed)
	{
		*vertex_type = _rasterizer_vertex_type_environment_compressed;
		*lightmap_vertex_type = _rasterizer_vertex_type_environment_lightmap_compressed;
	}
	else
	{
		*vertex_type = _rasterizer_vertex_type_environment_uncompressed;
		*lightmap_vertex_type = _rasterizer_vertex_type_environment_lightmap_uncompressed;
	}
	
	return;
}

/* port: NULL if the clusters are not two of the bsp's (row first) or their
pair's byte is not in the map's sound data. The offset is a long: a short
wrapped past 256 clusters */
byte *structure_bsp_get_cluster_encoded_sound_data(
	struct structure_bsp *structure_bsp,
	short row_index,
	short column_index)
{
	long offset = row_index * (structure_bsp->clusters.count-1)-row_index*(row_index+1)/2+column_index-1;

	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1202, row_index<column_index);
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1203, offset>=0 && offset<structure_bsp->sound_cluster_data.size);

	if (row_index < 0 ||
		row_index >= column_index ||
		column_index >= structure_bsp->clusters.count ||
		offset < 0 ||
		offset >= structure_bsp->sound_cluster_data.size)
	{
		return NULL;
	}

	return &((byte *)structure_bsp->sound_cluster_data.address)[offset];
}

byte structure_bsp_get_cluster_encoded_sound_distance(
	struct structure_bsp *structure_bsp,
	short from_cluster_index,
	short to_cluster_index)
{
	byte result;

	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1215, from_cluster_index>=0 && from_cluster_index<structure_bsp->clusters.count);
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1216, to_cluster_index>=0 && to_cluster_index<structure_bsp->clusters.count);

	if (from_cluster_index!=to_cluster_index)
	{
		byte *encoded_sound_data;

		if (from_cluster_index>to_cluster_index)
		{
			short temp = from_cluster_index;
			from_cluster_index = to_cluster_index;
			to_cluster_index = temp;
		}

		encoded_sound_data = structure_bsp_get_cluster_encoded_sound_data(
			structure_bsp,
			from_cluster_index,
			to_cluster_index);

		/* port: clusters (from the map) that are not two of the bsp's are
		unreachable from each other, at the greatest distance */
		result = encoded_sound_data ? *encoded_sound_data : UNSIGNED_CHAR_MAX;
	}
	else
	{
		result = 0;
	}

	return result;
}


/* ---------- private code */
