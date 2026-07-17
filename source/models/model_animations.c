/*
MODEL_ANIMATIONS.C

symbols in this file:
0010FE40 0010:
	_animation_loop_frame_index (0000)
0010FE50 0010:
	_animation_second_key_frame_index (0000)
0010FE60 0010:
	_animation_sound_frame_index (0000)
0010FE70 0050:
	_code_0010fe70 (0000)
0010FEC0 00a0:
	_build_damage_animation_index (0000)
0010FF60 0080:
	_animation_get_x_offsets (0000)
0010FFE0 0080:
	_animation_set_frame_size (0000)
00110060 0060:
	_quaternion_decompress_8byte (0000)
001100C0 00c0:
	_quaternion_decompress_6byte (0000)
00110180 0020:
	_quaternion_decompress_6byte_renormalized (0000)
001101A0 0060:
	_quaternion_compress_8byte (0000)
00110200 0090:
	_quaternion_compress_6byte (0000)
00110290 0160:
	_animation_graph_node_matrices_from_orientations (0000)
001103F0 0110:
	_interpolate_node_orientations (0000)
00110500 0060:
	_animation_graph_get_animation_by_name (0000)
00110560 01d0:
	_code_00110560 (0000)
00110730 0040:
	_animation_frame_get_xy_translation (0000)
00110770 00b0:
	_animation_choose_random_permutation_internal (0000)
00110820 0360:
	_inverse_kinematics_adjust_matrices (0000)
00110B80 0310:
	_code_00110b80 (0000)
00110E90 0300:
	_code_00110e90 (0000)
00111190 02f0:
	_code_00111190 (0000)
00111480 0130:
	_animation_update_internal (0000)
001115B0 0300:
	_animation_get_node_orientations (0000)
001118B0 01e0:
	_replacement_animation_apply (0000)
00111A90 0210:
	_overlay_animation_apply (0000)
00111CA0 0240:
	_overlay_animation_apply_scaled (0000)
00111EE0 03c0:
	_overlay_animation_apply_continuous (0000)
001122A0 0400:
	_overlay_animation_apply_continuous_scaled (0000)
001126A0 0620:
	_aiming_screen_apply (0000)
00112CC0 0040:
	_animation_get_root_matrix (0000)
00112D00 00b0:
	_animation_get_root_velocity (0000)
0027F208 000a:
	??_C@_09KCBFNMAJ@animation?$AA@ (0000)
0027F214 0029:
	??_C@_0CJ@JGGOLPOM@c?3?2halo?2SOURCE?2models?2model_anim@ (0000)
0027F240 0035:
	??_C@_0DF@NDBCCBGA@damage_part?$DO?$DN0?5?$CG?$CG?5damage_part?$DMNU@ (0000)
0027F278 004e:
	??_C@_0EO@PDDEEPOK@damage_direction?$DO?$DN0?5?$CG?$CG?5damage_di@ (0000)
0027F2C8 003f:
	??_C@_0DP@CAICGJCA@damage_type?$DO?$DN0?5?$CG?$CG?5damage_type?$DMNU@ (0000)
0027F308 0004:
	__real@38000100 (0000)
0027F30C 0024:
	??_C@_0CE@FLNDNLBA@write_index?$DMMAXIMUM_NODES_PER_MO@ (0000)
0027F330 0018:
	??_C@_0BI@NCOKEGAP@frame_index?$DMframe_count?$AA@ (0000)
0027F348 000e:
	??_C@_0O@HJPIDGAL@frame_count?$DO0?$AA@ (0000)
0027F358 007a:
	??_C@_0HK@JCHJDAIG@target_frame_index?$DO?$DNkeyframe_fra@ (0000)
0027F3D4 0035:
	??_C@_0DF@BHOCMEDJ@keyframe_index?$DO?$DN0?5?$CG?$CG?5keyframe_in@ (0000)
0027F40C 001b:
	??_C@_0BL@NIECDBGN@?$CL?$CLinfinite_loop_killer?$DM200?$AA@ (0000)
0027F428 0033:
	??_C@_0DD@DKIDONPD@keyframe_index?$DO?$DN0?5?$CG?$CG?5keyframe_in@ (0000)
0027F460 0055:
	??_C@_0FF@IFOFHFLN@target_frame_index?$DO?$DN0?5?$CG?$CG?5target_@ (0000)
0027F4B8 001c:
	??_C@_0BM@EIBPKAOD@keyframe_frame_indices?$FL0?$FN?$DO0?$AA@ (0000)
0027F4D4 0017:
	??_C@_0BH@FMNFCNKN@keyframe_frame_indices?$AA@ (0000)
0027F4EC 0011:
	??_C@_0BB@INOOFBHG@keyframe_count?$DO1?$AA@ (0000)
0027F500 008e:
	??_C@_0IO@GIPANHA@?$CIanimation_update_kind_affects_g@ (0000)
0027F590 0004:
	__real@3f7ae148 (0000)
0027F594 0032:
	??_C@_0DC@FHNJEFLD@real_frame_index?$DM?5?$CIreal?$CJnext_key@ (0000)
0027F5C8 0032:
	??_C@_0DC@FGDAEANG@real_frame_index?$DO?$DN?$CIreal?$CJthis_key@ (0000)
0027F600 0043:
	??_C@_0ED@ICKGJKPC@keyframe_frame_indices?$FLkeyframe_@ (0000)
0027F648 0048:
	??_C@_0EI@OOKDDBPM@frame_index?$DO?$DN0?5?$CG?$CG?5frame_index?$DM?$DNk@ (0000)
0027F690 0012:
	??_C@_0BC@GPLHPFMP@keyframe_count?$DO?$DN0?$AA@ (0000)
0027F6A4 002e:
	??_C@_0CO@NEGCNKNH@real_frame_index?$DM?$CIreal?$CJanimation@ (0000)
0027F6D4 0017:
	??_C@_0BH@JENGDKGC@real_frame_index?$DO?$DN0?40f?$AA@ (0000)
0027F6F0 006f:
	??_C@_0GP@FNDBHMKG@compressed?5?$HM?$HM?5?$CIbyte?5?$CK?$CJdefault_da@ (0000)
0027F760 006b:
	??_C@_0GL@JOCEOLB@compressed?5?$HM?$HM?5?$CIbyte?5?$CK?$CJdata?9?$CIbyte@ (0000)
0027F7D0 006d:
	??_C@_0GN@PACKELMI@compressed?5?$HM?$HM?5?$CI?$CIbyte?5?$CK?$CJdata?9?$CIbyt@ (0000)
0027F840 0077:
	??_C@_0HH@GKOAHDOE@compressed?5?$HM?$HM?5?$CI?$CIbyte?5?$CK?$CJnext_data@ (0000)
0027F8B8 0048:
	??_C@_0EI@JELMFEDP@?$CD?$CD?$CD?5ERROR?5animation?5frame?5index?5@ (0000)
0027F900 0048:
	??_C@_0EI@GGOLLJMH@?$CD?$CD?$CD?5ERROR?5animation?5frame?5index?5@ (0000)
0027F948 0026:
	??_C@_0CG@FPLKLLFK@d0?$DN?$DN?$CFf?5direction?$CI?$CFf?$CJ?5yaw_delta?$CI?$CF@ (0000)
0027F970 0019:
	??_C@_0BJ@LEBMJMIB@animation?9?$DOframe_count?$DO1?$AA@ (0000)
0030A388 0001:
	_hs_model_animation_compression_enabled (0000)
0045662C 0024:
	_hs_model_animation_data_compressed_size (0000)
	_hs_model_animation_data_uncompressed_size (0004)
	_hs_model_animation_data_compression_savings_in_bytes (0008)
	_hs_model_animation_data_compression_savings_in_bytes_at_import (000c)
	_hs_model_animation_data_compression_savings_in_percent (0010)
	_hs_model_animation_bullshit (0014)
*/

/* ---------- headers */

#include "cseries.h"
#include "model_animation_definitions.h"

/* ---------- constants */

enum
{
	COMPRESSED_QUATERNION_COMPONENT_MAXIMUM = 32767,
	NUMBER_OF_ANIMATION_DAMAGE_TYPES = 4,
	NUMBER_OF_ANIMATION_DAMAGE_DIRECTIONS = 4,
	NUMBER_OF_DAMAGE_PARTS = 11,
};

/* ---------- macros */

/* ---------- structures */

struct compressed_quaternion_8byte
{
	short i;
	short j;
	short k;
	short w;
};

struct compressed_quaternion_6byte
{
	word words[3];
};

typedef char verify_compressed_quaternion_8byte_size[
	sizeof(struct compressed_quaternion_8byte) == 0x08 ? 1 : -1];
typedef char verify_compressed_quaternion_6byte_size[
	sizeof(struct compressed_quaternion_6byte) == 0x06 ? 1 : -1];

struct animation_frame_info_dx_dy
{
	real dx;
	real dy;
};

struct animation_frame_info_dx_dy_dyaw
{
	real dx;
	real dy;
	real dyaw;
};

struct animation_frame_info_dx_dy_dz_dyaw
{
	real dx;
	real dy;
	real dz;
	real dyaw;
};

typedef char verify_animation_frame_info_dx_dy_size[
	sizeof(struct animation_frame_info_dx_dy) == 0x08 ? 1 : -1];
typedef char verify_animation_frame_info_dx_dy_dyaw_size[
	sizeof(struct animation_frame_info_dx_dy_dyaw) == 0x0C ? 1 : -1];
typedef char verify_animation_frame_info_dx_dy_dz_dyaw_size[
	sizeof(struct animation_frame_info_dx_dy_dz_dyaw) == 0x10 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

short animation_loop_frame_index(
	struct animation const *animation)
{
	return animation->private_loop_frame_index;
}

short animation_second_key_frame_index(
	struct animation const *animation)
{
	return animation->private_second_key_frame_index;
}

short animation_sound_frame_index(
	struct animation const *animation)
{
	return animation->private_sound_frame_index;
}

short build_damage_animation_index(
	short damage_type,
	short damage_direction,
	short damage_part)
{
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		55,
		damage_type>=0 && damage_type<NUMBER_OF_ANIMATION_DAMAGE_TYPES);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		56,
		damage_direction>=0 && damage_direction<NUMBER_OF_ANIMATION_DAMAGE_DIRECTIONS);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		57,
		damage_part>=0 && damage_part<NUMBER_OF_DAMAGE_PARTS);

	return (damage_type * NUMBER_OF_ANIMATION_DAMAGE_DIRECTIONS + damage_direction) *
		NUMBER_OF_DAMAGE_PARTS + damage_part;
}

void animation_get_x_offsets(
	struct animation const *animation,
	real *key_frame_x_offset,
	real *total_x_offset)
{
	short frame_index;
	real x_offset = 0.f;
	real key_x_offset = 0.f;
	byte const *frame_info = animation->frame_info.address;

	for (frame_index = 0; frame_index < animation->frame_count; frame_index++)
	{
		switch (animation->frame_info_type)
		{
		case 1:
			x_offset += ((struct animation_frame_info_dx_dy const *)frame_info)->dx;
			frame_info += sizeof(struct animation_frame_info_dx_dy);
			break;

		case 2:
			x_offset += ((struct animation_frame_info_dx_dy_dyaw const *)frame_info)->dx;
			frame_info += sizeof(struct animation_frame_info_dx_dy_dyaw);
			break;

		case 3:
			x_offset += ((struct animation_frame_info_dx_dy_dz_dyaw const *)frame_info)->dx;
			frame_info += sizeof(struct animation_frame_info_dx_dy_dz_dyaw);
			break;
		}

		if (frame_index == animation->private_key_frame_index)
		{
			key_x_offset = x_offset;
		}
	}

	if (total_x_offset)
	{
		*total_x_offset = x_offset;
	}
	if (key_frame_x_offset)
	{
		*key_frame_x_offset = key_x_offset;
	}

	return;
}

short animation_graph_get_animation_by_name(
	long animation_graph_index,
	char const *animation_name)
{
	struct animation_graph const *animation_graph = animation_graph_definition_get(animation_graph_index);
	short animation_index;

	for (animation_index = 0; animation_index < animation_graph->animations.count; animation_index++)
	{
		struct animation const *animation = TAG_BLOCK_GET_ELEMENT(
			&animation_graph->animations,
			animation_index,
			struct animation);

		if (!_stricmp(animation_name, animation->name))
		{
			return animation_index;
		}
	}

	return NONE;
}

void animation_frame_get_xy_translation(
	struct animation const *animation,
	short frame_index,
	real_vector2d *translation)
{
	if (animation->frame_info_type == 1)
	{
		*translation = *(real_vector2d const *)animation_get_frame_info(
			animation,
			frame_index,
			sizeof(struct animation_frame_info_dx_dy));
	}
	else
	{
		translation->i = 0.f;
		translation->j = 0.f;
	}

	return;
}

void animation_get_root_matrix(
	struct model const *model,
	struct animation const *animation,
	short frame_index,
	real_matrix4x3 *root_matrix)
{
	struct real_orientation node_orientations[MAXIMUM_NODES_PER_ANIMATION];

	animation_get_node_orientations(model, animation, frame_index, node_orientations);
	matrix4x3_from_point_and_quaternion(
		root_matrix,
		&node_orientations[0].translation,
		&node_orientations[0].rotation);

	return;
}

void animation_get_root_velocity(
	struct model const *model,
	struct animation const *animation,
	short frame_index,
	real_vector3d *root_velocity)
{
	struct real_orientation node_orientations[MAXIMUM_NODES_PER_ANIMATION];
	struct real_orientation previous_node_orientations[MAXIMUM_NODES_PER_ANIMATION];

	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		221,
		animation->frame_count>1);

	if (frame_index == 0)
	{
		frame_index = 1;
	}

	animation_get_node_orientations(model, animation, frame_index, node_orientations);
	animation_get_node_orientations(model, animation, frame_index - 1, previous_node_orientations);

	root_velocity->i = node_orientations[0].translation.x - previous_node_orientations[0].translation.x;
	root_velocity->j = node_orientations[0].translation.y - previous_node_orientations[0].translation.y;
	root_velocity->k = node_orientations[0].translation.z - previous_node_orientations[0].translation.z;

	return;
}

void quaternion_decompress_8byte(
	struct compressed_quaternion_8byte const *compressed,
	real_quaternion *quaternion)
{
	real const scale = 1.f / COMPRESSED_QUATERNION_COMPONENT_MAXIMUM;

	quaternion->v.i = compressed->i * scale;
	quaternion->v.j = compressed->j * scale;
	quaternion->v.k = compressed->k * scale;
	quaternion->w = compressed->w * scale;

	return;
}

void quaternion_decompress_6byte(
	struct compressed_quaternion_6byte const *compressed,
	real_quaternion *quaternion)
{
	word word0 = compressed->words[0];
	word word1 = compressed->words[1];
	word word2 = compressed->words[2];
	short i = (short)((word0 >> 12) | (word0 & 0xFFF0));
	short j = (short)(((word1 >> 4) & 0x0FF0) | (word0 & 0x000F) | (word0 << 12));
	short k = (short)(((((word2 >> 4) & 0x0F00) | (word1 & 0x00F0)) >> 4) | (word1 << 8));
	short w = (short)(((word2 >> 8) & 0x000F) | (word2 << 4));
	real const scale = 1.f / COMPRESSED_QUATERNION_COMPONENT_MAXIMUM;

	quaternion->v.i = i * scale;
	quaternion->v.j = j * scale;
	quaternion->v.k = k * scale;
	quaternion->w = w * scale;

	return;
}

void quaternion_decompress_6byte_renormalized(
	void const *compressed,
	real_quaternion *quaternion)
{
	quaternion_decompress_6byte(compressed, quaternion);
	quaternion_normalize(quaternion);

	return;
}

void quaternion_compress_8byte(
	real_quaternion const *quaternion,
	struct compressed_quaternion_8byte *compressed)
{
	compressed->i = (short)(quaternion->v.i * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);
	compressed->j = (short)(quaternion->v.j * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);
	compressed->k = (short)(quaternion->v.k * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);
	compressed->w = (short)(quaternion->w * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);

	return;
}

void quaternion_compress_6byte(
	real_quaternion const *quaternion,
	struct compressed_quaternion_6byte *compressed)
{
	long j = (long)(quaternion->v.j * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);
	long k = (long)(quaternion->v.k * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);
	long w = (long)(quaternion->w * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);
	long i = (long)(quaternion->v.i * COMPRESSED_QUATERNION_COMPONENT_MAXIMUM);

	compressed->words[0] = (word)((i & 0xFFF0) | ((word)j >> 12));
	compressed->words[1] = (word)(((j & 0xFFF0) << 4) | (((word)k >> 8) & 0x00FF));
	compressed->words[2] = (word)(((k & 0x00F0) << 8) | ((word)w >> 4));

	return;
}

/* ---------- private code */
