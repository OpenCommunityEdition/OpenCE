/* Minimal reproducer for the breakable_surfaces cross-product operand tie.

   Faithful to _breakable_surface_effect: BOTH cross operands are stack
   locals (surface_plane is filled by a call, s_normal is computed), and the
   two plane constructions follow the cross, so the s_plane.n = s_normal
   copy can be scheduled into the cross-product code.

   January emits, for the two products containing s_normal.k:
     fld <plane member>; fmul <s_normal.k>
   our build emits the operands the other way round.  */

#include "cseries.h"
#include "math/real_math.h"

extern real_plane3d *probe_get_plane(long designator, real_plane3d *result);

void probe_cross(
	real_point3d const *p0,
	real_point3d const *p1,
	long plane_designator,
	real_point3d const *origin,
	real_plane3d *out_s,
	real_plane3d *out_t)
{
	real_plane3d surface_plane;
	real_vector3d s_normal;
	real_vector3d t_normal;
	real_plane3d s_plane;
	real_plane3d t_plane;

	probe_get_plane(plane_designator, &surface_plane);

	vector_from_points3d(p0, p1, &s_normal);
	normalize3d(&s_normal);

	cross_product3d(&s_normal, &surface_plane.n, &t_normal);

	plane3d_from_point_and_normal(&s_plane, origin, &s_normal);
	plane3d_from_point_and_normal(&t_plane, origin, &t_normal);

	*out_s = s_plane;
	*out_t = t_plane;
	return;
}
