/* Deferred state implementation adapted from RXDK-Libs, 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * January PDB layouts and target instructions control version differences.
 * Requires valid device state, valid texture indices and allocated GPU packets.
 */
#include "lazy_internal.h"
#include "push_internal.h"
#include "state_internal.h"
#include "vshader_internal.h"
#include "math_internal.h"
#include "pixeljar.h"
#include "combiner_internal.h"
#pragma code_seg("D3D")
namespace D3D
{
DWORD WINAPI GetColorMaterial(
    void)
{
    DWORD colorMaterial = 0;
    if (D3D__RenderState[D3DRS_COLORVERTEX])
    {
        for (DWORD i = 0; i < 8; i++)
        {
            colorMaterial <<= 2;
            colorMaterial |= D3D__RenderState[D3DRS_BACKSPECULARMATERIALSOURCE + i];
        }
        CDevice *device = g_pDevice;
        if (!(device->m_pVertexShader->Flags & VERTEXSHADER_HASDIFFUSE))
            colorMaterial &= ~0x0055UL;
        if (!(device->m_pVertexShader->Flags & VERTEXSHADER_HASSPECULAR))
            colorMaterial &= ~0x00aaUL;
        if (!(device->m_pVertexShader->Flags & VERTEXSHADER_HASBACKDIFFUSE))
            colorMaterial &= ~0x5500UL;
        if (!(device->m_pVertexShader->Flags & VERTEXSHADER_HASBACKSPECULAR))
            colorMaterial &= ~0xaa00UL;
    }
    return colorMaterial;
}
void WINAPI LazySetShaderStageProgram(
    CDevice *device)
{
    int stage;
    DWORD shaderStageProgram = 0;
    device->m_DirtyFlags &= ~LAZY_DIRTY_SHADER_STAGE_PROGRAM;
    if (device->m_pPixelShader == NULL)
    {
        for (stage = 3; stage >= 0; stage--)
        {
            DWORD shaderMode = PS_TEXTUREMODES_PROJECT2D;
            D3DBaseTexture *texture = device->m_Textures[stage];
            if (texture == NULL)
                shaderMode = PS_TEXTUREMODES_NONE;
            else if (stage != 0 &&
                     D3D__TextureState[stage - 1][D3DTSS_COLOROP] == D3DTOP_BUMPENVMAP)
                shaderMode = PS_TEXTUREMODES_BUMPENVMAP;
            else if (stage != 0 &&
                     D3D__TextureState[stage - 1][D3DTSS_COLOROP] == D3DTOP_BUMPENVMAPLUMINANCE)
                shaderMode = PS_TEXTUREMODES_BUMPENVMAP_LUM;
            else if (texture->Format & D3DFORMAT_CUBEMAP)
                shaderMode = PS_TEXTUREMODES_CUBEMAP;
            else if ((texture->Format & D3DFORMAT_DIMENSION_MASK) ==
                     (3 << D3DFORMAT_DIMENSION_SHIFT))
                shaderMode = PS_TEXTUREMODES_PROJECT3D;
            shaderStageProgram = (shaderStageProgram << 5) | shaderMode;
        }
    }
    else
    {
        DWORD program = device->m_PSShaderStageProgram;
        if (!device->m_ShaderAdjustsTexMode)
            shaderStageProgram = program;
        else
        {
            for (stage = 3; stage >= 0; stage--)
            {
                DWORD shaderMode = (program >> (stage * 5)) & 0x1f;
                D3DBaseTexture *texture = device->m_Textures[stage];
                if (texture == NULL && shaderMode != PS_TEXTUREMODES_PASSTHRU &&
                    shaderMode != PS_TEXTUREMODES_CLIPPLANE &&
                    shaderMode != PS_TEXTUREMODES_DOT_ZW &&
                    shaderMode != PS_TEXTUREMODES_DOTPRODUCT)
                    shaderMode = PS_TEXTUREMODES_NONE;
                else if (shaderMode >= PS_TEXTUREMODES_PROJECT2D &&
                         shaderMode <= PS_TEXTUREMODES_CUBEMAP)
                {
                    DWORD format = texture->Format & D3DFORMAT_FORMAT_MASK;
                    if (texture->Format & D3DFORMAT_CUBEMAP)
                        shaderMode = PS_TEXTUREMODES_CUBEMAP;
                    else if ((texture->Format & D3DFORMAT_DIMENSION_MASK) ==
                             (3 << D3DFORMAT_DIMENSION_SHIFT) ||
                             (format >= (D3DFMT_D24S8 << D3DFORMAT_FORMAT_SHIFT) &&
                              format <= (D3DFMT_LIN_F16 << D3DFORMAT_FORMAT_SHIFT)))
                        shaderMode = PS_TEXTUREMODES_PROJECT3D;
                    else
                        shaderMode = PS_TEXTUREMODES_PROJECT2D;
                }
                else if (shaderMode == PS_TEXTUREMODES_DOT_STR_3D ||
                         shaderMode == PS_TEXTUREMODES_DOT_STR_CUBE)
                {
                    if (texture->Format & D3DFORMAT_CUBEMAP)
                        shaderMode = PS_TEXTUREMODES_DOT_STR_CUBE;
                    else
                        shaderMode = PS_TEXTUREMODES_DOT_STR_3D;
                }
                shaderStageProgram = (shaderStageProgram << 5) | shaderMode;
            }
        }
    }
    DWORD *push = device->StartPush();
    Push1(push, LAZY_SHADER_STAGE_METHOD, shaderStageProgram);
    device->EndPush(push + 2);
    return;
}
/* The named threshold has a private target data owner which remains to resolve. */
const float g_MaxPointSize = 64.0f;
void WINAPI LazySetPointParams(
    CDevice *device)
{
    device->m_DirtyFlags &= ~LAZY_DIRTY_POINT_PARAMS;
    DWORD *push = device->StartPush();
    float minimum = Floatify(D3D__RenderState[D3DRS_POINTSIZE_MIN]);
    float maximum = Floatify(D3D__RenderState[D3DRS_POINTSIZE_MAX]);
    float size = Floatify(D3D__RenderState[D3DRS_POINTSIZE]);
    BOOL scaleEnable = D3D__RenderState[D3DRS_POINTSCALEENABLE];
    if (!scaleEnable)
    {
        size *= device->m_SuperSampleScale;
        if (size < minimum) size = minimum;
        if (size > maximum) size = maximum;
        if (size > g_MaxPointSize) size = 64.0f;
    }
    else
    {
        float height = (float)device->m_Viewport.Height;
        float a = Floatify(D3D__RenderState[D3DRS_POINTSCALE_A]);
        float b = Floatify(D3D__RenderState[D3DRS_POINTSCALE_B]);
        float c = Floatify(D3D__RenderState[D3DRS_POINTSCALE_C]);
        float delta = maximum - minimum;
        float factor = delta / (size * height);
        factor *= factor;
        PushCount(push, LAZY_POINT_PARAMS_METHOD, 8);
        StoreFloat(push + 1, a * factor);
        StoreFloat(push + 2, b * factor);
        StoreFloat(push + 3, c * factor);
        StoreFloat(push + 4, delta);
        StoreFloat(push + 5, delta);
        StoreFloat(push + 6, delta);
        StoreFloat(push + 7, -minimum / delta);
        StoreFloat(push + 8, minimum);
        push += 9;
    }
    Push2(push, LAZY_POINT_ENABLE_METHOD, scaleEnable,
          D3D__RenderState[D3DRS_POINTSPRITEENABLE]);
    DWORD fixedSize = Round(8.0f * size);
    if (fixedSize > 0x1ff) fixedSize = 0x1ff;
    Push1(push + 3, LAZY_POINT_SIZE_METHOD, fixedSize);
    device->EndPush(push + 5);
    return;
}

void WINAPI UpdateProjectionViewportTransform(
    void)
{
    CDevice *device = g_pDevice;
    float fm11 = device->m_Viewport.Width * device->m_SuperSampleScaleX * 0.5f;
    float fm41 = fm11;
    float fm22 = device->m_Viewport.Height * device->m_SuperSampleScaleY * -0.5f;
    float fm42 = -fm22;
    float fm33 = (device->m_Viewport.MaxZ - device->m_Viewport.MinZ) * device->m_ZScale;
    float fm43 = device->m_ZScale * device->m_Viewport.MinZ;
    float fm44;
    if (D3D__RenderState[D3DRS_ZENABLE] == D3DZB_USEW)
    {
        fm44 = device->m_InverseWFar * device->m_ZScale;
        fm11 *= fm44;
        fm22 *= fm44;
        fm33 *= fm44;
        fm41 *= fm44;
        fm42 *= fm44;
        fm43 *= fm44;
    }
    else
        fm44 = 1.0f;
    __declspec(align(16)) D3DMATRIX viewportTransform;
    memset(&viewportTransform, 0, sizeof(viewportTransform));
    viewportTransform._11 = fm11;
    viewportTransform._22 = fm22;
    viewportTransform._33 = fm33;
    viewportTransform._41 = fm41;
    viewportTransform._42 = fm42;
    viewportTransform._43 = fm43;
    viewportTransform._44 = fm44;
    MatrixProduct4x4(&device->m_ProjectionViewportTransform,
                     &device->m_Transform[D3DTS_PROJECTION], &viewportTransform);
    device->m_DirtyFlags |= LAZY_DIRTY_TRANSFORM;
    return;
}
void WINAPI SetState(
    void)
{
    CDevice *device = g_pDevice;
    DWORD dirty = device->m_DirtyFlags;
    if (dirty)
    {
        if (dirty & LAZY_DIRTY_POINT_PARAMS) LazySetPointParams(device);
        if (dirty & LAZY_DIRTY_SHADER_STAGE_PROGRAM) LazySetShaderStageProgram(device);
        if (dirty & LAZY_DIRTY_COMBINERS) LazySetCombiners(device);
        if (dirty & LAZY_DIRTY_TEXTURE_STATE) LazySetTextureState(device);
        if (dirty & LAZY_DIRTY_SPEC_FOG) LazySetSpecFogCombiner(device);
        if (dirty & LAZY_DIRTY_TEXTURE_TRANSFORM) LazySetTextureTransform(device);
        if (dirty & LAZY_DIRTY_LIGHTS) LazySetLights(device);
        if (dirty & LAZY_DIRTY_TRANSFORM) LazySetTransform(device);
    }
    return;
}
const float g_MaxFogScale = 8192.0f;
void WINAPI LazySetSpecFogCombiner(
    CDevice *device)
{
    DWORD *push;
    float bias;
    float scale;
    DWORD fogMode;
    DWORD control0;
    DWORD control1 = 0x1c80;
    device->m_DirtyFlags &= ~LAZY_DIRTY_SPEC_FOG;
    CommonSetPassthroughProgram(device);
    if (D3D__RenderState[D3DRS_FOGENABLE])
    {
        DWORD tableMode = D3D__RenderState[D3DRS_FOGTABLEMODE];
        float start = Floatify(D3D__RenderState[D3DRS_FOGSTART]);
        float end = Floatify(D3D__RenderState[D3DRS_FOGEND]);
        float density = Floatify(D3D__RenderState[D3DRS_FOGDENSITY]);
        DWORD generator = D3D__RenderState[D3DRS_RANGEFOGENABLE] ? 1 : 2;
        if (tableMode == D3DFOG_NONE)
        {
            bias = 1.0f;
            scale = 1.0f;
            fogMode = 0x2601;
            generator = 0;
        }
        else if (tableMode == D3DFOG_LINEAR)
        {
            float linearScale;
            if (end != start) linearScale = 1.0f / (end - start);
            else linearScale = g_MaxFogScale;
            bias = 1.0f + end * linearScale;
            scale = -linearScale;
            fogMode = 0x2601;
        }
        else if (tableMode == D3DFOG_EXP)
        {
            bias = 1.5f;
            scale = -density * (1.0f / (2.0f * 5.5452f));
            fogMode = 0x800;
        }
        else
        {
            /* The remaining valid API mode is D3DFOG_EXP2. */
            bias = 1.5f;
            scale = -density * (1.0f / (2.0f * 2.354824834249885f));
            fogMode = 0x801;
        }
        push = device->StartPush();
        Push2(push, 0x2a0, generator, TRUE);
        Push1(push + 3, 0x29c, fogMode);
        PushCount(push + 5, 0x9c0, 3);
        StoreFloat(push + 6, bias);
        StoreFloat(push + 7, scale);
        push[8] = NV_FLOAT_ZERO;
        if (device->m_pPixelShader == NULL || device->m_ShaderUsesSpecFog == 0)
        {
            control0 = 0x13000300;
            if (D3D__RenderState[D3DRS_SPECULARENABLE]) control0 |= 0x000e0000;
            else control0 |= 0x000c0000;
            Push2(push + 9, 0x288, control0, control1);
            device->EndPush(push + 12);
        }
        else
            device->EndPush(push + 9);
    }
    else
    {
        push = device->StartPush();
        Push1(push, 0x2a4, FALSE);
        if (device->m_pPixelShader == NULL || device->m_ShaderUsesSpecFog == 0)
        {
            if (D3D__RenderState[D3DRS_SPECULARENABLE]) control0 = 0xe;
            else control0 = 0xc;
            Push2(push + 2, 0x288, control0, control1);
            device->EndPush(push + 5);
        }
        else
            device->EndPush(push + 2);
    }
    return;
}

DWORD *WINAPI SetLightColors(
    DWORD *push,
    const Light *light,
    DWORD lightNumber,
    DWORD colorMaterial)
{
    CDevice *device = g_pDevice;
    DWORD i = D3D__RenderState[D3DRS_TWOSIDEDLIGHTING];
    D3DMATERIAL8 *material = &device->m_Material;
    PushCount(push, 0x1000 + lightNumber * 0x80, 9);
    for (;;)
    {
        if (!(colorMaterial & 0x0c))
        {
            StoreFloat(push + 1, material->Ambient.r * light->Light8.Ambient.r);
            StoreFloat(push + 2, material->Ambient.g * light->Light8.Ambient.g);
            StoreFloat(push + 3, material->Ambient.b * light->Light8.Ambient.b);
        }
        else
        {
            StoreFloat(push + 1, light->Light8.Ambient.r);
            StoreFloat(push + 2, light->Light8.Ambient.g);
            StoreFloat(push + 3, light->Light8.Ambient.b);
        }
        if (!(colorMaterial & 0x30))
        {
            StoreFloat(push + 4, material->Diffuse.r * light->Light8.Diffuse.r);
            StoreFloat(push + 5, material->Diffuse.g * light->Light8.Diffuse.g);
            StoreFloat(push + 6, material->Diffuse.b * light->Light8.Diffuse.b);
        }
        else
        {
            StoreFloat(push + 4, light->Light8.Diffuse.r);
            StoreFloat(push + 5, light->Light8.Diffuse.g);
            StoreFloat(push + 6, light->Light8.Diffuse.b);
        }
        if (!(colorMaterial & 0xc0))
        {
            StoreFloat(push + 7, material->Specular.r * light->Light8.Specular.r);
            StoreFloat(push + 8, material->Specular.g * light->Light8.Specular.g);
            StoreFloat(push + 9, material->Specular.b * light->Light8.Specular.b);
        }
        else
        {
            StoreFloat(push + 7, light->Light8.Specular.r);
            StoreFloat(push + 8, light->Light8.Specular.g);
            StoreFloat(push + 9, light->Light8.Specular.b);
        }
        push += 10;
        if (i-- == 0) return push;
        PushCount(push, 0xc00 + lightNumber * 0x40, 9);
        colorMaterial >>= 8;
        material = &device->m_BackMaterial;
    }
}
DWORD *WINAPI SetSceneAmbientAndMaterialEmission(
    DWORD *push,
    DWORD colorMaterial)
{
    float emissiveR;
    float emissiveG;
    float emissiveB;
    CDevice *device = g_pDevice;
    DWORD i = D3D__RenderState[D3DRS_TWOSIDEDLIGHTING];
    DWORD ambient = D3D__RenderState[D3DRS_AMBIENT];
    D3DMATERIAL8 *material = &device->m_Material;
    PushCount(push, 0xa10, 3);
    PushCount(push + 4, 0x3a8, 3);
    PushCount(push + 8, 0x3b4, 1);
    for (;;)
    {
        float ambientR = (float)((ambient >> 16) & 0xff) / 255.0f;
        float ambientG = (float)((ambient >> 8) & 0xff) / 255.0f;
        float ambientB = (float)(ambient & 0xff) / 255.0f;
        if (colorMaterial & 0x0c)
        {
            emissiveR = ambientR;
            emissiveG = ambientG;
            emissiveB = ambientB;
            ambientR = material->Emissive.r;
            ambientG = material->Emissive.g;
            ambientB = material->Emissive.b;
        }
        else if (colorMaterial & 0x03)
        {
            ambientR *= material->Ambient.r;
            ambientG *= material->Ambient.g;
            ambientB *= material->Ambient.b;
            emissiveR = 1.0f;
            emissiveG = 1.0f;
            emissiveB = 1.0f;
        }
        else
        {
            ambientR = ambientR * material->Ambient.r + material->Emissive.r;
            ambientG = ambientG * material->Ambient.g + material->Emissive.g;
            ambientB = ambientB * material->Ambient.b + material->Emissive.b;
            emissiveR = 0.0f;
            emissiveG = 0.0f;
            emissiveB = 0.0f;
        }
        StoreFloat(push + 1, ambientR);
        StoreFloat(push + 2, ambientG);
        StoreFloat(push + 3, ambientB);
        StoreFloat(push + 5, emissiveR);
        StoreFloat(push + 6, emissiveG);
        StoreFloat(push + 7, emissiveB);
        StoreFloat(push + 9, material->Diffuse.a);
        push += 10;
        if (i-- == 0) return push;
        colorMaterial >>= 8;
        ambient = D3D__RenderState[D3DRS_BACKAMBIENT];
        material = &device->m_BackMaterial;
        PushCount(push, 0x17a0, 3);
        PushCount(push + 4, 0x17b0, 3);
        PushCount(push + 8, 0x17ac, 1);
    }
}
DWORD *WINAPI SetSpecularParameters(
    DWORD *push)
{
    float l, m, n, l1, m1, n1;
    CDevice *device = g_pDevice;
    DWORD i = D3D__RenderState[D3DRS_TWOSIDEDLIGHTING];
    float power = device->m_Material.Power;
    PushCount(push, 0x9e0, 6);
    for (;;)
    {
        Explut(power, &l, &m);
        n = 1.0f + l - m;
        Explut(0.5f * power, &l1, &m1);
        n1 = 1.0f + l1 - m1;
        StoreFloat(push + 1, l);
        StoreFloat(push + 2, m);
        StoreFloat(push + 3, n);
        StoreFloat(push + 4, l1);
        StoreFloat(push + 5, m1);
        StoreFloat(push + 6, n1);
        push += 7;
        if (i-- == 0) return push;
        power = device->m_BackMaterial.Power;
        PushCount(push, 0x1e28, 6);
    }
}

void WINAPI LazySetCombiners(
    CDevice *device)
{
    device->m_DirtyFlags &= ~LAZY_DIRTY_COMBINERS;
    if (device->m_pPixelShader == NULL)
    {
        DWORD combinerStage = 0;
        for (;;)
        {
            DWORD textureStage = combinerStage;
            if (D3D__RenderState[D3DRS_POINTSPRITEENABLE])
                textureStage = 3;
            BOOL moreStages = ConstructColorCombiners(device, combinerStage, textureStage);
            ConstructAlphaCombiners(device, combinerStage, textureStage, moreStages);
            if (moreStages || combinerStage == 0)
                combinerStage++;
            if (!moreStages || textureStage >= 3)
                break;
        }
        device->m_ActiveCombinerStagesCount = combinerStage;
        for (DWORD i = combinerStage; i < 8; i++)
        {
            device->m_ColorICW[i] = 0;
            device->m_ColorOCW[i] = 0;
            device->m_AlphaICW[i] = 0;
            device->m_AlphaOCW[i] = 0;
        }
        DWORD *push = device->StartPush();
        Push1(push, 0x1e60, combinerStage & 0xff);
        PushCount(push + 2, 0xac0, 8);
        for (DWORD word = 0; word < 8; word++)
            push[3 + word] = device->m_ColorICW[word];
        PushCount(push + 11, 0x1e40, 8);
        for (DWORD word = 0; word < 8; word++)
            push[12 + word] = device->m_ColorOCW[word];
        PushCount(push + 20, 0x260, 8);
        for (DWORD word = 0; word < 8; word++)
            push[21 + word] = device->m_AlphaICW[word];
        PushCount(push + 29, 0xaa0, 8);
        for (DWORD word = 0; word < 8; word++)
            push[30 + word] = device->m_AlphaOCW[word];
        device->EndPush(push + 38);
    }
    return;
}

/* Ordinary private row helper. Original PDB records identify Set4f and
 * all eight transformation helpers with these scalar/matrix parameter types. */
static __inline void WINAPI Set4f(
    float *out,
    float a,
    float b,
    float c,
    float d)
{
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = d;
    return;
}
static __inline void WINAPI SetTextureTransformMatrixType220(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, 0.0f, matrix->_31);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, 0.0f, matrix->_32);
    Set4f(&out->m[2][0], 0.0f, 0.0f, 0.0f, 0.0f);
    Set4f(&out->m[3][0], 0.0f, 0.0f, 0.0f, 1.0f);
    return;
}

static __inline void WINAPI SetTextureTransformMatrixType230(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, 0.0f, matrix->_31);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, 0.0f, matrix->_32);
    Set4f(&out->m[2][0], matrix->_13, matrix->_23, 0.0f, matrix->_33);
    Set4f(&out->m[3][0], 0.0f, 0.0f, 0.0f, 1.0f);
    return;
}

static __inline void WINAPI SetTextureTransformMatrixType231(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, 0.0f, matrix->_31);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, 0.0f, matrix->_32);
    Set4f(&out->m[2][0], 0.0f, 0.0f, 0.0f, 0.0f);
    Set4f(&out->m[3][0], matrix->_13, matrix->_23, 0.0f, matrix->_33);
    return;
}

static __inline void WINAPI SetTextureTransformMatrixType241(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, 0.0f, matrix->_31);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, 0.0f, matrix->_32);
    Set4f(&out->m[2][0], matrix->_13, matrix->_23, 0.0f, matrix->_33);
    Set4f(&out->m[3][0], matrix->_14, matrix->_24, 0.0f, matrix->_34);
    return;
}

static __inline void WINAPI SetTextureTransformMatrixType320(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, matrix->_31, matrix->_41);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, matrix->_32, matrix->_42);
    Set4f(&out->m[2][0], 0.0f, 0.0f, 0.0f, 0.0f);
    Set4f(&out->m[3][0], 0.0f, 0.0f, 0.0f, 1.0f);
    return;
}

static __inline void WINAPI SetTextureTransformMatrixType330(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, matrix->_31, matrix->_41);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, matrix->_32, matrix->_42);
    Set4f(&out->m[2][0], matrix->_13, matrix->_23, matrix->_33, matrix->_43);
    Set4f(&out->m[3][0], 0.0f, 0.0f, 0.0f, 1.0f);
    return;
}

static __inline void WINAPI SetTextureTransformMatrixType331(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, matrix->_31, matrix->_41);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, matrix->_32, matrix->_42);
    Set4f(&out->m[2][0], 0.0f, 0.0f, 0.0f, 0.0f);
    Set4f(&out->m[3][0], matrix->_13, matrix->_23, matrix->_33, matrix->_43);
    return;
}

static __inline void WINAPI SetTextureTransformMatrixType341(
    D3DMATRIX *out,
    const D3DMATRIX *matrix)
{
    Set4f(&out->m[0][0], matrix->_11, matrix->_21, matrix->_31, matrix->_41);
    Set4f(&out->m[1][0], matrix->_12, matrix->_22, matrix->_32, matrix->_42);
    Set4f(&out->m[2][0], matrix->_13, matrix->_23, matrix->_33, matrix->_43);
    Set4f(&out->m[3][0], matrix->_14, matrix->_24, matrix->_34, matrix->_44);
    return;
}

void WINAPI LazySetTextureTransform(
    CDevice *device)
{
    device->m_DirtyFlags &= ~LAZY_DIRTY_TEXTURE_TRANSFORM;
    if (!(device->m_pVertexShader->Flags & (VERTEXSHADER_PROGRAM | VERTEXSHADER_PASSTHROUGH)))
    {
        for (DWORD stage = 0; stage < 4; stage++)
        {
            DWORD transformFlags = D3D__TextureState[stage][D3DTSS_TEXTURETRANSFORMFLAGS];
            DWORD *push = device->StartPush();
            if (transformFlags == D3DTTFF_DISABLE)
            {
                Push1(push, 0x420 + stage * 4, FALSE);
                device->EndPush(push + 2);
            }
            else
            {
                DWORD texCoord = D3D__TextureState[stage][D3DTSS_TEXCOORDINDEX];
                DWORD inCount;
                if (texCoord & 0xffff0000)
                    inCount = 3;
                else
                {
                    DWORD index = texCoord & 0xffff;
                    inCount = index < device->m_pVertexShader->TextureCount
                        ? device->m_pVertexShader->Dimensionality[index] : 2;
                }
                DWORD matrixType = (inCount << 8) | ((transformFlags & 0xff) << 4)
                    | ((transformFlags & D3DTTFF_PROJECTED) != 0);
                const D3DMATRIX *matrix = &device->m_Transform[D3DTS_TEXTURE0 + stage];
                D3DMATRIX output;
                Push1(push, 0x420 + stage * 4, TRUE);
                PushCount(push + 2, 0x6c0 + stage * 0x40, 16);
                switch (matrixType)
                {
                case TEXTURE_MATRIX_220:
                    SetTextureTransformMatrixType220(&output, matrix);
                    break;
                case TEXTURE_MATRIX_230:
                    SetTextureTransformMatrixType230(&output, matrix);
                    break;
                case TEXTURE_MATRIX_231:
                    SetTextureTransformMatrixType231(&output, matrix);
                    break;
                case TEXTURE_MATRIX_241:
                    SetTextureTransformMatrixType241(&output, matrix);
                    break;
                case TEXTURE_MATRIX_320:
                    SetTextureTransformMatrixType320(&output, matrix);
                    break;
                case TEXTURE_MATRIX_330:
                    SetTextureTransformMatrixType330(&output, matrix);
                    break;
                case TEXTURE_MATRIX_331:
                    SetTextureTransformMatrixType331(&output, matrix);
                    break;
                case TEXTURE_MATRIX_341:
                default:
                    SetTextureTransformMatrixType341(&output, matrix);
                    break;
                }
                memcpy(push + 3, &output, sizeof(output));
                device->EndPush(push + 19);
            }
        }
    }
    return;
}

void WINAPI LazySetTransform(
    CDevice *device)
{
    __declspec(align(16)) D3DMATRIX modelView;
    D3DMATRIX inverseModelView;
    __declspec(align(16)) D3DMATRIX modelViewBlend;
    D3DMATRIX inverseModelViewBlend;
    __declspec(align(16)) D3DMATRIX composite;
    DWORD stride;
    DWORD i;
    device->m_DirtyFlags &= ~LAZY_DIRTY_TRANSFORM;
    if (device->m_DirtyFlags & 0x80000000UL)
        return;
    if (!(device->m_pVertexShader->Flags & (VERTEXSHADER_PROGRAM | VERTEXSHADER_PASSTHROUGH)))
    {
        DWORD *push = device->StartPush();
        MatrixProduct4x4(&modelView, &device->m_Transform[D3DTS_WORLD],
                         &device->m_Transform[D3DTS_VIEW]);
        PushMatrixTransposed(push, 0x480, &modelView);
        push += 17;
        if (device->m_TexGenInverseNeeded || D3D__RenderState[D3DRS_LIGHTING])
        {
            Inverse4x4(&inverseModelView, &modelView, !D3D__RenderState[D3DRS_NORMALIZENORMALS]);
            PushInverseModelViewMatrix(push, 0x580, &inverseModelView);
            push += 13;
        }
        if (!D3D__RenderState[D3DRS_VERTEXBLEND])
        {
            MatrixProduct4x4(&composite, &modelView, &device->m_ProjectionViewportTransform);
            PushMatrixTransposed(push, 0x680, &composite);
            push += 17;
        }
        else
        {
            PushMatrixTransposed(push, 0x680, &device->m_ProjectionViewportTransform);
            push += 17;
            device->EndPush(push);
            push = device->StartPush();
            for (i = 1; i < 4; i++)
            {
                MatrixProduct4x4(&modelViewBlend, &device->m_Transform[D3DTS_WORLD + i],
                                 &device->m_Transform[D3DTS_VIEW]);
                stride = 0x40;
                PushMatrixTransposed(push, 0x480 + i * stride, &modelViewBlend);
                push += 17;
                if (device->m_TexGenInverseNeeded || D3D__RenderState[D3DRS_LIGHTING])
                {
                    Inverse4x4(&inverseModelViewBlend, &modelViewBlend,
                               !D3D__RenderState[D3DRS_NORMALIZENORMALS]);
                    stride = 0x40;
                    PushInverseModelViewMatrix(push, 0x580 + i * stride, &inverseModelViewBlend);
                    push += 13;
                }
            }
        }
        device->EndPush(push);
    }
    return;
}

const D3DVECTOR g_EyeDirection = { 0.0f, 0.0f, -1.0f };
void WINAPI LazySetLights(
    CDevice *device)
{
    DWORD *push;
    D3DVECTOR position;
    D3DVECTOR direction;
    D3DVECTOR halfVector;
    device->m_DirtyFlags &= ~LAZY_DIRTY_LIGHTS;
    BOOL specularNeeded = D3D__RenderState[D3DRS_SPECULARENABLE]
        || (device->m_StateFlags & 0x40);
    if ((device->m_pVertexShader->Flags & (VERTEXSHADER_PROGRAM | VERTEXSHADER_PASSTHROUGH))
        || !D3D__RenderState[D3DRS_LIGHTING])
    {
        push = device->StartPush();
        Push1(push, 0x314, FALSE);
        Push1(push + 2, 0x3b8, specularNeeded);
        Push1(push + 4, 0x294, 0x20001);
        BOOL twoSided = D3D__RenderState[D3DRS_TWOSIDEDLIGHTING];
        Push1(push + 6, 0x17c4, twoSided);
        device->EndPush(push + 8);
    }
    else
    {
        push = device->StartPush();
        DWORD control = 1;
        if (specularNeeded)
        {
            if (D3D__RenderState[D3DRS_LOCALVIEWER] && device->m_pActiveLights != NULL)
                control |= 0x10000;
            push = SetSpecularParameters(push);
        }
        Push1(push, 0x294, control);
        Push1(push + 2, 0x314, TRUE);
        BOOL twoSided = D3D__RenderState[D3DRS_TWOSIDEDLIGHTING];
        Push1(push + 4, 0x17c4, twoSided);
        Push1(push + 6, 0x3b8, TRUE);
        DWORD colorMaterial = GetColorMaterial();
        Push1(push + 8, 0x298, colorMaterial);
        push = SetSceneAmbientAndMaterialEmission(push + 10, colorMaterial);
        DWORD enableMask = 0;
        DWORD lightNumber = 0;
        for (Light *light = device->m_pActiveLights; light != NULL; light = light->pNext)
        {
            device->EndPush(push);
            push = device->StartPush();
            push = SetLightColors(push, light, lightNumber, colorMaterial);
            if (light->Light8.Type == D3DLIGHT_DIRECTIONAL)
            {
                enableMask |= 1UL << (lightNumber << 1);
                XformBy4x3(&direction, &light->Direction, 0.0f, &device->m_Transform[D3DTS_VIEW]);
                NormalizeVector3(&direction);
                AddVectors3(&halfVector, &direction, &g_EyeDirection);
                NormalizeVector3(&halfVector);
                PushCount(push, 0x1024 + lightNumber * 0x80, 1);
                push[1] = NV_FLOAT_INFINITE_LIGHT_RANGE;
                PushCount(push + 2, 0x1028 + lightNumber * 0x80, 6);
                memcpy(push + 3, &halfVector.x, sizeof(float));
                memcpy(push + 4, &halfVector.y, sizeof(float));
                memcpy(push + 5, &halfVector.z, sizeof(float));
                memcpy(push + 6, &direction.x, sizeof(float));
                memcpy(push + 7, &direction.y, sizeof(float));
                memcpy(push + 8, &direction.z, sizeof(float));
                push += 9;
            }
            else
            {
                PushCount(push, 0x1024 + lightNumber * 0x80, 1);
                StoreFloat(push + 1, light->Light8.Range);
                XformBy4x3(&position, &light->Light8.Position, 1.0f, &device->m_Transform[D3DTS_VIEW]);
                PushCount(push + 2, 0x105c + lightNumber * 0x80, 6);
                memcpy(push + 3, &position.x, sizeof(float));
                memcpy(push + 4, &position.y, sizeof(float));
                memcpy(push + 5, &position.z, sizeof(float));
                memcpy(push + 6, &light->Light8.Attenuation0, sizeof(float));
                memcpy(push + 7, &light->Light8.Attenuation1, sizeof(float));
                memcpy(push + 8, &light->Light8.Attenuation2, sizeof(float));
                push += 9;
                if (light->Light8.Type == D3DLIGHT_POINT)
                    enableMask |= 2UL << (lightNumber << 1);
                else
                {
                    enableMask |= 3UL << (lightNumber << 1);
                    XformBy4x3(&direction, &light->Direction, 0.0f, &device->m_Transform[D3DTS_VIEW]);
                    NormalizeVector3(&direction);
                    ScaleVector3(&direction, &direction, light->Scale);
                    PushCount(push, 0x1040 + lightNumber * 0x80, 7);
                    memcpy(push + 1, &light->Falloff_L, sizeof(float));
                    memcpy(push + 2, &light->Falloff_M, sizeof(float));
                    memcpy(push + 3, &light->Falloff_N, sizeof(float));
                    memcpy(push + 4, &direction.x, sizeof(float));
                    memcpy(push + 5, &direction.y, sizeof(float));
                    memcpy(push + 6, &direction.z, sizeof(float));
                    memcpy(push + 7, &light->W, sizeof(float));
                    push += 8;
                }
            }
            if (++lightNumber == 8)
                break;
        }
        Push1(push, 0x3bc, enableMask);
        device->EndPush(push + 2);
    }
    return;
}

/* January remaps virtual texture slots 16..19 into hardware slots 9..12.
 * Valid shader inputs use bounded stream/slot indices and backed resources. */
void CDevice::SetStateVB(
    DWORD indexBase)
{
    DWORD i;
    DWORD *push;
    VertexShader *shader;
    CDevice *device = this;
    SetState();
    if (device->m_DirtyFlags & LAZY_DIRECT_INPUT)
        return;
    if (device->m_DirtyFlags & LAZY_DIRTY_VERTEX_FORMAT_VB)
    {
        device->m_DirtyFlags &= ~(LAZY_DIRTY_VERTEX_FORMAT_VB | LAZY_DIRECT_INPUT);
        device->m_DirtyFlags |= LAZY_DIRTY_VERTEX_FORMAT_UP | LAZY_DIRTY_VERTEX_OFFSETS;
        shader = device->m_pVertexShader;
        if (!(shader->Flags & VERTEXSHADER_PROGRAM))
        {
            DWORD textureCount = shader->TextureCount;
            for (i = 0; i < D3DTSS_MAXSTAGES; i++)
            {
                DWORD index = D3D__TextureState[i][D3DTSS_TEXCOORDINDEX] & 0xffff;
                if (index < textureCount)
                    shader->Slot[9 + i] = shader->Slot[SLOT_TEXTURE0 + index];
                else
                    shader->Slot[9 + i].SizeAndType = SIZEANDTYPE_DISABLED;
            }
            push = device->StartPush();
            if (!(shader->Flags & VERTEXSHADER_HASDIFFUSE))
            {
                Push1(push, 0x194c, 0xffffffffUL);
                push += 2;
            }
            if (!(shader->Flags & VERTEXSHADER_HASSPECULAR))
            {
                Push1(push, 0x1950, 0);
                push += 2;
            }
            if (!(shader->Flags & VERTEXSHADER_HASBACKDIFFUSE))
            {
                Push1(push, 0x195c, 0xffffffffUL);
                push += 2;
            }
            if (!(shader->Flags & VERTEXSHADER_HASBACKSPECULAR))
            {
                Push1(push, 0x1960, 0);
                push += 2;
            }
            device->EndPush(push);
        }
        for (i = 15; i != 0; i--)
        {
            if (shader->Slot[i].SizeAndType != SIZEANDTYPE_DISABLED)
                break;
        }
        shader->MaxSlot = i;
        push = device->StartPush();
        PushCount(push++, LAZY_VERTEX_FORMAT_METHOD, 16);
        for (i = 0; i < 16; i++)
        {
            *push++ = shader->Slot[i].SizeAndType
                + (g_Stream[shader->Slot[i].StreamIndex].Stride << 8);
        }
        device->EndPush(push);
    }
    if ((device->m_DirtyFlags & LAZY_DIRTY_VERTEX_OFFSETS)
        || device->m_CachedIndexBase != indexBase)
    {
        device->m_DirtyFlags &= ~(LAZY_DIRTY_VERTEX_OFFSETS | LAZY_DIRECT_INPUT);
        device->m_CachedIndexBase = indexBase;
        shader = device->m_pVertexShader;
        push = device->StartPush();
        DWORD maxSlot = shader->MaxSlot;
        if (indexBase == 0)
        {
            for (i = 0; i <= maxSlot; i++)
            {
                if (shader->Slot[i].SizeAndType != SIZEANDTYPE_DISABLED)
                {
                    Stream *stream = &g_Stream[shader->Slot[i].StreamIndex];
                    if (stream->pVertexBuffer != NULL)
                    {
                        DWORD offset = stream->pVertexBuffer->Data
                            + shader->Slot[i].Offset + stream->Offset;
                        Push1(push, LAZY_VERTEX_OFFSET_METHOD + i * 4, offset);
                        push += 2;
                    }
                }
            }
        }
        else
        {
            for (i = 0; i <= maxSlot; i++)
            {
                if (shader->Slot[i].SizeAndType != SIZEANDTYPE_DISABLED)
                {
                    Stream *stream = &g_Stream[shader->Slot[i].StreamIndex];
                    if (stream->pVertexBuffer != NULL)
                    {
                        DWORD offset = stream->pVertexBuffer->Data
                            + indexBase * stream->Stride
                            + shader->Slot[i].Offset + stream->Offset;
                        Push1(push, LAZY_VERTEX_OFFSET_METHOD + i * 4, offset);
                        push += 2;
                    }
                }
            }
        }
        device->EndPush(push);
    }
    return;
}

/* UP drawing requires at least one enabled, valid vertex attribute. Offsets
 * are byte offsets within the caller's vertex record, not fabricated pointers. */
void CDevice::SetStateUP(
    void)
{
    DWORD i;
    DWORD *push;
    CDevice *device = this;
    SetState();
    if (device->m_DirtyFlags & LAZY_DIRTY_VERTEX_FORMAT_UP)
    {
        device->m_DirtyFlags &= ~LAZY_DIRTY_VERTEX_FORMAT_UP;
        device->m_DirtyFlags |= LAZY_DIRTY_VERTEX_FORMAT_VB;
        VertexShader *shader = device->m_pVertexShader;
        if (!(shader->Flags & VERTEXSHADER_PROGRAM))
        {
            DWORD textureCount = shader->TextureCount;
            for (i = 0; i < D3DTSS_MAXSTAGES; i++)
            {
                DWORD index = D3D__TextureState[i][D3DTSS_TEXCOORDINDEX] & 0xffff;
                if (index < textureCount)
                    shader->Slot[9 + i] = shader->Slot[SLOT_TEXTURE0 + index];
                else
                    shader->Slot[9 + i].SizeAndType = SIZEANDTYPE_DISABLED;
            }
            push = device->StartPush();
            if (!(shader->Flags & VERTEXSHADER_HASDIFFUSE))
            {
                Push1(push, 0x194c, 0xffffffffUL);
                push += 2;
            }
            if (!(shader->Flags & VERTEXSHADER_HASSPECULAR))
            {
                Push1(push, 0x1950, 0);
                push += 2;
            }
            if (!(shader->Flags & VERTEXSHADER_HASBACKDIFFUSE))
            {
                Push1(push, 0x195c, 0xffffffffUL);
                push += 2;
            }
            if (!(shader->Flags & VERTEXSHADER_HASBACKSPECULAR))
            {
                Push1(push, 0x1960, 0);
                push += 2;
            }
            device->EndPush(push);
        }
        DWORD dwords = 0;
        DWORD attributes = 0;
        for (i = 0; i < 16; i++)
        {
            DWORD sizeAndType = shader->Slot[i].SizeAndType;
            if (sizeAndType != SIZEANDTYPE_DISABLED)
            {
                DWORD bytes = g_UnitsOfElement[(sizeAndType & 0xf0) >> 4]
                    * g_BytesPerUnit[sizeAndType & 0xf];
                DWORD count = (bytes + 3) >> 2;
                DWORD offset = shader->Slot[i].Offset;
                device->m_InlineAttributeData[attributes].UP_Count = count;
                device->m_InlineAttributeData[attributes].UP_Delta = offset;
                dwords += count;
                attributes++;
            }
        }
        device->m_InlineAttributeCount = attributes;
        device->m_InlineVertexDwords = dwords;
        device->m_InlineStartOffset = device->m_InlineAttributeData[0].UP_Delta;
        device->m_InlineDelta = -device->m_InlineAttributeData[attributes - 1].UP_Count * sizeof(DWORD)
            - device->m_InlineAttributeData[attributes - 1].UP_Delta
            + device->m_InlineStartOffset;
        for (i = 1; i < attributes; i++)
        {
            device->m_InlineAttributeData[i - 1].UP_Delta =
                device->m_InlineAttributeData[i].UP_Delta
                - device->m_InlineAttributeData[i - 1].UP_Count * sizeof(DWORD)
                - device->m_InlineAttributeData[i - 1].UP_Delta;
        }
        push = device->StartPush();
        PushCount(push++, LAZY_VERTEX_FORMAT_METHOD, 16);
        for (i = 0; i < 16; i++)
            *push++ = shader->Slot[i].SizeAndType;
        device->EndPush(push);
    }
    return;
}
}
