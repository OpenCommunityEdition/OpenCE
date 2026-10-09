/* port: whether a closed convex shell holds every vertex of another part
(models_fix_transparent_part_links, once as a map's tags load); a position is
the first three reals of both model vertex formats */
#ifndef __RASTERIZER_TRANSPARENT_ENCLOSURE_H
#define __RASTERIZER_TRANSPARENT_ENCLOSURE_H

static boolean rasterizer_transparent_encloses(
	byte const *outer, long outer_count, long outer_stride,
	word const *indices, long triangle_count, boolean strip,
	byte const *inner, long inner_count, long inner_stride)
{
	real points[64][3], center[3] = {0}, extent = 0, epsilon;
	short welded[64], triangles[128][3];
	long i, j, k, unique_count = 0, face_count = 0;

	if (!outer || !inner || !indices || outer_count < 4 || outer_count > 64 ||
		inner_count <= 0 || inner_count > 1024 || triangle_count < 4 || triangle_count > 128 ||
		outer_stride < 3 * sizeof(real) || inner_stride < 3 * sizeof(real))
	{
		return FALSE;
	}
	for (i = 0; i < outer_count; ++i)
	{
		real const *p = (real const *)(outer + i * outer_stride);
		for (k = 0; k < 3; ++k)
		{
			if (!(p[k] > -1000000.0f && p[k] < 1000000.0f))
			{
				return FALSE;
			}
			center[k] += p[k] / outer_count;
		}
	}
	for (i = 0; i < outer_count; ++i)
	{
		for (k = 0; k < 3; ++k)
		{
			extent = MAX(extent, fabs(((real const *)(outer + i * outer_stride))[k] - center[k]));
		}
	}
	if (!(extent > 0.000001f))
	{
		return FALSE;
	}
	epsilon = extent * 0.00001f;
	for (i = 0; i < outer_count; ++i)
	{
		real const *p = (real const *)(outer + i * outer_stride);
		for (j = 0; j < unique_count; ++j)
		{
			if (fabs(p[0] - points[j][0]) < epsilon &&
				fabs(p[1] - points[j][1]) < epsilon && fabs(p[2] - points[j][2]) < epsilon)
			{
				break;
			}
		}
		welded[i] = (short)j;
		if (j == unique_count)
		{
			memcpy(points[j], p, sizeof(points[j]));
			++unique_count;
		}
	}
	for (i = 0; i < triangle_count; ++i)
	{
		long start = strip ? i : 3 * i;
		short ids[3];
		real ab[3], ac[3], normal[3], length, distance = 0;
		for (k = 0; k < 3; ++k)
		{
			word index = indices[start + k];
			if (index >= outer_count)
			{
				return FALSE;
			}
			ids[k] = welded[index];
		}
		if (strip && (i & 1))
		{
			short swap = ids[0];
			ids[0] = ids[1];
			ids[1] = swap;
		}
		if (ids[0] == ids[1] || ids[1] == ids[2] || ids[2] == ids[0])
		{
			continue;
		}
		for (k = 0; k < 3; ++k)
		{
			ab[k] = points[ids[1]][k] - points[ids[0]][k];
			ac[k] = points[ids[2]][k] - points[ids[0]][k];
		}
		normal[0] = ab[1] * ac[2] - ab[2] * ac[1];
		normal[1] = ab[2] * ac[0] - ab[0] * ac[2];
		normal[2] = ab[0] * ac[1] - ab[1] * ac[0];
		length = square_root(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
		if (!(length > epsilon * epsilon))
		{
			return FALSE;
		}
		for (k = 0; k < 3; ++k)
		{
			normal[k] /= length;
			distance += normal[k] * (center[k] - points[ids[0]][k]);
		}
		/* Halo's model winding points inward. Require convexity and strict
		   containment, so intersecting or reversed meshes keep their order. */
		if (!(distance > epsilon))
		{
			return FALSE;
		}
		for (j = 0; j < unique_count + inner_count; ++j)
		{
			real const *p = j < unique_count ? points[j] : (real const *)(inner + (j - unique_count) * inner_stride);
			distance = 0;
			for (k = 0; k < 3; ++k)
			{
				if (!(p[k] > -1000000.0f && p[k] < 1000000.0f))
				{
					return FALSE;
				}
				distance += normal[k] * (p[k] - points[ids[0]][k]);
			}
			if (!(distance >= (j < unique_count ? -epsilon : epsilon)))
			{
				return FALSE;
			}
		}
		memcpy(triangles[face_count++], ids, sizeof(ids));
	}
	if (face_count < 4)
	{
		return FALSE;
	}
	/* Every welded edge needs exactly one reverse edge. */
	for (i = 0; i < face_count; ++i)
	{
		for (k = 0; k < 3; ++k)
		{
			short a = triangles[i][k], b = triangles[i][(k + 1) % 3];
			long reverse = 0, forward = 0, edge;
			for (j = 0; j < face_count; ++j)
			{
				for (edge = 0; edge < 3; ++edge)
				{
					short c = triangles[j][edge], d = triangles[j][(edge + 1) % 3];
					reverse += c == b && d == a;
					forward += c == a && d == b;
				}
			}
			if (reverse != 1 || forward != 1)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

#endif
