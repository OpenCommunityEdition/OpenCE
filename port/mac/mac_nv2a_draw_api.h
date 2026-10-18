#ifndef HALO_MAC_NV2A_DRAW_API_H
#define HALO_MAC_NV2A_DRAW_API_H
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
/* Native draw view of the shared xgpu.h constants. */
#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define XGPU_VERTEX_CONSTANT_COUNT 192
#define XGPU_VERTEX_CONSTANT_BIAS 96
struct nv2a_vertex_lighting
{
    int lights;
    unsigned long normal_instruction,normal_register;
    unsigned long position_instruction,position_register;
};
struct nv2a_pixel_shader_key
{
    DWORD combiner_state[D3DRS_PS_MAX];
    DWORD texture_modes;
    unsigned char sampler_type[4],alpha_kill[4],color_sign[4];
    unsigned long alpha_test_function;
    unsigned char fog_enable,fog_table_mode,count_samples,coverage_alpha;
    unsigned char point_threshold,per_pixel_lighting,alpha_test_samples,source_blend;
};
BOOL nv2a_vertex_shader_lighting(const DWORD *,unsigned long,struct nv2a_vertex_lighting *);
char *nv2a_vertex_shader_to_glsl(const DWORD *,unsigned long,unsigned long,const struct nv2a_vertex_lighting *);
char *nv2a_pixel_shader_to_glsl(const struct nv2a_pixel_shader_key *);
#endif
