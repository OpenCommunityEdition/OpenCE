/* Loading-only material selection; no geometry or texture pixels are drawn. */
#ifndef MATERIAL_PREPARE_H
#define MATERIAL_PREPARE_H
struct bitmap_data;
struct shader;
void material_prepare_map(void);
int material_prepare_draw(void);
int material_prepare_skip_shader(struct shader const *shader);
int material_prepare_bitmap(struct bitmap_data *bitmap, void **header);
int render_material_prepare_mode(int enabled);
void render_material_prepare_draw(void);
int texture_cache_prepare_descriptor(struct bitmap_data *bitmap, void *header);
#endif
