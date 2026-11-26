/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "tool/error_geometry.h"

/* ---------- constants */

#define MAXIMUM_ERROR_GEOMETRY_FILENAME_LENGTH 63
#define ERROR_GEOMETRY_FILENAME_SUFFIX ".wrl"
#define MAXIMUM_ERROR_GEOMETRY_NAME_LENGTH (MAXIMUM_ERROR_GEOMETRY_FILENAME_LENGTH-(sizeof(ERROR_GEOMETRY_FILENAME_SUFFIX)-1))

/* world units are scaled by this factor before being written out */
#define ERROR_GEOMETRY_SCALE 100.f

#define ERROR_GEOMETRY_POINT_RADIUS 0.01f

/* ---------- macros */

/* ---------- structures */

struct error_geometry_globals
{
	char filename[MAXIMUM_ERROR_GEOMETRY_FILENAME_LENGTH+1];
	real_matrix4x3 transform;
};

typedef char verify_error_geometry_globals_size[
	sizeof(struct error_geometry_globals) == 0x74 ? 1 : -1];
typedef char verify_error_geometry_transform_offset[
	offsetof(struct error_geometry_globals, transform) == 0x40 ? 1 : -1];

/* ---------- prototypes */

static boolean error_geometry_file_is_open(
	void);

/* ---------- globals */

static FILE *error_geometry_file = NULL;

static struct error_geometry_globals error_geometry_globals =
{
	"debug.wrl",
	{
		1.f,
		{
			{ 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f }
		}
	}
};

/* ---------- private code */

static boolean error_geometry_file_is_open(
	void)
{
	if (!error_geometry_file)
	{
		error_geometry_file= fopen(error_geometry_globals.filename, "w");
		if (error_geometry_file)
		{
			fprintf(error_geometry_file, "#VRML V1.0 ascii\n\n");
			fflush(error_geometry_file);
		}
	}

	return error_geometry_file!=NULL;
}

/* ---------- public code */

void error_geometry_initialize(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		0x44,
		error_geometry_file==NULL);
	remove(error_geometry_globals.filename);
	return;
}

void error_geometry_dispose(
	void)
{
	if (error_geometry_file)
	{
		fclose(error_geometry_file);
		error_geometry_file = NULL;
	}

	return;
}

void error_geometry_set_name(
	char const *name)
{
	if (strncmp(error_geometry_globals.filename, name, MAXIMUM_ERROR_GEOMETRY_NAME_LENGTH)!=0)
	{
		error_geometry_dispose();
		strncpy(error_geometry_globals.filename, name, MAXIMUM_ERROR_GEOMETRY_NAME_LENGTH);
		error_geometry_globals.filename[MAXIMUM_ERROR_GEOMETRY_NAME_LENGTH]= 0;
		strcat(error_geometry_globals.filename, ERROR_GEOMETRY_FILENAME_SUFFIX);
		error_geometry_initialize();
	}

	return;
}

void error_geometry_set_transform(
	real_matrix4x3 const *matrix)
{
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		0x6B,
		matrix);
	match_assert_valid_real_matrix4x3(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		0x6C,
		matrix);
	error_geometry_globals.transform= *matrix;

	return;
}

void error_geometry_point(
	real_point3d const *point,
	real_argb_color const *color)
{
	real_rectangle3d bounds;

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		119,
		point);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		120,
		color);

	if (error_geometry_file_is_open())
	{
		bounds.x0= point->x - ERROR_GEOMETRY_POINT_RADIUS;
		bounds.x1= point->x + ERROR_GEOMETRY_POINT_RADIUS;
		bounds.y0= point->y - ERROR_GEOMETRY_POINT_RADIUS;
		bounds.y1= point->y + ERROR_GEOMETRY_POINT_RADIUS;
		bounds.z0= point->z - ERROR_GEOMETRY_POINT_RADIUS;
		bounds.z1= point->z + ERROR_GEOMETRY_POINT_RADIUS;
		error_geometry_rectangle3d(&bounds, color);
	}

	return;
}

void error_geometry_line(
	real_point3d const *p0,
	real_point3d const *p1,
	real_argb_color const *color)
{
	real_point3d points[2];

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		143,
		p0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		144,
		p1);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		145,
		color);

	if (error_geometry_file_is_open())
	{
		matrix4x3_transform_point(&error_geometry_globals.transform, p0, &points[0]);
		matrix4x3_transform_point(&error_geometry_globals.transform, p1, &points[1]);

		fprintf(error_geometry_file, "Separator\n{\n");
		fprintf(error_geometry_file, "\tCoordinate3 { point[%f %f %f, %f %f %f] }\n",
			points[0].x*ERROR_GEOMETRY_SCALE, points[0].y*ERROR_GEOMETRY_SCALE, points[0].z*ERROR_GEOMETRY_SCALE,
			points[1].x*ERROR_GEOMETRY_SCALE, points[1].y*ERROR_GEOMETRY_SCALE, points[1].z*ERROR_GEOMETRY_SCALE);
		fprintf(error_geometry_file, "\tMaterialBinding { value PER_VERTEX }\n");
		fprintf(error_geometry_file, "\tMaterial { diffuseColor[%f %f %f, %f %f %f] transparency[%f, %f] }\n",
			color->red, color->green, color->blue,
			color->red, color->green, color->blue,
			1.f-color->alpha, 1.f-color->alpha);
		fprintf(error_geometry_file, "\tIndexedLineSet { coordIndex[0,1,-1] }\n");
		fprintf(error_geometry_file, "}\n");
		fflush(error_geometry_file);
	}

	return;
}

void error_geometry_triangle(
	real_point3d const *p0,
	real_point3d const *p1,
	real_point3d const *p2,
	real_argb_color const *color)
{
	real_point3d points[3];

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		177,
		p0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		178,
		p1);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		179,
		p2);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		180,
		color);

	if (error_geometry_file_is_open())
	{
		matrix4x3_transform_point(&error_geometry_globals.transform, p0, &points[0]);
		matrix4x3_transform_point(&error_geometry_globals.transform, p1, &points[1]);
		matrix4x3_transform_point(&error_geometry_globals.transform, p2, &points[2]);

		fprintf(error_geometry_file, "Separator\n{\n");
		fprintf(error_geometry_file, "\tCoordinate3 { point[%f %f %f, %f %f %f, %f %f %f] }\n",
			points[0].x*ERROR_GEOMETRY_SCALE, points[0].y*ERROR_GEOMETRY_SCALE, points[0].z*ERROR_GEOMETRY_SCALE,
			points[1].x*ERROR_GEOMETRY_SCALE, points[1].y*ERROR_GEOMETRY_SCALE, points[1].z*ERROR_GEOMETRY_SCALE,
			points[2].x*ERROR_GEOMETRY_SCALE, points[2].y*ERROR_GEOMETRY_SCALE, points[2].z*ERROR_GEOMETRY_SCALE);
		fprintf(error_geometry_file, "\tMaterialBinding { value PER_FACE }\n");
		fprintf(error_geometry_file, "\tMaterial { diffuseColor[%f %f %f] transparency[%f] }\n",
			color->red, color->green, color->blue, 1.f-color->alpha);
		fprintf(error_geometry_file, "\tIndexedFaceSet { coordIndex[0,1,2,-1] }\n");
		fprintf(error_geometry_file, "}\n");
		fflush(error_geometry_file);
	}

	return;
}

void error_geometry_polygon(
	short point_count,
	real_point3d const *points,
	real_argb_color const *color)
{
	real_point3d point;
	short point_index;

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		211,
		point_count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		212,
		points);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		213,
		color);

	if (point_count>=3)
	{
		if (error_geometry_file_is_open())
		{
			fprintf(error_geometry_file, "Separator\n{\n");
			fprintf(error_geometry_file, "\tCoordinate3 { point[");
			for (point_index= 0; point_index<point_count; ++point_index)
			{
				matrix4x3_transform_point(&error_geometry_globals.transform, &points[point_index], &point);
				fprintf(error_geometry_file, "%f %f %f%s",
					point.x*ERROR_GEOMETRY_SCALE, point.y*ERROR_GEOMETRY_SCALE, point.z*ERROR_GEOMETRY_SCALE,
					(point_index<point_count-1) ? ", " : "] }\n");
			}
			fprintf(error_geometry_file, "\tMaterialBinding { value PER_FACE }\n");
			fprintf(error_geometry_file, "\tMaterial { diffuseColor[%f %f %f] transparency[%f] }\n",
				color->red, color->green, color->blue, 1.f-color->alpha);
			fprintf(error_geometry_file, "\tIndexedFaceSet { coordIndex[");
			for (point_index= 0; point_index<point_count; ++point_index)
			{
				fprintf(error_geometry_file, "%d,", point_index);
			}
			fprintf(error_geometry_file, "-1] }\n");
			fprintf(error_geometry_file, "}\n");
			fflush(error_geometry_file);
		}
	}

	return;
}

void error_geometry_polygon_list(
	long polygon_count,
	short const *point_counts,
	real_point3d const *points,
	real_argb_color const *colors)
{
	real_point3d point;
	long point_index;
	long polygon_index;

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		332,
		polygon_count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		333,
		point_counts);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		334,
		points);

	if (polygon_count>0 && error_geometry_file_is_open())
	{
		fprintf(error_geometry_file, "Separator\n{\n");
		fprintf(error_geometry_file, "\tCoordinate3\n\t{\n\t\tpoint\n\t\t[\n");
		point_index = 0;
		for (polygon_index= 0; polygon_index<polygon_count; ++polygon_index)
		{
			short polygon_point_index;

			for (
				polygon_point_index= 0;
				polygon_point_index<point_counts[polygon_index];
				++polygon_point_index, ++point_index)
			{
				matrix4x3_transform_point(&error_geometry_globals.transform, &points[point_index], &point);
				fprintf(error_geometry_file, "\t\t\t%f %f %f,\n",
					point.x*ERROR_GEOMETRY_SCALE,
					point.y*ERROR_GEOMETRY_SCALE,
					point.z*ERROR_GEOMETRY_SCALE);
			}
		}
		fprintf(error_geometry_file, "\t\t]\n\t}\n");
		fprintf(error_geometry_file, "\tMaterialBinding\n\t{\n\t\tvalue PER_FACE\n\t}\n");
		if (colors)
		{
			fprintf(error_geometry_file, "\tMaterial\n\t{\n\t\tdiffuseColor\n\t\t[\n");
			for (polygon_index= 0; polygon_index<polygon_count; ++polygon_index)
			{
				short triangle_index;

				for (triangle_index= 2; triangle_index<point_counts[polygon_index]; ++triangle_index)
				{
					fprintf(error_geometry_file, "\t\t\t%f %f %f, ",
						colors[polygon_index].red,
						colors[polygon_index].green,
						colors[polygon_index].blue);
				}
				fprintf(error_geometry_file, "\n");
			}
			fprintf(error_geometry_file, "\t\t]\n\t\ttransparency[%f]\n\t}\n", 1.f-colors->alpha);
		}
		fprintf(error_geometry_file, "\tIndexedFaceSet\n\t{\n\t\tcoordIndex\n\t\t[\n");
		point_index = 0;
		for (polygon_index= 0; polygon_index<polygon_count; ++polygon_index)
		{
			short triangle_index;

			fprintf(error_geometry_file, "\t\t\t");
			for (triangle_index= 2; triangle_index<point_counts[polygon_index]; ++triangle_index)
			{
				fprintf(error_geometry_file, "%d,%d,%d,-1, ",
					point_index,
					point_index+triangle_index-1,
					point_index+triangle_index);
			}
			fprintf(error_geometry_file, "\n");
			point_index+= point_counts[polygon_index];
		}
		fprintf(error_geometry_file, "\t\t]\n\t}\n}\n");
		fflush(error_geometry_file);
	}

	return;
}

void error_geometry_polygon_mesh__textured_with_no_import_scale(
	long width,
	long height,
	real_point3d const *points,
	real_point2d const *texcoords)
{
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		428,
		width>0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		429,
		height>0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		430,
		points);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		431,
		texcoords);

	if (error_geometry_file_is_open())
	{
		long point_count;
		long point_index, x, y;

		fprintf(error_geometry_file, "Separator\n{\n");
		fprintf(error_geometry_file, "\tCoordinate3\n\t{\n\t\tpoint\n\t\t[\n");
		point_count= width*height;
		for (point_index= 0; point_index<point_count; ++point_index)
		{
			fprintf(error_geometry_file, "\t\t\t%f %f %f,\n",
				points[point_index].x, points[point_index].y, points[point_index].z);
		}
		fprintf(error_geometry_file, "\t\t]\n\t}\n");
		fprintf(error_geometry_file, "\tTextureCoordinate\n\t{\n\t\tpoint\n\t\t[\n");
		for (point_index= 0; point_index<point_count; ++point_index)
		{
			fprintf(error_geometry_file, "\t\t\t%f %f,\n",
				texcoords[point_index].x, texcoords[point_index].y);
		}
		fprintf(error_geometry_file, "\t\t]\n\t}\n");
		fprintf(error_geometry_file, "\tMaterialBinding\n\t{\n\t\tvalue PER_FACE\n\t}\n");
		fprintf(error_geometry_file, "\tIndexedFaceSet\n\t{\n\t\tcoordIndex\n\t\t[\n");
		for (y= 0; y<height-1; ++y)
		{
			for (x= 0; x<width-1; ++x)
			{
				fprintf(error_geometry_file, "\t\t\t");
				fprintf(error_geometry_file, "%d,%d,%d,%d,-1,",
					y*width+x, y*width+x+1, (y+1)*width+x+1, (y+1)*width+x);
				fprintf(error_geometry_file, "\n");
			}
		}
		fprintf(error_geometry_file, "\t\t]\n\t}\n}\n");
		fflush(error_geometry_file);
	}

	return;
}

void error_geometry_rectangle3d(
	real_rectangle3d const *bounds,
	real_argb_color const *color)
{
	real_point3d points[4];

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		491,
		bounds);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		492,
		color);

	if (error_geometry_file_is_open())
	{
		set_real_point3d(&points[0], bounds->x0, bounds->y0, bounds->z0);
		set_real_point3d(&points[1], bounds->x0, bounds->y1, bounds->z0);
		set_real_point3d(&points[2], bounds->x0, bounds->y1, bounds->z1);
		set_real_point3d(&points[3], bounds->x0, bounds->y0, bounds->z1);
		error_geometry_polygon(NUMBEROF(points), points, color);

		set_real_point3d(&points[0], bounds->x1, bounds->y0, bounds->z0);
		set_real_point3d(&points[1], bounds->x1, bounds->y1, bounds->z0);
		set_real_point3d(&points[2], bounds->x1, bounds->y1, bounds->z1);
		set_real_point3d(&points[3], bounds->x1, bounds->y0, bounds->z1);
		error_geometry_polygon(NUMBEROF(points), points, color);

		set_real_point3d(&points[0], bounds->x0, bounds->y0, bounds->z0);
		set_real_point3d(&points[1], bounds->x1, bounds->y0, bounds->z0);
		set_real_point3d(&points[2], bounds->x1, bounds->y0, bounds->z1);
		set_real_point3d(&points[3], bounds->x0, bounds->y0, bounds->z1);
		error_geometry_polygon(NUMBEROF(points), points, color);

		set_real_point3d(&points[0], bounds->x0, bounds->y1, bounds->z0);
		set_real_point3d(&points[1], bounds->x1, bounds->y1, bounds->z0);
		set_real_point3d(&points[2], bounds->x1, bounds->y1, bounds->z1);
		set_real_point3d(&points[3], bounds->x0, bounds->y1, bounds->z1);
		error_geometry_polygon(NUMBEROF(points), points, color);

		set_real_point3d(&points[0], bounds->x0, bounds->y0, bounds->z0);
		set_real_point3d(&points[1], bounds->x0, bounds->y1, bounds->z0);
		set_real_point3d(&points[2], bounds->x1, bounds->y1, bounds->z0);
		set_real_point3d(&points[3], bounds->x1, bounds->y0, bounds->z0);
		error_geometry_polygon(NUMBEROF(points), points, color);

		set_real_point3d(&points[0], bounds->x0, bounds->y0, bounds->z1);
		set_real_point3d(&points[1], bounds->x0, bounds->y1, bounds->z1);
		set_real_point3d(&points[2], bounds->x1, bounds->y1, bounds->z1);
		set_real_point3d(&points[3], bounds->x1, bounds->y0, bounds->z1);
		error_geometry_polygon(NUMBEROF(points), points, color);
	}

	return;
}

void error_geometry_bounded_point(
	real_point3d const *point,
	real radius,
	real_argb_color const *color)
{
	real_rectangle3d bounds;
	real_argb_color bounds_color;

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		539,
		point);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		540,
		color);

	if (error_geometry_file_is_open())
	{
		bounds.x0= point->x - radius;
		bounds.x1= point->x + radius;
		bounds.y0= point->y - radius;
		bounds.y1= point->y + radius;
		bounds.z0= point->z - radius;
		bounds.z1= point->z + radius;
		bounds_color.rgb= color->rgb;
		bounds_color.alpha= color->alpha*0.5f;
		error_geometry_rectangle3d(&bounds, &bounds_color);
		error_geometry_point(point, color);
	}

	return;
}

void error_geometry_bounded_line(
	real_point3d const *p0,
	real_point3d const *p1,
	real radius,
	real_argb_color const *color)
{
	real_rectangle3d bounds;
	real_argb_color bounds_color;

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		567,
		p0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		568,
		p1);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		569,
		color);

	if (error_geometry_file_is_open())
	{
		bounds.x0= MIN(p0->x, p1->x) - radius;
		bounds.x1= MAX(p0->x, p1->x) + radius;
		bounds.y0= MIN(p0->y, p1->y) - radius;
		bounds.y1= MAX(p0->y, p1->y) + radius;
		bounds.z0= MIN(p0->z, p1->z) - radius;
		bounds.z1= MAX(p0->z, p1->z) + radius;
		bounds_color.rgb= color->rgb;
		bounds_color.alpha= color->alpha*0.5f;
		error_geometry_rectangle3d(&bounds, &bounds_color);
		error_geometry_line(p0, p1, color);
	}

	return;
}

void error_geometry_bounded_triangle(
	real_point3d const *p0,
	real_point3d const *p1,
	real_point3d const *p2,
	real radius,
	real_argb_color const *color)
{
	real_rectangle3d bounds;
	real_argb_color bounds_color;

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		597,
		p0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		598,
		p1);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		599,
		p2);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		600,
		color);

	if (error_geometry_file_is_open())
	{
		bounds.x0= MIN(p0->x, MIN(p1->x, p2->x)) - radius;
		bounds.x1= MAX(p0->x, MAX(p1->x, p2->x)) + radius;
		bounds.y0= MIN(p0->y, MIN(p1->y, p2->y)) - radius;
		bounds.y1= MAX(p0->y, MAX(p1->y, p2->y)) + radius;
		bounds.z0= MIN(p0->z, MIN(p1->z, p2->z)) - radius;
		bounds.z1= MAX(p0->z, MAX(p1->z, p2->z)) + radius;
		bounds_color.alpha= color->alpha*0.5f;
		bounds_color.rgb= color->rgb;
		error_geometry_rectangle3d(&bounds, &bounds_color);
		error_geometry_triangle(p0, p1, p2, color);
	}

	return;
}

void error_geometry_bounded_polygon(
	short point_count,
	real_point3d const *points,
	real radius,
	real_argb_color const *color)
{
	real_rectangle3d bounds;
	real_argb_color bounds_color;
	short point_index;

	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		627,
		point_count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		628,
		points);
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		629,
		color);

	if (point_count>=3)
	{
		if (error_geometry_file_is_open())
		{
			bounds.x0= REAL_MAX;
			bounds.x1= -REAL_MAX;
			bounds.y0= REAL_MAX;
			bounds.y1= -REAL_MAX;
			bounds.z0= REAL_MAX;
			bounds.z1= -REAL_MAX;
			for (point_index= 0; point_index<point_count; ++point_index)
			{
				real_point3d const *point= &points[point_index];

				bounds.x0= MIN(bounds.x0, point->x);
				bounds.x1= MAX(bounds.x1, point->x);
				bounds.y0= MIN(bounds.y0, point->y);
				bounds.y1= MAX(bounds.y1, point->y);
				bounds.z0= MIN(bounds.z0, point->z);
				bounds.z1= MAX(bounds.z1, point->z);
			}
			bounds.x0-= radius;
			bounds.x1+= radius;
			bounds.y0-= radius;
			bounds.y1+= radius;
			bounds.z0-= radius;
			bounds.z1+= radius;
			bounds_color.rgb= color->rgb;
			bounds_color.alpha= color->alpha*0.5f;
			error_geometry_rectangle3d(&bounds, &bounds_color);
			error_geometry_polygon(point_count, points, color);
		}
	}

	return;
}

void error_geometry_comment(
	char const *format,
	...)
{
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		669,
		format);

	if (error_geometry_file_is_open())
	{
		va_list arglist;

		fprintf(error_geometry_file, "#");
		va_start(arglist, format);
		vfprintf(error_geometry_file, format, arglist);
		va_end(arglist);
		fprintf(error_geometry_file, "\n");
		fflush(error_geometry_file);
	}

	return;
}
