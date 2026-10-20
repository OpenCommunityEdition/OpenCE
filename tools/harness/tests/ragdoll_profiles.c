#include "harness.h"
#include <math.h>
#include <strings.h>

/* The ragdolls of the stock bipeds (ragdoll_bodies.c): each model's nodes
and collision pieces as the maps have them (profiles.inc, written by
tools/test_ragdoll_profiles.py), built with ragdolls.txt's overrides, and
printed for the test to check:

	MODEL <name> BODY <parts> <mass>   or   MODEL <name> NONE
	PART <part> <node> <parent part> <shape> <size x3> <centre x3> <mass> <twist min max> <swing normal plane> <break damage>
	NODE <node> <part> */
struct tag_block { long count; void *address; long definition; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address + (index))
typedef struct { real_vector3d v; real w; } real_quaternion;
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
struct model_node { char name[32]; short next_sibling_node_index, first_child_node_index, parent_node_index; short pad;
	real_point3d default_translation; real_quaternion default_rotation; real node_distance_from_parent; long unused[8];
	real_matrix4x3 runtime_default_inverse_matrix; };
struct collision_vertex { real_point3d point; long first_edge_index; };
struct collision_bsp { struct tag_block vertices; };
struct collision_node { char name[32]; short region_index, parent_node_index, next_sibling_node_index, first_child_node_index;
	long unused[2]; short pad; short runtime_damage_part; struct tag_block bsps; };
#define _pi ((real)3.14159265358979323846f)
#define DEGREES_TO_RADIANS(angle) ((real)(((real)angle) * ((real)M_PI) / 180.f))
#define square_root(x) sqrtf(x)
#define csstrcasecmp(a, b) strcasecmp(a, b)

#include "config.inc"
#include "under_test.inc"

/* the profiles: models, their nodes, their collision pieces, the pieces' vertices */
struct profile_model { char const *name; long first_node, node_count, first_piece, piece_count; };
struct profile_node { char const *name; short parent; real x, y, z; };
struct profile_piece { char const *name; long first_vertex, vertex_count; };
#include "profiles.inc"

#define COUNT(a) ((long)(sizeof(a) / sizeof((a)[0])))

static struct model_node nodes[RAGDOLL_BODY_NODES_MAX + 1];
static struct collision_node pieces[256];
static struct collision_bsp bsps[256];
static struct collision_vertex vertices[65536];
static struct ragdoll_body body;

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long model_index;

	CASE("all")
	{
		for (model_index = 0; model_index < COUNT(profile_models); model_index++)
		{
			struct profile_model const *model = &profile_models[model_index];
			struct tag_block model_block, piece_block;
			long index;
			long vertex_count = 0;

			CHECK(model->node_count <= RAGDOLL_BODY_NODES_MAX + 1 && model->piece_count <= COUNT(pieces),
				"%s: too big for the test", model->name);
			memset(nodes, 0, sizeof(nodes));
			memset(pieces, 0, sizeof(pieces));
			for (index = 0; index < model->node_count; index++)
			{
				struct profile_node const *node = &profile_nodes[model->first_node + index];

				strncpy(nodes[index].name, node->name, 31);
				nodes[index].parent_node_index = node->parent;
				nodes[index].default_translation.x = node->x;
				nodes[index].default_translation.y = node->y;
				nodes[index].default_translation.z = node->z;
			}
			for (index = 0; index < model->piece_count; index++)
			{
				struct profile_piece const *piece = &profile_pieces[model->first_piece + index];
				long vertex;

				CHECK(vertex_count + piece->vertex_count <= COUNT(vertices), "%s: too many vertices", model->name);
				strncpy(pieces[index].name, piece->name, 31);
				pieces[index].bsps.count = piece->vertex_count ? 1 : 0;
				pieces[index].bsps.address = &bsps[index];
				bsps[index].vertices.count = piece->vertex_count;
				bsps[index].vertices.address = &vertices[vertex_count];
				for (vertex = 0; vertex < piece->vertex_count; vertex++)
				{
					real const *point = &profile_vertices[3 * (piece->first_vertex + vertex)];

					vertices[vertex_count + vertex].point.x = point[0];
					vertices[vertex_count + vertex].point.y = point[1];
					vertices[vertex_count + vertex].point.z = point[2];
				}
				vertex_count += piece->vertex_count;
			}
			model_block.count = model->node_count;
			model_block.address = nodes;
			piece_block.count = model->piece_count;
			piece_block.address = pieces;
			if (!ragdoll_body_build(model->name, &model_block, model->piece_count ? &piece_block : NULL,
				profile_overrides, (long)sizeof(profile_overrides) - 1, &body))
			{
				printf("MODEL %s NONE\n", model->name);
				continue;
			}
			printf("MODEL %s BODY %d %.4f\n", model->name, body.part_count, body.mass);
			for (index = 0; index < body.part_count; index++)
			{
				struct ragdoll_part const *part = &body.parts[index];

				printf("PART %ld %d %d %d %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f\n", index,
					part->node_index, part->parent_part_index, part->shape, part->size[0], part->size[1], part->size[2],
					part->center.x, part->center.y, part->center.z, part->mass, part->twist_min, part->twist_max,
					part->swing_normal, part->swing_plane, part->break_damage);
			}
			for (index = 0; index < body.node_count; index++)
				printf("NODE %ld %d\n", index, body.node_parts[index]);
		}
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
