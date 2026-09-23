#ifndef D3D8_LAZY_INTERNAL_H
#define D3D8_LAZY_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
/* Dirty bits and NV2A method fields established by the January routines. */
enum
{
    LAZY_DIRTY_POINT_PARAMS = 0x00000001,
    LAZY_DIRTY_TRANSFORM = 0x00000002,
    LAZY_DIRTY_TEXTURE_TRANSFORM = 0x00000004,
    LAZY_DIRTY_COMBINERS = 0x00000008,
    LAZY_DIRTY_TEXTURE_STATE = 0x00000010,
    LAZY_DIRTY_LIGHTS = 0x00000020,
    LAZY_DIRTY_SPEC_FOG = 0x00000040,
    LAZY_DIRTY_SHADER_STAGE_PROGRAM = 0x00000400,
    LAZY_DIRTY_VERTEX_FORMAT_VB = 0x00000080,
    LAZY_DIRTY_VERTEX_FORMAT_UP = 0x00000100,
    LAZY_DIRTY_VERTEX_OFFSETS = 0x00000200,
    LAZY_DIRECT_INPUT = 0x40000000,
    LAZY_VERTEX_FORMAT_METHOD = 0x1760,
    LAZY_VERTEX_OFFSET_METHOD = 0x1720,
    LAZY_SHADER_STAGE_METHOD = 0x1e70,
    LAZY_POINT_PARAMS_METHOD = 0xa30,
    LAZY_POINT_ENABLE_METHOD = 0x318,
    LAZY_POINT_SIZE_METHOD = 0x43c
};
extern const D3DVECTOR g_EyeDirection;
DWORD WINAPI GetColorMaterial(
    void);
void WINAPI LazySetShaderStageProgram(
    CDevice *device);
void WINAPI LazySetPointParams(
    CDevice *device);
void WINAPI UpdateProjectionViewportTransform(
    void);
void WINAPI SetState(
    void);
void WINAPI LazySetCombiners(
    CDevice *device);
void WINAPI LazySetTextureState(
    CDevice *device);
void WINAPI LazySetSpecFogCombiner(
    CDevice *device);
void WINAPI LazySetTextureTransform(
    CDevice *device);
void WINAPI LazySetLights(
    CDevice *device);
void WINAPI LazySetTransform(
    CDevice *device);
DWORD *WINAPI SetLightColors(
    DWORD *push,
    const Light *light,
    DWORD lightNumber,
    DWORD colorMaterial);
DWORD *WINAPI SetSceneAmbientAndMaterialEmission(
    DWORD *push,
    DWORD colorMaterial);
DWORD *WINAPI SetSpecularParameters(
    DWORD *push);
enum TextureMatrixType
{
    TEXTURE_MATRIX_220 = 0x220,
    TEXTURE_MATRIX_230 = 0x230,
    TEXTURE_MATRIX_231 = 0x231,
    TEXTURE_MATRIX_241 = 0x241,
    TEXTURE_MATRIX_320 = 0x320,
    TEXTURE_MATRIX_330 = 0x330,
    TEXTURE_MATRIX_331 = 0x331,
    TEXTURE_MATRIX_341 = 0x341
};
/* The stream is DWORD-backed; use representation copies, not float aliasing. */
__inline void WINAPI StoreFloat(
    DWORD *word,
    float value)
{
    memcpy(word, &value, sizeof(value));
    return;
}
__inline float WINAPI Floatify(
    DWORD word)
{
    float value;
    memcpy(&value, &word, sizeof(value));
    return value;
}
}
#endif
