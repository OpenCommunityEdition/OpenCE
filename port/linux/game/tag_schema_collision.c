/*
TAG_SCHEMA_COLLISION.C

The schemas (tag_schema.h) of collision models, physics and structure bsps
(coll, phys, pphy, sbsp), and the checks of their graphs: a collision bsp's
3d and 2d trees and its surfaces' edge rings (both a collision model's and a
structure bsp's), and a structure bsp's render data (its materials' vertices
and the clusters' surface lists that draw them), visibility, fog, breakable
surfaces and detail objects.
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_schema.h"
#include "bitmaps/bitmap_group.h"
#include "cache/predicted_resources.h"
#include "game/game_globals.h"
#include "math/integer_math.h"
#include "math/periodic_functions.h"
#include "models/model_definitions.h"
#include "objects/object_definitions.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/physics_definitions.h"
/* (which names its group tag as physics_definitions.h does phys's) */
#define POINT_PHYSICS_DEFINITION_TAG POINT_PHYSICS_POINT_DEFINITION_TAG
#define POINT_PHYSICS_DEFINITION_VERSION POINT_PHYSICS_POINT_DEFINITION_VERSION
#include "physics/point_physics.h"
#undef POINT_PHYSICS_DEFINITION_TAG
#undef POINT_PHYSICS_DEFINITION_VERSION
#include "rasterizer/rasterizer_geometry.h"
#include "render/render.h"
#include "structures/leaf_map.h"
#include "structures/structure_bsp_definitions.h"
#include "structures/structures.h"
#include "tag_files/tag_files.h"

#include <string.h>

/* ---------- constants */

enum
{
	/* collision_bsp.c (its plane stack: a bsp3d is walked no deeper) */
	MAXIMUM_BSP3D_DEPTH = 128,
	/* how deep a bsp2d may be, which the game walks recursively (the
	retail ones are at most 8 deep) */
	MAXIMUM_BSP2D_DEPTH = 128,
	/* how many edges a collision vertex may have, which path_smoothing.c
	walks around (the retail ones have at most 47) */
	/* collision_bsp_definitions.c, the game's own tag definitions of a
	collision bsp's blocks */
	MAXIMUM_BSP3D_NODES_PER_COLLISION_BSP = 0x20000,
	MAXIMUM_PLANES_PER_COLLISION_BSP = 0x10000,
	MAXIMUM_LEAVES_PER_COLLISION_BSP = 0x10000,
	MAXIMUM_BSP2D_NODES_PER_COLLISION_BSP = 0xFFFF,
	/* physics.c, friction_evaluate */
	NUMBER_OF_FRICTION_TYPES = 4,
	/* (MAXIMUM_MASS_POINTS_PER_PHYSICS, MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS:
	physics_definitions.h, vehicle_update's mass_points[32], powered_mass_points[32]) */
	/* physics.c reads the second of the two matrices */
	NUMBER_OF_INERTIAL_MATRICES = 2,
	/* structure_visibility.c, portal_hull_from_points' viewer_points
	(a mirror's points are seen through it) */
	MAXIMUM_PORTAL_HULL_VERTICES = 256,
	/* structure_detail_objects.c (a cell's valid_layers) */
	MAXIMUM_DETAIL_OBJECT_LAYERS = 32,
	/* the size of a compressed structure vertex and of its lightmap
	vertex, which follow each other in a material's compressed data
	(structures.c, object_lights.c) */
	COMPRESSED_ENVIRONMENT_VERTEX_SIZE = 0x20,
	COMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE = 0x8,
	/* (rasterizer_geometry.c's sizes of the others) */
	UNCOMPRESSED_ENVIRONMENT_VERTEX_SIZE = 0x38,
	UNCOMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE = 0x14,

	/* the tool's limits, where the game has none of its own */
	MAXIMUM_MATERIALS_PER_COLLISION_MODEL = 32,
	MAXIMUM_PERMUTATIONS_PER_DAMAGE_REGION = 32,
	MAXIMUM_BSPS_PER_COLLISION_NODE = 32,
	MAXIMUM_PATHFINDING_SPHERES_PER_COLLISION_MODEL = 32,
	MAXIMUM_COLLISION_BSPS_PER_STRUCTURE = 1,
	MAXIMUM_NODES_PER_STRUCTURE = 0x20000,
	MAXIMUM_LEAVES_PER_STRUCTURE = 0x10000,
	MAXIMUM_PREDICTED_RESOURCES_PER_CLUSTER = 1024,
	MAXIMUM_VERTICES_PER_FOG_PLANE = MAXIMUM_VERTICES_PER_STRUCTURE_FOG_PLANE,
	MAXIMUM_PATHFINDING_SURFACES_PER_STRUCTURE = MAXIMUM_SURFACES_PER_COLLISION_BSP,
	MAXIMUM_PATHFINDING_EDGES_PER_STRUCTURE = MAXIMUM_EDGES_PER_COLLISION_BSP,
	MAXIMUM_SOUND_CLUSTER_DATA_SIZE = MAXIMUM_CLUSTER_DATA_SIZE,
	MAXIMUM_DETAIL_OBJECT_DATA_PER_STRUCTURE = 1,
	MAXIMUM_DETAIL_OBJECT_CELLS = 0x40000,
	MAXIMUM_DETAIL_OBJECTS = 0x200000,
	MAXIMUM_DETAIL_OBJECT_COUNTS = 0x800000,
	MAXIMUM_LEAF_MAP_LEAVES = 0x10000,
	MAXIMUM_LEAF_MAP_PORTALS = 0x20000,
	MAXIMUM_FACES_PER_LEAF_MAP_LEAF = 0x10000,
	MAXIMUM_PORTALS_PER_LEAF_MAP_LEAF = 0x10000,
	MAXIMUM_VERTICES_PER_LEAF_MAP_FACE = 0x10000,
	MAXIMUM_VERTICES_PER_LEAF_MAP_PORTAL = 0x10000,
};

/* ---------- structures */

/* collision_bsp.c */
struct collision_leaf
{
	word flags;
	short bsp2d_reference_count;
	long first_bsp2d_reference_index;
};

struct bsp2d_reference
{
	long plane_designator;
	long root_index;
};

typedef char verify_collision_leaf_size[sizeof(struct collision_leaf) == 0x8 ? 1 : -1];
typedef char verify_bsp2d_reference_size[sizeof(struct bsp2d_reference) == 0x8 ? 1 : -1];

/* damage.c */
struct damage_region
{
	char name[TAG_STRING_LENGTH+1];
	unsigned long flags;
	long unused0;
	real damage_threshold;
	long unused1[3];
	struct tag_reference destroyed_effect;
	struct tag_block permutations;
};

/* a damage region's permutation (only counted: damage.c,
object_permutation_shield_regions) */
struct damage_permutation
{
	char name[TAG_STRING_LENGTH+1];
};

typedef char verify_damage_region_size[sizeof(struct damage_region) == 0x54 ? 1 : -1];
typedef char verify_damage_permutation_size[sizeof(struct damage_permutation) == 0x20 ? 1 : -1];

/* physics.c */
struct powered_mass_point_definition
{
	char name[32];
	unsigned long flags;
	real antigrav_strength;
	real antigrav_offset;
	real antigrav_height;
	real antigrav_damp_fraction;
	real antigrav_normal_k1;
	real antigrav_normal_k0;
	real unused[17];
};

typedef char verify_powered_mass_point_definition_size[
	sizeof(struct powered_mass_point_definition) == 0x80 ? 1 : -1];

/* structure_visibility.c's structure_visibility_cluster (structures.c's
structure_cluster_graph) */
struct structure_cluster_schema
{
	short sky_index;
	short fog_designator;
	short background_sound_palette_index;
	short sound_environment_palette_index;
	short weather_palette_index;
	short transition_structure_bsp_index;
	short first_runtime_decal_index;
	word runtime_decal_count;
	long unused[6];
	struct tag_block predicted_resources;
	struct tag_block subclusters;
	word first_lens_flare_marker_index;
	word lens_flare_marker_count;
	struct tag_block surface_indices;
	struct tag_block mirrors;
	struct tag_block portal_indices;
};

/* structure_visibility.c */
struct structure_subcluster
{
	real_rectangle3d world_bounds;
	struct tag_block surface_indices;
};

struct structure_mirror
{
	real_plane3d plane;
	long unused[5];
	struct tag_reference shader;
	struct tag_block points;
};

/* a cluster's portal (structure_visibility.c reads them as shorts) */
struct structure_portal_index
{
	short portal_index;
};

/* structures.c */
struct structure_cluster_portal
{
	short cluster_indices[2];
	long plane_index;
	real_point3d centroid;
	real bounding_radius;
	unsigned long flags;
	long unused[6];
	struct tag_block vertices;
};

struct structure_surface_reference
{
	long surface_index;
	long bsp3d_node_index;
};

/* structures.c's structure_fog_plane_render, with
structure_bsp_definitions.h's runtime_material_type */
struct structure_fog_plane_schema
{
	short region_index;
	short runtime_material_type;
	real_plane3d plane;
	struct tag_block vertices;
};

typedef char verify_structure_cluster_schema_size[sizeof(struct structure_cluster_schema) == 0x68 ? 1 : -1];
typedef char verify_structure_subcluster_size[sizeof(struct structure_subcluster) == 0x24 ? 1 : -1];
typedef char verify_structure_mirror_size[sizeof(struct structure_mirror) == 0x40 ? 1 : -1];
typedef char verify_structure_cluster_portal_size[sizeof(struct structure_cluster_portal) == 0x40 ? 1 : -1];
typedef char verify_structure_surface_reference_size[sizeof(struct structure_surface_reference) == 0x8 ? 1 : -1];
typedef char verify_structure_fog_plane_schema_size[
	sizeof(struct structure_fog_plane_schema) == sizeof(struct structure_fog_plane) ? 1 : -1];

/* structure_lens_flares.c */
struct structure_lens_flare
{
	struct tag_reference lens_flare;
};

struct structure_lens_flare_marker
{
	real_point3d position;
	char direction[3];
	byte lens_flare_index;
};

typedef char verify_structure_lens_flare_marker_size[sizeof(struct structure_lens_flare_marker) == 0x10 ? 1 : -1];

/* wind.c, weather_particle_systems.c */
struct structure_weather_palette_entry
{
	char name[32];
	struct tag_reference particle_system;
	word pad30;
	short runtime_particle_system_global_function_index;
	char particle_system_global_function_name[32];
	long particle_system_unused[11];
	struct tag_reference wind;
	real_vector3d wind_direction;
	real wind_magnitude;
	word padA0;
	short wind_global_function_index;
	char wind_global_function_name[32];
	long wind_unused[11];
};

struct structure_weather_polyhedron
{
	real_point3d bounding_sphere_center;
	real bounding_sphere_radius;
	long unused;
	struct tag_block planes;
};

typedef char verify_structure_weather_palette_entry_size[
	sizeof(struct structure_weather_palette_entry) == 0xF0 ? 1 : -1];
typedef char verify_structure_weather_polyhedron_size[sizeof(struct structure_weather_polyhedron) == 0x20 ? 1 : -1];

/* structure_detail_objects.c */
struct detail_object_cell_definition
{
	short cell_x;
	short cell_y;
	short cell_z;
	short offset_z;
	unsigned long valid_layers;
	long start_index;
	long count_index;
	long unused14[3];
};

struct structure_detail_object_data
{
	struct tag_block cells;
	struct tag_block detail_objects;
	struct tag_block counts;
	struct tag_block z_reference_vectors;
	byte valid;
	byte pad31[3];
	long unused34[3];
};

struct detail_object
{
	byte position[3];
	byte data;
	word color;
};

typedef char verify_detail_object_cell_definition_size[
	sizeof(struct detail_object_cell_definition) == 0x20 ? 1 : -1];
typedef char verify_structure_detail_object_data_size[sizeof(struct structure_detail_object_data) == 0x40 ? 1 : -1];
typedef char verify_detail_object_size[sizeof(struct detail_object) == 0x6 ? 1 : -1];

/* leaf_map.c */
struct map_leaf_face
{
	long node_index;
	struct tag_block vertices;
};

struct map_leaf
{
	struct tag_block faces;
	struct tag_block portal_designators;
};

typedef char verify_map_leaf_face_size[sizeof(struct map_leaf_face) == 0x10 ? 1 : -1];
typedef char verify_map_leaf_size[sizeof(struct map_leaf) == 0x18 ? 1 : -1];

/* the bsp graph checks' marks: a bit for each node */
static unsigned long collision_bsp_visited[
	BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_BSP3D_NODES_PER_COLLISION_BSP)];

/* ---------- prototypes */

static boolean collision_model_bsp_check(struct tag_validation *validation, void *base);
static boolean structure_collision_bsp_check(struct tag_validation *validation, void *base);
static boolean structure_detail_object_data_check(struct tag_validation *validation, void *base);
static boolean structure_bsp_check(struct tag_validation *validation, void *base);

/* ---------- globals */

/* elements with nothing to check */

static struct tag_schema_field const empty_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const long_schema =
	TAG_SCHEMA_DEFINITION(long, long, empty_fields);
static struct tag_schema_definition const byte_schema =
	TAG_SCHEMA_DEFINITION(byte, byte, empty_fields);
static struct tag_schema_definition const real_point2d_schema =
	TAG_SCHEMA_DEFINITION(real_point2d, real_point2d, empty_fields);
static struct tag_schema_definition const real_point3d_schema =
	TAG_SCHEMA_DEFINITION(real_point3d, real_point3d, empty_fields);
static struct tag_schema_definition const real_plane3d_schema =
	TAG_SCHEMA_DEFINITION(real_plane3d, real_plane3d, empty_fields);

/* collision bsps (a collision model's and a structure bsp's: the same but
for what their surfaces' materials index) */

static struct tag_schema_field const bsp3d_node_fields[] =
{
	/* (its children: collision_bsp_graph_check) */
	TAG_SCHEMA_BLOCK_INDEX(struct bsp3d_node, plane_designator, 1, offsetof(struct collision_bsp, bsp3d.planes), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const bsp3d_node_schema =
	TAG_SCHEMA_DEFINITION(bsp3d_node, struct bsp3d_node, bsp3d_node_fields);

/* (a leaf's bsp2d references, a bsp2d reference's and a bsp2d node's:
collision_bsp_graph_check) */
static struct tag_schema_definition const collision_leaf_schema =
	TAG_SCHEMA_DEFINITION(collision_leaf, struct collision_leaf, empty_fields);
static struct tag_schema_definition const bsp2d_reference_schema =
	TAG_SCHEMA_DEFINITION(bsp2d_reference, struct bsp2d_reference, empty_fields);
static struct tag_schema_definition const bsp2d_node_schema =
	TAG_SCHEMA_DEFINITION(bsp2d_node, struct bsp2d_node, empty_fields);

/* (a surface's plane and edge ring: collision_bsp_graph_check) */
static struct tag_schema_field const collision_model_surface_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct collision_surface, first_edge_index, 1, offsetof(struct collision_bsp, edges), 0),
	TAG_SCHEMA_BLOCK_INDEX(struct collision_surface, material_index, TAG_SCHEMA_ROOT,
		offsetof(struct collision_model, resistance.materials), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_field const structure_collision_surface_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct collision_surface, first_edge_index, 1, offsetof(struct collision_bsp, edges), 0),
	TAG_SCHEMA_BLOCK_INDEX(struct collision_surface, material_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, collision_materials), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const collision_model_surface_schema =
	TAG_SCHEMA_DEFINITION(collision_surface, struct collision_surface, collision_model_surface_fields);
static struct tag_schema_definition const structure_collision_surface_schema =
	TAG_SCHEMA_DEFINITION(collision_surface, struct collision_surface, structure_collision_surface_fields);

static struct tag_schema_field const collision_edge_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX_ARRAY(struct collision_edge, vertex_indices, 1, offsetof(struct collision_bsp, vertices), 0),
	TAG_SCHEMA_BLOCK_INDEX_ARRAY(struct collision_edge, edge_indices, 1, offsetof(struct collision_bsp, edges), 0),
	/* (none for an edge with no surface on that side) */
	TAG_SCHEMA_BLOCK_INDEX_ARRAY(struct collision_edge, surface_indices, 1, offsetof(struct collision_bsp, surfaces),
		FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const collision_edge_schema =
	TAG_SCHEMA_DEFINITION(collision_edge, struct collision_edge, collision_edge_fields);

static struct tag_schema_field const collision_vertex_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct collision_vertex, first_edge_index, 1, offsetof(struct collision_bsp, edges), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const collision_vertex_schema =
	TAG_SCHEMA_DEFINITION(collision_vertex, struct collision_vertex, collision_vertex_fields);

/* (its graph once every index in it is: collision_bsp_graph_check) */
#define COLLISION_BSP_FIELDS(surface_schema, check) \
	TAG_SCHEMA_BLOCK(struct collision_bsp, bsp3d.planes, real_plane3d_schema, MAXIMUM_PLANES_PER_COLLISION_BSP), \
	TAG_SCHEMA_BLOCK(struct collision_bsp, bsp3d.nodes, bsp3d_node_schema, MAXIMUM_BSP3D_NODES_PER_COLLISION_BSP), \
	TAG_SCHEMA_BLOCK(struct collision_bsp, leaves, collision_leaf_schema, MAXIMUM_LEAVES_PER_COLLISION_BSP), \
	TAG_SCHEMA_BLOCK(struct collision_bsp, bsp2d_references, bsp2d_reference_schema, \
		MAXIMUM_BSP2D_REFERENCES_PER_COLLISION_BSP), \
	TAG_SCHEMA_BLOCK(struct collision_bsp, bsp2d.nodes, bsp2d_node_schema, MAXIMUM_BSP2D_NODES_PER_COLLISION_BSP), \
	TAG_SCHEMA_BLOCK(struct collision_bsp, vertices, collision_vertex_schema, MAXIMUM_VERTICES_PER_COLLISION_BSP), \
	TAG_SCHEMA_BLOCK(struct collision_bsp, edges, collision_edge_schema, MAXIMUM_EDGES_PER_COLLISION_BSP), \
	TAG_SCHEMA_BLOCK(struct collision_bsp, surfaces, surface_schema, MAXIMUM_SURFACES_PER_COLLISION_BSP), \
	TAG_SCHEMA_CHECK(check), \
	TAG_SCHEMA_END

static struct tag_schema_field const collision_model_bsp_fields[] =
{
	COLLISION_BSP_FIELDS(collision_model_surface_schema, collision_model_bsp_check)
};

static struct tag_schema_field const structure_collision_bsp_fields[] =
{
	COLLISION_BSP_FIELDS(structure_collision_surface_schema, structure_collision_bsp_check)
};

static struct tag_schema_definition const collision_model_bsp_schema =
	TAG_SCHEMA_DEFINITION(collision_bsp, struct collision_bsp, collision_model_bsp_fields);
static struct tag_schema_definition const structure_collision_bsp_schema =
	TAG_SCHEMA_DEFINITION(collision_bsp, struct collision_bsp, structure_collision_bsp_fields);

/* collision models */

static struct tag_schema_field const damage_resistance_material_fields[] =
{
	TAG_SCHEMA_ENUM(struct damage_resistance_material, material_type, NUMBER_OF_MATERIAL_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const damage_resistance_material_schema =
	TAG_SCHEMA_DEFINITION(damage_resistance_material, struct damage_resistance_material,
		damage_resistance_material_fields);

static struct tag_schema_definition const damage_permutation_schema =
	TAG_SCHEMA_DEFINITION(damage_permutation, struct damage_permutation, empty_fields);

static struct tag_schema_field const damage_region_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct damage_region, destroyed_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_BLOCK(struct damage_region, permutations, damage_permutation_schema,
		MAXIMUM_PERMUTATIONS_PER_DAMAGE_REGION),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const damage_region_schema =
	TAG_SCHEMA_DEFINITION(damage_region, struct damage_region, damage_region_fields);

static struct tag_schema_field const damage_resistance_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct damage_resistance, indirect_damage_material_index, TAG_SCHEMA_STRUCTURE,
		offsetof(struct damage_resistance, materials), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, localized_damage_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, area_damage_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, body_damaged_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, body_depleted_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, body_destroyed_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_ENUM(struct damage_resistance, shield_material_type, NUMBER_OF_MATERIAL_TYPES, 0),
	TAG_SCHEMA_ENUM(struct damage_resistance, shield_failure_function, NUMBER_OF_TRANSITION_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, shield_damaged_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, shield_depleted_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct damage_resistance, shield_recharging_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_BLOCK(struct damage_resistance, materials, damage_resistance_material_schema,
		MAXIMUM_MATERIALS_PER_COLLISION_MODEL),
	/* (an object's region permutations hold MAXIMUM_REGIONS_PER_OBJECT) */
	TAG_SCHEMA_BLOCK(struct damage_resistance, regions, damage_region_schema, MAXIMUM_REGIONS_PER_OBJECT),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const damage_resistance_schema =
	TAG_SCHEMA_DEFINITION(damage_resistance, struct damage_resistance, damage_resistance_fields);

static struct tag_schema_field const pathfinding_sphere_fields[] =
{
	/* (a node of the object's model, of which it has no more than this) */
	TAG_SCHEMA_ENUM(struct pathfinding_sphere, node_index, MAXIMUM_NODES_PER_MODEL, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const pathfinding_sphere_schema =
	TAG_SCHEMA_DEFINITION(pathfinding_sphere, struct pathfinding_sphere, pathfinding_sphere_fields);

static struct tag_schema_field const collision_node_fields[] =
{
	TAG_SCHEMA_STRING(struct collision_node, name),
	TAG_SCHEMA_BLOCK_INDEX(struct collision_node, region_index, TAG_SCHEMA_ROOT,
		offsetof(struct collision_model, resistance.regions), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct collision_node, parent_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct collision_model, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct collision_node, next_sibling_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct collision_model, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct collision_node, first_child_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct collision_model, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK(struct collision_node, bsps, collision_model_bsp_schema, MAXIMUM_BSPS_PER_COLLISION_NODE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const collision_node_schema =
	TAG_SCHEMA_DEFINITION(collision_node, struct collision_node, collision_node_fields);

/* (a collision node indexes the object's node matrices, as many as its
model has nodes) */
static struct tag_schema_field const collision_model_fields[] =
{
	TAG_SCHEMA_STRUCT(struct collision_model, resistance, damage_resistance_schema),
	TAG_SCHEMA_BLOCK(struct collision_model, pathfinding_spheres, pathfinding_sphere_schema,
		MAXIMUM_PATHFINDING_SPHERES_PER_COLLISION_MODEL),
	TAG_SCHEMA_BLOCK(struct collision_model, nodes, collision_node_schema, MAXIMUM_NODES_PER_MODEL),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const collision_model_schema =
	TAG_SCHEMA_DEFINITION(collision_model, struct collision_model, collision_model_fields);

/* physics */

static struct tag_schema_definition const inertial_matrix_schema =
	TAG_SCHEMA_DEFINITION(inertial_matrix, real_matrix3x3, empty_fields);
static struct tag_schema_definition const powered_mass_point_schema =
	TAG_SCHEMA_DEFINITION(powered_mass_point, struct powered_mass_point_definition, empty_fields);

static struct tag_schema_field const mass_point_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct mass_point_definition, powered_mass_point_index, TAG_SCHEMA_ROOT,
		offsetof(struct physics_definition, powered_mass_points), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct mass_point_definition, friction_type, NUMBER_OF_FRICTION_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const mass_point_schema =
	TAG_SCHEMA_DEFINITION(mass_point, struct mass_point_definition, mass_point_fields);

static struct tag_schema_field const physics_fields[] =
{
	TAG_SCHEMA_BLOCK(struct physics_definition, inertial_matrix, inertial_matrix_schema, NUMBER_OF_INERTIAL_MATRICES),
	TAG_SCHEMA_BLOCK(struct physics_definition, powered_mass_points, powered_mass_point_schema,
		MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS),
	TAG_SCHEMA_BLOCK(struct physics_definition, mass_points, mass_point_schema, MAXIMUM_MASS_POINTS_PER_PHYSICS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const physics_schema =
	TAG_SCHEMA_DEFINITION(physics, struct physics_definition, physics_fields);

/* (point physics: numbers only) */
static struct tag_schema_definition const point_physics_schema =
	TAG_SCHEMA_DEFINITION(point_physics, struct point_physics_definition, empty_fields);

/* structure bsps */

/* (an object's lighting is the bsp's default: render_lighting's counts
index its lights, and the point lights would be the game's own) */
static struct tag_schema_field const render_lighting_fields[] =
{
	TAG_SCHEMA_ENUM(struct render_lighting, distant_light_count, MAXIMUM_RENDERED_DISTANT_LIGHTS + 1, 0),
	TAG_SCHEMA_ENUM(struct render_lighting, point_light_count, 1, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const render_lighting_schema =
	TAG_SCHEMA_DEFINITION(render_lighting, struct render_lighting, render_lighting_fields);

static struct tag_schema_field const structure_collision_material_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_collision_material, shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_ENUM(struct structure_collision_material, runtime_physics_material_type, NUMBER_OF_MATERIAL_TYPES,
		FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_collision_material_schema =
	TAG_SCHEMA_DEFINITION(structure_collision_material, struct structure_collision_material,
		structure_collision_material_fields);

/* (a node's bounds, as the collision bsp's node of its index) */
static struct tag_schema_definition const structure_node_schema =
	TAG_SCHEMA_DEFINITION(structure_node, byte_rectangle3d, empty_fields);

/* (its surface references: structure_bsp_check) */
static struct tag_schema_field const structure_leaf_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct structure_leaf, cluster_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, clusters), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_leaf_schema =
	TAG_SCHEMA_DEFINITION(structure_leaf, struct structure_leaf, structure_leaf_fields);

/* (its node: structure_bsp_check) */
static struct tag_schema_field const structure_surface_reference_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct structure_surface_reference, surface_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, surfaces), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_surface_reference_schema =
	TAG_SCHEMA_DEFINITION(structure_surface_reference, struct structure_surface_reference,
		structure_surface_reference_fields);

/* (its vertices, as its material's: structure_bsp_check) */
static struct tag_schema_definition const structure_surface_schema =
	TAG_SCHEMA_DEFINITION(structure_surface, struct structure_surface, empty_fields);

/* (its surfaces, buffers and vertices: structure_bsp_check) */
static struct tag_schema_field const structure_material_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_material, shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_STRUCT(struct structure_material, lighting, render_lighting_schema),
	TAG_SCHEMA_BLOCK_INDEX(struct structure_material, breakable_surface_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, breakable_surfaces), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_DATA(struct structure_material, uncompressed_vertex_data,
		MAXIMUM_VERTICES_PER_STRUCTURE_MATERIAL *
			(UNCOMPRESSED_ENVIRONMENT_VERTEX_SIZE + UNCOMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE)),
	TAG_SCHEMA_DATA(struct structure_material, compressed_vertex_data,
		MAXIMUM_VERTICES_PER_STRUCTURE_MATERIAL *
			(COMPRESSED_ENVIRONMENT_VERTEX_SIZE + COMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_material_schema =
	TAG_SCHEMA_DEFINITION(structure_material, struct structure_material, structure_material_fields);

/* (its bitmap: structure_bsp_check) */
static struct tag_schema_field const structure_lightmap_fields[] =
{
	TAG_SCHEMA_BLOCK(struct structure_lightmap, materials, structure_material_schema,
		MAXIMUM_MATERIALS_PER_STRUCTURE_LIGHTMAP),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_lightmap_schema =
	TAG_SCHEMA_DEFINITION(structure_lightmap, struct structure_lightmap, structure_lightmap_fields);

static struct tag_schema_field const structure_lens_flare_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_lens_flare, lens_flare, TAG_SCHEMA_GROUPS('lens')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_lens_flare_schema =
	TAG_SCHEMA_DEFINITION(structure_lens_flare, struct structure_lens_flare, structure_lens_flare_fields);

static struct tag_schema_field const structure_lens_flare_marker_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct structure_lens_flare_marker, lens_flare_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, lens_flares), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_lens_flare_marker_schema =
	TAG_SCHEMA_DEFINITION(structure_lens_flare_marker, struct structure_lens_flare_marker,
		structure_lens_flare_marker_fields);

/* predicted resources (predicted_resources.c) */

static struct tag_schema_field const predicted_resource_fields[] =
{
	TAG_SCHEMA_ENUM(struct predicted_resource, type, _predicted_resource_sound + 1, 0),
	TAG_SCHEMA_TAG_INDEX(struct predicted_resource, tag_index, TAG_SCHEMA_GROUPS('bitm', 'snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const predicted_resource_schema =
	TAG_SCHEMA_DEFINITION(predicted_resource, struct predicted_resource, predicted_resource_fields);

static struct tag_schema_field const structure_subcluster_fields[] =
{
	/* (a surface index past the surfaces is skipped: structure_visibility.c) */
	TAG_SCHEMA_BLOCK(struct structure_subcluster, surface_indices, long_schema, MAXIMUM_SURFACES_PER_SUBCLUSTER),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_subcluster_schema =
	TAG_SCHEMA_DEFINITION(structure_subcluster, struct structure_subcluster, structure_subcluster_fields);

static struct tag_schema_field const structure_mirror_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_mirror, shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_BLOCK(struct structure_mirror, points, real_point3d_schema, MAXIMUM_PORTAL_HULL_VERTICES),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_mirror_schema =
	TAG_SCHEMA_DEFINITION(structure_mirror, struct structure_mirror, structure_mirror_fields);

static struct tag_schema_field const structure_portal_index_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct structure_portal_index, portal_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, cluster_portals), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_portal_index_schema =
	TAG_SCHEMA_DEFINITION(structure_portal_index, struct structure_portal_index, structure_portal_index_fields);

/* (its fog, decals, lens flare markers and surface list:
structure_bsp_check) */
static struct tag_schema_field const structure_cluster_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct structure_cluster_schema, background_sound_palette_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, background_sound_palette), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct structure_cluster_schema, sound_environment_palette_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, sound_environment_palette), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct structure_cluster_schema, weather_palette_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, weather_palette), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK(struct structure_cluster_schema, predicted_resources, predicted_resource_schema,
		MAXIMUM_PREDICTED_RESOURCES_PER_CLUSTER),
	TAG_SCHEMA_BLOCK(struct structure_cluster_schema, subclusters, structure_subcluster_schema,
		MAXIMUM_SUBCLUSTERS_PER_CLUSTER),
	TAG_SCHEMA_BLOCK(struct structure_cluster_schema, surface_indices, long_schema, MAXIMUM_SURFACES_PER_CLUSTER),
	TAG_SCHEMA_BLOCK(struct structure_cluster_schema, mirrors, structure_mirror_schema, MAXIMUM_MIRRORS_PER_CLUSTER),
	TAG_SCHEMA_BLOCK(struct structure_cluster_schema, portal_indices, structure_portal_index_schema,
		MAXIMUM_CLUSTER_PORTALS_PER_CLUSTER),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_cluster_schema =
	TAG_SCHEMA_DEFINITION(structure_cluster, struct structure_cluster_schema, structure_cluster_fields);

/* (its plane: structure_bsp_check) */
static struct tag_schema_field const structure_cluster_portal_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX_ARRAY(struct structure_cluster_portal, cluster_indices, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, clusters), 0),
	TAG_SCHEMA_BLOCK(struct structure_cluster_portal, vertices, real_point3d_schema,
		MAXIMUM_VERTICES_PER_CLUSTER_PORTAL),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_cluster_portal_schema =
	TAG_SCHEMA_DEFINITION(structure_cluster_portal, struct structure_cluster_portal, structure_cluster_portal_fields);

/* (its collision surface: structure_bsp_check) */
static struct tag_schema_definition const structure_breakable_surface_schema =
	TAG_SCHEMA_DEFINITION(structure_breakable_surface, struct structure_breakable_surface, empty_fields);

static struct tag_schema_field const structure_fog_plane_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct structure_fog_plane_schema, region_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, fog_regions), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct structure_fog_plane_schema, runtime_material_type, NUMBER_OF_MATERIAL_TYPES,
		FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK(struct structure_fog_plane_schema, vertices, real_point3d_schema, MAXIMUM_VERTICES_PER_FOG_PLANE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_fog_plane_schema =
	TAG_SCHEMA_DEFINITION(structure_fog_plane, struct structure_fog_plane_schema, structure_fog_plane_fields);

static struct tag_schema_field const structure_fog_region_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct structure_fog_region, fog_palette_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, fog_palette), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct structure_fog_region, weather_palette_index, TAG_SCHEMA_ROOT,
		offsetof(struct structure_bsp, weather_palette), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_fog_region_schema =
	TAG_SCHEMA_DEFINITION(structure_fog_region, struct structure_fog_region, structure_fog_region_fields);

static struct tag_schema_field const structure_fog_palette_entry_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_fog_palette_entry, fog, TAG_SCHEMA_GROUPS('fog ')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_fog_palette_entry_schema =
	TAG_SCHEMA_DEFINITION(structure_fog_palette_entry, struct structure_fog_palette_entry,
		structure_fog_palette_entry_fields);

static struct tag_schema_field const structure_weather_palette_entry_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_weather_palette_entry, particle_system, TAG_SCHEMA_GROUPS('rain')),
	TAG_SCHEMA_REFERENCE(struct structure_weather_palette_entry, wind, TAG_SCHEMA_GROUPS('wind')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_weather_palette_entry_schema =
	TAG_SCHEMA_DEFINITION(structure_weather_palette_entry, struct structure_weather_palette_entry,
		structure_weather_palette_entry_fields);

static struct tag_schema_field const structure_weather_polyhedron_fields[] =
{
	TAG_SCHEMA_BLOCK(struct structure_weather_polyhedron, planes, real_plane3d_schema,
		MAXIMUM_PLANES_PER_WEATHER_POLYHEDRON),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_weather_polyhedron_schema =
	TAG_SCHEMA_DEFINITION(structure_weather_polyhedron, struct structure_weather_polyhedron,
		structure_weather_polyhedron_fields);

static struct tag_schema_field const structure_background_sound_palette_entry_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_background_sound_palette_entry, background_sound,
		TAG_SCHEMA_GROUPS('lsnd')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_background_sound_palette_entry_schema =
	TAG_SCHEMA_DEFINITION(structure_background_sound_palette_entry, struct structure_background_sound_palette_entry,
		structure_background_sound_palette_entry_fields);

static struct tag_schema_field const structure_sound_environment_palette_entry_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_sound_environment_palette_entry, sound_environment,
		TAG_SCHEMA_GROUPS('snde')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_sound_environment_palette_entry_schema =
	TAG_SCHEMA_DEFINITION(structure_sound_environment_palette_entry, struct structure_sound_environment_palette_entry,
		structure_sound_environment_palette_entry_fields);

/* detail objects (their cells: structure_detail_object_data_check) */

static struct tag_schema_definition const detail_object_cell_schema =
	TAG_SCHEMA_DEFINITION(detail_object_cell, struct detail_object_cell_definition, empty_fields);
static struct tag_schema_definition const detail_object_schema =
	TAG_SCHEMA_DEFINITION(detail_object, struct detail_object, empty_fields);
static struct tag_schema_definition const detail_object_count_schema =
	TAG_SCHEMA_DEFINITION(detail_object_count, word, empty_fields);
static struct tag_schema_definition const detail_object_z_reference_vector_schema =
	TAG_SCHEMA_DEFINITION(detail_object_z_reference_vector, real_vector4d, empty_fields);

static struct tag_schema_field const structure_detail_object_data_fields[] =
{
	TAG_SCHEMA_BLOCK(struct structure_detail_object_data, cells, detail_object_cell_schema,
		MAXIMUM_DETAIL_OBJECT_CELLS),
	TAG_SCHEMA_BLOCK(struct structure_detail_object_data, detail_objects, detail_object_schema,
		MAXIMUM_DETAIL_OBJECTS),
	TAG_SCHEMA_BLOCK(struct structure_detail_object_data, counts, detail_object_count_schema,
		MAXIMUM_DETAIL_OBJECT_COUNTS),
	TAG_SCHEMA_BLOCK(struct structure_detail_object_data, z_reference_vectors,
		detail_object_z_reference_vector_schema, MAXIMUM_DETAIL_OBJECT_COUNTS),
	TAG_SCHEMA_CHECK(structure_detail_object_data_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_detail_object_data_schema =
	TAG_SCHEMA_DEFINITION(structure_detail_object_data, struct structure_detail_object_data,
		structure_detail_object_data_fields);

/* (a decal of the scenario's palette, which tag_block_get_element checks) */
static struct tag_schema_definition const structure_runtime_decal_schema =
	TAG_SCHEMA_DEFINITION(structure_runtime_decal, struct structure_runtime_decal, empty_fields);

/* the leaf map (only drawn by debugging: structure_render.c) */

static struct tag_schema_field const map_leaf_face_fields[] =
{
	TAG_SCHEMA_BLOCK(struct map_leaf_face, vertices, real_point2d_schema, MAXIMUM_VERTICES_PER_LEAF_MAP_FACE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const map_leaf_face_schema =
	TAG_SCHEMA_DEFINITION(map_leaf_face, struct map_leaf_face, map_leaf_face_fields);

static struct tag_schema_field const map_leaf_fields[] =
{
	TAG_SCHEMA_BLOCK(struct map_leaf, faces, map_leaf_face_schema, MAXIMUM_FACES_PER_LEAF_MAP_LEAF),
	TAG_SCHEMA_BLOCK(struct map_leaf, portal_designators, long_schema, MAXIMUM_PORTALS_PER_LEAF_MAP_LEAF),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const map_leaf_schema =
	TAG_SCHEMA_DEFINITION(map_leaf, struct map_leaf, map_leaf_fields);

static struct tag_schema_field const leaf_portal_fields[] =
{
	TAG_SCHEMA_BLOCK(struct leaf_portal, vertices, real_point3d_schema, MAXIMUM_VERTICES_PER_LEAF_MAP_PORTAL),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const leaf_portal_schema =
	TAG_SCHEMA_DEFINITION(leaf_portal, struct leaf_portal, leaf_portal_fields);

static struct tag_schema_field const leaf_map_fields[] =
{
	/* (the bsp the tools built it from, which the game never sets) */
	TAG_SCHEMA_RESET(struct leaf_map, bsp, 0),
	TAG_SCHEMA_BLOCK(struct leaf_map, leaves, map_leaf_schema, MAXIMUM_LEAF_MAP_LEAVES),
	TAG_SCHEMA_BLOCK(struct leaf_map, portals, leaf_portal_schema, MAXIMUM_LEAF_MAP_PORTALS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const leaf_map_schema =
	TAG_SCHEMA_DEFINITION(leaf_map, struct leaf_map, leaf_map_fields);

/* (the markers are never read; what the blocks index in each other's and
in the collision bsp's: structure_bsp_check, once their own checks ran) */
static struct tag_schema_field const structure_bsp_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct structure_bsp, lightmap_group, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_STRUCT(struct structure_bsp, default_lighting, render_lighting_schema),
	TAG_SCHEMA_BLOCK(struct structure_bsp, collision_materials, structure_collision_material_schema,
		MAXIMUM_COLLISION_MATERIALS_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, collision_bsp, structure_collision_bsp_schema,
		MAXIMUM_COLLISION_BSPS_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, nodes, structure_node_schema, MAXIMUM_NODES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, leaves, structure_leaf_schema, MAXIMUM_LEAVES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, surface_references, structure_surface_reference_schema,
		MAXIMUM_SURFACE_REFERENCES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, surfaces, structure_surface_schema, MAXIMUM_SURFACES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, lightmaps, structure_lightmap_schema, MAXIMUM_LIGHTMAPS_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, lens_flares, structure_lens_flare_schema, MAXIMUM_LENS_FLARES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, lens_flare_markers, structure_lens_flare_marker_schema,
		MAXIMUM_LENS_FLARE_MARKERS_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, clusters, structure_cluster_schema, MAXIMUM_CLUSTERS_PER_STRUCTURE),
	TAG_SCHEMA_DATA(struct structure_bsp, cluster_data, MAXIMUM_CLUSTER_DATA_SIZE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, cluster_portals, structure_cluster_portal_schema,
		MAXIMUM_CLUSTER_PORTALS_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, breakable_surfaces, structure_breakable_surface_schema,
		MAXIMUM_BREAKABLE_SURFACES_PER_MAP),
	TAG_SCHEMA_BLOCK(struct structure_bsp, fog_planes, structure_fog_plane_schema, MAXIMUM_FOG_PLANES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, fog_regions, structure_fog_region_schema,
		MAXIMUM_FOG_REGIONS_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, fog_palette, structure_fog_palette_entry_schema,
		MAXIMUM_FOG_PALETTE_ENTRIES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, weather_palette, structure_weather_palette_entry_schema,
		MAXIMUM_WEATHER_PALETTE_ENTRIES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, weather_polyhedra, structure_weather_polyhedron_schema,
		MAXIMUM_WEATHER_POLYHEDRA_PER_STRUCTURE),
	/* (a byte for each collision surface: structure_bsp_check) */
	TAG_SCHEMA_BLOCK(struct structure_bsp, pathfinding_surfaces, byte_schema,
		MAXIMUM_PATHFINDING_SURFACES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, pathfinding_edges, byte_schema, MAXIMUM_PATHFINDING_EDGES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, background_sound_palette, structure_background_sound_palette_entry_schema,
		MAXIMUM_BACKGROUND_SOUND_PALETTE_ENTRIES_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, sound_environment_palette, structure_sound_environment_palette_entry_schema,
		MAXIMUM_SOUND_ENVIRONMENT_PALETTE_ENTRIES_PER_STRUCTURE),
	TAG_SCHEMA_DATA(struct structure_bsp, sound_cluster_data, MAXIMUM_SOUND_CLUSTER_DATA_SIZE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, detail_object_data, structure_detail_object_data_schema,
		MAXIMUM_DETAIL_OBJECT_DATA_PER_STRUCTURE),
	TAG_SCHEMA_BLOCK(struct structure_bsp, runtime_decals, structure_runtime_decal_schema,
		MAXIMUM_DECALS_PER_STRUCTURE),
	TAG_SCHEMA_STRUCT(struct structure_bsp, leaf_map, leaf_map_schema),
	TAG_SCHEMA_CHECK(structure_bsp_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_bsp_schema =
	TAG_SCHEMA_DEFINITION(structure_bsp, struct structure_bsp, structure_bsp_fields);

struct tag_schema_group const tag_schema_collision_groups[] =
{
	{ 'coll', { NONE, NONE }, &collision_model_schema },
	{ 'phys', { NONE, NONE }, &physics_schema },
	{ 'pphy', { NONE, NONE }, &point_physics_schema },
	{ 'sbsp', { NONE, NONE }, &structure_bsp_schema },
	{ 0 }
};

/* ---------- private code */

/* the bsp graph checks' walk: a node and its depth (the root's is 1) */
struct bsp_walk_entry
{
	long node_index;
	long depth;
};

static struct bsp_walk_entry bsp_walk_stack[MAXIMUM_BSP3D_DEPTH + MAXIMUM_BSP2D_DEPTH + 2];

static void bsp_visited_clear(
	long count)
{
	memset(collision_bsp_visited, 0, BIT_VECTOR_SIZE_IN_BYTES(count));

	return;
}

/* whether a child (of a node at depth) is one the walk may take: a node
not yet visited (it is marked so and pushed), no deeper than maximum_depth */
static boolean bsp_walk_push_node(
	long *stack_count,
	long node_index,
	long node_count,
	long depth,
	long maximum_depth)
{
	if (node_index < 0 || node_index >= node_count || depth > maximum_depth ||
		*stack_count >= (long)NUMBEROF(bsp_walk_stack) ||
		BIT_VECTOR_TEST_FLAG(collision_bsp_visited, node_index))
	{
		return FALSE;
	}
	BIT_VECTOR_SET_FLAG(collision_bsp_visited, node_index, TRUE);
	bsp_walk_stack[*stack_count].node_index = node_index;
	bsp_walk_stack[*stack_count].depth = depth;
	(*stack_count)++;

	return TRUE;
}

/* the bsp3d: a tree from node 0 (no node reached twice), its leaves the
bsp's, no deeper than the plane stack (collision_bsp.c) */
static void bsp3d_check(
	struct tag_validation *validation,
	struct collision_bsp *bsp)
{
	long node_count = bsp->bsp3d.nodes.count;
	long stack_count = 0;

	if (!node_count)
		return;
	bsp_visited_clear(node_count);
	bsp_walk_push_node(&stack_count, 0, node_count, 1, MAXIMUM_BSP3D_DEPTH);
	while (stack_count > 0)
	{
		struct bsp_walk_entry entry = bsp_walk_stack[--stack_count];
		struct bsp3d_node *node = (struct bsp3d_node *)bsp->bsp3d.nodes.address + entry.node_index;
		short child_index;

		for (child_index = 0; child_index < NUMBEROF(node->children); child_index++)
		{
			long child = node->children[child_index];

			if (child & LONG_MIN ?
				child == NONE || (child & LONG_MAX) < bsp->leaves.count :
				bsp_walk_push_node(&stack_count, child, node_count, entry.depth + 1, MAXIMUM_BSP3D_DEPTH))
			{
				continue;
			}
			tag_validate_correct(validation, "bsp3d node %ld's child %08lx is not a leaf nor a node of the tree"
				" (at depth %ld): none", entry.node_index, child, entry.depth + 1);
			node->children[child_index] = NONE;
		}
	}

	return;
}

/* a bsp2d child: a surface (LONG_MIN | its index), or a node of the tree */
static boolean bsp2d_child_valid(
	struct collision_bsp *bsp,
	long *stack_count,
	long child,
	long depth)
{
	return child & LONG_MIN ?
		(child & LONG_MAX) < bsp->surfaces.count :
		bsp_walk_push_node(stack_count, child, bsp->bsp2d.nodes.count, depth, MAXIMUM_BSP2D_DEPTH);
}

/* the bsp2d references' trees (whose leaves are surfaces: a reference to
none would have its surface's edges walked from no surface's) */
static void bsp2d_check(
	struct tag_validation *validation,
	struct collision_bsp *bsp)
{
	long reference_index;

	if (bsp->bsp2d_references.count && !bsp->surfaces.count)
	{
		tag_validate_correct(validation, "has %ld bsp2d references and no surfaces: none",
			bsp->bsp2d_references.count);
		bsp->bsp2d_references.count = 0;
	}
	bsp_visited_clear(bsp->bsp2d.nodes.count);
	for (reference_index = 0; reference_index < bsp->bsp2d_references.count; reference_index++)
	{
		struct bsp2d_reference *reference = (struct bsp2d_reference *)bsp->bsp2d_references.address + reference_index;
		long stack_count = 0;

		if ((reference->plane_designator & LONG_MAX) >= bsp->bsp3d.planes.count)
		{
			tag_validate_correct(validation, "bsp2d reference %ld's plane %08lx is not one of its %ld: 0",
				reference_index, reference->plane_designator, bsp->bsp3d.planes.count);
			reference->plane_designator = 0;
		}
		if (!bsp2d_child_valid(bsp, &stack_count, reference->root_index, 1))
		{
			tag_validate_correct(validation, "bsp2d reference %ld's root %08lx is not a surface nor a node of a tree:"
				" surface 0", reference_index, reference->root_index);
			reference->root_index = (long)LONG_MIN;
		}
		while (stack_count > 0)
		{
			struct bsp_walk_entry entry = bsp_walk_stack[--stack_count];
			struct bsp2d_node *node = (struct bsp2d_node *)bsp->bsp2d.nodes.address + entry.node_index;
			short child_index;

			for (child_index = 0; child_index < NUMBEROF(node->child_indices); child_index++)
			{
				if (!bsp2d_child_valid(bsp, &stack_count, node->child_indices[child_index], entry.depth + 1))
				{
					tag_validate_correct(validation, "bsp2d node %ld's child %08lx is not a surface nor a node of"
						" the tree (at depth %ld): surface 0",
						entry.node_index, node->child_indices[child_index], entry.depth + 1);
					node->child_indices[child_index] = (long)LONG_MIN;
				}
			}
		}
	}

	return;
}

/* each leaf's bsp2d references the bsp's */
static void collision_leaves_check(
	struct tag_validation *validation,
	struct collision_bsp *bsp)
{
	long leaf_index;

	for (leaf_index = 0; leaf_index < bsp->leaves.count; leaf_index++)
	{
		struct collision_leaf *leaf = (struct collision_leaf *)bsp->leaves.address + leaf_index;

		/* (a leaf of none has none's first, NONE) */
		if (leaf->bsp2d_reference_count < 0 ||
			(leaf->bsp2d_reference_count &&
				(leaf->first_bsp2d_reference_index < 0 ||
					leaf->first_bsp2d_reference_index > bsp->bsp2d_references.count - leaf->bsp2d_reference_count)))
		{
			tag_validate_correct(validation, "leaf %ld's %d bsp2d references from %ld are not the bsp's %ld: none",
				leaf_index, leaf->bsp2d_reference_count, leaf->first_bsp2d_reference_index,
				bsp->bsp2d_references.count);
			leaf->bsp2d_reference_count = 0;
			leaf->first_bsp2d_reference_index = 0;
		}
	}

	return;
}

/* each surface's plane, and its edges: a ring from its first edge back to
it in at most MAXIMUM_EDGES_PER_COLLISION_SURFACE (collision_surface_polygon's
points), each edge the surface's (on one side or the other), as the game
walks it (collision_bsp.c and the others): FALSE if one is not, which would
be walked without end */
static boolean collision_surfaces_check(
	struct tag_validation *validation,
	struct collision_bsp *bsp)
{
	long surface_index;

	for (surface_index = 0; surface_index < bsp->surfaces.count; surface_index++)
	{
		struct collision_surface *surface = (struct collision_surface *)bsp->surfaces.address + surface_index;
		long edge_index = surface->first_edge_index;
		short edge_count = 0;

		if ((surface->plane_designator & LONG_MAX) >= bsp->bsp3d.planes.count)
		{
			tag_validate_correct(validation, "surface %ld's plane %08lx is not one of its %ld: 0",
				surface_index, surface->plane_designator, bsp->bsp3d.planes.count);
			surface->plane_designator = 0;
		}
		do
		{
			struct collision_edge const *edge;

			/* (the edges' indices are the bsp's: the schema checked them) */
			if (edge_index < 0 || edge_index >= bsp->edges.count || edge_count >= MAXIMUM_EDGES_PER_COLLISION_SURFACE)
			{
				tag_validate_refuse(validation, "has surface %ld whose edges do not close within %d",
					surface_index, MAXIMUM_EDGES_PER_COLLISION_SURFACE);
				return FALSE;
			}
			edge = (struct collision_edge const *)bsp->edges.address + edge_index;
			if (edge->surface_indices[0] != surface_index && edge->surface_indices[1] != surface_index)
			{
				tag_validate_refuse(validation, "has surface %ld with edge %ld, which is not its", surface_index,
					edge_index);
				return FALSE;
			}
			edge_index = edge->edge_indices[edge->surface_indices[1] == surface_index];
			edge_count++;
		}
		while (edge_index != surface->first_edge_index);
	}

	return TRUE;
}

static boolean collision_bsp_graph_check(
	struct tag_validation *validation,
	struct collision_bsp *bsp,
	boolean structure)
{
	bsp3d_check(validation, bsp);
	bsp2d_check(validation, bsp);
	collision_leaves_check(validation, bsp);

	/* (a structure's vertices are walked around by the path finding, which
	bounds its own walks: path_smoothing.c) */
	(void)structure;
	return collision_surfaces_check(validation, bsp);
}

static boolean collision_model_bsp_check(
	struct tag_validation *validation,
	void *base)
{
	return collision_bsp_graph_check(validation, base, FALSE);
}

static boolean structure_collision_bsp_check(
	struct tag_validation *validation,
	void *base)
{
	return collision_bsp_graph_check(validation, base, TRUE);
}

/* the collision bsp a structure bsp has (its first: the game's) */
static struct collision_bsp *structure_collision_bsp_get(
	struct structure_bsp *structure_bsp)
{
	return structure_bsp->collision_bsp.count ? structure_bsp->collision_bsp.address : NULL;
}

/* a material's vertex buffer: its hardware buffer one of the bsp header's
(of vertex buffers, or of lightmap vertex buffers, which the validator
calls its index buffers) whose data holds its vertices, or none, which is
not drawn (rasterizer_xbox_draw_primitives.c) */
static void structure_vertex_buffer_check(
	struct tag_validation *validation,
	struct vertex_buffer *vertices,
	boolean lightmap,
	short lightmap_index,
	short material_index)
{
	/* (the sizes of the environment's vertex types, in their order) */
	static short const vertex_sizes[] =
	{
		UNCOMPRESSED_ENVIRONMENT_VERTEX_SIZE,
		COMPRESSED_ENVIRONMENT_VERTEX_SIZE,
		UNCOMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE,
		COMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE,
	};
	void *data;

	if (!vertices->hardware_format)
		return;
	data = lightmap ?
		tag_validate_index_buffer_data(validation, vertices->hardware_format) :
		tag_validate_vertex_buffer_data(validation, vertices->hardware_format);
	/* (its type is one of the environment's and its count no more than
	MAXIMUM_VERTICES_PER_STRUCTURE_MATERIAL: structure_material_check) */
	if (!data ||
		!tag_validate_contains(validation, data, (unsigned long)vertices->count * vertex_sizes[vertices->type]))
	{
		tag_validate_correct(validation, "lightmap %d's material %d's %svertices (%ld) are not in a buffer of the"
			" bsp's: none", lightmap_index, material_index, lightmap ? "lightmap " : "", vertices->count);
		vertices->hardware_format = NULL;
	}

	return;
}

/* a material's surfaces and vertices: its surfaces the bsp's, its vertices
in its compressed data (each vertex, then each lightmap vertex: structures.c,
object_lights.c, structure_visibility.c read them there) and its buffers,
and every vertex its surfaces name its (as are those of the surfaces before
it that no material has, which structure_render_pass draws with it) */
static void structure_material_check(
	struct tag_validation *validation,
	struct structure_bsp *structure_bsp,
	struct structure_material *material,
	short lightmap_index,
	short material_index,
	long *previous_surface_end)
{
	struct structure_surface *surfaces = structure_bsp->surfaces.address;
	struct structure_lightmap const *lightmap = (struct structure_lightmap const *)structure_bsp->lightmaps.address +
		lightmap_index;
	long vertex_data_size = material->compressed_vertex_data.size;
	long vertex_count;
	long surface_index;
	long end;

	if (material->first_surface_index < 0 || material->surface_count < 0 ||
		material->first_surface_index > structure_bsp->surfaces.count - material->surface_count)
	{
		tag_validate_correct(validation, "lightmap %d's material %d's %ld surfaces from %ld are not the bsp's %ld: none",
			lightmap_index, material_index, material->surface_count, material->first_surface_index,
			structure_bsp->surfaces.count);
		material->first_surface_index = 0;
		material->surface_count = 0;
	}
	if (material->vertices.type != _rasterizer_vertex_type_environment_uncompressed &&
		material->vertices.type != _rasterizer_vertex_type_environment_compressed)
	{
		tag_validate_correct(validation, "lightmap %d's material %d's vertices are of type %d: compressed",
			lightmap_index, material_index, material->vertices.type);
		material->vertices.type = _rasterizer_vertex_type_environment_compressed;
	}
	if (material->lightmap_vertices.type != _rasterizer_vertex_type_environment_lightmap_uncompressed &&
		material->lightmap_vertices.type != _rasterizer_vertex_type_environment_lightmap_compressed)
	{
		tag_validate_correct(validation, "lightmap %d's material %d's lightmap vertices are of type %d: compressed",
			lightmap_index, material_index, material->lightmap_vertices.type);
		material->lightmap_vertices.type = _rasterizer_vertex_type_environment_lightmap_compressed;
	}
	/* (its counts no more than its compressed data holds, nor than the
	tools make, less than a word: its surfaces index them by words) */
	if (material->vertices.count < 0 || material->lightmap_vertices.count < 0 ||
		material->vertices.count > MAXIMUM_VERTICES_PER_STRUCTURE_MATERIAL ||
		material->lightmap_vertices.count > MAXIMUM_VERTICES_PER_STRUCTURE_MATERIAL ||
		material->vertices.count * COMPRESSED_ENVIRONMENT_VERTEX_SIZE +
			material->lightmap_vertices.count * COMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE > vertex_data_size)
	{
		long fit_count = PIN(material->vertices.count, 0,
			MIN(vertex_data_size / COMPRESSED_ENVIRONMENT_VERTEX_SIZE, MAXIMUM_VERTICES_PER_STRUCTURE_MATERIAL));
		long fit_lightmap_count = PIN(material->lightmap_vertices.count, 0,
			MIN((vertex_data_size - fit_count * COMPRESSED_ENVIRONMENT_VERTEX_SIZE) /
				COMPRESSED_ENVIRONMENT_LIGHTMAP_VERTEX_SIZE, MAXIMUM_VERTICES_PER_STRUCTURE_MATERIAL));

		tag_validate_correct(validation, "lightmap %d's material %d's %ld vertices and %ld lightmap vertices are not in"
			" its %ld bytes: %ld and %ld", lightmap_index, material_index, material->vertices.count,
			material->lightmap_vertices.count, vertex_data_size, fit_count, fit_lightmap_count);
		material->vertices.count = fit_count;
		material->lightmap_vertices.count = fit_lightmap_count;
	}
	structure_vertex_buffer_check(validation, &material->vertices, FALSE, lightmap_index, material_index);
	structure_vertex_buffer_check(validation, &material->lightmap_vertices, TRUE, lightmap_index, material_index);

	/* (its surfaces' vertices: its own, and its lightmap's when it has them
	or its lightmap a bitmap, which object_lights.c samples them in) */
	vertex_count = material->vertices.count;
	if ((material->lightmap_vertices.count || lightmap->bitmap_index != NONE) &&
		material->lightmap_vertices.count < vertex_count)
	{
		vertex_count = material->lightmap_vertices.count;
	}
	end = material->first_surface_index + material->surface_count;
	/* (a material with no vertices draws no surfaces: none of their indices
	could name one) */
	if (!vertex_count && material->surface_count)
	{
		tag_validate_correct(validation, "lightmap %d's material %d has %ld surfaces and no vertices: none",
			lightmap_index, material_index, material->surface_count);
		material->surface_count = 0;
		return;
	}
	for (surface_index = MIN(*previous_surface_end, material->first_surface_index); surface_index < end; surface_index++)
	{
		struct structure_surface *surface = &surfaces[surface_index];
		short vertex_index;

		for (vertex_index = 0; vertex_index < NUMBEROF(surface->vertex_indices); vertex_index++)
		{
			if (surface->vertex_indices[vertex_index] >= vertex_count)
			{
				tag_validate_correct(validation, "surface %ld's vertex %d is not lightmap %d's material %d's %ld: 0",
					surface_index, surface->vertex_indices[vertex_index], lightmap_index, material_index, vertex_count);
				surface->vertex_indices[vertex_index] = 0;
			}
		}
	}
	if (end > *previous_surface_end)
		*previous_surface_end = end;

	return;
}

/* a cluster's fog, decals, lens flare markers and surface lists */
static void structure_cluster_check(
	struct tag_validation *validation,
	struct structure_bsp *structure_bsp,
	struct structure_cluster_schema *cluster,
	short cluster_index)
{
	long *surface_indices = cluster->surface_indices.address;
	long count = cluster->surface_indices.count;
	long index;

	/* (a fog plane's index and the sign bit, or a fog region's) */
	if (cluster->fog_designator != NONE &&
		(cluster->fog_designator & SHORT_MIN ?
			(cluster->fog_designator & SHORT_MAX) >= structure_bsp->fog_planes.count :
			cluster->fog_designator >= structure_bsp->fog_regions.count))
	{
		tag_validate_correct(validation, "cluster %d's fog %04x is not a fog plane or region of the bsp's: none",
			cluster_index, (word)cluster->fog_designator);
		cluster->fog_designator = NONE;
	}
	if (cluster->first_runtime_decal_index != NONE && cluster->runtime_decal_count &&
		(cluster->first_runtime_decal_index < 0 ||
			cluster->first_runtime_decal_index > structure_bsp->runtime_decals.count - cluster->runtime_decal_count))
	{
		tag_validate_correct(validation, "cluster %d's %d decals from %d are not the bsp's %ld: none",
			cluster_index, cluster->runtime_decal_count, cluster->first_runtime_decal_index,
			structure_bsp->runtime_decals.count);
		cluster->first_runtime_decal_index = NONE;
		cluster->runtime_decal_count = 0;
	}
	if (cluster->first_lens_flare_marker_index >
		structure_bsp->lens_flare_markers.count - cluster->lens_flare_marker_count)
	{
		tag_validate_correct(validation, "cluster %d's %d lens flare markers from %d are not the bsp's %ld: none",
			cluster_index, cluster->lens_flare_marker_count, cluster->first_lens_flare_marker_index,
			structure_bsp->lens_flare_markers.count);
		cluster->first_lens_flare_marker_index = 0;
		cluster->lens_flare_marker_count = 0;
	}

	/* (its surfaces in groups of a material's: lightmap index, material index,
	count, then the surfaces, each of whose vertices are read in the material's
	compressed data: structure_visibility.c, which ends the list at a group that
	is not whole, skips a surface that is none of the bsp's) */
	index = 0;
	while (index + 3 <= count)
	{
		struct structure_lightmap *lightmap;
		struct structure_material *material;
		long lightmap_index = surface_indices[index];
		long material_index = surface_indices[index + 1];
		long group_count = surface_indices[index + 2];
		long group_end;

		if (surface_indices[index] < 0 || surface_indices[index] >= structure_bsp->lightmaps.count)
			break;
		lightmap = (struct structure_lightmap *)structure_bsp->lightmaps.address + surface_indices[index];
		if (surface_indices[index + 1] < 0 || surface_indices[index + 1] >= lightmap->materials.count)
			break;
		material = (struct structure_material *)lightmap->materials.address + surface_indices[index + 1];
		index += 3;
		group_end = group_count <= 0 ? index : group_count > count - index ? count : index + group_count;
		for (; index < group_end; index++)
		{
			long surface_index = surface_indices[index];
			struct structure_surface const *surface;

			if ((unsigned long)surface_index >= (unsigned long)structure_bsp->surfaces.count)
				continue;
			surface = (struct structure_surface const *)structure_bsp->surfaces.address + surface_index;
			if (surface->vertex_indices[0] >= material->vertices.count ||
				surface->vertex_indices[1] >= material->vertices.count ||
				surface->vertex_indices[2] >= material->vertices.count)
			{
				tag_validate_correct(validation, "cluster %d's surface %ld is not of its group's material (lightmap %ld's"
					" %ld): none", cluster_index, surface_index, lightmap_index, material_index);
				surface_indices[index] = NONE;
			}
		}
	}

	return;
}

/* the detail objects' cells: in order (structure_detail_objects.c finds a
column's by binary search, and holds no more than a layer's 27 around the
camera, which cells of their own place make it), and their counts and
objects the data's; data whose cells are not is not drawn */
static boolean structure_detail_object_data_check(
	struct tag_validation *validation,
	void *base)
{
	struct structure_detail_object_data *data = base;
	struct detail_object_cell_definition const *cells = data->cells.address;
	word const *counts = data->counts.address;
	long cell_index;

	for (cell_index = 0; cell_index < data->cells.count; cell_index++)
	{
		struct detail_object_cell_definition const *cell = &cells[cell_index];
		struct detail_object_cell_definition const *previous = cell_index ? &cells[cell_index - 1] : NULL;
		long layer_count = 0;
		long object_count = 0;
		short layer_index;

		for (layer_index = 0; layer_index < MAXIMUM_DETAIL_OBJECT_LAYERS; layer_index++)
		{
			if (TEST_FLAG(cell->valid_layers, layer_index))
				layer_count++;
		}
		if (cell->count_index >= 0 && cell->count_index <= data->counts.count - layer_count)
		{
			long count_index;

			for (count_index = 0; count_index < layer_count; count_index++)
				object_count += counts[cell->count_index + count_index];
		}
		if ((previous &&
				(previous->cell_x > cell->cell_x ||
					(previous->cell_x == cell->cell_x &&
						(previous->cell_y > cell->cell_y ||
							(previous->cell_y == cell->cell_y && previous->cell_z >= cell->cell_z))))) ||
			cell->count_index < 0 || cell->count_index > data->counts.count - layer_count ||
			(data->z_reference_vectors.count && cell->count_index > data->z_reference_vectors.count - layer_count) ||
			cell->start_index < 0 || cell->start_index > data->detail_objects.count - object_count)
		{
			if (data->valid)
			{
				tag_validate_correct(validation, "has detail object cell %ld out of order or past its objects:"
					" none drawn", cell_index);
				data->valid = FALSE;
			}
			break;
		}
	}

	return TRUE;
}

/* what a structure bsp's blocks index in each other's (and in its collision
bsp's, and its lightmap bitmaps): the indices a schema cannot say */
static boolean structure_bsp_check(
	struct tag_validation *validation,
	void *base)
{
	struct structure_bsp *structure_bsp = base;
	struct collision_bsp *collision_bsp = structure_collision_bsp_get(structure_bsp);
	long surface_count = collision_bsp ? collision_bsp->surfaces.count : 0;
	long previous_surface_end = 0;
	long index;

	/* (bsp3d_test_point walks it from node 0, which must be one. A bsp
	with no detail object data draws none, and a surface with no pathfinding
	surface is not walkable: the game checks both) */
	if (!collision_bsp || !collision_bsp->bsp3d.nodes.count)
	{
		tag_validate_refuse(validation, "has no collision bsp");
		return FALSE;
	}

	for (index = 0; index < structure_bsp->leaves.count; index++)
	{
		struct structure_leaf *leaf = (struct structure_leaf *)structure_bsp->leaves.address + index;

		if (leaf->surface_reference_count < 0 ||
			(leaf->surface_reference_count &&
				(leaf->first_surface_reference_index < 0 ||
					leaf->first_surface_reference_index >
						structure_bsp->surface_references.count - leaf->surface_reference_count)))
		{
			tag_validate_correct(validation, "leaf %ld's %d surface references from %ld are not the bsp's %ld: none",
				index, leaf->surface_reference_count, leaf->first_surface_reference_index,
				structure_bsp->surface_references.count);
			leaf->surface_reference_count = 0;
			leaf->first_surface_reference_index = 0;
		}
	}
	for (index = 0; index < structure_bsp->surface_references.count; index++)
	{
		struct structure_surface_reference *reference =
			(struct structure_surface_reference *)structure_bsp->surface_references.address + index;

		if (reference->bsp3d_node_index != NONE &&
			(reference->bsp3d_node_index < 0 || reference->bsp3d_node_index >= collision_bsp->bsp3d.nodes.count))
		{
			tag_validate_correct(validation, "surface reference %ld's node %ld is not the collision bsp's %ld: none",
				index, reference->bsp3d_node_index, collision_bsp->bsp3d.nodes.count);
			reference->bsp3d_node_index = NONE;
		}
	}
	for (index = 0; index < structure_bsp->cluster_portals.count; index++)
	{
		struct structure_cluster_portal *portal =
			(struct structure_cluster_portal *)structure_bsp->cluster_portals.address + index;

		if (portal->plane_index < 0 || portal->plane_index >= collision_bsp->bsp3d.planes.count)
		{
			tag_validate_correct(validation, "cluster portal %ld's plane %ld is not the collision bsp's %ld: 0",
				index, portal->plane_index, collision_bsp->bsp3d.planes.count);
			portal->plane_index = 0;
		}
	}
	/* (a breakable surface is broken from its collision surface, whose
	edges are walked as its) */
	if (structure_bsp->breakable_surfaces.count && !surface_count)
	{
		long lightmap_index;

		tag_validate_correct(validation, "has %ld breakable surfaces and no surfaces: none",
			structure_bsp->breakable_surfaces.count);
		structure_bsp->breakable_surfaces.count = 0;
		structure_bsp->breakable_surfaces.address = NULL;
		/* (and its materials' breakable surfaces, checked against the count
		before, are none) */
		for (lightmap_index = 0; lightmap_index < structure_bsp->lightmaps.count; lightmap_index++)
		{
			struct structure_lightmap *lightmap = (struct structure_lightmap *)structure_bsp->lightmaps.address +
				lightmap_index;
			long material_index;

			for (material_index = 0; material_index < lightmap->materials.count; material_index++)
				((struct structure_material *)lightmap->materials.address + material_index)->breakable_surface_index = NONE;
		}
	}
	for (index = 0; index < structure_bsp->breakable_surfaces.count; index++)
	{
		struct structure_breakable_surface *breakable_surface =
			(struct structure_breakable_surface *)structure_bsp->breakable_surfaces.address + index;

		if (breakable_surface->collision_surface_index < 0 || breakable_surface->collision_surface_index >= surface_count)
		{
			tag_validate_correct(validation, "breakable surface %ld's surface %ld is not the collision bsp's %ld: 0",
				index, breakable_surface->collision_surface_index, surface_count);
			breakable_surface->collision_surface_index = 0;
		}
	}

	/* the lightmaps' bitmaps and materials (in the order they draw) */
	{
		struct bitmap_group const *bitmap_group = structure_bsp->lightmap_group.index != NONE ?
			tag_validate_tag_get(validation, structure_bsp->lightmap_group.index, 'bitm') :
			NULL;
		long bitmap_count = bitmap_group ? bitmap_group->bitmaps.count : 0;

		for (index = 0; index < structure_bsp->lightmaps.count; index++)
		{
			struct structure_lightmap *lightmap = (struct structure_lightmap *)structure_bsp->lightmaps.address + index;
			long material_index;

			if (lightmap->bitmap_index != NONE && (lightmap->bitmap_index < 0 || lightmap->bitmap_index >= bitmap_count))
			{
				tag_validate_correct(validation, "lightmap %ld's bitmap %d is not its bitmaps' %ld: none",
					index, lightmap->bitmap_index, bitmap_count);
				lightmap->bitmap_index = NONE;
			}
			for (material_index = 0; material_index < lightmap->materials.count; material_index++)
			{
				structure_material_check(validation, structure_bsp,
					(struct structure_material *)lightmap->materials.address + material_index,
					(short)index, (short)material_index, &previous_surface_end);
			}
		}
	}

	for (index = 0; index < structure_bsp->clusters.count; index++)
	{
		structure_cluster_check(validation, structure_bsp,
			(struct structure_cluster_schema *)structure_bsp->clusters.address + index, (short)index);
	}

	return TRUE;
}
