#ifndef HALO_PORT_TAG_PREVIEW_H
#define HALO_PORT_TAG_PREVIEW_H

#include "cseries.h"
#include "bitmaps/bitmaps.h"

#define TAG_PREVIEW_WIDTH 128
#define TAG_PREVIEW_HEIGHT 128

struct tag_injector_texture
{
	short width;
	short height;
	short format;
	unsigned long size;
	unsigned char *pixels;
};

struct tag_injector_material
{
	short texture_index;
	short color_source;
	float u_scale;
	float v_scale;
	unsigned char base_map_alpha_Replicates;
};

struct tag_injector_mesh
{
	float (*positions)[3];
	float (*normals)[3];
	float (*texcoords)[2];
	long vertex_count;
	long (*triangles)[3];
	short *triangle_materials;
	long triangle_count;
	struct tag_injector_material *materials;
	long material_count;
	struct tag_injector_texture *textures;
	long texture_count;
	float color_a[3];
	float color_b[3];
};

void tag_preview_mesh_free(struct tag_injector_mesh *mesh);
void tag_preview_select(int mode, int category_tab, int list_index, int revision);
void tag_preview_clear(void);
struct bitmap_data *tag_preview_picture(void);

#endif
