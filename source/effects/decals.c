/*
DECALS.C

symbols in this file:
00086FE0 0130:
	_code_00086fe0 (0000)
00087110 0080:
	_code_00087110 (0000)
00087190 0180:
	_code_00087190 (0000)
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
	_code_00087d20 (0000)
00087EB0 0070:
	_code_00087eb0 (0000)
00087F20 02c0:
	_code_00087f20 (0000)
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
	_code_00088970 (0000)
00088C10 06f0:
	_code_00088c10 (0000)
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
00435CB0 78d2:
	_bss_00435cb0 (0000)
*/

/* ---------- headers */

#define projection_sign_from_vector3d projection_sign_from_vector3d_inline
#define plane3d_negate plane3d_negate_inline
#define plane3d_distance_to_point plane3d_distance_to_point_inline
#include "effects/decals.h"
#include "cseries/cseries.h"
#include "math/real_math.h"
#undef projection_sign_from_vector3d
#undef plane3d_negate
#undef plane3d_distance_to_point

#include "cseries/errors.h"
#include "effects/decal_definitions.h"
#include "game/game.h"
#include "memory/data.h"
#include "rasterizer/rasterizer.h"
#include "structures/structures.h"

/* ---------- constants */

enum
{
	NUMBER_OF_DECAL_LAYERS = 5,
};

enum
{
	_decal_locked_bit,
	_decal_permanent_bit,
};

/* ---------- macros */

#define DECAL_GET(index) ((struct decal_datum *)datum_get(global_decal_data, (index)))

/* ---------- structures */

struct collision_result;
struct decal_editor_geometry;
union real_vector3d;

struct decal_datum
{
	struct datum_header header;
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
	byte pad;
	short quad_count;
	long definition_index;
	long prev_decal_index;
	long next_decal_index;
};

typedef char verify_decal_datum_size[
	sizeof(struct decal_datum) == 0x38 ? 1 : -1];
typedef char verify_decal_datum_flags_offset[
	offsetof(struct decal_datum, flags) == 0x2 ? 1 : -1];
typedef char verify_decal_datum_creation_time_offset[
	offsetof(struct decal_datum, creation_time) == 0x14 ? 1 : -1];
typedef char verify_decal_datum_intensity_offset[
	offsetof(struct decal_datum, intensity) == 0x28 ? 1 : -1];
typedef char verify_decal_datum_definition_index_offset[
	offsetof(struct decal_datum, definition_index) == 0x2C ? 1 : -1];

struct decal_globals
{
	long first_decal_indices[NUMBER_OF_DECAL_LAYERS][MAXIMUM_CLUSTERS_PER_STRUCTURE];
	long first_disconnected_decal_index;
	long locked_count;
	long permanent_count;
};

typedef char verify_decal_globals_size[
	sizeof(struct decal_globals) == 0x280C ? 1 : -1];
typedef char verify_decal_globals_locked_count_offset[
	offsetof(struct decal_globals, locked_count) == 0x2804 ? 1 : -1];

/* ---------- prototypes */

long decal_get_first_decal_index(
	short cluster_index,
	short layer);
static void decal_update(
	long decal_index);

/* ---------- globals */

extern struct data_array *global_decal_data;
static struct decal_globals *decal_globals;
static boolean decal_locked_count_reported;

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

void decal_new_from_media_collision(
	long decal_definition_index,
	struct collision_result const *collision,
	union real_vector3d const *velocity,
	float radius_modifier,
	boolean permanent,
	short forced_sequence_index,
	struct decal_editor_geometry *editor_geometry)
{
	return;
}

boolean projection_sign_from_vector3d(
	real_vector3d const *n,
	short projection)
{
	match_assert("..\\math\\real_math.h", 848,
		projection>=_x && projection<=_z);

	return n->n[projection] > 0.0f;
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
