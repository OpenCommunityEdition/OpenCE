/*
DECALS.C

symbols in this file:
00086FE0 0130:
	_decal_verify_neighbors (0000)
00087110 0080:
	_decal_set_first_decal_index (0000)
00087190 0180:
	_decal_sprite_get_bounds (0000)
00087310 00a0:
	_decals_initialize (0000)
000873B0 00b0:
	_decals_initialize_for_new_map (0000)
00087460 0070:
	_decals_dispose_from_old_map (0000)
000874D0 0010:
	_decals_dispose (0000)
000874E0 0170:
	_decals_unlock (0000)
00087650 0080:
	_decal_get_first_decal_index (0000)
000876D0 0010:
	_decal_new_from_media_collision (0000)
000876E0 01b0:
	_render_debug_decals (0000)
00087890 0050:
	_projection_from_vector3d (0000)
000878E0 0060:
	_projection_sign_from_vector3d (0000)
00087940 00e0:
	_project_point2d (0000)
00087A20 0050:
	_triple_product3d (0000)
00087A70 0090:
	_plane2d_from_points (0000)
00087B00 0040:
	_plane3d_from_point_and_normal (0000)
00087B40 0030:
	_plane3d_negate (0000)
00087B70 0030:
	_plane3d_distance_to_point (0000)
00087BA0 0110:
	_real_a_rgb_color_to_pixel32 (0000)
00087CB0 0070:
	_bsp3d_get_plane_from_designator (0000)
00087D20 0190:
	_decal_update (0000)
00087EB0 0070:
	_decal_reinsert (0000)
00087F20 02c0:
	_decal_insert (0000)
000881E0 01f0:
	_decals_reconnect_to_structure_bsp (0000)
000883D0 0220:
	_decals_disconnect_from_structure_bsp (0000)
000885F0 0050:
	_decals_update (0000)
00088640 0190:
	_decals_delete_permanent_from_cluster (0000)
000887D0 01a0:
	_decal_delete (0000)
00088970 02a0:
	_decal_projection_create (0000)
00088C10 06f0:
	_decal_clip_to_surface (0000)
00089300 1820:
	_decal_new_from_collision (0000)
0008AB20 01a0:
	_decal_new (0000)
00259930 0044:
	_decal_wrap_parameters (0000)
00259974 0024:
	??_C@_0CE@PKGKLMN@?$CBlayer_check?5?$HM?$HM?5layer?$DN?$DNdecal?9?$DOla@ (0000)
00259998 0024:
	??_C@_0CE@PMCDPOFC@cluster_index?$DN?$DNdecal?9?$DOcluster_in@ (0000)
002599BC 0020:
	??_C@_0CA@FFOMHEBD@c?3?2halo?2SOURCE?2effects?2decals?4c?$AA@ (0000)
002599DC 0029:
	??_C@_0CJ@BKINAOBK@layer?$DO?$DN0?5?$CG?$CG?5layer?$DMNUMBER_OF_DECA@ (0000)
00259A08 0041:
	??_C@_0EB@OBNKLKGI@cluster_index?$DO?$DN0?5?$CG?$CG?5cluster_inde@ (0000)
00259A4C 0007:
	??_C@_06FFNMNHIJ@extent?$AA@ (0000)
00259A54 000e:
	??_C@_0O@MAIKJNJC@sprite_bounds?$AA@ (0000)
00259A64 000b:
	??_C@_0L@KFJNHKLL@definition?$AA@ (0000)
00259A70 000e:
	??_C@_0O@EKGCOMJI@decal_globals?$AA@ (0000)
00259A80 000e:
	??_C@_0O@GFHIOAJM@decal?5globals?$AA@ (0000)
00259A90 0012:
	??_C@_0BC@DBAPGLGJ@global_decal_data?$AA@ (0000)
00259AA4 0007:
	??_C@_06FNIEFDBE@decals?$AA@ (0000)
00259AB0 0044:
	??_C@_0EE@OLGOOHOI@?$CD?$CD?$CD?5ERROR?5decals?3?5permanent?5coun@ (0000)
00259AF8 0041:
	??_C@_0EB@DFPFDBH@?$CD?$CD?$CD?5ERROR?5decals?3?5locked?5count?5i@ (0000)
00259B40 0046:
	??_C@_0EG@JHKMCNDN@?$CD?$CD?$CD?5ERROR?5decals?3?5duplicate?5surf@ (0000)
00259B88 0006:
	??_C@_05PEENBMOG@color?$AA@ (0000)
00259B90 001b:
	??_C@_0BL@NGHLOACP@alpha?$DO?$DN0?40f?5?$CG?$CG?5alpha?$DM?$DN1?40f?$AA@ (0000)
00259BAC 001d:
	??_C@_0BN@KCHNANLB@?4?4?2bitmaps?2bitmaps_inlines?4h?$AA@ (0000)
00259BCC 0013:
	??_C@_0BD@LPCEAGAP@f?$DO?$DN0?40f?5?$CG?$CG?5f?$DM?$DN1?40f?$AA@ (0000)
00259BE0 001e:
	??_C@_0BO@HLBLEDNJ@decal?9?$DOdefinition_index?$CB?$DNNONE?$AA@ (0000)
00259C00 0021:
	??_C@_0CB@PFKIGAKK@?$CD?$CD?$CD?5ERROR?5failed?5to?5insert?5decal@ (0000)
00259C24 0023:
	??_C@_0CD@DCCMGEFL@next?9?$DOcluster_index?$DN?$DNcluster_ind@ (0000)
00259C48 0049:
	??_C@_0EJ@NBFMBGG@?$CD?$CD?$CD?5ERROR?5decals?3?5failed?5to?5unlo@ (0000)
00259C94 0031:
	??_C@_0DB@JNHNEGFP@?$CD?$CD?$CD?5ERROR?5decals?3?5infinite?5loop?5@ (0000)
00259CC8 003b:
	??_C@_0DL@ENKJDMFL@decal_globals?9?$DOfirst_disconnecte@ (0000)
00259D04 0037:
	??_C@_0DH@MPACCKAA@decal?9?$DOlayer?$DO?$DN0?5?$CG?$CG?5decal?9?$DOlayer?$DM@ (0000)
00259D3C 001b:
	??_C@_0BL@ODAOPDKM@decal?9?$DOcluster_index?$DN?$DNNONE?$AA@ (0000)
00259D58 0024:
	??_C@_0CE@KECMKPKI@decal?9?$DOcluster_index?$DN?$DNcluster_in@ (0000)
00259D7C 0022:
	??_C@_0CC@MKGKINNH@decal_globals?9?$DOpermanent_count?$DO?$DN@ (0000)
00259DA0 002c:
	??_C@_0CM@MAEMJIML@?$CBTEST_FLAG?$CIdecal?9?$DOflags?0?5_decal_@ (0000)
00259DD0 004d:
	??_C@_0EN@LNGBHJKL@decal_get_first_decal_index?$CIdeca@ (0000)
00259E20 0042:
	??_C@_0EC@DGIGMKEO@?$CD?$CD?$CD?5ERROR?5decals?3?5deleting?5perma@ (0000)
00259E64 003f:
	??_C@_0DP@BICALKPG@?$CD?$CD?$CD?5ERROR?5decals?3?5deleting?5locke@ (0000)
00259EA4 0006:
	??_C@_05CFEEDGHM@decal?$AA@ (0000)
00259EAC 000b:
	??_C@_0L@PPHAJBEH@projection?$AA@ (0000)
00259EB8 0006:
	??_C@_05JFMNHLBI@basis?$AA@ (0000)
00259EC0 003f:
	??_C@_0DP@PCODPOLL@geometry?9?$DOdecal_surface_count?$DMMA@ (0000)
00259F00 006f:
	??_C@_0GP@JPNMMOPK@deviant_surface_count?5?$CG?$CG?5?$CKdevian@ (0000)
00259F70 0015:
	??_C@_0BF@NGACPBPD@deviant_surface_list?$AA@ (0000)
00259F88 007b:
	??_C@_0HL@CPOHEJM@surface_queue_write_index?5?$CG?$CG?5?$CKsu@ (0000)
0025A004 000e:
	??_C@_0O@MONIPKDM@surface_queue?$AA@ (0000)
0025A014 0009:
	??_C@_08GMGMDIBM@geometry?$AA@ (0000)
0025A020 0026:
	??_C@_0CG@IGINKMHI@type?$DO?$DN0?5?$CG?$CG?5type?$DMNUMBER_OF_DECAL_@ (0000)
0025A048 0046:
	??_C@_0EG@JHOMKFJL@?$CD?$CD?$CD?5WTF?5decals?3?5failed?5to?5alloca@ (0000)
0025A090 0041:
	??_C@_0EB@NPLPNMDO@?$CD?$CD?$CD?5WTF?5decals?3?5failed?5to?5insert@ (0000)
0025A0D4 0028:
	??_C@_0CI@FELCGKJB@?$CD?$CD?$CD?5WTF?5decals?3?5failed?5to?5lock?5v@ (0000)
0025A0FC 001d:
	??_C@_0BN@FJJJPKIB@quad_index?$DN?$DNdecal_quad_count?$AA@ (0000)
0025A11C 0038:
	??_C@_0DI@NKLICGOE@decal_surface_vertex_index?$CL1?$DMdec@ (0000)
0025A154 003c:
	??_C@_0DM@CJCALELL@decal_surface_vertex_count?$DO?$DNNUMB@ (0000)
0025A190 001f:
	??_C@_0BP@DMKBANMF@?$CIu?$CG0x8000?$CJ?$DN?$DN0?5?$CG?$CG?5?$CIv?$CG0x8000?$CJ?$DN?$DN0?$AA@ (0000)
0025A1B0 0004:
	__real@46fffe00 (0000)
0025A1B4 0004:
	__real@47000000 (0000)
0025A1B8 0061:
	??_C@_0GB@BFKPFOAD@decal_geometry?4decal_surface_ver@ (0000)
0025A220 0045:
	??_C@_0EF@GKPEALIB@?$CD?$CD?$CD?5ERROR?3?5decals?3?5failed?5to?5wra@ (0000)
0025A268 001c:
	??_C@_0BM@OHCKIEGH@closest_surface_index?$CB?$DNNONE?$AA@ (0000)
0025A284 003c:
	??_C@_0DM@IKBFNNO@deviant_surface_bunch_size?$DMMAXIM@ (0000)
0025A2C0 003c:
	??_C@_0DM@PHDNGGDN@surface_queue_write_index?$DM?$DNMAXIM@ (0000)
0025A2FC 003a:
	??_C@_0DK@BPOMGHAN@surface_queue_read_index?$DMMAXIMUM@ (0000)
0025A338 0059:
	??_C@_0FJ@FJNHAALP@?$CD?$CD?$CD?5DECALS?3?5Bernie?5doesn?8t?5under@ (0000)
0025A394 002b:
	??_C@_0CL@PJMHEHFM@?$CD?$CD?$CD?5ERROR?5unsupported?5projection@ (0000)
0025A3C0 0004:
	__real@b8d1b717 (0000)
0025A3C4 0011:
	??_C@_0BB@INIEPHFC@?$CBeditor_geometry?$AA@ (0000)
0025A3D8 000f:
	??_C@_0P@FPCAJBIO@decals_enabled?$AA@ (0000)
0025A3E8 0009:
	??_C@_08ICEJNDOI@velocity?$AA@ (0000)
0025A3F4 000a:
	??_C@_09BHPEDNK@collision?$AA@ (0000)
0025A400 0007:
	??_C@_06NHCMNMEH@origin?$AA@ (0000)
0025A408 001a:
	??_C@_0BK@DEMGPIOC@local_random_seed_address?$AA@ (0000)
002DD190 0001:
	_decals_enabled (0000)
00435D78 7804:
	_decal_geometry (0000)
*/

/* ---------- headers */

#define projection_from_vector3d projection_from_vector3d_inline
#define projection_sign_from_vector3d projection_sign_from_vector3d_inline
#define project_point2d project_point2d_inline
#define triple_product3d triple_product3d_inline
#define plane2d_from_points plane2d_from_points_inline
#define plane3d_from_point_and_normal plane3d_from_point_and_normal_inline
#define plane3d_negate plane3d_negate_inline
#define plane3d_distance_to_point plane3d_distance_to_point_inline
#include "effects/decals.h"
#include "cseries/cseries.h"
#include "math/real_math.h"
#include "physics/collision_bsp_definitions.h"
#undef projection_from_vector3d
#undef projection_sign_from_vector3d
#undef project_point2d
#undef triple_product3d
#undef plane2d_from_points
#undef plane3d_from_point_and_normal
#undef plane3d_negate
#undef plane3d_distance_to_point

#include "cseries/errors.h"
#include "game/game.h"
#include "memory/data.h"
#include "rasterizer/rasterizer.h"
#include "structures/structures.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "cache/texture_cache.h"
#include "effects/decal_definitions.h"
#include "rasterizer/rasterizer_debug_options.h"
#include "shaders/shader_definitions.h"

/* ---------- constants */

enum
{
	NUMBER_OF_DECAL_LAYERS = 5,
	MAXIMUM_DECALS_PER_MAP = 2048,
	MAXIMUM_DECAL_VERTICES = 1024,
	MAXIMUM_DECAL_SURFACE_QUEUE_SIZE = 1024
};

enum
{
	_decal_locked_bit,
	_decal_permanent_bit
};

enum
{
	_bitmap_group_type_sprites = 3
};

/* ---------- macros */

#define DECAL_GET(index) ((struct decal_datum *)datum_get(global_decal_data, (index)))

/* ---------- structures */

struct decal_editor_geometry;

struct decal_shader_definition
{
	struct shader shader;
	word flags;
	short type;
	short framebuffer_blend_function;
	word pad02E;
	long unused030[5];
	struct tag_reference map;
	long unused054[5];
};

struct decal_definition
{
	word flags;                                 /* 0x000 */
	short type;                                 /* 0x002 */
	short layer;                                /* 0x004 */
	word pad006;
	struct tag_reference next_decal_in_chain;   /* 0x008 */
	real radius_lower_bound;                    /* 0x018 */
	real radius_upper_bound;                    /* 0x01C */
	long unused020[3];
	real intensity_lower_bound;                 /* 0x02C */
	real intensity_upper_bound;                 /* 0x030 */
	real_rgb_color color_lower_bound;           /* 0x034 */
	real_rgb_color color_upper_bound;           /* 0x040 */
	long unused04C[3];
	short animation_loop_frame_index;
	short animation_speed;
	long unused05C[7];
	real lifetime_lower_bound;                  /* 0x078 */
	real lifetime_upper_bound;                  /* 0x07C */
	real decay_time_lower_bound;                /* 0x080 */
	real decay_time_upper_bound;                /* 0x084 */
	long unused088[3];
	struct decal_shader_definition shader;      /* 0x094 */
	real runtime_maximum_sprite_extent;         /* 0x0FC */
	word runtime_incremental_counter;
	word pad102;
	long unused104[2];
};

typedef char decal_definition_size_check[
	sizeof(struct decal_definition) == 0x10C ? 1 : -1];

typedef char decal_definition_map_index_offset_check[
	offsetof(struct decal_definition, shader.map.index) == 0xE4 ? 1 : -1];

struct bitmap_group_sprite
{
	short bitmap_index;
	word pad002;
	long unused004;
	real_rectangle2d bounds;
	real_point2d registration_point;
};

struct bitmap_group_sequence
{
	char name[32];
	short first_bitmap_index;
	short bitmap_count;
	long unused024[4];
	struct tag_block sprites;
};

struct decal_vertex
{
	real_point3d position;
	real_point2d texcoord;
	boolean clipped;
	byte pad015[3];
};

struct decal_geometry
{
	struct decal_vertex decal_vertices[MAXIMUM_DECAL_VERTICES];
	short decal_vertex_count;
	short decal_surface_vertex_counts[MAXIMUM_DECAL_SURFACE_QUEUE_SIZE];
	short decal_surface_count;
	long decal_surface_indices[MAXIMUM_DECAL_SURFACE_QUEUE_SIZE];
};

typedef char decal_geometry_size_check[
	sizeof(struct decal_geometry) == 0x7804 ? 1 : -1];

struct decal_projection
{
	real_matrix4x3 basis;
	real_rectangle2d extent;
	real_plane3d plane;
	short projection;
	boolean sign;
	byte pad057;
	real_point2d decal_points2d[4];
	real_vector2d texture_u_axis;
	real_vector2d texture_v_axis;
	real texture_scale;
};

typedef char decal_projection_size_check[
	sizeof(struct decal_projection) == 0x8C ? 1 : -1];

struct decal_render_vertex
{
	real_point3d position;
	short texcoord[2];
};

struct decal_quad
{
	struct decal_render_vertex vertices[4];
};

struct decal_wrap_parameters
{
	real minimum_wrap_angle;
	real minimum_skip_angle;
	real radius_exclusion_multiplier;
	boolean wrap;
	byte pad00D[3];
};

struct decal_datum
{
	short identifier;
	word flags;
	short cluster_index;
	short layer;
	real_point3d position;
	long creation_time;
	byte sequence_index;
	byte unused_was_frames_remaining;
	byte sprite_index;
	byte bitmap_index;
	real lifetime;
	real decay_time;
	pixel32 color;
	byte intensity;
	byte unused;
	short quad_count;
	long definition_index;
	long previous_decal_index;
	long next_decal_index;
};

typedef char decal_size_check[
	sizeof(struct decal_datum) == 0x38 ? 1 : -1];

struct decal_globals
{
	long first_decal_indices[NUMBER_OF_DECAL_LAYERS][MAXIMUM_CLUSTERS_PER_STRUCTURE];
	long first_disconnected_decal_index;
	long locked_count;
	long permanent_count;
};

/* ---------- prototypes */

static void decal_verify_neighbors(
	long decal_index,
	boolean layer_check);
static void decal_update(
	long decal_index);
static void decal_set_first_decal_index(
	short cluster_index,
	short layer,
	long decal_index);
static void decal_reinsert(
	long decal_index,
	short cluster_index,
	short layer);
static void decal_sprite_get_bounds(
	struct decal_definition const *definition,
	short sequence_index,
	short sprite_index,
	real radius,
	real_rectangle2d *sprite_bounds,
	real_rectangle2d *extent);
/* These three January-private callees are not yet reconstructed.  Keep their
   typed declarations local to this translation unit; their external linkage
   is only the compiler's unresolved-symbol representation until the static
   definitions are restored. */
void decal_projection_create(
	real_matrix4x3 const *basis,
	real_rectangle2d const *extent,
	struct decal_projection *projection);
void decal_clip_to_surface(
	struct decal_geometry *geometry,
	struct decal_projection const *projection,
	long surface_index,
	boolean update_surface_queue,
	real radius,
	short type,
	long *surface_queue,
	short *surface_queue_write_index,
	long *deviant_surface_list,
	short *deviant_surface_count);
long decal_insert(
	long cache_index,
	short cluster_index,
	short layer,
	long next_decal_index,
	boolean permanent);

/* ---------- globals */

extern struct data_array *global_decal_data;

boolean decals_enabled= TRUE;
static boolean decal_locked_count_reported;
static struct decal_globals *decal_globals;
static struct decal_geometry decal_geometry;

struct decal_wrap_parameters const decal_wrap_parameters[NUMBER_OF_DECAL_TYPES] =
{
	{ 40.0f, 110.0f, 1.5f, TRUE },
	{ 40.0f, 110.0f, 1.5f, TRUE },
	{ 40.0f, 110.0f, 1.5f, TRUE },
	{ 10.0f,  10.0f, 1.5f, FALSE }
};

/* ---------- public code */

void decals_dispose_from_old_map(
	void)
{
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 777, global_decal_data);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 778, decal_globals);
	rasterizer_decals_dispose_from_old_map();
	data_make_invalid(global_decal_data);

	return;
}

void decals_dispose(
	void)
{
	global_decal_data = NULL;
	rasterizer_decals_dispose();

	return;
}

long decal_get_first_decal_index(
	short cluster_index,
	short layer)
{
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1006,
		cluster_index>=0 && cluster_index<MAXIMUM_CLUSTERS_PER_STRUCTURE);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1007,
		layer>=0 && layer<NUMBER_OF_DECAL_LAYERS);

	return decal_globals->first_decal_indices[layer][cluster_index];
}

void decal_new_from_media_collision(
	long decal_definition_index,
	struct collision_result const *collision,
	real_vector3d const *velocity,
	real radius_modifier,
	boolean permanent,
	short forced_sequence_index,
	struct decal_editor_geometry *editor_geometry)
{
	return;
}

static void decal_sprite_get_bounds(
	struct decal_definition const *definition,
	short sequence_index,
	short sprite_index,
	real radius,
	real_rectangle2d *sprite_bounds,
	real_rectangle2d *extent)
{
	struct bitmap_group *bitmap_group_definition;
	struct bitmap_group_sequence *sequence;
	struct bitmap_group_sprite *sprite;
	struct bitmap_data *bitmap;
	real aspect_scale = 1.0f;
	real scale;
	real width_scale;
	real height_scale;

	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 0x107, definition);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 0x108, sprite_bounds);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 0x109, extent);

	bitmap_group_definition = bitmap_group_get(definition->shader.map.index);
	sequence = TAG_BLOCK_GET_ELEMENT(
		&bitmap_group_definition->sequences,
		sequence_index,
		struct bitmap_group_sequence);
	sprite = TAG_BLOCK_GET_ELEMENT(
		&sequence->sprites,
		sprite_index,
		struct bitmap_group_sprite);
	bitmap = TAG_BLOCK_GET_ELEMENT(
		&bitmap_group_definition->bitmap_data,
		sprite->bitmap_index,
		struct bitmap_data);

	*sprite_bounds = sprite->bounds;

	if (TEST_FLAG(definition->flags,
		_decal_definition_preserve_aspect_bit))
	{
		aspect_scale = ((sprite->bounds.x1 - sprite->bounds.x0)
			/ (sprite->bounds.y1 - sprite->bounds.y0))
			* ((real)bitmap->height / (real)bitmap->width);
	}

	scale = radius / definition->runtime_maximum_sprite_extent;
	width_scale = (real)bitmap->width * scale;
	height_scale = (real)bitmap->height * scale * aspect_scale;

	extent->x0 = -sprite->registration_point.x * width_scale;
	extent->x1 = (sprite->bounds.x1 - sprite->registration_point.x
		- sprite->bounds.x0) * width_scale;
	extent->y0 = -sprite->registration_point.y * height_scale;
	extent->y1 = (sprite->bounds.y1 - sprite->registration_point.y
		- sprite->bounds.y0) * height_scale;

	return;
}

short projection_from_vector3d(
	real_vector3d const *n)
{
	real i = fabs(n->i);
	real j = fabs(n->j);
	real k = fabs(n->k);

	if (k >= j && k >= i)
	{
		return _z;
	}
	else if (j >= i)
	{
		return _y;
	}
	else
	{
		return _x;
	}
}

boolean projection_sign_from_vector3d(
	real_vector3d const *n,
	short projection)
{
	match_assert("..\\math\\real_math.h", 848,
		projection>=_x && projection<=_z);

	return n->n[projection] > 0.0f;
}

real_point3d *project_point2d(
	real_point2d const *p2d,
	real_plane3d const *plane,
	short projection,
	boolean sign,
	real_point3d *p3d)
{
	short x = global_projection3d_mappings[projection][sign][0];
	short y = global_projection3d_mappings[projection][sign][1];

	match_assert("..\\math\\real_math.h", 879, projection>=_x && projection<=_z);
	match_assert("..\\math\\real_math.h", 880, ~(sign&~1));

	p3d->n[x] = p2d->x;
	p3d->n[y] = p2d->y;
	p3d->n[projection] = (fabs(plane->n.n[projection])<_real_epsilon)
		? 0.0f
		: ((plane->d - (p2d->x * plane->n.n[x])) - (p2d->y * plane->n.n[y])) / plane->n.n[projection];

	return p3d;
}

real triple_product3d(
	real_vector3d const *a,
	real_vector3d const *b,
	real_vector3d const *n)
{
	real_vector3d cross;

	cross_product3d(a, b, &cross);

	return dot_product3d(&cross, n);
}

real_plane2d *plane2d_from_points(
	real_plane2d *plane,
	real_point2d const *point0,
	real_point2d const *point1)
{
	plane->n.i = point1->y - point0->y;
	plane->n.j = point0->x - point1->x;

	if (normalize2d(&plane->n) == 0.0f)
	{
		plane->d = 0.0f;

		return NULL;
	}

	plane->d = dot_product2d((real_vector2d *)point0, &plane->n);

	return plane;
}

real_plane3d *plane3d_from_point_and_normal(
	real_plane3d *plane,
	real_point3d const *point,
	real_vector3d const *normal)
{
	plane->n = *normal;
	plane->d = dot_product3d((real_vector3d *)point, &plane->n);

	return plane;
}

real_plane3d *plane3d_negate(
	real_plane3d const *p1,
	real_plane3d *plane)
{
	plane->n.i = -p1->n.i;
	plane->n.j = -p1->n.j;
	plane->n.k = -p1->n.k;
	plane->d = -p1->d;

	return plane;
}

real plane3d_distance_to_point(
	real_plane3d const *plane,
	real_point3d const *point)
{
	return point->x * plane->n.i
		+ point->y * plane->n.j
		+ point->z * plane->n.k
		- plane->d;
}

void decals_update(
	void)
{
	if (global_decal_data->valid)
	{
		struct data_iterator iterator;

		data_iterator_new(&iterator, global_decal_data);

		while (data_iterator_next(&iterator))
		{
			decal_update(iterator.datum_index);
		}
	}

	return;
}

void decals_delete_permanent_from_cluster(
	short cluster_index)
{
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 887,
		cluster_index>=0 && cluster_index<MAXIMUM_CLUSTERS_PER_STRUCTURE);

	if (global_decal_data->valid)
	{
		short layer;

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 893, decal_globals);

		for (layer = 0; layer<NUMBER_OF_DECAL_LAYERS; layer++)
		{
			long decal_index;

			if (cluster_index==NONE)
			{
				decal_index = (layer==0)
					? decal_globals->first_disconnected_decal_index
					: NONE;
			}
			else
			{
				decal_index = decal_get_first_decal_index(cluster_index, layer);
			}

			while (decal_index!=NONE)
			{
				struct decal_datum *decal = DECAL_GET(decal_index);
				long next_decal_index = decal->next_decal_index;

				match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 920, decal->cluster_index==cluster_index);

				if (TEST_FLAG(decal->flags, _decal_permanent_bit))
				{
					SET_FLAG(decal->flags, _decal_permanent_bit, FALSE);
					decal_globals->permanent_count -= 1;

					match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 927,
						!TEST_FLAG(decal->flags, _decal_locked_bit));

					rasterizer_decal_vertices_delete(decal_index);
				}

				decal_index = next_decal_index;
			}
		}

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 936, decal_globals->permanent_count>=0);
	}

	return;
}

void decals_disconnect_from_structure_bsp(
	void)
{
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 713, global_decal_data);

	if (global_decal_data->valid)
	{
		short cluster_index;

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 719, decal_globals);

		for (cluster_index= 0; cluster_index<MAXIMUM_CLUSTERS_PER_STRUCTURE; cluster_index++)
		{
			short layer;

			for (layer= 0; layer<NUMBER_OF_DECAL_LAYERS; layer++)
			{
				long first_decal_index= decal_get_first_decal_index(cluster_index, layer);
				long decal_index= first_decal_index;
				long iteration_count= 0;

				while (decal_index!=NONE)
				{
					struct decal_datum *decal= DECAL_GET(decal_index);
					long next_decal_index= decal->next_decal_index;

					if (iteration_count++>MAXIMUM_DECALS_PER_MAP)
					{
						error(_error_silent, "### ERROR decals: infinite loop -- tell Bernie!!");
						break;
					}

					match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 746, decal->cluster_index==cluster_index);

					decal->cluster_index= NONE;

					if (decal->next_decal_index==NONE)
					{
						decal->next_decal_index= decal_globals->first_disconnected_decal_index;

						if (decal_globals->first_disconnected_decal_index!=NONE)
						{
							DECAL_GET(decal_globals->first_disconnected_decal_index)->previous_decal_index= decal_index;
						}

						decal_globals->first_disconnected_decal_index= first_decal_index;

						decal_set_first_decal_index(cluster_index, layer, NONE);
					}

					decal_index= next_decal_index;
				}
			}
		}
	}

	return;
}

void decals_reconnect_to_structure_bsp(
	void)
{
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 633, global_decal_data);

	if (global_decal_data->valid)
	{
		long decal_index;
		long iteration_count= 0;

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 641, decal_globals);

		decal_index= decal_globals->first_disconnected_decal_index;

		while (decal_index!=NONE)
		{
			struct decal_datum *decal= DECAL_GET(decal_index);
			long next_decal_index= decal->next_decal_index;
			struct location location;

			if (iteration_count++>MAXIMUM_DECALS_PER_MAP)
			{
				error(_error_silent, "### ERROR decals: infinite loop -- tell Bernie!!");
				break;
			}

			match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 660, decal->cluster_index==NONE);
			match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 661,
				decal->layer>=0 && decal->layer<NUMBER_OF_DECAL_LAYERS);

			decal_verify_neighbors(decal_index, FALSE);

			scenario_location_from_point(&location, &decal->position);

			if (location.cluster_index!=NONE)
			{
				if (decal->next_decal_index!=NONE)
				{
					DECAL_GET(decal->next_decal_index)->previous_decal_index= decal->previous_decal_index;
				}

				if (decal->previous_decal_index!=NONE)
				{
					DECAL_GET(decal->previous_decal_index)->next_decal_index= decal->next_decal_index;
				}
				else
				{
					match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 682,
						decal_globals->first_disconnected_decal_index==decal_index);

					decal_globals->first_disconnected_decal_index= decal->next_decal_index;
				}

				decal_reinsert(decal_index, location.cluster_index, decal->layer);
			}

			decal_verify_neighbors(decal_index, FALSE);

			decal_index= next_decal_index;
		}
	}

	return;
}

void decal_new_from_collision(
	long decal_definition_index,
	struct collision_result const *collision,
	real_vector3d const *velocity,
	real radius_modifier,
	boolean permanent,
	short forced_sequence_index,
	struct decal_editor_geometry *editor_geometry)
{
	struct collision_bsp *collision_bsp = global_collision_bsp_get();
	real_matrix4x3 basis;
	real_rectangle2d extent;
	real_rectangle2d sprite_bounds;
	real radius = 0.0f;
	short sequence_index = 0;
	short bitmap_index = 0;
	boolean reuse_previous_geometry = FALSE;

	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2033, collision);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2034, velocity);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2035, decals_enabled);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2037, !editor_geometry);

	while (decal_definition_index!=NONE)
	{
		struct decal_definition *definition = decal_definition_get(decal_definition_index);
		struct bitmap_group *bitmap_group = bitmap_group_get(definition->shader.map.index);

		if (!reuse_previous_geometry)
		{
			real_vector3d tangent;
			real_vector3d bitangent;
			real cosine;
			real sine;
			real tangent_magnitude;
			real bitangent_magnitude;

			if (TEST_FLAG(definition->flags, _decal_definition_no_random_rotation_bit) &&
				dot_product3d(&collision->plane.n, velocity) < -_real_epsilon)
			{
				cosine = -1.0f;
				sine = 0.0f;

				if (TEST_FLAG(definition->flags, _decal_definition_SAPIEN_ONLY_snap_to_axis_bit))
				{
					real_vector3d axis_vector = { 0.0f, 0.0f, 0.0f };
					short projection = projection_from_vector3d(velocity);
					real sign = projection_sign_from_vector3d(velocity, projection)
						? 1.0f
						: -1.0f;

					switch (projection)
					{
					case _x:
						axis_vector.i = sign;
						break;
					case _y:
						axis_vector.j = sign;
						break;
					case _z:
						axis_vector.k = sign;
						break;
					default:
						match_vassert("c:\\halo\\SOURCE\\effects\\decals.c", 2120,
							FALSE, "### ERROR unsupported projection");
						break;
					}

					if (dot_product3d(&axis_vector, &collision->plane.n) <= 0.0f)
					{
						axis_vector.i -= collision->plane.n.i;
						axis_vector.j -= collision->plane.n.j;
						axis_vector.k -= collision->plane.n.k;
					}
					else
					{
						axis_vector.i += collision->plane.n.i;
						axis_vector.j += collision->plane.n.j;
						axis_vector.k += collision->plane.n.k;
					}

					normalize3d(&axis_vector);
					cross_product3d(&collision->plane.n, &axis_vector, &tangent);
					cross_product3d(&collision->plane.n, &tangent, &bitangent);

					if (magnitude_squared3d(&tangent) < _real_epsilon ||
						magnitude_squared3d(&bitangent) < _real_epsilon)
					{
						real_vector3d reflected_velocity;
						real reflection_scale = -dot_product3d(velocity, &collision->plane.n);

						reflected_velocity.i = reflection_scale * collision->plane.n.i + velocity->i;
						reflected_velocity.j = reflection_scale * collision->plane.n.j + velocity->j;
						reflected_velocity.k = reflection_scale * collision->plane.n.k + velocity->k;

						projection = projection_from_vector3d(&reflected_velocity);
						sign = projection_sign_from_vector3d(&reflected_velocity, projection)
							? 1.0f
							: -1.0f;
						axis_vector.i = 0.0f;
						axis_vector.j = 0.0f;
						axis_vector.k = 0.0f;

						switch (projection)
						{
						case _x:
							axis_vector.i = sign;
							break;
						case _y:
							axis_vector.j = sign;
							break;
						case _z:
							axis_vector.k = sign;
							break;
						default:
							match_vassert("c:\\halo\\SOURCE\\effects\\decals.c", 2152,
								FALSE, "### ERROR unsupported projection");
							break;
						}

						if (dot_product3d(&axis_vector, &collision->plane.n) <= 0.0f)
						{
							axis_vector.i -= collision->plane.n.i;
							axis_vector.j -= collision->plane.n.j;
							axis_vector.k -= collision->plane.n.k;
						}
						else
						{
							axis_vector.i += collision->plane.n.i;
							axis_vector.j += collision->plane.n.j;
							axis_vector.k += collision->plane.n.k;
						}

						normalize3d(&axis_vector);
						cross_product3d(&collision->plane.n, &axis_vector, &tangent);
						cross_product3d(&collision->plane.n, &tangent, &bitangent);
					}
				}
				else
				{
					cross_product3d(&collision->plane.n, velocity, &tangent);
					cross_product3d(&collision->plane.n, &tangent, &bitangent);
				}
			}
			else
			{
				real angle = real_seed_random_range(
					get_global_local_random_seed_address(),
					0.0f,
					2.0f * _pi);

				cosine = (real)cos(angle);
				sine = (real)sin(angle);
				perpendicular3d(&collision->plane.n, &tangent);
				cross_product3d(&collision->plane.n, &tangent, &bitangent);
			}

			tangent_magnitude = magnitude3d(&tangent);
			if (fabs(tangent_magnitude) >= _real_epsilon)
			{
				scale_vector3d(&tangent, 1.0f / tangent_magnitude, &tangent);
			}

			bitangent_magnitude = magnitude3d(&bitangent);
			if (fabs(bitangent_magnitude) >= _real_epsilon)
			{
				scale_vector3d(&bitangent, 1.0f / bitangent_magnitude, &bitangent);
			}

			basis.scale = 1.0f;
			basis.forward.i = cosine * bitangent.i - tangent.i * sine;
			basis.forward.j = cosine * bitangent.j - tangent.j * sine;
			basis.forward.k = cosine * bitangent.k - tangent.k * sine;
			basis.left.i = tangent.i * cosine + bitangent.i * sine;
			basis.left.j = tangent.j * cosine + bitangent.j * sine;
			basis.left.k = tangent.k * cosine + bitangent.k * sine;
			basis.up = collision->plane.n;
			basis.position = collision->point;

			if (forced_sequence_index==NONE)
			{
				sequence_index = seed_random_range(
					get_global_local_random_seed_address(),
					0,
					(short)bitmap_group->sequences.count);

				if (sequence_index>=bitmap_group->sequences.count)
				{
					error(_error_silent,
						"### DECALS: Bernie doesn't understand how local_random_range() works, don't tell Jason!!");
					sequence_index = (short)(bitmap_group->sequences.count - 1);
				}
			}
			else
			{
				sequence_index = forced_sequence_index;
			}

			if (radius_modifier==0.0f)
			{
				radius_modifier = 1.0f;
			}

			radius = real_seed_random_range(
				get_global_local_random_seed_address(),
				definition->radius_lower_bound,
				definition->radius_upper_bound) * radius_modifier;
		}

		if (bitmap_group->type==_bitmap_group_type_sprites)
		{
			struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
				&bitmap_group->sequences,
				sequence_index,
				struct bitmap_group_sequence);
			struct bitmap_group_sprite *sprite = TAG_BLOCK_GET_ELEMENT(
				&sequence->sprites,
				0,
				struct bitmap_group_sprite);

			bitmap_index = sprite->bitmap_index;
			decal_sprite_get_bounds(
				definition,
				sequence_index,
				0,
				radius,
				&sprite_bounds,
				&extent);
		}
		else
		{
			real aspect = 1.0f;

			bitmap_index = 0;
			if (TEST_FLAG(definition->flags, _decal_definition_preserve_aspect_bit))
			{
				struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(
					&bitmap_group->bitmap_data,
					0,
					struct bitmap_data);

				aspect = (real)bitmap->height / (real)bitmap->width;
			}

			extent.x0 = -radius;
			extent.x1 = radius;
			extent.y0 = -(aspect * radius);
			extent.y1 = aspect * radius;
			sprite_bounds.x0 = 0.0f;
			sprite_bounds.x1 = 1.0f;
			sprite_bounds.y0 = 0.0f;
			sprite_bounds.y1 = 1.0f;
		}

		if (!permanent)
		{
			struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(
				&bitmap_group->bitmap_data,
				bitmap_index,
				struct bitmap_data);

			if (!_texture_cache_bitmap_get_hardware_format(bitmap, FALSE, TRUE))
			{
				return;
			}
		}

		{
			struct decal_projection projection;
			long surface_queue[MAXIMUM_DECAL_SURFACE_QUEUE_SIZE];
			long deviant_surface_list[MAXIMUM_DECAL_SURFACE_QUEUE_SIZE];
			long deviant_surface_bunch[MAXIMUM_DECAL_SURFACE_QUEUE_SIZE];
			struct decal_render_vertex render_vertices[MAXIMUM_DECAL_VERTICES];
			short surface_queue_read_index;
			short surface_queue_write_index = 1;
			short deviant_surface_count = 0;
			real_vector3d minimum_normal = basis.up;
			real_vector3d maximum_normal = basis.up;

			decal_projection_create(&basis, &extent, &projection);
			decal_geometry.decal_vertex_count = 0;
			decal_geometry.decal_surface_count = 0;
			surface_queue[0] = collision->surface_index;

			for (surface_queue_read_index = 0;
				surface_queue_read_index<surface_queue_write_index;
				surface_queue_read_index++)
			{
				match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2353,
					surface_queue_read_index<MAXIMUM_DECAL_SURFACE_QUEUE_SIZE);
				match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2354,
					surface_queue_write_index<=MAXIMUM_DECAL_SURFACE_QUEUE_SIZE);

				decal_clip_to_surface(
					&decal_geometry,
					&projection,
					surface_queue[surface_queue_read_index],
					TRUE,
					radius,
					definition->type,
					surface_queue,
					&surface_queue_write_index,
					deviant_surface_list,
					&deviant_surface_count);
			}

			if (decal_wrap_parameters[definition->type].wrap && deviant_surface_count>0)
			{
				short remaining_deviant_surface_count = deviant_surface_count;
				short deviant_surface_index;

				for (deviant_surface_index = 0;
					deviant_surface_index<deviant_surface_count && remaining_deviant_surface_count>0;
					deviant_surface_index++)
				{
					long deviant_surface = deviant_surface_list[deviant_surface_index];

					if (deviant_surface!=NONE)
					{
						struct collision_surface *surface = TAG_BLOCK_GET_ELEMENT(
							&collision_bsp->surfaces,
							deviant_surface,
							struct collision_surface);
						real_plane3d surface_plane;
						long plane_designator = surface->plane_designator;
						short deviant_surface_bunch_size = 0;
						short next_deviant_surface_index;

						bsp3d_get_plane_from_designator(
							&collision_bsp->bsp3d,
							plane_designator,
							&surface_plane);

						match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2399,
							deviant_surface_bunch_size<MAXIMUM_DECAL_SURFACE_QUEUE_SIZE);
						deviant_surface_bunch[deviant_surface_bunch_size++] = deviant_surface;
						deviant_surface_list[deviant_surface_index] = NONE;

						for (next_deviant_surface_index = (short)(deviant_surface_index + 1);
							next_deviant_surface_index<deviant_surface_count;
							next_deviant_surface_index++)
						{
							long next_deviant_surface = deviant_surface_list[next_deviant_surface_index];

							if (next_deviant_surface!=NONE)
							{
								struct collision_surface *next_surface = TAG_BLOCK_GET_ELEMENT(
									&collision_bsp->surfaces,
									next_deviant_surface,
									struct collision_surface);
								long next_plane_designator = next_surface->plane_designator;
								real_plane3d next_surface_plane;

								bsp3d_get_plane_from_designator(
									&collision_bsp->bsp3d,
									next_plane_designator,
									&next_surface_plane);

								if (angle_between_normals3d(&surface_plane.n, &next_surface_plane.n) <=
									DEGREES_TO_RADIANS(decal_wrap_parameters[definition->type].minimum_wrap_angle))
								{
									match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2422,
										deviant_surface_bunch_size<MAXIMUM_DECAL_SURFACE_QUEUE_SIZE);
									deviant_surface_bunch[deviant_surface_bunch_size++] = next_deviant_surface;
									deviant_surface_list[next_deviant_surface_index] = NONE;
								}
							}
						}

						{
							long closest_surface_index = NONE;
							real closest_minimum_distance = 0.0f;
							real closest_maximum_distance = 0.0f;
							real_plane3d closest_surface_plane;
							real_point3d closest_edge_start;
							real_point3d closest_edge_end;
							short bunch_index;

							for (bunch_index = 0;
								bunch_index<deviant_surface_bunch_size;
								bunch_index++)
							{
								long bunch_surface_index = deviant_surface_bunch[bunch_index];
								struct collision_surface *bunch_surface = TAG_BLOCK_GET_ELEMENT(
									&collision_bsp->surfaces,
									bunch_surface_index,
									struct collision_surface);
								long edge_index = bunch_surface->first_edge_index;

								do
								{
									struct collision_edge *edge = TAG_BLOCK_GET_ELEMENT(
										&collision_bsp->edges,
										edge_index,
										struct collision_edge);
									boolean surface_on_right = edge->surface_indices[1]==bunch_surface_index;
									struct collision_vertex *edge_start = TAG_BLOCK_GET_ELEMENT(
										&collision_bsp->vertices,
										edge->vertex_indices[surface_on_right ? 0 : 1],
										struct collision_vertex);
									struct collision_vertex *edge_end = TAG_BLOCK_GET_ELEMENT(
										&collision_bsp->vertices,
										edge->vertex_indices[surface_on_right ? 1 : 0],
										struct collision_vertex);
									real start_distance = fabs(plane3d_distance_to_point(&projection.plane, &edge_start->point));
									real end_distance = fabs(plane3d_distance_to_point(&projection.plane, &edge_end->point));
									real minimum_distance = MIN(start_distance, end_distance);
									real maximum_distance = MAX(start_distance, end_distance);

									if (closest_surface_index==NONE ||
										(minimum_distance<=closest_minimum_distance &&
										 maximum_distance<=closest_maximum_distance))
									{
										long closest_plane_designator = bunch_surface->plane_designator;

										closest_surface_index = bunch_surface_index;
										closest_minimum_distance = minimum_distance;
										closest_maximum_distance = maximum_distance;
										closest_edge_start = edge_start->point;
										closest_edge_end = edge_end->point;
										bsp3d_get_plane_from_designator(
											&collision_bsp->bsp3d,
											closest_plane_designator,
											&closest_surface_plane);
									}

									edge_index = edge->edge_indices[surface_on_right ? 1 : 0];
								}
								while (edge_index!=bunch_surface->first_edge_index);
							}

							match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2493,
								closest_surface_index!=NONE);

							{
								struct decal_projection wrapped_projection;
								real_vector3d edge_axis;
								real edge_length;

								vector_from_points3d(&closest_edge_start, &closest_edge_end, &edge_axis);
								edge_length = magnitude3d(&edge_axis);

								if (fabs(edge_length)>=_real_epsilon && edge_length>0.0f)
								{
									real handedness;
									real sign = 1.0f;
									real angle;
									real_matrix4x3 rotation;
									real_matrix4x3 wrapped_basis;
									real_point3d relative_position;

									scale_vector3d(&edge_axis, 1.0f / edge_length, &edge_axis);
									handedness = triple_product3d(
										&closest_surface_plane.n,
										&projection.plane.n,
										&edge_axis);
									if (handedness>=0.0f)
									{
										sign = -1.0f;
									}

									angle = angle_between_normals3d(
										&closest_surface_plane.n,
										&projection.plane.n) * sign;
									matrix4x3_rotation_from_axis_and_angle(
										&rotation,
										&edge_axis,
										(real)sin(angle),
										(real)cos(angle));

									vector_from_points3d(
										&closest_edge_start,
										&basis.position,
										(real_vector3d *)&relative_position);
									matrix4x3_transform_point(&rotation, &relative_position, &wrapped_basis.position);
									matrix4x3_transform_normal(&rotation, &basis.forward, &wrapped_basis.forward);
									matrix4x3_transform_normal(&rotation, &basis.left, &wrapped_basis.left);
									matrix4x3_transform_normal(&rotation, &basis.up, &wrapped_basis.up);
									wrapped_basis.scale = 1.0f;
									wrapped_basis.position.x += closest_edge_start.x;
									wrapped_basis.position.y += closest_edge_start.y;
									wrapped_basis.position.z += closest_edge_start.z;

									decal_projection_create(&wrapped_basis, &extent, &wrapped_projection);
									minimum_normal.i = MIN(minimum_normal.i, wrapped_basis.up.i);
									maximum_normal.i = MAX(maximum_normal.i, wrapped_basis.up.i);
									minimum_normal.j = MIN(minimum_normal.j, wrapped_basis.up.j);
									maximum_normal.j = MAX(maximum_normal.j, wrapped_basis.up.j);
									minimum_normal.k = MIN(minimum_normal.k, wrapped_basis.up.k);
									maximum_normal.k = MAX(maximum_normal.k, wrapped_basis.up.k);
								}
								else
								{
									error(_error_silent,
										"### ERROR: decals: failed to wrap decal around edge -- tell Bernie!!");
									wrapped_projection = projection;
								}

								for (bunch_index = 0;
									bunch_index<deviant_surface_bunch_size;
									bunch_index++)
								{
									decal_clip_to_surface(
										&decal_geometry,
										&wrapped_projection,
										deviant_surface_bunch[bunch_index],
										FALSE,
										radius,
										definition->type,
										NULL,
										NULL,
										NULL,
										NULL);
								}
							}

							remaining_deviant_surface_count -= deviant_surface_bunch_size;
						}
					}
				}
			}

			if (decal_geometry.decal_surface_count<=0 || decal_geometry.decal_vertex_count<=0)
			{
				return;
			}

			{
				real_vector3d offset = { 0.0f, 0.0f, 0.0f };
				short decal_quad_count = 0;
				short decal_surface_index;
				long cache_size;
				long cache_index;

				if (maximum_normal.i - minimum_normal.i<=0.5f &&
					maximum_normal.j - minimum_normal.j<=0.5f &&
					maximum_normal.k - minimum_normal.k<=0.5f)
				{
					offset.i = minimum_normal.i + maximum_normal.i;
					offset.j = minimum_normal.j + maximum_normal.j;
					offset.k = minimum_normal.k + maximum_normal.k;
					normalize3d(&offset);
					scale_vector3d(&offset, rasterizer_debug_options.zoffset, &offset);
				}

				for (decal_surface_index = 0;
					decal_surface_index<decal_geometry.decal_surface_count;
					decal_surface_index++)
				{
					short decal_surface_vertex_count =
						decal_geometry.decal_surface_vertex_counts[decal_surface_index];

					match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2650,
						decal_geometry.decal_surface_vertex_counts[decal_surface_index]>=NUMBER_OF_VERTICES_PER_TRIANGLE);
					decal_quad_count += (short)((decal_surface_vertex_count - 1) / 2);
				}

				cache_size = decal_quad_count * sizeof(struct decal_quad);
				cache_index = rasterizer_decal_vertices_new(cache_size);

				if (cache_index!=NONE)
				{
					long decal_index = decal_insert(
						cache_index,
						collision->location.cluster_index,
						definition->layer,
						NONE,
						permanent);

					if (decal_index!=NONE)
					{
						struct decal_datum *decal = DECAL_GET(decal_index);
						struct decal_quad *quads = rasterizer_decal_vertices_lock(
							(short)cache_index,
							cache_size);

						if (quads)
						{
							short decal_vertex_index;
							real u_scale = sprite_bounds.x1 - sprite_bounds.x0;
							real v_scale = sprite_bounds.y1 - sprite_bounds.y0;

							for (decal_vertex_index = 0;
								decal_vertex_index<decal_geometry.decal_vertex_count;
								decal_vertex_index++)
							{
								struct decal_vertex *decal_vertex =
									&decal_geometry.decal_vertices[decal_vertex_index];
								real u_real = u_scale * decal_vertex->texcoord.x + sprite_bounds.x0;
								real v_real = v_scale * decal_vertex->texcoord.y + sprite_bounds.y0;
								long u;
								long v;

								u_real = MAX(0.0f, MIN(u_real, 1.0f));
								v_real = MAX(0.0f, MIN(v_real, 1.0f));
								u_real = MAX(0.0f, MIN(u_real * 32768.0f, 32767.0f));
								v_real = MAX(0.0f, MIN(v_real * 32768.0f, 32767.0f));
								u = (long)floor(u_real + 0.5f);
								v = (long)floor(v_real + 0.5f);
								render_vertices[decal_vertex_index].texcoord[0] =
									(short)u;
								render_vertices[decal_vertex_index].texcoord[1] =
									(short)v;

								match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2696,
									(u&0x8000)==0 && (v&0x8000)==0);

								render_vertices[decal_vertex_index].position.x = offset.i + decal_vertex->position.x;
								render_vertices[decal_vertex_index].position.y = offset.j + decal_vertex->position.y;
								render_vertices[decal_vertex_index].position.z = offset.k + decal_vertex->position.z;
							}

							decal->position = collision->point;
							decal->creation_time = game_time_get();
							decal->sequence_index = (byte)sequence_index;
							decal->sprite_index = 0;
							decal->bitmap_index = (byte)bitmap_index;
							decal->lifetime = real_seed_random_range(
								get_global_local_random_seed_address(),
								definition->lifetime_lower_bound,
								definition->lifetime_upper_bound);
							decal->decay_time = real_seed_random_range(
								get_global_local_random_seed_address(),
								definition->decay_time_lower_bound,
								definition->decay_time_upper_bound);
							decal->definition_index = decal_definition_index;
							decal->quad_count = decal_quad_count;

							{
								real intensity = real_seed_random_range(
									get_global_local_random_seed_address(),
									definition->intensity_lower_bound,
									definition->intensity_upper_bound);
								real_rgb_color color;
								real interpolation = real_seed_random_range(
									get_global_local_random_seed_address(),
									0.0f,
									1.0f);

								rgb_colors_interpolate(
									&color,
									(definition->flags >> _decal_definition_color_interpolate_in_hsv_bit) & 3,
									&definition->color_lower_bound,
									&definition->color_upper_bound,
									interpolation);
								decal->color = real_a_rgb_color_to_pixel32(intensity, &color);
								decal->intensity = 255;
							}

							{
								short quad_index = 0;
								short decal_surface_vertex_base = 0;

								for (decal_surface_index = 0;
									decal_surface_index<decal_geometry.decal_surface_count;
									decal_surface_index++)
								{
									short decal_surface_vertex_count =
										decal_geometry.decal_surface_vertex_counts[decal_surface_index];
									short decal_surface_vertex_index;

									match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2744,
										decal_surface_vertex_count>=NUMBER_OF_VERTICES_PER_TRIANGLE);

									for (decal_surface_vertex_index = 1;
										decal_surface_vertex_index + 1<decal_surface_vertex_count;
										decal_surface_vertex_index += 2)
									{
										short final_vertex_index;

									match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2748,
										decal_surface_vertex_index+1<decal_surface_vertex_count);

										if (decal_surface_vertex_index + 2<decal_surface_vertex_count)
										{
											final_vertex_index = (short)(decal_surface_vertex_base + decal_surface_vertex_index + 2);
										}
										else
										{
											final_vertex_index = decal_surface_vertex_base;
										}

										quads[quad_index].vertices[0] = render_vertices[decal_surface_vertex_base];
										quads[quad_index].vertices[1] = render_vertices[decal_surface_vertex_base + decal_surface_vertex_index];
										quads[quad_index].vertices[2] = render_vertices[decal_surface_vertex_base + decal_surface_vertex_index + 1];
										quads[quad_index].vertices[3] = render_vertices[final_vertex_index];
										quad_index++;
									}

									decal_surface_vertex_base += decal_surface_vertex_count;
								}

								match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 2764,
									quad_index==decal_quad_count);
							}

							rasterizer_decal_vertices_unlock();
							reuse_previous_geometry = TEST_FLAG(
								definition->flags,
								_decal_definition_geometry_inherited_by_next_decal_in_chain_bit);
							decal_definition_index = definition->next_decal_in_chain.index;
						}
						else
						{
							rasterizer_decal_vertices_delete(cache_index);
							if (debug_decals)
							{
								error(_error_silent, "### WTF decals: failed to lock vertices");
							}
							return;
						}
					}
					else
					{
						rasterizer_decal_vertices_delete(cache_index);
						if (debug_decals)
						{
							error(_error_silent,
								"### WTF decals: failed to insert decal (locked=%d, permanent=%d)",
								decal_globals->locked_count,
								decal_globals->permanent_count);
						}
						return;
					}
				}
				else
				{
					if (debug_decals)
					{
						error(_error_silent,
							"### WTF decals: failed to allocate vertices (locked=%d, permanent=%d)",
							decal_globals->locked_count,
							decal_globals->permanent_count);
					}
					return;
				}
			}
		}
	}

	return;
}

void decal_new(
	long decal_definition_index,
	real_point3d const *origin,
	real_vector3d const *velocity,
	real radius_modifier,
	boolean permanent,
	short forced_sequence_index,
	struct decal_editor_geometry *editor_geometry)
{
	if (decals_enabled)
	{
		unsigned long *local_random_seed_address= get_global_local_random_seed_address();
		unsigned long local_random_seed= 0;
		struct collision_result collision;

		if (permanent)
		{
			match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1533, local_random_seed_address);
			match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1534, origin);

			local_random_seed= *local_random_seed_address;
			*local_random_seed_address= ((unsigned long const *)origin)[2]
				^ ((unsigned long const *)origin)[1]
				^ ((unsigned long const *)origin)[0]
				^ 0xdeadc0de;
		}

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1550,
			global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
		global_current_collision_users[global_current_collision_user_depth++] =
			_collision_user_decals;

		if (collision_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit) |
				FLAG(_collision_test_structure_bit) |
				FLAG(_collision_test_media_bit) |
				FLAG(_collision_test_try_to_keep_location_valid_bit),
			origin,
			velocity,
			NONE,
			&collision))
		{
			if (collision.type==_collision_result_media)
			{
				decal_new_from_media_collision(decal_definition_index, &collision, velocity,
					radius_modifier, permanent, forced_sequence_index, editor_geometry);
			}
			else if (collision.type==_collision_result_structure)
			{
				struct decal_definition *definition= decal_definition_get(decal_definition_index);

				if (!TEST_FLAG(definition->flags, _decal_definition_water_effect_bit))
				{
					decal_new_from_collision(decal_definition_index, &collision, velocity,
						radius_modifier, permanent, forced_sequence_index, editor_geometry);
				}
			}
		}

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1586,
			global_current_collision_user_depth > 1);
		--global_current_collision_user_depth;

		if (permanent)
		{
			match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 1590, local_random_seed_address);

			*local_random_seed_address= local_random_seed;
		}
	}

	return;
}

/* ---------- private code */

static void decal_update(
	long decal_index)
{
	struct decal_datum *decal = DECAL_GET(decal_index);
	real elapsed = (game_time_get() - decal->creation_time) * (1.0f / TICKS_PER_SECOND);

	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 307, decal->definition_index!=NONE);

	decal_definition_get(decal->definition_index);

	decal->intensity = 255;

	if (!TEST_FLAG(decal->flags, _decal_permanent_bit))
	{
		if (decal->lifetime != 0.0f && elapsed >= decal->lifetime)
		{
			if (TEST_FLAG(decal->flags, _decal_locked_bit))
			{
				SET_FLAG(decal->flags, _decal_locked_bit, FALSE);

				if (--decal_globals->locked_count < 0 && !decal_locked_count_reported)
				{
					error(_error_silent,
						"### ERROR decals: locked count is invalid (#%d) -- tell Bernie!!",
						decal_globals->locked_count);
					decal_locked_count_reported = TRUE;
				}
			}

			rasterizer_decal_vertices_delete(decal_index);
		}
		else if (decal->lifetime > 0.0f && decal->decay_time > 0.0f)
		{
			real f = decal->lifetime - elapsed;

			if (f < decal->decay_time)
			{
				f /= decal->decay_time;

				match_assert("..\\bitmaps\\bitmaps_inlines.h", 322, f>=0.0f && f<=1.0f);

				decal->intensity = (byte)(f * 255.0f);
			}
		}
	}

	return;
}
static void decal_verify_neighbors(
	long decal_index,
	boolean layer_check)
{
	struct decal_datum *decal = DECAL_GET(decal_index);

	if (decal->previous_decal_index!=NONE)
	{
		short cluster_index= DECAL_GET(decal->previous_decal_index)->cluster_index;
		short layer= DECAL_GET(decal->previous_decal_index)->layer;

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 194, cluster_index==decal->cluster_index);
		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 195, !layer_check || layer==decal->layer);
	}

	if (decal->next_decal_index!=NONE)
	{
		short cluster_index= DECAL_GET(decal->next_decal_index)->cluster_index;
		short layer= DECAL_GET(decal->next_decal_index)->layer;

		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 203, cluster_index==decal->cluster_index);
		match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 204, !layer_check || layer==decal->layer);
	}

	return;
}

static void decal_reinsert(
	long decal_index,
	short cluster_index,
	short layer)
{
	long first_decal_index= decal_get_first_decal_index(cluster_index, layer);
	struct decal_datum *decal = DECAL_GET(decal_index);

	decal->previous_decal_index= NONE;
	decal->next_decal_index= first_decal_index;
	decal->cluster_index= cluster_index;
	decal->layer= layer;

	if (first_decal_index!=NONE)
	{
		DECAL_GET(first_decal_index)->previous_decal_index= decal_index;
	}

	decal_set_first_decal_index(cluster_index, layer, decal_index);

	return;
}



static void decal_set_first_decal_index(
	short cluster_index,
	short layer,
	long decal_index)
{
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 216,
		cluster_index>=0 && cluster_index<MAXIMUM_CLUSTERS_PER_STRUCTURE);
	match_assert("c:\\halo\\SOURCE\\effects\\decals.c", 217,
		layer>=0 && layer<NUMBER_OF_DECAL_LAYERS);

	decal_globals->first_decal_indices[layer][cluster_index]= decal_index;

	return;
}

/* ---------- end of file */
