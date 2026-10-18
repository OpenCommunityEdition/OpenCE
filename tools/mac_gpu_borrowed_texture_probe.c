#include "engine_text_gl.h"
#include "mac_gpu_textures.h"
#include "engine_device_state.h"
#include "engine_physical_memory.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#undef strcmp

static unsigned checks, failures, resolver_calls;
static int resolver_mode;
static struct D3DBaseTexture atlas_placeholder, unknown_texture;

#define CHECK(expression) do { \
    ++checks; \
    if (!(expression)) { \
        ++failures; \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
    } \
} while (0)

static int resolve_texture(void *context, const void *resource, unsigned char sampler,
    GLuint *name, GLenum *target, unsigned *levels, float scale[4], const char **error)
{
    const D3DBaseTexture *texture = (const D3DBaseTexture *)resource;
    unsigned int resolved_name = 0;
    int result;
    (void)context;
    ++resolver_calls;
    if (resource != &atlas_placeholder) return 0;
    if (resolver_mode < 0) {
        if (error) *error = "intentional resolver refusal";
        return -1;
    }
    if (resolver_mode > 0) return 7;
    if (sampler != MAC_GPU_TEXTURE_SAMPLER_2D) return -1;
    *target = GL_TEXTURE_2D;
    *levels = 1;
    result = mac_text_gl_resolve((unsigned long)texture->Data, &resolved_name, scale);
    *name = (GLuint)resolved_name;
    return result;
}

static void initialize_sampler(void)
{
    DWORD *state = D3D__TextureState[0];
    state[D3DTSS_ADDRESSU] = D3DTADDRESS_WRAP;
    state[D3DTSS_ADDRESSV] = D3DTADDRESS_MIRROR;
    state[D3DTSS_ADDRESSW] = D3DTADDRESS_CLAMP;
    state[D3DTSS_MINFILTER] = D3DTEXF_POINT;
    state[D3DTSS_MAGFILTER] = D3DTEXF_POINT;
    state[D3DTSS_MIPFILTER] = D3DTEXF_NONE;
    state[D3DTSS_MAXANISOTROPY] = 1;
    state[D3DTSS_MAXMIPLEVEL] = 0;
}

static void bind_placeholder(struct mac_gpu_texture_scope *scope,
    const void *resource, const char **error)
{
    const void *textures[D3DTSS_MAXSTAGES] = { NULL, NULL, NULL, NULL };
    unsigned char sampler[D3DTSS_MAXSTAGES];
    float scale[D3DTSS_MAXSTAGES][4];
    textures[0] = resource;
    CHECK(!mac_gpu_textures_bind(scope, textures, 1, sampler, scale, error));
}

static int arg_is(const char *value, const char *expected)
{
    while (*value && *expected && *value == *expected) { ++value; ++expected; }
    return !*value && !*expected;
}

static int invalid_cache_setting_probe(void)
{
    const void *textures[D3DTSS_MAXSTAGES] = { NULL, NULL, NULL, NULL };
    unsigned char sampler[D3DTSS_MAXSTAGES];
    float scale[D3DTSS_MAXSTAGES][4];
    struct mac_gpu_texture_scope scope;
    const char *error = NULL;
    CHECK(!mac_gpu_textures_bind(&scope, textures, 0, sampler, scale, &error));
    CHECK(error && arg_is(error, "HALO_MAC_RETAIL_TEXTURE_CACHE_16M must be unset or exactly 1"));
    printf("native retail texture cache invalid setting: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}

static int clear_cache_option(void)
{
    unsetenv("HALO_MAC_RETAIL_TEXTURE_CACHE_16M");
    return getenv("HALO_MAC_RETAIL_TEXTURE_CACHE_16M") == NULL;
}

int main(int argc, char **argv)
{
    CGLPixelFormatAttribute attrs[] = { kCGLPFAAccelerated,
        kCGLPFAColorSize, (CGLPixelFormatAttribute)24, 0 };
    CGLPixelFormatObj format = NULL;
    CGLContextObj context = NULL;
    GLint format_count = 0;
    struct mac_gpu_texture_scope scope;
    GLuint previous_texture = 0, atlas_name = 0, inactive_textures[3] = {0,0,0};
    GLint active_before, binding_before, pack_before, unpack_before;
    GLint active_after, binding_after, pack_after, unpack_after;
    GLint min_before, mag_before, wrap_s_before, wrap_t_before;
    GLint min_after, mag_after, wrap_s_after, wrap_t_after;
    GLint inactive_binding_before[3], inactive_binding_after;
    GLfloat inactive_bias_before, inactive_bias_after;
    GLint failed_stage1_bindings[3], failed_stage_binding;
    GLfloat failed_stage1_bias, failed_stage_bias;
    GLfloat stage0_bias_before, stage0_bias_after;
    const char *error = NULL;
    short widths[TEXT_HIRES_FIT_LETTERS];
    struct text_hires_glyph glyph;
    long font;
    unsigned i, calls;
    unsigned int resolved_name = 0;
    float resolved_scale[4];
    const char *cache_setting;
    int cache_large;

    if (argc == 2 && arg_is(argv[1], "--invalid-cache-setting"))
        return invalid_cache_setting_probe();

    cache_setting = getenv("HALO_MAC_RETAIL_TEXTURE_CACHE_16M");
    cache_large = cache_setting && cache_setting[0] == '1' && !cache_setting[1];
    if (cache_setting && cache_setting[0] && !cache_large) {
        fprintf(stderr, "invalid cache setting test requires --invalid-cache-setting\n");
        return 2;
    }

    memset(&atlas_placeholder, 0, sizeof(atlas_placeholder));
    memset(&unknown_texture, 0, sizeof(unknown_texture));
    memset(D3D__TextureState, 0, sizeof(D3D__TextureState));
    atlas_placeholder.Data = 0x1234;
    atlas_placeholder.Format = 2U << D3DFORMAT_DIMENSION_SHIFT;
    atlas_placeholder.Size = 255U | (255U << D3DSIZE_HEIGHT_SHIFT);
    unknown_texture.Data = 0x87654321U;
    unknown_texture.Format = (2U << D3DFORMAT_DIMENSION_SHIFT) | (0x04U << D3DFORMAT_FORMAT_SHIFT);
    unknown_texture.Size = atlas_placeholder.Size;
    CHECK(CGLChoosePixelFormat(attrs, &format, &format_count) == kCGLNoError && format);
    if (!format) return 1;
    CHECK(CGLCreateContext(format, NULL, &context) == kCGLNoError && context);
    if (!context) return 1;
    CHECK(CGLSetCurrentContext(context) == kCGLNoError);
    CHECK(mac_engine_physical_memory_start());
    CHECK(mac_device_headless_start(64, 64));
    initialize_sampler();
    CHECK(mac_text_gl_start(NULL));
    for (i = 0; i < TEXT_HIRES_FIT_LETTERS; ++i) widths[i] = 12;
    font = text_hires_font("ui\\large_ui", 20, widths, 1);
    CHECK(font >= 0);
    text_hires_register_atlas((const unsigned long *)&atlas_placeholder, 256, 256);
    CHECK(text_hires_glyph(font, 'A', &glyph));
    CHECK(mac_gpu_textures_set_borrowed_resolver(resolve_texture, NULL));

    glGenTextures(1, &previous_texture);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D, previous_texture);
    glPixelStorei(GL_PACK_ALIGNMENT, 8);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    CHECK(mac_text_gl_resolve((unsigned long)atlas_placeholder.Data, &resolved_name, resolved_scale) == 1);
    atlas_name = (GLuint)resolved_name;
    CHECK(atlas_name && glIsTexture(atlas_name));
    CHECK(resolved_scale[0] == 1.0f / 256.0f && resolved_scale[1] == 1.0f / 256.0f);
    glBindTexture(GL_TEXTURE_2D, atlas_name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_before);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag_before);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrap_s_before);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrap_t_before);
    glBindTexture(GL_TEXTURE_2D, previous_texture);
    glGenTextures(3, inactive_textures);
    glActiveTextureARB(GL_TEXTURE0_ARB + 3);
    glBindTexture(GL_TEXTURE_2D, inactive_textures[0]);
    glBindTexture(GL_TEXTURE_CUBE_MAP, inactive_textures[1]);
    glBindTexture(GL_TEXTURE_3D, inactive_textures[2]);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0.375f);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &inactive_binding_before[0]);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &inactive_binding_before[1]);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &inactive_binding_before[2]);
    glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, &inactive_bias_before);
    glActiveTextureARB(GL_TEXTURE0_ARB + 1);
    glBindTexture(GL_TEXTURE_2D, inactive_textures[0]);
    glBindTexture(GL_TEXTURE_CUBE_MAP, inactive_textures[1]);
    glBindTexture(GL_TEXTURE_3D, inactive_textures[2]);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -0.25f);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &failed_stage1_bindings[0]);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &failed_stage1_bindings[1]);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &failed_stage1_bindings[2]);
    glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, &failed_stage1_bias);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0.625f);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_before);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding_before);
    glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,
        &stage0_bias_before);
    glGetIntegerv(GL_PACK_ALIGNMENT, &pack_before);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_before);

    /* An empty sampler set must not query or rewrite any stage bindings. */
    {
        const void *textures[D3DTSS_MAXSTAGES] = { NULL, NULL, NULL, NULL };
        unsigned char sampler[D3DTSS_MAXSTAGES];
        float scale[D3DTSS_MAXSTAGES][4];
        CHECK(mac_gpu_textures_bind(&scope, textures, 0, sampler, scale, &error));
        CHECK(scope.changed_stages == 0U);
        {
            struct mac_gpu_texture_cache_stats stats;
            mac_gpu_textures_cache_get_stats(&stats);
            CHECK(stats.bytes_limit == (cache_large ? 16U : 8U) * 1024U * 1024U);
            CHECK(stats.entry_limit == (cache_large ? 256U : 128U));
            mac_gpu_textures_end(&scope);
            /* Cache policy is sampled once for the device/cache lifetime. */
            if (cache_large) CHECK(clear_cache_option());
            else CHECK(setenv("HALO_MAC_RETAIL_TEXTURE_CACHE_16M", "1", 1) == 0);
            CHECK(mac_gpu_textures_bind(&scope, textures, 0, sampler, scale, &error));
            mac_gpu_textures_cache_get_stats(&stats);
            CHECK(stats.bytes_limit == (cache_large ? 16U : 8U) * 1024U * 1024U);
            CHECK(stats.entry_limit == (cache_large ? 256U : 128U));
            mac_gpu_textures_end(&scope);
            if (cache_large) CHECK(setenv("HALO_MAC_RETAIL_TEXTURE_CACHE_16M", "1", 1) == 0);
            else CHECK(clear_cache_option());
        }
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_after);
        CHECK(active_after == active_before);
        mac_gpu_textures_end(&scope);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_after);
        CHECK(active_after == active_before);
        glActiveTextureARB(GL_TEXTURE0_ARB + 3);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &inactive_binding_after);
        if (inactive_binding_after != inactive_binding_before[0])
            fprintf(stderr,"inactive stage 3 changed during bind: 2D %d -> %d\n",
                (int)inactive_binding_before[0], (int)inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[0]);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[1]);
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[2]);
        glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,
            &inactive_bias_after);
        CHECK(inactive_bias_after == inactive_bias_before);
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }

    {
        const void *textures[D3DTSS_MAXSTAGES] = { &atlas_placeholder, NULL, NULL, NULL };
        unsigned char sampler[D3DTSS_MAXSTAGES];
        float scale[D3DTSS_MAXSTAGES][4];
        CHECK(mac_gpu_textures_bind(&scope, textures, 1, sampler, scale, &error));
        CHECK(!error && sampler[0] == MAC_GPU_TEXTURE_SAMPLER_2D);
        CHECK(scope.changed_stages == 1U);
        CHECK(scale[0][0] == 1.0f / 256.0f && scale[0][1] == 1.0f / 256.0f);
        CHECK(glIsTexture(atlas_name));
        glActiveTextureARB(GL_TEXTURE0_ARB + 3);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &inactive_binding_after);
        if (inactive_binding_after != inactive_binding_before[0])
            fprintf(stderr,"inactive stage 3 changed after partial failure: 2D %d -> %d\n",
                (int)inactive_binding_before[0], (int)inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[0]);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[1]);
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[2]);
        glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, &inactive_bias_after);
        CHECK(inactive_bias_after == inactive_bias_before);
        /* Simulate an intervening caller using a different stage. Borrowed
           sampler restoration must bind only on a stage in this scope. */
        glActiveTextureARB(GL_TEXTURE0_ARB + 3);
        mac_gpu_textures_end(&scope);
    }
    CHECK(glIsTexture(atlas_name));
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_after);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding_after);
    glGetIntegerv(GL_PACK_ALIGNMENT, &pack_after);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_after);
    CHECK(active_after == active_before && binding_after == binding_before);
    CHECK(pack_after == pack_before && unpack_after == unpack_before);
    glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,
        &stage0_bias_after);
    CHECK(stage0_bias_after == stage0_bias_before);
    glActiveTextureARB(GL_TEXTURE0_ARB + 3);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &inactive_binding_after);
    CHECK(inactive_binding_after == inactive_binding_before[0]);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &inactive_binding_after);
    CHECK(inactive_binding_after == inactive_binding_before[1]);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &inactive_binding_after);
    CHECK(inactive_binding_after == inactive_binding_before[2]);
    glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, &inactive_bias_after);
    CHECK(inactive_bias_after == inactive_bias_before);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D, atlas_name);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_after);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag_after);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrap_s_after);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrap_t_after);
    CHECK(min_after == min_before && mag_after == mag_before);
    CHECK(wrap_s_after == wrap_s_before && wrap_t_after == wrap_t_before);
    glBindTexture(GL_TEXTURE_2D, (GLuint)binding_before);

    /* Fail after stage 0 has bound the borrowed atlas. Both active stages
       must be restored, while inactive stage 3 remains outside the snapshot. */
    {
        const void *textures[D3DTSS_MAXSTAGES] = {
            &atlas_placeholder, NULL, NULL, NULL
        };
        unsigned char sampler[D3DTSS_MAXSTAGES];
        float scale[D3DTSS_MAXSTAGES][4];
        error = NULL;
        CHECK(!mac_gpu_textures_bind(&scope, textures, 1U | (1U << 5),
            sampler, scale, &error));
        CHECK(error && strstr(error, "no resource"));
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &failed_stage_binding);
        CHECK(failed_stage_binding == binding_before);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &failed_stage_binding);
        CHECK(failed_stage_binding == 0);
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &failed_stage_binding);
        CHECK(failed_stage_binding == 0);
        glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,
            &stage0_bias_after);
        CHECK(stage0_bias_after == stage0_bias_before);
        glActiveTextureARB(GL_TEXTURE0_ARB + 1);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &failed_stage_binding);
        CHECK(failed_stage_binding == failed_stage1_bindings[0]);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &failed_stage_binding);
        CHECK(failed_stage_binding == failed_stage1_bindings[1]);
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &failed_stage_binding);
        CHECK(failed_stage_binding == failed_stage1_bindings[2]);
        glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,
            &failed_stage_bias);
        CHECK(failed_stage_bias == failed_stage1_bias);
        glActiveTextureARB(GL_TEXTURE0_ARB + 3);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[0]);
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[1]);
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &inactive_binding_after);
        CHECK(inactive_binding_after == inactive_binding_before[2]);
        glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS,
            &inactive_bias_after);
        CHECK(inactive_bias_after == inactive_bias_before);
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }

    calls = resolver_calls;
    bind_placeholder(&scope, &unknown_texture, &error);
    CHECK(resolver_calls == calls + 1);
    CHECK(error && strstr(error, "owned allocation"));
    CHECK(glIsTexture(atlas_name));

    resolver_mode = -1;
    error = NULL;
    bind_placeholder(&scope, &atlas_placeholder, &error);
    CHECK(error && strstr(error, "intentional resolver refusal"));
    CHECK(glIsTexture(atlas_name));

    resolver_mode = 1;
    error = NULL;
    bind_placeholder(&scope, &atlas_placeholder, &error);
    CHECK(error && strstr(error, "invalid status"));
    CHECK(glIsTexture(atlas_name));

    CHECK(mac_gpu_textures_set_borrowed_resolver(NULL, NULL));
    error = NULL;
    bind_placeholder(&scope, &atlas_placeholder, &error);
    CHECK(error != NULL);
    CHECK(glIsTexture(atlas_name));
    mac_text_dispose();
    CHECK(!glIsTexture(atlas_name));
    glActiveTextureARB(GL_TEXTURE0_ARB + 3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glDeleteTextures(1, &previous_texture);
    glDeleteTextures(3, inactive_textures);
    CHECK(glGetError() == GL_NO_ERROR);
    mac_device_state_stop();
    mac_engine_physical_memory_stop();
    CGLSetCurrentContext(NULL);
    CGLDestroyContext(context);
    CGLDestroyPixelFormat(format);
    printf("native borrowed texture resolver: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
