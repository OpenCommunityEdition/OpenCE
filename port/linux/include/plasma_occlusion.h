/* Bounded, render-only collision visibility for plasma lights. */
#ifndef PLASMA_OCCLUSION_H
#define PLASMA_OCCLUSION_H
void plasma_occlusion_reset(void);
void plasma_occlusion_window(void);
void plasma_occlusion_submit(long light, long rasterizer_index);
void plasma_occlusion_model_slot(long rasterizer_index, short slot);
void *plasma_occlusion_texture(long rasterizer_index, int specular);
void *plasma_occlusion_model_texture(int slot, float transform[4]);
unsigned long plasma_occlusion_view_serial(void);
/* Owned by the linked program; locations cannot outlive or alias its GL name. */
struct plasma_occlusion_uniforms
{
	int enabled, transform[2];
	unsigned int known;
	float last_transform[2][4], last_enabled[2];
};
void plasma_occlusion_model_initialize(unsigned long program, struct plasma_occlusion_uniforms *uniforms);
void plasma_occlusion_bind_model(struct plasma_occlusion_uniforms *uniforms);
float plasma_occlusion_specular_scale(long rasterizer_index);
#endif
