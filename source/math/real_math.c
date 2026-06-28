/*
REAL_MATH.C

symbols in this file:
000FADE0 0010:
	_real_math_initialize (0000)
000FADF0 0010:
	_real_math_dispose (0000)
000FAE00 0020:
	_real_math_reset_precision (0000)
000FAE20 0020:
	_perpendicular2d (0000)
000FAE40 0090:
	_perpendicular3d (0000)
000FAED0 0030:
	_perpendicular4d (0000)
000FAF00 00a0:
	_rotate_vector_about_axis (0000)
000FAFA0 0050:
	_vectors_interpolate (0000)
000FAFF0 0050:
	_points_interpolate (0000)
000FB040 0020:
	_scalars_interpolate (0000)
000FB060 0060:
	_scalars_interpolate_and_clamp_0_to_1 (0000)
000FB0C0 0070:
	_component_vectors_from_normal3d (0000)
000FB130 00b0:
	_component_vectors_from_direction3d (0000)
000FB1E0 00d0:
	_quaternions_multiply (0000)
000FB2B0 0090:
	_quaternions_interpolate (0000)
000FB340 00a0:
	_quaternion_transform_point (0000)
000FB3E0 00b0:
	_vectors3d_from_euler_angles3d (0000)
000FB490 0100:
	_fast_vector_intersects_sphere (0000)
000FB590 0050:
	_point_in_rectangle2d (0000)
000FB5E0 0060:
	_point_in_rectangle3d (0000)
000FB640 01d0:
	_vector_intersects_rectangle2d (0000)
000FB810 0290:
	_vector_intersects_rectangle3d (0000)
000FBAA0 0010:
	_pill_intersects_rectangle3d (0000)
000FBAB0 0050:
	_fast_normalize2d (0000)
000FBB00 0060:
	_fast_normalize3d (0000)
000FBB60 0050:
	_cross_product_magnitude3d (0000)
000FBBB0 0030:
	_dequantize_byte_to_real (0000)
000FBBE0 0080:
	_signed_angle_between_vectors2d (0000)
000FBC60 00d0:
	_angle_between_vectors2d (0000)
000FBD30 00f0:
	_angle_between_vectors3d (0000)
000FBE20 0090:
	_angle_between_normals3d (0000)
000FBEB0 0070:
	_yaw_vectors (0000)
000FBF20 0080:
	_pitch_vectors (0000)
000FBFA0 0050:
	_fast_normals_interpolate (0000)
000FBFF0 0110:
	_normals_interpolate (0000)
000FC100 0040:
	_reflect_vector3d (0000)
000FC140 0110:
	_refract_vector3d (0000)
000FC250 0080:
	_quaternion_normalize (0000)
000FC2D0 0040:
	_quaternion_from_angle_and_vector (0000)
000FC310 0070:
	_quaternion_to_angle_and_vector (0000)
000FC380 0030:
	_quaternions_interpolate_and_normalize (0000)
000FC3B0 0070:
	_orientations_interpolate (0000)
000FC420 0040:
	_euler_angles2d_from_vector3d (0000)
000FC460 0030:
	_vector3d_from_euler_angles2d (0000)
000FC490 0020:
	_vector3d_from_angle (0000)
000FC4B0 00b0:
	_point_in_pill2d (0000)
000FC560 00d0:
	_point_to_line_distance_squared3d (0000)
000FC630 0570:
	_vector_to_line_distance_squared3d (0000)
000FCBA0 0140:
	_sphere_test_vector3d (0000)
000FCCE0 01c0:
	_pill_test_vector3d (0000)
000FCEA0 00f0:
	_fast_vector_intersection_with_sphere (0000)
000FCF90 00c0:
	_point_in_triangle2d (0000)
000FD050 01b0:
	_point_in_triangle3d (0000)
000FD200 00b0:
	_point_in_cone2d (0000)
000FD2B0 00c0:
	_point_in_cone3d (0000)
000FD370 00a0:
	_point_in_sector2d (0000)
000FD410 00c0:
	_point_in_sector3d (0000)
000FD4D0 0390:
	_vector_intersects_pill2d (0000)
000FD860 0490:
	_vector_intersects_pill3d (0000)
000FDCF0 0220:
	_vector_intersects_triangle2d (0000)
000FDF10 01b0:
	_vector_intersects_triangle3d (0000)
000FE0C0 0090:
	_circle_intersects_rectangle2d (0000)
000FE150 00c0:
	_sphere_intersects_rectangle3d (0000)
000FE210 0120:
	_circle_intersects_triangle2d (0000)
000FE330 0260:
	_sphere_intersects_triangle3d (0000)
000FE590 0140:
	_pill_intersects_rectangle2d (0000)
000FE6D0 0120:
	_pill_intersects_triangle2d (0000)
000FE7F0 0340:
	_pill_intersects_triangle3d (0000)
000FEB30 0170:
	_point_from_planes3d (0000)
000FECA0 0130:
	_line_from_planes3d (0000)
000FEDD0 01c0:
	_accelerate_to_position (0000)
000FEF90 0240:
	_angular_accelerate_to_position (0000)
000FF1D0 0080:
	_accelerate_to_velocity3d (0000)
000FF250 00f0:
	_accerate_to_position3d (0000)
000FF340 0140:
	_quantize_real_to_byte_lower_bound (0000)
000FF480 0130:
	_quantize_real_to_byte_upper_bound (0000)
000FF5B0 00f0:
	_quantize_real_to_byte_rectangle3d (0000)
000FF6A0 0050:
	_valid_real_sine_cosine (0000)
000FF6F0 01f0:
	_pin_normal_to_cone3d (0000)
000FF8E0 0150:
	_circle_intersects_cone2d (0000)
000FFA30 0170:
	_sphere_intersects_cone3d (0000)
000FFBA0 0160:
	_circle_intersects_sector2d (0000)
000FFD00 0170:
	_sphere_intersects_sector3d (0000)
000FFE70 00e0:
	_accelerate_to_velocity (0000)
0027AFB0 00a8:
	_rdata_0027afb0 (0000)
	_global_projection3d_mappings (0090)
0027B058 0003:
	??_C@_02HNLLPFKA@up?$AA@ (0000)
0027B05C 0008:
	??_C@_07CJMBKHEI@forward?$AA@ (0000)
0027B064 0020:
	??_C@_0CA@CJADJNKK@c?3?2halo?2SOURCE?2math?2real_math?4c?$AA@ (0000)
0027B084 001d:
	??_C@_0BN@CGIKENHM@?$CIt1?5?$DO?$DN?50?40f?$CJ?5?$CG?$CG?5?$CIt1?5?$DM?$DN?51?40f?$CJ?$AA@ (0000)
0027B0A4 001d:
	??_C@_0BN@IJIIIJPN@?$CIt0?5?$DO?$DN?50?40f?$CJ?5?$CG?$CG?5?$CIt0?5?$DM?$DN?51?40f?$CJ?$AA@ (0000)
0027B0C4 0023:
	??_C@_0CD@KMDBEJKN@?$CId0?5?$DM?5REAL_MAX?$CJ?5?$HM?$HM?5?$CId1?5?$DM?5REAL_MA@ (0000)
0027B0E8 000d:
	??_C@_0N@OJFEJAHD@cosine?$DO?$DN0?40f?$AA@ (0000)
0027B0F8 007c:
	??_C@_0HM@KOBDCKBG@dequantize_byte_to_real?$CImin?0?5max@ (0000)
0027B174 001f:
	??_C@_0BP@GNFGOPKP@?$CFlf?5is?5not?5between?5?$CFlf?5and?5?$CFlf?$AA@ (0000)
0027B198 008c:
	??_C@_0IM@POGCMEOK@dequantize_byte_to_real?$CImin?0?5max@ (0000)
0027B228 0084:
	??_C@_0IE@ICHPOHNI@rectangle?9?$DOx1?$DN?$DNREAL_MIN?5?$CG?$CG?5recta@ (0000)
0027B2AC 002e:
	??_C@_0CO@CNINPCHF@?$CFs?0?5?$CFs?3?5assert_valid_real_sine_c@ (0000)
0027B2DC 0005:
	??_C@_04JLECFDLM@sine?$AA@ (0000)
0027B2E4 001a:
	??_C@_0BK@BKKKOIMF@sine?$DO0?40f?5?$CG?$CG?5cosine?$DO?$DN0?40f?$AA@ (0000)
0030797C 007c:
	_global_origin2d (0000)
	_global_x_axis2d (0004)
	_global_y_axis2d (0008)
	_global_negative_x_axis2d (000c)
	_global_negative_y_axis2d (0010)
	_global_zero_vector2d (0014)
	_global_forward2d (0018)
	_global_left2d (001c)
	_global_backward2d (0020)
	_global_right2d (0024)
	_global_origin3d (0028)
	_global_x_axis3d (002c)
	_global_y_axis3d (0030)
	_global_z_axis3d (0034)
	_global_negative_x_axis3d (0038)
	_global_negative_y_axis3d (003c)
	_global_negative_z_axis3d (0040)
	_global_zero_vector3d (0044)
	_global_forward3d (0048)
	_global_left3d (004c)
	_global_up3d (0050)
	_global_backward3d (0054)
	_global_right3d (0058)
	_global_down3d (005c)
	_global_zero_angles2d (0060)
	_global_zero_angles3d (0064)
	_global_identity_quaternion (0068)
	_global_identity4x3 (006c)
	_global_negative_identity4x3 (0070)
	_global_null_rectangle2d (0074)
	_global_null_rectangle3d (0078)
*/

/* ---------- headers */

#include "cseries.h"
#include "real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

short const global_projection3d_mappings[3][2][2] =
{
	{
		{ 2, 1 },
		{ 1, 2 },
	},
	{
		{ 0, 2 },
		{ 2, 0 },
	},
	{
		{ 1, 0 },
		{ 0, 1 },
	}
};

static const real_matrix4x3 private_identity4x3 =
{
	1.f,
	{
		{ 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f }
	},
};

static const real_matrix4x3 private_negative_identity4x3 =
{
	1.f,
	{
		{ -1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f }
	},
};


const real_point3d *const global_origin3d = (const real_point3d *const)&private_identity4x3.position;

const real_point3d *const global_x_axis3d = (const real_point3d *const)&private_identity4x3.forward;
const real_point3d *const global_y_axis3d = (const real_point3d *const)&private_identity4x3.left;
const real_point3d *const global_z_axis3d = (const real_point3d *const)&private_identity4x3.up;
const real_point3d *const global_negative_x_axis3d = (const real_point3d *const)&private_negative_identity4x3.forward;
const real_point3d *const global_negative_y_axis3d = (const real_point3d *const)&private_negative_identity4x3.left;
const real_point3d *const global_negative_z_axis3d = (const real_point3d *const)&private_negative_identity4x3.up;

const real_vector3d *const global_zero_vector3d = (const real_vector3d *const)&private_identity4x3.position;

const real_vector3d *const global_forward3d = (const real_vector3d *const)&private_identity4x3.forward;
const real_vector3d *const global_left3d = (const real_vector3d *const)&private_identity4x3.left;
const real_vector3d *const global_up3d = (const real_vector3d *const)&private_identity4x3.up;

const real_vector3d *const global_backward3d = (const real_vector3d *const)&private_negative_identity4x3.forward;
const real_vector3d *const global_right3d = (const real_vector3d *const)&private_negative_identity4x3.left;
const real_vector3d *const global_down3d = (const real_vector3d *const)&private_negative_identity4x3.up;

/* ---------- public code */

void real_math_initialize(
	void)
{
	random_math_initialize();
	periodic_functions_initialize();
}

/* ---------- private code */

void real_math_dispose(
	void)
{
	random_math_dispose();
	periodic_functions_dispose();
}

void real_math_reset_precision(
	void)
{
	_control87(CW_DEFAULT, 0xFFFFF);
}

real_vector2d *perpendicular2d(
	const real_vector2d *a,
	real_vector2d *result)
{
	result->i = -a->j;
	result->j = a->i;
	return result;
}

real_vector3d *perpendicular3d(
	const real_vector3d *a,
	real_vector3d *result)
{
	real x, y, z;
	x = fabs(a->i);
	y = fabs(a->j);
	z = fabs(a->k);

	if (x <= y && x <= z)
	{
		result->i = 0;
		result->j = a->k;
		result->k = -a->j;
	}
	else
	{
		if (y <= z)
		{
			result->i = -a->k;
			result->j = 0;
			result->k = a->i;
		}
		else
		{
			result->i = a->j;
			result->j = -a->i;
			result->k = 0;
		}
	}

	return result;
}

real_vector4d *perpendicular4d(
	const real_vector4d *a,
	real_vector4d *result)
{
	result->i = a->k;
	result->j = a->l;
	result->k = -a->i;
	result->l = -a->j;
	return result;
}

real_vector3d *rotate_vector_about_axis(
	real_vector3d *v,
	const real_vector3d *n,
	real sine,
	real cosine
)
{
	real dot;
	real i, j, k;

	dot = dot_product3d(n, v) * (1-cosine);

	i = n->k*v->j - v->k*n->j;
	j = n->i*v->k - v->i*n->k;
	k = n->j*v->i - v->j*n->i;

	v->i = cosine*v->i + dot*n->i - sine*i;
	v->j = cosine*v->j + dot*n->j - sine*j;
	v->k = cosine*v->k + dot*n->k - sine*k;

	return v;
}

void vectors_interpolate(
	const real_vector3d *a,
	const real_vector3d *b,
	real t,
	real_vector3d *result
)
{
	real f = 1-t;
	result->i = f*a->i + t*b->i;
	result->j = f*a->j + t*b->j;
	result->k = f*a->k + t*b->k;
}

void points_interpolate(
	const real_point3d *a,
	const real_point3d *b,
	real t,
	real_point3d *result)
{
	real f = 1-t;
	result->x = f*a->x + t*b->x;
	result->y = f*a->y + t*b->y;
	result->z = f*a->z + t*b->z;
}

void scalars_interpolate(
	real a,
	real b,
	real t,
	real *result)
{
	*result = (1-t) * a+b * t;
}

void scalars_interpolate_and_clamp_0_to_1(
	real a,
	real b,
	real t,
	real *result)
{
	real f = (1-t)*a + t*b;
	*result = PIN(f, 0, 1);
}

void component_vectors_from_normal3d(
	const real_vector3d *vector,
	const real_vector3d *normal,
	real_vector3d *parallel,
	real_vector3d *perpendicular
)
{
	real_vector3d back_chain;
	real dot = dot_product3d(vector, normal);

	if (parallel== NULL)
		parallel = &back_chain;

	scale_vector3d(normal, dot, parallel);

	if (perpendicular!= NULL)
	{
		subtract_vectors3d(vector, parallel, perpendicular);
	}
}

void component_vectors_from_direction3d(
		const real_vector3d *vector,
		const real_vector3d *direction,
		real_vector3d *parallel,
		real_vector3d *perpendicular)
{
	real dot, reflect;
	dot = dot_product3d(direction, direction);
	if (dot != 0)
	{
		reflect = dot_product3d(vector, direction) / dot;
		scale_vector3d(direction, reflect, parallel);
		subtract_vectors3d(vector, parallel, perpendicular);
	}
	else
	{
		set_real_vector3d(parallel, 0, 0, 0);
		*perpendicular = *vector;
	}
}

/* TODO: mismatch in result regalloc */
void quaternions_multiply(
    const real_quaternion *q0, 
    const real_quaternion *q1, 
    real_quaternion *result
)
{
    real_quaternion back_chain;
    if (q0== result)
    {
        back_chain.v.i= q0->v.i;
        back_chain.v.j= q0->v.j;
        back_chain.v.k= q0->v.k;
        back_chain.w= q0->w;
        q0= &back_chain;
    }
    if (q1== result)
    {
        back_chain.v.i= q1->v.i;
        back_chain.v.j= q1->v.j;
        back_chain.v.k= q1->v.k;
        back_chain.w= q1->w;
        q1= &back_chain;
    }

    result->v.i =     
                    + q0->v.j*q1->v.k
                    + q0->v.i*q1->w
                    + q0->w  *q1->v.i 
                    - q0->v.k*q1->v.j
                    ;

    result->v.j =     
                    + q0->w  *q1->v.j 
                    + q0->v.j*q1->w
                    + q0->v.k*q1->v.i 
                    - q0->v.i*q1->v.k
                    ;

    result->v.k =     
                    + q0->v.k*q1->w 
                    + q0->v.i*q1->v.j
                    + q0->w  *q1->v.k 
                    - q0->v.j*q1->v.i
                    ;

    result->w =       
                    + q0->w  *q1->w 
                    - q0->v.i*q1->v.i 
                    - q0->v.j*q1->v.j 
                    - q0->v.k*q1->v.k
                    ;
}

void quaternions_interpolate(
	const real_quaternion *q0,
	const real_quaternion *q1,
	real t,
	real_quaternion *result)
{
	real t_inv = 1-t;

	if (q0->v.i*q1->v.i + q0->v.j*q1->v.j + q0->v.k*q1->v.k + q0->w*q1->w < 0)
		t = -t;

	result->v.i = q0->v.i*t_inv + q1->v.i*t;
	result->v.j = q0->v.j*t_inv + q1->v.j*t;
	result->v.k = q0->v.k*t_inv + q1->v.k*t;
	result->w = q0->w*t_inv + q1->w*t;
}

// TODO: needs to use inlines
void quaternion_transform_point(
	const real_quaternion *q,
	const real_point3d *p,
	real_point3d *result)
{
	real ww2_minus1;
	real dot_vp;
	real dot_vp2;
	real w2;
	real cross_x, cross_y;

	ww2_minus1 = (q->w*q->w) + (q->w*q->w) - 1;
	dot_vp = (q->v.i*p->x + q->v.k*p->z + q->v.j*p->y);
	dot_vp2 = dot_vp+dot_vp;
	w2 = q->w+q->w;
	cross_y = (q->v.k*p->x) - (p->z*q->v.i);
	cross_x = (q->v.i*p->y) - (q->v.j*p->x);

	result->x = (q->v.j*p->z - q->v.k*p->y) * w2+dot_vp2*q->v.i + ww2_minus1*p->x;
	result->y = dot_vp2*q->v.j + ww2_minus1*p->y + cross_y*w2;
	result->z = dot_vp2*q->v.k + ww2_minus1*p->z + cross_x*w2;
}

void vectors3d_from_euler_angles3d(
	real_vector3d *forward,
	real_vector3d *up,
	const real_euler_angles3d *angles)
{
	real_matrix4x3 matrix;
	real_point3d point;

	match_assert("c:\\halo\\SOURCE\\math\\real_math.c", 832, forward);
	match_assert("c:\\halo\\SOURCE\\math\\real_math.c", 833, up);
	match_assert("c:\\halo\\SOURCE\\math\\real_math.c", 834, angles);

	matrix4x3_rotation_from_angles(&matrix, angles->yaw, angles->pitch, angles->roll);
	matrix4x3_to_point_and_vectors(&matrix, &point, forward, up);
}

// TODO: mismatch in calculating dot and 'a', funky compiler optimization
// probably needs to take advantage of inlines
boolean fast_vector_intersects_sphere(
	const real_point3d *point,
	const real_vector3d *vector,
	const real_point3d *center,
	real radius)
{
	real_vector3d d;
	real a, c;
	real dot;
	real discrim;
	real tcheck;
	real d1, d2, d3;

	d.i = point->x - center->x;
	d.j = point->y - center->y;
	d.k = point->z - center->z;

	c = (d.i * d.i) + (d.j * d.j) + (d.k * d.k) - (radius * radius);
	if (c < 0)
		return TRUE;

	dot = (vector->i * d.i) + (vector->j * d.j) + (vector->k * d.k);
	if (dot >= 0)
		return FALSE;

	a = (vector->i * vector->i) + (vector->j * vector->j) + (vector->k * vector->k);
	discrim = dot * dot - a * c;
	if (discrim <= 0)
		return FALSE;

	tcheck = -a - dot;
	if (tcheck < 0)
		return TRUE;

	return tcheck * tcheck < discrim;
}

boolean point_in_rectangle2d(
	const real_point2d *point,
	const real_rectangle2d *bounds)
{
	return point->x >= bounds->x0
		&& point->x <= bounds->x1
		&& point->y >= bounds->y0
		&& point->y <= bounds->y1;
}

boolean point_in_rectangle3d(
	const real_point3d *point,
	const real_rectangle3d *bounds)
{
	return point->x >= bounds->x0
		&& point->x <= bounds->x1
		&& point->y >= bounds->y0
		&& point->y <= bounds->y1
		&& point->z >= bounds->z0
		&& point->z <= bounds->z1;
}

boolean vector_intersects_rectangle2d(
	const real_point2d *point,
	const real_vector2d *vector,
	const real_rectangle2d *bounds)
{
	real t;
	real min, max;
	real tmin, tmax;

	min = -FLT_MAX;
	max = FLT_MAX;

	if (!(fabs(vector->i) < _real_epsilon))
	{
		t = 1 / vector->i;
		tmin = (bounds->x0-point->x) * t;
		tmax = (bounds->x1-point->x) * t;

		if (vector->i > 0)
		{
			if (min < tmin) min = tmin;
			if (max > tmax) max = tmax;
		}
		else
		{
			if (min < tmax) min = tmax;
			if (max > tmin) max = tmin;
		}

		if (min > max)
			return FALSE;
	}
	else
	{
		if (point->x < bounds->x0 || point->x > bounds->x1)
			return FALSE;
	}

	if (!(fabs(vector->j) < _real_epsilon))
	{
		t = 1 / vector->j;
		tmin = (bounds->y0-point->y) * t;
		tmax = (bounds->y1-point->y) * t;

		if (vector->j > 0)
		{
			if (min < tmin) min = tmin;
			if (max > tmax) max = tmax;
		}
		else
		{
			if (min < tmax) min = tmax;
			if (max > tmin) max = tmin;
		}

		if (min > max)
			return FALSE;
	}
	else
	{
		if (point->y < bounds->y0 || point->y > bounds->y1)
			return FALSE;
	}

	return max >= 0 && min <= 1;
}


boolean vector_intersects_rectangle3d(
	const real_point3d *point,
	const real_vector3d *vector,
	const real_rectangle3d *bounds)
{
	real t;
	real min, max;
	real tmin, tmax;

	min = -FLT_MAX;
	max = FLT_MAX;

	if (!(fabs(vector->i) < _real_epsilon))
	{
		t = 1 / vector->i;
		tmin = (bounds->x0-point->x) * t;
		tmax = (bounds->x1-point->x) * t;

		if (vector->i > 0)
		{
			if (min < tmin) min = tmin;
			if (max > tmax) max = tmax;
		}
		else
		{
			if (min < tmax) min = tmax;
			if (max > tmin) max = tmin;
		}

		if (min > max)
			return FALSE;
	}
	else
	{
		if (point->x < bounds->x0 || point->x > bounds->x1)
			return FALSE;
	}

	if (!(fabs(vector->j) < _real_epsilon))
	{
		t = 1 / vector->j;
		tmin = (bounds->y0-point->y) * t;
		tmax = (bounds->y1-point->y) * t;

		if (vector->j > 0)
		{
			if (min < tmin) min = tmin;
			if (max > tmax) max = tmax;
		}
		else
		{
			if (min < tmax) min = tmax;
			if (max > tmin) max = tmin;
		}

		if (min > max)
			return FALSE;
	}
	else
	{
		if (point->y < bounds->y0 || point->y > bounds->y1)
			return FALSE;
	}

	if (!(fabs(vector->k) < _real_epsilon))
	{
		t = 1 / vector->k;
		tmin = (bounds->z0-point->z) * t;
		tmax = (bounds->z1-point->z) * t;

		if (vector->k > 0)
		{
			if (min < tmin) min = tmin;
			if (max > tmax) max = tmax;
		}
		else
		{
			if (min < tmax) min = tmax;
			if (max > tmin) max = tmin;
		}

		if (min > max)
			return FALSE;
	}
	else
	{
		if (point->z < bounds->z0 || point->z > bounds->z1)
			return FALSE;
	}

	return max >= 0 && min <= 1;
}

boolean pill_intersects_rectangle3d(
	const real_point3d *base,
	const real_vector3d *height,
	real width,
	const real_rectangle3d *bounds)
{
	/* originally stubbed */
	return FALSE;
}

real_vector2d *fast_normalize2d(
	real_vector2d *v)
{
	real magsqr = magnitude_squared2d(v);
	if (magsqr != 0)
	{
		scale_vector2d(v, 1.f / square_root(magsqr), v);
	}

	return v;
}

real_vector3d *fast_normalize3d(
	real_vector3d *v)
{
	real magsqr = magnitude_squared3d(v);
	if (magsqr != 0)
	{
		scale_vector3d(v, 1.f / square_root(magsqr), v);
	}

	return v;
}

// TODO: regalloc. this matches when expanded
real cross_product_magnitude3d(
	real_vector3d const *a,
	real_vector3d const *b)
{
	real_vector3d v;
	cross_product3d(a, b, &v);
	return magnitude3d(&v);
}

real dequantize_byte_to_real(
	real min,
	real max,
	unsigned char value)
{
	real norm;
	if (value == 0xFF)
		return max;

	norm = (real)value;
	norm *= (1/255.f);
	return norm*(max-min)+min;
}

real signed_angle_between_vectors2d(
	const real_vector2d *a,
	const real_vector2d *b)
{
	real dot = dot_product2d(a, b);
	real cosine, angle;

	cosine = PIN(dot, -1, 1);
	angle = arccosine(cosine);

	if ((a->i*b->j) - (a->j*b->i) < 0)
		return -angle;

	return angle;
}

real angle_between_vectors2d(
	const real_vector2d *a,
	const real_vector2d *b)
{
	real dot, dot2;
	real cosine, theta;
	real angle = 0;

	// TODO: identify the inline
	dot2 = (a->i*a->i + a->j*a->j) * (b->i*b->i + b->j*b->j);
	if (dot2 != 0)
	{
		dot = dot_product2d(a, b);
		theta = (dot/dot2*dot) + (dot/dot2*dot) - 1;
		cosine = PIN(theta, -1, 1);

		angle = arccosine(cosine) * 0.5f;
		if (dot< 0)
			return _pi - angle;
	}

	return angle;
}

real angle_between_vectors3d(
	const real_vector3d *a,
	const real_vector3d *b)
{
	real dot, dot2;
	real cosine, theta;
	real angle = 0;

	// TODO: identify the inline
	dot2 = (a->i*a->i + a->j*a->j + a->k*a->k)
		* (b->i*b->i + b->j*b->j + b->k*b->k);
	if (dot2 != 0)
	{
		dot = dot_product3d(a, b);
		theta = (dot/dot2*dot) + (dot/dot2*dot) - 1;
		cosine = PIN(theta, -1, 1);

		angle = arccosine(cosine) * 0.5f;
		if (dot< 0)
			return _pi - angle;
	}

	return angle;
}

real angle_between_normals3d(
	const real_vector3d *a,
	const real_vector3d *b)
{
	real dot;
	real angle;

	if ((*(int *)&a->i) == (*(int *)&b->i)
	  && (*(int *)&a->j) == (*(int *)&b->j)
	  && (*(int *)&a->k) == (*(int *)&b->k))
	{
		return 0;
	}

	dot = dot_product3d(a, b);
	angle = PIN(dot, -1, 1);

	return arccosine(angle);
}

void yaw_vectors(
	real_vector3d *forward,
	const real_vector3d *up,
	real sine,
	real cosine)
{
	real_vector3d right;
	cross_product3d(up, forward, &right);

	forward->i = forward->i*cosine + right.i*sine;
	forward->j = forward->j*cosine + right.j*sine;
	forward->k = forward->k*cosine + right.k*sine;
}

void pitch_vectors(
	real_vector3d *forward,
	real_vector3d *up,
	real sine,
	real cosine)
{
	real_vector3d back;
	negate_vector3d(forward, &back);

	forward->i = forward->i*cosine + up->i*sine;
	forward->j = forward->j*cosine + up->j*sine;
	forward->k = forward->k*cosine + up->k*sine;

	up->i = up->i*cosine + back.i*sine;
	up->j = up->j*cosine + back.j*sine;
	up->k = up->k*cosine + back.k*sine;
}

void fast_normals_interpolate(
	const real_vector3d *a,
	const real_vector3d *b,
	real t,
	real_vector3d *result)
{
	real f = 1-t;
	result->i = t*b->i + f * a->i;
	result->j = t*b->j + f * a->j;
	result->k = t*b->k + f * a->k;
	fast_normalize3d(result);
}

void normals_interpolate(
	const real_vector3d *a,
	const real_vector3d *b,
	real t,
	real_vector3d *result)
{
	real angle, theta;
	real_vector3d axis;

	angle = angle_between_vectors3d(a, b);

	if (t <= 0.5f)
	{
		theta = angle * t;
		cross_product3d(a, b, &axis);
		*result = *a;
	}
	else
	{
		theta = angle * (1-t);
		cross_product3d(b, a, &axis);
		*result = *b;
	}

	if (normalize3d(&axis) == 0.0f)
	{
		return;
	}

	rotate_vector_about_axis(result, &axis, sine(theta), cosine(theta));
}

real_vector3d *reflect_vector3d(
	const real_vector3d *incident,
	const real_vector3d *normal,
	real_vector3d *reflection)
{
	real dot = dot_product3d(incident, normal);
	real reflect = dot + dot;
	reflection->i = incident->i - normal->i*reflect;
	reflection->j = incident->j - normal->j*reflect;
	reflection->k = incident->k - normal->k*reflect;
	return reflection;
}

// TODO: mismatch in operation order. need to identify inlines
real_vector3d *refract_vector3d(
	const real_vector3d *incident,
	const real_vector3d *normal,
	real coefficient_of_refraction,
	real_vector3d *refraction)
{
	real v6, v9, v11, v12, v13, v14, v15, v17;
	real incidenta;
	real_vector3d refract;

	v6 = square_root(incident->i * incident->i + incident->j * incident->j + incident->k * incident->k);

	refract.i = (normal->k * incident->j) - (normal->j * incident->k);
	refract.j = (normal->i * incident->k) - (normal->k * incident->i);
	refract.k = (normal->j * incident->i) - (normal->i * incident->j);

	v9 = 1 / v6;
	v11 = square_root(refract.i * refract.i + refract.j * refract.j + refract.k * refract.k) * v9;
	v12 = coefficient_of_refraction * v11;
	v13 = incident->i * normal->i + normal->j * incident->j + normal->k * incident->k;
	v17 = square_root(1 - v12 * v12) * v11 + (v13)*v9 * v12;

	incidenta = v17 / v12;

	refraction->i = normal->i * incidenta + incident->i;
	refraction->j = normal->j * incidenta + incident->j;
	refraction->k = normal->k * incidenta + incident->k;

	v15 = square_root(refraction->i * refraction->i + refraction->j * refraction->j + refraction->k * refraction->k);
	v14 = v6 / v15;
	refraction->i = refraction->i * v14;
	refraction->j = refraction->j * v14;
	refraction->k = refraction->k * v14;

	return refraction;
}

void quaternion_normalize(
	real_quaternion *q)
{
	real mag;
	real dot = q->v.i*q->v.i + q->v.j*q->v.j + q->v.k*q->v.k + q->w*q->w;
	if (dot > 0)
	{
		mag = 1/square_root(dot);
		set_real_quaternion(q, mag*q->v.i, mag*q->v.j, mag*q->v.k, mag*q->w);
	}
	else
	{
		set_real_quaternion(q, 0, 0, 0, 1);
	}
}

void quaternion_from_angle_and_vector(
	real_quaternion *q,
	real a,
	const real_vector3d *v)
{
	real angle = a*0.5f;
	real s = sine(angle);
	q->w = cosine(angle);
	q->v.i = s*v->i;
	q->v.j = s*v->j;
	q->v.k = s*v->k;
}

/* mismatch in regalloc for v assignment */
void quaternion_to_angle_and_vector(
	const real_quaternion *q,
	real *a,
	real_vector3d *v)
{
	real angle, mag;

	*v = q->v;
	mag = normalize3d(v);

	angle = arctangent(mag, q->w);
	*a = angle + angle;
	if (*a > _pi)
	{
		v->i = -v->i;
		v->j = -v->j;
		v->k = -v->k;
		*a = _pi * 2.0f - *a;
	}
}

void quaternions_interpolate_and_normalize(
	const real_quaternion *q0,
	const real_quaternion *q1,
	real t,
	real_quaternion *result)
{
	quaternions_interpolate(q0, q1, t, result);
	quaternion_normalize(result);
}

void orientations_interpolate(
	const real_orientation *a,
	const real_orientation *b,
	real t,
	real_orientation *result)
{
	real f;

	quaternions_interpolate(&a->rotation, &b->rotation, t, &result->rotation);
	quaternion_normalize(&result->rotation);

	f = 1 - t;
	result->translation.x = a->translation.x*f + b->translation.x*t;
	result->translation.y = a->translation.y*f + b->translation.y*t;
	result->translation.z = a->translation.z*f + b->translation.z*t;
	result->scale = a->scale*f + b->scale*t;
}

real_euler_angles2d *euler_angles2d_from_vector3d(
	real_euler_angles2d *angles,
	const real_vector3d *vector)
{
	angles->yaw = arctangent(vector->j, vector->i);
	angles->pitch = arctangent(vector->k, square_root(vector->i*vector->i+vector->j*vector->j));
	return angles;
}

real_vector3d *vector3d_from_angle(
	real_vector3d *vector,
	real angle)
{
	vector->k = 0;
	vector->i = cosine(angle);
	vector->j = sine(angle);
	return vector;
}

real_vector3d *vector3d_from_euler_angles2d(
	real_vector3d *vector,
	const real_euler_angles2d *angles)
{
	double c = cosine(angles->pitch);
	vector->i = cosine(angles->yaw)* c;
	vector->j = sine(angles->yaw)* c;
	vector->k = sine(angles->pitch);
	return vector;
}

boolean point_in_pill2d(
	const real_point2d *point,
	const real_point2d *base,
	const real_vector2d *height,
	real width)
{
	real_vector2d dir;
	real_vector2d closest;
	real proj, proj_neg, dist_sqr;

	vector_from_points2d(base, point, &dir);

	proj = dot_product2d(&dir, height) / dot_product2d(height, height);
	proj_neg = -PIN(proj, 0, 1);

	closest.i = height->i*proj_neg + dir.i;
	closest.j = height->j*proj_neg + dir.j;

	// TODO: this is an inline
	dist_sqr = (closest.i*closest.i) + closest.j*closest.j;
	return dist_sqr <= width*width;
}

real point_to_line_distance_squared3d(
	const real_point3d *point,
	const real_point3d *base,
	const real_vector3d *height)
{
	real proj, proj_neg;
	real_vector3d d;
	real dist_sqr;
	
	vector_from_points3d(base, point, &d);

	proj = dot_product3d(&d, height) / dot_product3d(height, height);
	proj_neg = -PIN(proj, 0, 1);

	d.i = height->i*proj_neg + d.i;
	d.j = height->j*proj_neg + d.j;
	d.k = height->k*proj_neg + d.k;

	// TODO: this is an inline
	dist_sqr = (d.i*d.i) + d.j*d.j + (d.k*d.k);
	return dist_sqr;
}

// TODO: mismatch in fpu stack when writing normal->k 
// likely need to identify inlines
boolean sphere_test_vector3d(
	const real_point3d *center,
	real radius,
	const real_point3d *point,
	const real_vector3d *vector,
	real *t, real_vector3d *normal)
{
	real_vector3d v;
	real dist_sqr;
	real inside_test;
	real inv_dist;
	real disc;
	real hit;
	real_vector3d *hit_normal;
	real dirlen_sqr;
	real dot;
	real intersect;

	vector_from_points3d(center, point, &v);

	dot = v.i*vector->i + v.j*vector->j + v.k*vector->k;
	if (dot < 0)
	{
		dist_sqr = (v.i*v.i) + v.j*v.j + (v.k*v.k);
		inside_test = dist_sqr - radius*radius;

		if (inside_test <= 0)
		{
			*t = 0;
			inv_dist = 1/square_root(dist_sqr);
			normal->i = v.i*inv_dist;
			normal->j = v.j*inv_dist;
			normal->k = v.k*inv_dist;
			return TRUE;
		}
		else
		{
			dirlen_sqr = vector->i*vector->i + vector->j*vector->j + vector->k*vector->k;
			disc = dot*dot - dirlen_sqr*inside_test;

			if (disc >= 0)
			{
				hit = -(square_root(disc) + dot);
				if (hit <= dirlen_sqr)
				{
					*t = hit/dirlen_sqr;
					hit_normal = (real_vector3d *)point_from_line3d((real_point3d *)&v, vector, *t, (real_point3d *)normal);
					fast_normalize3d(hit_normal);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

boolean valid_real_sine_cosine(
	real sine, 
	real cosine)
{
	return valid_realcmp(sine * sine + cosine * cosine, 1.0f);
}
