/*
REAL_MATH.C
*/

/* ---------- headers */

#include "cseries.h"
#include "real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

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

static const real_quaternion private_identity_quaternion =
{
	{ { 0.f, 0.f, 0.f } },
	1.f,
};

static const real_rectangle3d private_null_rectangle3d =
{
	{ REAL_MAX, REAL_MIN, REAL_MAX, REAL_MIN, REAL_MAX, REAL_MIN },
};

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

const real_point2d *global_origin2d = (const real_point2d *)&private_identity4x3.up;
const real_point2d *global_x_axis2d = (const real_point2d *)&private_identity4x3.forward;
const real_point2d *global_y_axis2d = (const real_point2d *)&private_identity4x3.left;
const real_point2d *global_negative_x_axis2d = (const real_point2d *)&private_negative_identity4x3.forward;
const real_point2d *global_negative_y_axis2d = (const real_point2d *)&private_negative_identity4x3.left;

const real_vector2d *global_zero_vector2d = (const real_vector2d *)&private_identity4x3.up;
const real_vector2d *global_forward2d = (const real_vector2d *)&private_identity4x3.forward;
const real_vector2d *global_left2d = (const real_vector2d *)&private_identity4x3.left;
const real_vector2d *global_backward2d = (const real_vector2d *)&private_negative_identity4x3.forward;
const real_vector2d *global_right2d = (const real_vector2d *)&private_negative_identity4x3.left;

const real_point3d *global_origin3d = (const real_point3d *)&private_identity4x3.position;

const real_point3d *global_x_axis3d = (const real_point3d *)&private_identity4x3.forward;
const real_point3d *global_y_axis3d = (const real_point3d *)&private_identity4x3.left;
const real_point3d *global_z_axis3d = (const real_point3d *)&private_identity4x3.up;
const real_point3d *global_negative_x_axis3d = (const real_point3d *)&private_negative_identity4x3.forward;
const real_point3d *global_negative_y_axis3d = (const real_point3d *)&private_negative_identity4x3.left;
const real_point3d *global_negative_z_axis3d = (const real_point3d *)&private_negative_identity4x3.up;

const real_vector3d *global_zero_vector3d = (const real_vector3d *)&private_identity4x3.position;

const real_vector3d *global_forward3d = (const real_vector3d *)&private_identity4x3.forward;
const real_vector3d *global_left3d = (const real_vector3d *)&private_identity4x3.left;
const real_vector3d *global_up3d = (const real_vector3d *)&private_identity4x3.up;

const real_vector3d *global_backward3d = (const real_vector3d *)&private_negative_identity4x3.forward;
const real_vector3d *global_right3d = (const real_vector3d *)&private_negative_identity4x3.left;
const real_vector3d *global_down3d = (const real_vector3d *)&private_negative_identity4x3.up;

const real_euler_angles2d *global_zero_angles2d = (const real_euler_angles2d *)&private_identity4x3.up;
const real_euler_angles3d *global_zero_angles3d = (const real_euler_angles3d *)&private_identity4x3.up;
const real_quaternion *global_identity_quaternion = &private_identity_quaternion;
const real_matrix4x3 *global_identity4x3 = &private_identity4x3;
const real_matrix4x3 *global_negative_identity4x3 = &private_negative_identity4x3;
const real_rectangle2d *global_null_rectangle2d = (const real_rectangle2d *)&private_null_rectangle3d;
const real_rectangle3d *global_null_rectangle3d = &private_null_rectangle3d;

/* ---------- public code */

void real_math_initialize(
	void)
{
	random_math_initialize();
	periodic_functions_initialize();
	return;
}

void real_math_dispose(
	void)
{
	random_math_dispose();
	periodic_functions_dispose();
	return;
}

void real_math_reset_precision(
	void)
{
	_control87(CW_DEFAULT, 0xFFFFF);
	return;
}

real_vector2d *perpendicular2d(
	real_vector2d const *a,
	real_vector2d *result)
{
	result->i = -a->j;
	result->j = a->i;
	return result;
}

real_vector3d *perpendicular3d(
	real_vector3d const *a,
	real_vector3d *result)
{
	real i = fabs(a->i);
	real j = fabs(a->j);
	real k = fabs(a->k);

	if (i <= j && i <= k)
	{
		result->i = 0.f;
		result->j = a->k;
		result->k = -a->j;
	}
	else
	{
		if (j <= k)
		{
			result->i = -a->k;
			result->j = 0.f;
			result->k = a->i;
		}
		else
		{
			result->i = a->j;
			result->j = -a->i;
			result->k = 0.f;
		}
	}

	return result;
}

real_vector4d *perpendicular4d(
	real_vector4d const *a,
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
	real_vector3d const *n,
	real sine,
	real cosine)
{
	real one_minus_cosine_times_v_dot_n = dot_product3d(n, v) * (1-cosine);

	real v_cross_n_i = n->k*v->j - v->k*n->j;
	real v_cross_n_j = n->i*v->k - v->i*n->k;
	real v_cross_n_k = n->j*v->i - v->j*n->i;

	v->i = cosine*v->i + one_minus_cosine_times_v_dot_n*n->i - sine*v_cross_n_i;
	v->j = cosine*v->j + one_minus_cosine_times_v_dot_n*n->j - sine*v_cross_n_j;
	v->k = cosine*v->k + one_minus_cosine_times_v_dot_n*n->k - sine*v_cross_n_k;

	return v;
}

void vectors_interpolate(
	real_vector3d const *a,
	real_vector3d const *b,
	real t,
	real_vector3d *result)
{
	real one_minus_t = 1-t;
	result->i = one_minus_t*a->i + t*b->i;
	result->j = one_minus_t*a->j + t*b->j;
	result->k = one_minus_t*a->k + t*b->k;
	return;
}

void points_interpolate(
	real_point3d const *a,
	real_point3d const *b,
	real t,
	real_point3d *result)
{
	real one_minus_t = 1-t;
	result->x = one_minus_t*a->x + t*b->x;
	result->y = one_minus_t*a->y + t*b->y;
	result->z = one_minus_t*a->z + t*b->z;
	return;
}

void scalars_interpolate(
	real a,
	real b,
	real t,
	real *result)
{
	*result = (1-t) * a+b * t;
	return;
}

void scalars_interpolate_and_clamp_0_to_1(
	real a,
	real b,
	real t,
	real *result)
{
	*result = PIN((1.f-t)*a + t*b, 0.f, 1.f);
	return;
}

void component_vectors_from_normal3d(
	real_vector3d const *vector,
	real_vector3d const *normal,
	real_vector3d *parallel,
	real_vector3d *perpendicular)
{
	real_vector3d storage;
	real vn = dot_product3d(vector, normal);

	if (parallel== NULL)
	{
		parallel = &storage;
	}

	scale_vector3d(normal, vn, parallel);

	if (perpendicular!= NULL)
	{
		subtract_vectors3d(vector, parallel, perpendicular);
	}
	return;
}

void component_vectors_from_direction3d(
	real_vector3d const *vector,
	real_vector3d const *direction,
	real_vector3d *parallel,
	real_vector3d *perpendicular)
{
	real dd = dot_product3d(direction, direction);
	if (dd != 0.f)
	{
		real vd_dd = dot_product3d(vector, direction) / dd;
		scale_vector3d(direction, vd_dd, parallel);
		subtract_vectors3d(vector, parallel, perpendicular);
	}
	else
	{
		set_real_vector3d(parallel, 0.f, 0.f, 0.f);
		*perpendicular = *vector;
	}
	return;
}

void quaternions_multiply(
	real_quaternion const *q0,
	real_quaternion const *q1,
	real_quaternion *result)
{
	real_quaternion temporary;

	if (q0== result)
	{
		temporary = *q0;
		q0= &temporary;
	}

	if (q1== result)
	{
		temporary = *q1;
		q1= &temporary;
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

	return;
}

void quaternions_interpolate(
	real_quaternion const *q0,
	real_quaternion const *q1,
	real t,
	real_quaternion *result)
{
	real v = 1.f-t;
	
	if (q0->v.i*q1->v.i + q0->v.j*q1->v.j + q0->v.k*q1->v.k + q0->w*q1->w < 0.f)
	{
		t = -t;
	}

	result->v.i = q0->v.i*v + q1->v.i*t;
	result->v.j = q0->v.j*v + q1->v.j*t;
	result->v.k = q0->v.k*v + q1->v.k*t;
	result->w = q0->w*v + q1->w*t;
	return;
}

void quaternion_transform_point(
	real_quaternion const *q,
	real_point3d const *p,
	real_point3d *result)
{

	real w_squared = q->w*q->w;
	real two_w_squared_minus_one = 2.f * w_squared - 1.f;
	real v_dot_p = (q->v.i*p->x + q->v.k*p->z + q->v.j*p->y);
	
	real two_v_dot_p = v_dot_p+v_dot_p;
	real two_w = q->w+q->w;

	real v_cross_r_i = (q->v.k*p->x) - (p->z*q->v.i);
	real v_cross_r_j = (q->v.i*p->y) - (q->v.j*p->x);
	real v_cross_r_k = (q->v.j*p->z) - (q->v.k*p->y);

	result->x = v_cross_r_k * two_w+two_v_dot_p*q->v.i + two_w_squared_minus_one*p->x;
	result->y = two_v_dot_p*q->v.j + two_w_squared_minus_one*p->y + v_cross_r_i*two_w;
	result->z = two_v_dot_p*q->v.k + two_w_squared_minus_one*p->z + v_cross_r_j*two_w;
	return;
}

void vectors3d_from_euler_angles3d(
	real_vector3d *forward,
	real_vector3d *up,
	real_euler_angles3d const *angles)
{
	real_matrix4x3 matrix;
	real_point3d dummy;

	match_assert("c:\\halo\\SOURCE\\math\\real_math.c", 832, forward);
	match_assert("c:\\halo\\SOURCE\\math\\real_math.c", 833, up);
	match_assert("c:\\halo\\SOURCE\\math\\real_math.c", 834, angles);

	matrix4x3_rotation_from_angles(&matrix, angles->yaw, angles->pitch, angles->roll);
	matrix4x3_to_point_and_vectors(&matrix, &dummy, forward, up);
	return;
}

// TODO: mismatch in calculating dot and 'a', funky compiler optimization
boolean fast_vector_intersects_sphere(
	real_point3d const *point,
	real_vector3d const *vector,
	real_point3d const *center,
	real radius)
{
	real_point3d p = {point->x-center->x, point->y-center->y, point->z - center->z};
	real c = (p.x*p.x) + (p.y*p.y) + (p.z*p.z) - (radius*radius);

	if (c < 0.f)
	{
		return TRUE;
	}
	else
	{
		real b = vector->i*p.x + vector->j*p.y + vector->k*p.z;
		
		if (b >= 0.f)
		{
			return FALSE;
		}
		else
		{
			real a = vector->i*vector->i + vector->j*vector->j + vector->k*vector->k;
			real disc = b * b - a * c;
			
			if (disc <= 0.f)
			{
				return FALSE;
			}
			else
			{
				real neg_a_minus_b = -a - b;

				if (neg_a_minus_b < 0.f)
				{
					return TRUE;
				}
				else
				{
					return neg_a_minus_b * neg_a_minus_b < disc;
				}
			}
		}
	}
}

boolean point_in_rectangle2d(
	real_point2d const *point,
	real_rectangle2d const *bounds)
{
	return point->x >= bounds->x0
		&& point->x <= bounds->x1
		&& point->y >= bounds->y0
		&& point->y <= bounds->y1;
}

boolean point_in_rectangle3d(
	real_point3d const *point,
	real_rectangle3d const *bounds)
{
	return point->x >= bounds->x0
		&& point->x <= bounds->x1
		&& point->y >= bounds->y0
		&& point->y <= bounds->y1
		&& point->z >= bounds->z0
		&& point->z <= bounds->z1;
}

boolean circle_intersects_rectangle2d(
	real_point2d const *center,
	real radius,
	real_rectangle2d const *bounds)
{
	real dx;
	real dy;

	if (center->x <= bounds->x1)
	{
		if (center->x < bounds->x0)
		{
			dx = bounds->x0 - center->x;
		}
		else
		{
			dx = 0.f;
		}
	}
	else
	{
		dx = center->x - bounds->x1;
	}

	if (center->y <= bounds->y1)
	{
		if (center->y < bounds->y0)
		{
			dy = bounds->y0 - center->y;
		}
		else
		{
			dy = 0.f;
		}
	}
	else
	{
		dy = center->y - bounds->y1;
	}

	return dx * dx + dy * dy <= radius * radius;
}

boolean point_in_cone2d(
	real_point2d const *point,
	real_point2d const *center,
	real_vector2d const *direction,
	real length,
	real cosine)
{
	real_vector2d offset;
	real projection;

	match_assert(
		"c:\\halo\\SOURCE\\math\\real_math.c",
		1375,
		cosine>=0.0f);

	vector_from_points2d(center, point, &offset);
	projection = dot_product2d(&offset, direction);

	return (boolean)(projection >= 0.f &&
		projection <= length &&
		projection * projection >= magnitude_squared2d(&offset) * cosine * cosine);
}

boolean point_in_cone3d(
	real_point3d const *point,
	real_point3d const *center,
	real_vector3d const *direction,
	real length,
	real cosine)
{
	real_vector3d offset;
	real projection;

	match_assert(
		"c:\\halo\\SOURCE\\math\\real_math.c",
		1404,
		cosine>=0.0f);

	vector_from_points3d(center, point, &offset);
	projection = dot_product3d(&offset, direction);

	return (boolean)(projection >= 0.f &&
		projection <= length &&
		projection * projection >= magnitude_squared3d(&offset) * cosine * cosine);
}

boolean point_in_sector2d(
	real_point2d const *point,
	real_point2d const *center,
	real_vector2d const *direction,
	real radius,
	real cosine)
{
	real_vector2d offset;
	real distance_squared;
	real projection;
	boolean result;

	match_assert(
		"c:\\halo\\SOURCE\\math\\real_math.c",
		1433,
		cosine>=0.0f);

	vector_from_points2d(center, point, &offset);
	distance_squared = magnitude_squared2d(&offset);
	if (distance_squared <= radius * radius)
	{
		projection = dot_product2d(&offset, direction);
		if (projection >= 0.f &&
			projection * projection >= distance_squared * cosine * cosine)
		{
			result = TRUE;
		}
		else
		{
			result = FALSE;
		}
	}
	else
	{
		result = FALSE;
	}

	return result;
}

boolean point_in_sector3d(
	real_point3d const *point,
	real_point3d const *center,
	real_vector3d const *direction,
	real radius,
	real cosine)
{
	real_vector3d offset;
	real distance_squared;
	real projection;
	boolean result;

	match_assert(
		"c:\\halo\\SOURCE\\math\\real_math.c",
		1462,
		cosine>=0.0f);

	vector_from_points3d(center, point, &offset);
	distance_squared = magnitude_squared3d(&offset);
	if (distance_squared <= radius * radius)
	{
		projection = dot_product3d(&offset, direction);
		if (projection >= 0.f &&
			projection * projection >= distance_squared * cosine * cosine)
		{
			result = TRUE;
		}
		else
		{
			result = FALSE;
		}
	}
	else
	{
		result = FALSE;
	}

	return result;
}

boolean point_in_triangle2d(
	real_point2d const *point,
	real_point2d const *triangle0,
	real_point2d const *triangle1,
	real_point2d const *triangle2,
	real *t0,
	real *t1)
{
	real_vector2d edge0;
	real_vector2d edge1;
	real_vector2d offset;
	real cross0;
	real cross1;
	real determinant;
	boolean result;

	vector_from_points2d(triangle0, triangle1, &edge0);
	vector_from_points2d(triangle0, point, &offset);
	cross0 = cross_product2d(&edge0, &offset);
	if (cross0 >= 0.f)
	{
		vector_from_points2d(triangle0, triangle2, &edge1);
		cross1 = cross_product2d(&offset, &edge1);
		if (cross1 >= 0.f)
		{
			determinant = cross_product2d(&edge0, &edge1);
			if (cross0 + cross1 <= determinant)
			{
				real inverse_determinant = 1.f / determinant;
				*t0 = cross1 * inverse_determinant;
				*t1 = cross0 * inverse_determinant;
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
		}
		else
		{
			result = FALSE;
		}
	}
	else
	{
		result = FALSE;
	}

	return result;
}

boolean sphere_intersects_rectangle3d(
	real_point3d const *center,
	real radius,
	real_rectangle3d const *bounds)
{
	real dx;
	real dy;
	real dz;

	if (center->x <= bounds->x1)
	{
		if (center->x < bounds->x0)
		{
			dx = bounds->x0 - center->x;
		}
		else
		{
			dx = 0.f;
		}
	}
	else
	{
		dx = center->x - bounds->x1;
	}

	if (center->y <= bounds->y1)
	{
		if (center->y < bounds->y0)
		{
			dy = bounds->y0 - center->y;
		}
		else
		{
			dy = 0.f;
		}
	}
	else
	{
		dy = center->y - bounds->y1;
	}

	if (center->z <= bounds->z1)
	{
		if (center->z < bounds->z0)
		{
			dz = bounds->z0 - center->z;
		}
		else
		{
			dz = 0.f;
		}
	}
	else
	{
		dz = center->z - bounds->z1;
	}

	return dx * dx + dy * dy + dz * dz < radius * radius;
}

boolean vector_intersects_rectangle2d(
	real_point2d const *point,
	real_vector2d const *vector,
	real_rectangle2d const *bounds)
{
	real t_in = -FLT_MAX;
	real t_out = FLT_MAX;

	if (!(fabs(vector->i) < _real_epsilon))
	{
		real ooi = 1.f / vector->i;
		real t0 = (bounds->x0-point->x) * ooi;
		real t1 = (bounds->x1-point->x) * ooi;

		if (vector->i > 0.f)
		{
			if (t_in < t0) t_in = t0;
			if (t_out > t1) t_out = t1;
		}
		else
		{
			if (t_in < t1) t_in = t1;
			if (t_out > t0) t_out = t0;
		}

		if (t_in > t_out)
		{
			return FALSE;
		}
	}
	else
	{
		if (point->x < bounds->x0 || point->x > bounds->x1)
		{
			return FALSE;
		}
	}

	if (!(fabs(vector->j) < _real_epsilon))
	{
		real ooj = 1.f  / vector->j;
		real t0 = (bounds->y0-point->y) * ooj;
		real t1 = (bounds->y1-point->y) * ooj;

		if (vector->j > 0.f)
		{
			if (t_in < t0) t_in = t0;
			if (t_out > t1) t_out = t1;
		}
		else
		{
			if (t_in < t1) t_in = t1;
			if (t_out > t0) t_out = t0;
		}

		if (t_in > t_out)
		{
			return FALSE;
		}
	}
	else
	{
		if (point->y < bounds->y0 || point->y > bounds->y1)
		{
			return FALSE;
		}
	}

	return t_out >= 0.f && t_in <= 1.f;
}

boolean vector_intersects_rectangle3d(
	real_point3d const *point,
	real_vector3d const *vector,
	real_rectangle3d const *bounds)
{
	real t_in = -FLT_MAX;
	real t_out = FLT_MAX;

	if (!(fabs(vector->i) < _real_epsilon))
	{
		real ooi = 1.f / vector->i;
		real t0 = (bounds->x0-point->x) * ooi;
		real t1 = (bounds->x1-point->x) * ooi;

		if (vector->i > 0.f)
		{
			if (t_in < t0) t_in = t0;
			if (t_out > t1) t_out = t1;
		}
		else
		{
			if (t_in < t1) t_in = t1;
			if (t_out > t0) t_out = t0;
		}

		if (t_in > t_out)
		{
			return FALSE;
		}
	}
	else
	{
		if (point->x < bounds->x0 || point->x > bounds->x1)
			return FALSE;
	}

	if (!(fabs(vector->j) < _real_epsilon))
	{
		real ooj = 1.f / vector->j;
		real t0 = (bounds->y0-point->y) * ooj;
		real t1 = (bounds->y1-point->y) * ooj;

		if (vector->j > 0.f)
		{
			if (t_in < t0) t_in = t0;
			if (t_out > t1) t_out = t1;
		}
		else
		{
			if (t_in < t1) t_in = t1;
			if (t_out > t0) t_out = t0;
		}

		if (t_in > t_out)
		{
			return FALSE;
		}
	}
	else
	{
		if (point->y < bounds->y0 || point->y > bounds->y1)
		{
			return FALSE;
		}
	}

	if (!(fabs(vector->k) < _real_epsilon))
	{
		real ook = 1 / vector->k;
		real t0 = (bounds->z0-point->z) * ook;
		real t1 = (bounds->z1-point->z) * ook;

		if (vector->k > 0.f)
		{
			if (t_in < t0) t_in = t0;
			if (t_out > t1) t_out = t1;
		}
		else
		{
			if (t_in < t1) t_in = t1;
			if (t_out > t0) t_out = t0;
		}

		if (t_in > t_out)
		{
			return FALSE;
		}
	}
	else
	{
		if (point->z < bounds->z0 || point->z > bounds->z1)
		{
			return FALSE;
		}
	}

	return t_out >= 0.f && t_in <= 1.f;
}

boolean pill_intersects_rectangle3d(
	real_point3d const *base,
	real_vector3d const *height,
	real width,
	real_rectangle3d const *bounds)
{
	/* originally stubbed */
	return FALSE;
}

real_vector2d *fast_normalize2d(
	real_vector2d *v)
{
	real magnitude_squared = magnitude_squared2d(v);
	
	if (magnitude_squared != 0.f)
	{
		scale_vector2d(v, 1.f / square_root(magnitude_squared), v);
	}

	return v;
}

real_vector3d *fast_normalize3d(
	real_vector3d *v)
{
	real magnitude_squared = magnitude_squared3d(v);
	
	if (magnitude_squared != 0.f)
	{
		scale_vector3d(v, 1.f / square_root(magnitude_squared), v);
	}

	return v;
}

real cross_product_magnitude3d(
	real_vector3d const *a,
	real_vector3d const *b)
{
	real_vector3d c;
	cross_product3d(a, b, &c);
	return magnitude3d(&c);
}

real dequantize_byte_to_real(
	real min,
	real max,
	byte value)
{
	if (value == UNSIGNED_CHAR_MAX)
	{
		return max;
	}
	
	return (max-min) * ((real)value / 255.f) + min;
}

byte_rectangle3d *quantize_real_to_byte_rectangle3d(
	real_rectangle3d const *parent,
	real_rectangle3d const *rectangle,
	byte_rectangle3d *result)
{
	if (rectangle->x0 == REAL_MAX)
	{
		match_assert(
			"c:\\halo\\SOURCE\\math\\real_math.c",
			2843,
			rectangle->x1==REAL_MIN && rectangle->y0==REAL_MAX && rectangle->y1==REAL_MIN && rectangle->z0==REAL_MAX && rectangle->z1==REAL_MIN);

		csmemset(result, 0, sizeof(*result));
	}
	else
	{
		result->x0 = quantize_real_to_byte_lower_bound(parent->x0, parent->x1, rectangle->x0);
		result->x1 = quantize_real_to_byte_upper_bound(parent->x0, parent->x1, rectangle->x1);
		result->y0 = quantize_real_to_byte_lower_bound(parent->y0, parent->y1, rectangle->y0);
		result->y1 = quantize_real_to_byte_upper_bound(parent->y0, parent->y1, rectangle->y1);
		result->z0 = quantize_real_to_byte_lower_bound(parent->z0, parent->z1, rectangle->z0);
		result->z1 = quantize_real_to_byte_upper_bound(parent->z0, parent->z1, rectangle->z1);
	}

	return result;
}

real signed_angle_between_vectors2d(
	real_vector2d const *a,
	real_vector2d const *b)
{
	real dot_product = dot_product2d(a, b);
	real result = arccosine(PIN(dot_product, -1.f, 1.f));

	if ((a->i*b->j) - (a->j*b->i) < 0.f)
	{
		result = -result;
	}

	return result;
}

real angle_between_vectors2d(
	real_vector2d const *a,
	real_vector2d const *b)
{
	real angle = 0.f;
	real aa_bb = (a->i*a->i + a->j*a->j) * (b->i*b->i + b->j*b->j);
	
	if (aa_bb != 0.f)
	{
		real ab = dot_product2d(a, b);
		real c = PIN((ab/aa_bb*ab) + (ab/aa_bb*ab) - 1.f, -1.f, 1.f);

		angle = arccosine(c) * 0.5f;
		
		if (ab < 0.f)
		{
			angle = _pi - angle;
		}
	}

	return angle;
}

real angle_between_vectors3d(
	real_vector3d const *a,
	real_vector3d const *b)
{

	real angle = 0.f;
	real aa_bb = (a->i*a->i + a->j*a->j + a->k*a->k) * (b->i*b->i + b->j*b->j + b->k*b->k);

	if (aa_bb != 0.f)
	{
		real ab = dot_product3d(a, b);
		real c = PIN((ab/aa_bb*ab) + (ab/aa_bb*ab) - 1.f, -1.f, 1.f);

		angle = arccosine(c) * 0.5f;

		if (ab < 0.f)
		{
			angle = _pi - angle;
		}
	}

	return angle;
}

real angle_between_normals3d(
	real_vector3d const *a,
	real_vector3d const *b)
{
	real result = 0.f;

	if ((*(long *)&a->i) == (*(long *)&b->i)
		&& (*(long *)&a->j) == (*(long *)&b->j)
		&& (*(long *)&a->k) == (*(long *)&b->k))
	{
		return result;
	}

	result = PIN(dot_product3d(a, b), -1.f, 1.f);
	return arccosine(result);
}

void yaw_vectors(
	real_vector3d *forward,
	real_vector3d const  *up,
	real sine,
	real cosine)
{
	real_vector3d cross;
	cross_product3d(up, forward, &cross);

	forward->i = forward->i*cosine + cross.i*sine;
	forward->j = forward->j*cosine + cross.j*sine;
	forward->k = forward->k*cosine + cross.k*sine;
	return;
}

void pitch_vectors(
	real_vector3d *forward,
	real_vector3d *up,
	real sine,
	real cosine)
{
	real_vector3d backward;
	negate_vector3d(forward, &backward);

	forward->i = forward->i*cosine + up->i*sine;
	forward->j = forward->j*cosine + up->j*sine;
	forward->k = forward->k*cosine + up->k*sine;

	up->i = up->i*cosine + backward.i*sine;
	up->j = up->j*cosine + backward.j*sine;
	up->k = up->k*cosine + backward.k*sine;
	return;
}

void fast_normals_interpolate(
	real_vector3d const *a,
	real_vector3d const *b,
	real t,
	real_vector3d *result)
{
	real one_minus_t = 1-t;
	result->i = t*b->i + one_minus_t * a->i;
	result->j = t*b->j + one_minus_t * a->j;
	result->k = t*b->k + one_minus_t * a->k;
	fast_normalize3d(result);
	return;
}

void normals_interpolate(
	real_vector3d const *a,
	real_vector3d const *b,
	real t,
	real_vector3d *result)
{
	real angle;
	real_vector3d c;

	angle = angle_between_vectors3d(a, b);

	if (t <= 0.5f)
	{
		angle *= t;
		cross_product3d(a, b, &c);
		*result = *a;
	}
	else
	{
		angle *= (1-t);
		cross_product3d(b, a, &c);
		*result = *b;
	}

	if (normalize3d(&c) != 0.0f)
	{
		rotate_vector_about_axis(result, &c, sine(angle), cosine(angle));
	}

	return;
}

real_vector3d *reflect_vector3d(
	real_vector3d const *incident,
	real_vector3d const *normal,
	real_vector3d *reflection)
{
	real incident_dot_normal = dot_product3d(incident, normal);
	real twice_incident_dot_normal = incident_dot_normal + incident_dot_normal;
	reflection->i = incident->i - normal->i*twice_incident_dot_normal;
	reflection->j = incident->j - normal->j*twice_incident_dot_normal;
	reflection->k = incident->k - normal->k*twice_incident_dot_normal;
	return reflection;
}

real_vector3d *refract_vector3d(
	real_vector3d const *incident,
	real_vector3d const *normal,
	real coefficient_of_refraction,
	real_vector3d *refraction)
{
	real refraction_scale;

	real incident_magnitude = magnitude3d(incident);
	real one_over_incident_magnitude = 1.f / incident_magnitude;

	real sine_incident = cross_product_magnitude3d(incident, normal) * one_over_incident_magnitude;
	real cosine_incident = dot_product3d(incident, normal) * one_over_incident_magnitude;
	real sine_refraction = coefficient_of_refraction * sine_incident;

	real cosine_refraction = square_root(1.f - sine_refraction * sine_refraction);
	
	real delta = (sine_incident * cosine_refraction + cosine_incident * sine_refraction) / sine_refraction;

	refraction->i = delta * normal->i + incident->i;
	refraction->j = delta * normal->j + incident->j;
	refraction->k = delta * normal->k + incident->k;

	refraction_scale = incident_magnitude / magnitude3d(refraction);
	
	refraction->i *= refraction_scale;
	refraction->j *= refraction_scale;
	refraction->k *= refraction_scale;

	return refraction;
}

void quaternion_normalize(
	real_quaternion *q)
{
	real magnitude_squared = q->v.i*q->v.i + q->v.j*q->v.j + q->v.k*q->v.k + q->w*q->w;
	if (magnitude_squared > 0.f)
	{
		real one_over_magnitude = reciprocal_square_root(magnitude_squared);
		set_real_quaternion(q, one_over_magnitude*q->v.i, one_over_magnitude*q->v.j, one_over_magnitude*q->v.k, one_over_magnitude*q->w);
	}
	else
	{
		set_real_quaternion(q, 0.f, 0.f, 0.f, 1.f);
	}
	return;
}

void quaternion_from_angle_and_vector(
	real_quaternion *q,
	real a,
	real_vector3d const *v)
{
	real half_angle = a * 0.5f;
	real sine_half_angle = sine(half_angle);

	q->w = cosine(half_angle);
	q->v.i = sine_half_angle*v->i;
	q->v.j = sine_half_angle*v->j;
	q->v.k = sine_half_angle*v->k;

	return;
}

void quaternion_to_angle_and_vector(
	real_quaternion const *q,
	real *a,
	real_vector3d *v)
{
	*v = q->v;
	*a = 2 * arctangent(normalize3d(v), q->w) ;
	
	if (*a > _pi)
	{
		negate_vector3d(v, v);
		*a = _pi * 2.0f - *a;
	}

	return;
}

void quaternions_interpolate_and_normalize(
	real_quaternion const *q0,
	real_quaternion const *q1,
	real t,
	real_quaternion *result)
{
	quaternions_interpolate(q0, q1, t, result);
	quaternion_normalize(result);
	return;
}

void orientations_interpolate(
	real_orientation const *a,
	real_orientation const *b,
	real t,
	real_orientation *result)
{
	quaternions_interpolate(&a->rotation, &b->rotation, t, &result->rotation);
	quaternion_normalize(&result->rotation);

	result->translation.x = a->translation.x*(1.f - t) + b->translation.x*t;
	result->translation.y = a->translation.y*(1.f - t) + b->translation.y*t;
	result->translation.z = a->translation.z*(1.f - t) + b->translation.z*t;
	result->scale = a->scale*(1.f - t) + b->scale*t;
	return;
}

real_euler_angles2d *euler_angles2d_from_vector3d(
	real_euler_angles2d *angles,
	real_vector3d const *vector)
{
	angles->yaw = arctangent(vector->j, vector->i);
	angles->pitch = arctangent(vector->k, square_root(vector->i*vector->i+vector->j*vector->j));
	return angles;
}

real_vector3d *vector3d_from_angle(
	real_vector3d *vector,
	real angle)
{
	vector->k = 0.f;
	vector->i = cosine(angle);
	vector->j = sine(angle);
	return vector;
}

real_vector3d *vector3d_from_euler_angles2d(
	real_vector3d *vector,
	real_euler_angles2d const *angles)
{
	real cosine_pitch = cosine(angles->pitch);
	vector->i = cosine(angles->yaw)* cosine_pitch;
	vector->j = sine(angles->yaw)* cosine_pitch;
	vector->k = sine(angles->pitch);
	return vector;
}

boolean point_in_pill2d(
	real_point2d const *point,
	real_point2d const *base,
	real_vector2d const *height,
	real width)
{
	real_vector2d v;
	real height_magnitude_squared;
	real t;

	vector_from_points2d(base, point, &v);
	height_magnitude_squared = dot_product2d(&v, height) / magnitude_squared2d(height);
	t = -PIN(height_magnitude_squared, 0.f, 1.f);
	point_from_line2d((real_point2d *)&v, height, t, (real_point2d *)&v);

	return magnitude_squared2d(&v)<=width*width;
}

real point_to_line_distance_squared3d(
	real_point3d const *point,
	real_point3d const *base,
	real_vector3d const *height)
{
	real t;
	real distance_squared;
	real_vector3d v;
	
	vector_from_points3d(base, point, &v);
	t = PIN(dot_product3d(&v, height) / magnitude_squared3d(height), 0.f, 1.f);
	point_from_line3d((real_point3d *)&v, height, -t, (real_point3d *)&v);
	distance_squared = magnitude_squared3d(&v);

	return distance_squared;
}

boolean sphere_test_vector3d(
	real_point3d const *center,
	real radius,
	real_point3d const *point,
	real_vector3d const *vector,
	real *t,
	real_vector3d *normal)
{
	real_vector3d x;
	real xv;

	vector_from_points3d(center, point, &x);
	xv = dot_product3d(&x, vector);

	if (xv < 0)
	{
		real xx = magnitude_squared3d(&x);
		real xx_rr = xx - radius*radius;

		if (xx_rr <= 0)
		{
			*t = 0;
			scale_vector3d(&x, reciprocal_square_root(xx), normal);
			return TRUE;
		}
		else
		{
			real d = vector->i*vector->i + vector->j*vector->j + vector->k*vector->k;
			real vv = xv*xv - d*xx_rr;

			if (vv >= 0)
			{
				real n = -(square_root(vv) + xv);

				if (n <= d)
				{
					*t = n/d;
					fast_normalize3d((real_vector3d *)point_from_line3d((real_point3d *)&x, vector, *t, (real_point3d *)normal));
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

boolean accelerate_to_velocity3d(
	real_vector3d *velocity,
	real_vector3d const *target_velocity,
	real acceleration)
{
	real_vector3d delta;
	boolean result;

	subtract_vectors3d(target_velocity, velocity, &delta);
	if (limit3d(&delta, acceleration))
	{
		add_vectors3d(velocity, &delta, velocity);
		result = FALSE;
	}
	else
	{
		*velocity = *target_velocity;
		result = TRUE;
	}

	return result;
}

boolean valid_real_sine_cosine(
	real sine, 
	real cosine)
{
	return valid_realcmp(sine * sine + cosine * cosine, 1.0f);
}

/* ---------- private code */
