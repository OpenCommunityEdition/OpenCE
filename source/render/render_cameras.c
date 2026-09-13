/*
RENDER_CAMERAS.C

symbols in this file:
00174FC0 00a0:
	_code_00174fc0 (0000)
00175060 0020:
	_render_camera_new (0000)
00175080 0120:
	_render_camera_hack_frustum_z (0000)
001751A0 0120:
	_render_camera_build_frustum_bounds (0000)
001752C0 0060:
	_render_frustum_sphere_diameter_in_pixels (0000)
00175320 04b0:
	_render_frustum_cube_view_fraction (0000)
001757D0 00d0:
	_render_frustum_get_projection_bounds (0000)
001758A0 01e0:
	_render_camera_view_to_screen (0000)
00175A80 0100:
	_render_camera_screen_to_view (0000)
00175B80 0130:
	_render_camera_screen_to_world (0000)
00175CB0 0020:
	_render_camera_get_adjusted_field_of_view_tangent (0000)
00175CD0 0160:
	_render_camera_build_clipped_frustum_bounds (0000)
00175E30 00b0:
	_render_camera_triangle_frontfacing (0000)
00175EE0 0100:
	_render_frustum_build_point_flags (0000)
00175FE0 0060:
	_render_frustum_triangle_visible (0000)
00176040 02d0:
	_render_frustum_cube_visible (0000)
00176310 01b0:
	_render_frustum_sphere_visible (0000)
001764C0 00d0:
	_render_camera_world_to_screen (0000)
00176590 01b0:
	_render_camera_debug_frustum (0000)
00176740 0360:
	_render_camera_mirror (0000)
00176AA0 0d30:
	_render_camera_build_frustum (0000)
0029F768 0042:
	??_C@_0EC@HLNCAGFG@?$CD?$CD?$CD?5ERROR?5cameras?3?5frustum?9integ@ (0000)
0029F7AC 0035:
	??_C@_0DF@KOAPDDIF@id?$DO?$DN0?5?$CG?$CG?5id?$DMMAXIMUM_RENDER_CAMER@ (0000)
0029F7E4 0027:
	??_C@_0CH@JIJIIAGP@c?3?2halo?2SOURCE?2render?2render_cam@ (0000)
0029F80C 0025:
	??_C@_0CF@NAAMLNBF@frustum?5?$CG?$CG?5frustum?9?$DOprojection_v@ (0000)
0029F834 000f:
	??_C@_0P@IMNLCIED@frustum_bounds?$AA@ (0000)
0029F844 0027:
	??_C@_0CH@MLEDHCGL@frustum?9?$DOprojection_matrix?$FL3?$FN?$FL3?$FN@ (0000)
0029F86C 0028:
	??_C@_0CI@LBJFINLO@frustum?9?$DOprojection_matrix?$FL2?$FN?$FL3?$FN@ (0000)
0029F894 0027:
	??_C@_0CH@CHHIOMPE@frustum?9?$DOprojection_matrix?$FL1?$FN?$FL3?$FN@ (0000)
0029F8BC 0027:
	??_C@_0CH@LMNNKAJL@frustum?9?$DOprojection_matrix?$FL0?$FN?$FL3?$FN@ (0000)
0029F8E4 0027:
	??_C@_0CH@OFLFFKON@frustum?9?$DOprojection_matrix?$FL3?$FN?$FL1?$FN@ (0000)
0029F90C 0027:
	??_C@_0CH@JCCLIIBN@frustum?9?$DOprojection_matrix?$FL0?$FN?$FL1?$FN@ (0000)
0029F934 0027:
	??_C@_0CH@PCMOEOKO@frustum?9?$DOprojection_matrix?$FL3?$FN?$FL0?$FN@ (0000)
0029F95C 0027:
	??_C@_0CH@BOPFNADB@frustum?9?$DOprojection_matrix?$FL1?$FN?$FL0?$FN@ (0000)
0029F984 001a:
	??_C@_0BK@KOEOFHLL@frustum?9?$DOprojection_valid?$AA@ (0000)
0029F9A0 0017:
	??_C@_0BH@OICCFDNN@bounds?9?$DOz0?$DM?$DNbounds?9?$DOz1?$AA@ (0000)
0029F9B8 0017:
	??_C@_0BH@PLBJIHPN@bounds?9?$DOy0?$DM?$DNbounds?9?$DOy1?$AA@ (0000)
0029F9D0 0017:
	??_C@_0BH@EDCADGCC@bounds?9?$DOx0?$DM?$DNbounds?9?$DOx1?$AA@ (0000)
0029F9E8 0008:
	??_C@_07HEAHMGFP@frustum?$AA@ (0000)
0029F9F0 000d:
	??_C@_0N@GAIFNDPJ@screen_point?$AA@ (0000)
0029FA00 000b:
	??_C@_0L@JFJFDDCJ@view_point?$AA@ (0000)
0029FA0C 000c:
	??_C@_0M@PIACODLM@view_vector?$AA@ (0000)
0029FA18 000d:
	??_C@_0N@DOBKNIBB@world_vector?$AA@ (0000)
0029FA28 000c:
	??_C@_0M@IBHHCMBD@world_point?$AA@ (0000)
0029FA34 0004:
	__real@3f59999a (0000)
0029FA38 0021:
	??_C@_0CB@HKCALOIB@valid_real_vector3d?$CI?$CGresult?9?$DOup?$CJ@ (0000)
0029FA5C 0026:
	??_C@_0CG@PFGJGAON@valid_real_vector3d?$CI?$CGresult?9?$DOfor@ (0000)
0029FA84 0024:
	??_C@_0CE@KJKFBGON@valid_real_plane3d?$CI?$CGadjusted_pla@ (0000)
0029FAA8 0004:
	__real@3bc00000 (0000)
0029FAB0 0008:
	__real@3f899999a0000000 (0000)
0029FAB8 002e:
	??_C@_0CO@BEMDFKEN@valid_real_matrix4x3?$CI?$CGfrustum?9?$DOv@ (0000)
0029FAE8 002e:
	??_C@_0CO@FKHJEIFF@valid_real_matrix4x3?$CI?$CGfrustum?9?$DOw@ (0000)
0029FB18 0036:
	??_C@_0DG@PFOKCMGI@camera?9?$DOviewport_bounds?4y0?$DMcamer@ (0000)
0029FB50 0036:
	??_C@_0DG@MIEIKFFH@camera?9?$DOviewport_bounds?4x0?$DMcamer@ (0000)
0029FB88 001d:
	??_C@_0BN@MDHKMJIG@camera?9?$DOz_far?$DOcamera?9?$DOz_near?$AA@ (0000)
0029FBA8 0015:
	??_C@_0BF@GIODIKMM@camera?9?$DOz_near?$DO?$DN0?40f?$AA@ (0000)
0029FBC0 0030:
	??_C@_0DA@DCMPADCP@?$CD?$CD?$CD?5FATAL?5ERROR?3?5field?5of?5view?5s@ (0000)
0029FBF0 0033:
	??_C@_0DD@EPCFLHOD@camera?9?$DOvertical_field_of_view?$DM_@ (0000)
0029FC24 0004:
	__real@40490e38 (0000)
004B8B28 0113:
	_bss_004b8b28 (0000)
	_debug_no_frustum_clip (0111)
	_render_camera_debug_this_fucking_frustum (0112)
*/

/* ---------- headers */

#include "cseries.h"
#include "render/render_cameras.h"
#include "render/render_camera_projection.h"
#include "render/render_cameras_internal.h"

/* ---------- constants */

enum render_frustum_point_flag_bits
{
	_render_frustum_point_flags_left_bit,
	_render_frustum_point_flags_right_bit,
	_render_frustum_point_flags_top_bit,
	_render_frustum_point_flags_bottom_bit,
	_render_frustum_point_flags_near_bit,
	_render_frustum_point_flags_far_bit,
	NUMBER_OF_RENDER_FRUSTUM_POINT_FLAGS,

	RENDER_FRUSTUM_POINT_FLAGS_PLANE_MASK = FLAG(NUMBER_OF_RENDER_FRUSTUM_POINT_FLAGS) - 1,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- globals */

boolean debug_no_frustum_clip;

/* ---------- public code */

void render_camera_new(
	struct render_camera *camera)
{
	csmemset(camera, 0, sizeof(*camera));
	return;
}

void render_camera_screen_to_view(
	const struct render_camera *camera,
	const struct render_frustum *frustum,
	const real_point2d *screen_point,
	real_vector3d *view_vector)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1018,
		camera);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1019,
		frustum);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1020,
		screen_point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1021,
		view_vector);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1028,
		frustum->projection_valid);

	view_vector->n[0] =
		(screen_point->n[0] - frustum->projection_matrix[2][0]) /
		frustum->projection_matrix[0][0];
	view_vector->n[1] =
		-((frustum->projection_matrix[2][1] + screen_point->n[1]) /
		frustum->projection_matrix[1][1]);
	view_vector->n[2] = -1.0f;
	return;
}

real render_camera_get_adjusted_field_of_view_tangent(
	real field_of_view)
{
	return tangent(field_of_view * 0.5f) * 0.85f;
}

boolean render_camera_world_to_screen(
	const struct render_camera *camera,
	const struct render_frustum *frustum,
	const real_point3d *world_point,
	real_point2d *screen_point)
{
	real_point3d view_point;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		961,
		camera);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		962,
		frustum);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		963,
		world_point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		964,
		screen_point);

	matrix4x3_transform_point(
		&frustum->world_to_view,
		world_point,
		&view_point);
	return render_camera_view_to_screen(
		camera,
		frustum,
		&view_point,
		screen_point);
}

real render_frustum_sphere_diameter_in_pixels(
	const struct render_frustum *frustum,
	const real_point3d *point,
	real radius)
{
	real depth =
		frustum->world_to_view.up.k * point->z +
		frustum->world_to_view.left.k * point->y +
		frustum->world_to_view.forward.k * point->x +
		frustum->world_to_view.position.z;
	real absolute_depth = ABS(depth);
	real clamped_depth = MAX(absolute_depth, 0.1f);

	return (frustum->projection_world_to_screen.j / clamped_depth) * radius * 2.0f;
}

real render_frustum_cube_view_fraction(
	struct render_frustum const *frustum,
	real_rectangle3d const *bounds)
{
	real fraction;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		876,
		frustum);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		877,
		bounds);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		878,
		bounds->x0<=bounds->x1);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		879,
		bounds->y0<=bounds->y1);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		880,
		bounds->z0<=bounds->z1);

	if (bounds->z0 >= 0.0f)
	{
		fraction = 0.0f;
	}
	else if (bounds->z1 >= 0.0f)
	{
		fraction = 1.0f;
	}
	else
	{
		real inverse_z0 = 1.0f / bounds->z0;
		real projection_x = frustum->projection_matrix[0][0];
		real projection_offset_x = frustum->projection_matrix[2][0];
		real projection_y = frustum->projection_matrix[1][1];
		real projection_offset_y = frustum->projection_matrix[2][1];
		real inverse_z1 = 1.0f / bounds->z1;
		real positive_inverse_z0;
		real positive_inverse_z1;
		real left;
		real bottom;
		real right;
		real top;

		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			900,
			frustum->projection_valid);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			903,
			frustum->projection_matrix[1][0]==0.0f);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			904,
			frustum->projection_matrix[3][0]==0.0f);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			905,
			frustum->projection_matrix[0][1]==0.0f);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			906,
			frustum->projection_matrix[3][1]==0.0f);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			907,
			frustum->projection_matrix[0][3]==0.0f);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			908,
			frustum->projection_matrix[1][3]==0.0f);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			909,
			frustum->projection_matrix[2][3]==-1.0f);
		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			910,
			frustum->projection_matrix[3][3]==0.0f);

		positive_inverse_z0 = -inverse_z0;
		positive_inverse_z1 = -inverse_z1;
		left = MAX(
			MIN(
				(bounds->z0 * projection_offset_x + bounds->x0 * projection_x) *
					positive_inverse_z0,
				(bounds->z1 * projection_offset_x + bounds->x0 * projection_x) *
					positive_inverse_z1),
			-1.0f);
		bottom = MAX(
			MIN(
				(bounds->z0 * projection_offset_y + bounds->y0 * projection_y) *
					positive_inverse_z0,
				(bounds->z1 * projection_offset_y + bounds->y0 * projection_y) *
					positive_inverse_z1),
			-1.0f);
		right = MIN(
			MAX(
				(bounds->z0 * projection_offset_x + bounds->x1 * projection_x) *
					positive_inverse_z0,
				(bounds->z1 * projection_offset_x + bounds->x1 * projection_x) *
					positive_inverse_z1),
			1.0f);
		top = MIN(
			MAX(
				(bounds->z0 * projection_offset_y + bounds->y1 * projection_y) *
					positive_inverse_z0,
				(bounds->z1 * projection_offset_y + bounds->y1 * projection_y) *
					positive_inverse_z1),
			1.0f);
		fraction = (right - left) * (top - bottom) * 0.25f;
		if (!(fraction > 0.0f))
		{
			fraction = 0.0f;
		}
	}

	return fraction;
}

word render_frustum_build_point_flags(
	const struct render_frustum *frustum,
	const real_point3d *point)
{
	word flags = plane3d_distance_to_point(&frustum->world_planes[0], point) > 0.0f ?
		FLAG(_render_frustum_point_flags_left_bit) : 0;

	flags |= plane3d_distance_to_point(&frustum->world_planes[1], point) > 0.0f ?
		FLAG(_render_frustum_point_flags_right_bit) : 0;
	flags |= plane3d_distance_to_point(&frustum->world_planes[2], point) > 0.0f ?
		FLAG(_render_frustum_point_flags_bottom_bit) : 0;
	flags |= plane3d_distance_to_point(&frustum->world_planes[3], point) > 0.0f ?
		FLAG(_render_frustum_point_flags_top_bit) : 0;

	return flags;
}

boolean render_frustum_triangle_visible(
	struct render_frustum const *frustum,
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2)
{
	word flags = RENDER_FRUSTUM_POINT_FLAGS_PLANE_MASK;
	word point_flags;

	point_flags = render_frustum_build_point_flags(frustum, point0);
	if (!point_flags)
		return TRUE;
	flags &= point_flags;
	point_flags = render_frustum_build_point_flags(frustum, point1);
	if (!point_flags)
		return TRUE;
	flags &= point_flags;
	point_flags = render_frustum_build_point_flags(frustum, point2);
	if (!point_flags)
		return TRUE;
	flags &= point_flags;

	return flags == 0;
}

boolean render_camera_triangle_frontfacing(
	struct render_camera const *camera,
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2)
{
	real_vector3d camera_to_point;
	real_vector3d edge0;
	real_vector3d edge1;
	real_vector3d normal;

	vector_from_points3d(&camera->position, point0, &camera_to_point);
	vector_from_points3d(point0, point1, &edge0);
	vector_from_points3d(point1, point2, &edge1);
	cross_product3d(&edge0, &edge1, &normal);

	return dot_product3d(&normal, &camera_to_point) > -_real_epsilon;
}

void render_frustum_get_projection_bounds(
	struct render_frustum const *frustum,
	real_rectangle2d *bounds)
{
	real x;
	real y;
	real inverse_x_scale;
	real inverse_y_scale;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		935,
		frustum && frustum->projection_valid);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		936,
		bounds);

	x = -frustum->projection_matrix[2][0];
	y = -frustum->projection_matrix[2][1];
	inverse_x_scale = -1.0f / frustum->projection_matrix[0][0];
	inverse_y_scale = -1.0f / frustum->projection_matrix[1][1];

	bounds->x0 = (x - 1.0f) * inverse_x_scale;
	bounds->x1 = (x + 1.0f) * inverse_x_scale;
	bounds->y0 = (y - 1.0f) * inverse_y_scale;
	bounds->y1 = (y + 1.0f) * inverse_y_scale;
	return;
}

void render_camera_screen_to_world(
	const struct render_camera *camera,
	const struct render_frustum *frustum,
	const real_point2d *screen_point,
	real_point3d *world_point,
	real_vector3d *world_vector)
{
	real_vector3d view_vector;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1052,
		camera);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1053,
		frustum);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1054,
		screen_point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1055,
		world_point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1056,
		world_vector);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		1058,
		frustum->projection_valid);

	render_camera_screen_to_view(camera, frustum, screen_point, &view_vector);
	*world_point = camera->position;
	matrix4x3_transform_vector(&frustum->view_to_world, &view_vector, world_vector);

	return;
}

void render_camera_build_frustum_bounds(
	struct render_camera const *camera,
	real_rectangle2d *frustum_bounds)
{
	real aspect_ratio;
	real inverse_window_height;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		305,
		camera);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		306,
		frustum_bounds);

	aspect_ratio = (real)(camera->viewport_bounds.y1 - camera->viewport_bounds.y0) /
		(camera->viewport_bounds.x1 - camera->viewport_bounds.x0);
	inverse_window_height = 1.0f /
		(camera->window_bounds.y1 - camera->window_bounds.y0);

	frustum_bounds->x0 =
		(2 * camera->viewport_bounds.x0 -
		camera->window_bounds.x0 - camera->window_bounds.x1) *
		inverse_window_height;
	frustum_bounds->x1 =
		(2 * camera->viewport_bounds.x1 -
		camera->window_bounds.x0 - camera->window_bounds.x1) *
		inverse_window_height;
	frustum_bounds->y0 =
		(2 * camera->viewport_bounds.y0 -
		camera->window_bounds.y0 - camera->window_bounds.y1) *
		inverse_window_height;
	frustum_bounds->y1 =
		(2 * camera->viewport_bounds.y1 -
		camera->window_bounds.y0 - camera->window_bounds.y1) *
		inverse_window_height;

	frustum_bounds->x0 *= aspect_ratio;
	frustum_bounds->x1 *= aspect_ratio;

	{
		real temporary_y0 = frustum_bounds->y0;

		frustum_bounds->y0 = -frustum_bounds->y1;
		frustum_bounds->y1 = -temporary_y0;
	}
	return;
}

boolean render_camera_build_clipped_frustum_bounds(
	struct render_camera const *camera,
	real_rectangle2d const *clip,
	real_rectangle2d *frustum_bounds)
{
	boolean use_full_bounds = TRUE;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		342,
		camera);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		343,
		clip);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		344,
		frustum_bounds);

	if (!debug_no_frustum_clip &&
		clip->x0 < clip->x1 &&
		clip->y0 < clip->y1)
	{
		long viewport_height = camera->viewport_bounds.y1 - camera->viewport_bounds.y0;
		long viewport_width = camera->viewport_bounds.x1 - camera->viewport_bounds.x0;
		real aspect_ratio = (real)viewport_height / (real)viewport_width;
		real inverse_tangent = 1.0f /
			tangent(camera->vertical_field_of_view * 0.5f);
		real horizontal_scale = inverse_tangent * aspect_ratio;

		frustum_bounds->x0 = clip->x0 * horizontal_scale;
		frustum_bounds->x1 = clip->x1 * horizontal_scale;
		frustum_bounds->y0 = inverse_tangent * clip->y0;
		frustum_bounds->y1 = clip->y1 * inverse_tangent;
		use_full_bounds =
			frustum_bounds->x0 >= frustum_bounds->x1 ||
			frustum_bounds->y0 >= frustum_bounds->y1;
	}

	if (use_full_bounds)
	{
		frustum_bounds->y1 = 1.0f;
		frustum_bounds->x1 = 1.0f;
		frustum_bounds->y0 = -1.0f;
		frustum_bounds->x0 = -1.0f;
	}

	return !use_full_bounds;
}

boolean render_camera_view_to_screen(
	struct render_camera const *camera,
	struct render_frustum const *frustum,
	real_point3d const *view_point,
	real_point2d *screen_point)
{
	boolean result = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		978,
		camera);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		979,
		frustum);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		980,
		view_point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_cameras.c",
		981,
		screen_point);

	if (view_point->z < 0.0f)
	{
		real inverse_depth = -1.0f / view_point->z;

		match_assert(
			"c:\\halo\\SOURCE\\render\\render_cameras.c",
			988,
			frustum->projection_valid);

		screen_point->x =
			(frustum->projection_matrix[0][0] * view_point->x +
			frustum->projection_matrix[2][0] * view_point->z) * inverse_depth;
		screen_point->y =
			-(frustum->projection_matrix[1][1] * view_point->y +
			frustum->projection_matrix[2][1] * view_point->z) * inverse_depth;
		if (screen_point->x >= -1.0f && screen_point->x <= 1.0f &&
			screen_point->y >= -1.0f && screen_point->y <= 1.0f)
		{
			real viewport_height = (real)(
				camera->viewport_bounds.y1 - camera->viewport_bounds.y0);
			long viewport_x0 = camera->viewport_bounds.x0;
			real viewport_width = (real)(
				camera->viewport_bounds.x1 - viewport_x0);

			result = TRUE;
			screen_point->x =
				(real)viewport_width * ((screen_point->x + 1.0f) * 0.5f) +
				(real)viewport_x0;
			{
				long viewport_y0 = camera->viewport_bounds.y0;

				screen_point->y =
					((screen_point->y + 1.0f) * 0.5f) *
					(real)viewport_height +
					(real)viewport_y0;
			}
		}
	}

	return result;
}

short render_frustum_sphere_visible(
	struct render_frustum const *frustum,
	real_point3d const *point,
	real radius)
{
	real distance0;
	real distance1;
	real distance2;
	real distance3;
	real distance5;
	real negative_radius;

	if (frustum->world_bounds.x1 < point->x - radius ||
		frustum->world_bounds.y1 < point->y - radius ||
		frustum->world_bounds.z1 < point->z - radius ||
		frustum->world_bounds.x0 > point->x + radius ||
		frustum->world_bounds.y0 > point->y + radius ||
		frustum->world_bounds.z0 > point->z + radius)
	{
		return 0;
	}

	distance0 = plane3d_distance_to_point(&frustum->world_planes[0], point);
	if (distance0 > radius)
		return 0;
	distance1 = plane3d_distance_to_point(&frustum->world_planes[1], point);
	if (distance1 > radius)
		return 0;
	distance2 = plane3d_distance_to_point(&frustum->world_planes[2], point);
	if (distance2 > radius)
		return 0;
	distance3 = plane3d_distance_to_point(&frustum->world_planes[3], point);
	if (distance3 > radius)
		return 0;
	if (plane3d_distance_to_point(&frustum->world_planes[4], point) > radius)
		return 0;
	distance5 = plane3d_distance_to_point(&frustum->world_planes[5], point);
	if (distance5 > radius)
	{
		return 0;
	}
	else
	{
		short result;

		negative_radius = -radius;
		if (distance0 < negative_radius &&
			distance1 < negative_radius &&
			distance2 < negative_radius &&
			distance3 < negative_radius &&
			distance5 < negative_radius)
		{
			result = 2;
		}
		else
		{
			result = 1;
		}

		return result;
	}
}

/* ---------- private code */
