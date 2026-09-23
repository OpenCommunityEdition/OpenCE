#ifndef D3D8_PRESENT_INTERNAL_H
#define D3D8_PRESENT_INTERNAL_H
/* January presentation owner. Layout and sizes are authenticated from the
 * supplied PDB, not copied from later SwapSavedState revisions. */
#include "device_layout.h"
namespace D3D
{
struct AntiAliasedSavedState
{
    D3DSurface *pRenderTarget;
    D3DSurface *pZBuffer;
    D3DBaseTexture *pTexture;
    D3D::PixelShader *pPixelShader;
    unsigned long VertexShaderHandle;
    _D3DVIEWPORT8 Viewport;
    unsigned long Stage1ColorOp;
    unsigned long PSTextureModes;
    unsigned long RenderState[16];
    unsigned long TextureState[11];
    unsigned long PixelShaderState[57];
    unsigned long VertexShaders[48];
    D3DTexture SrcTexture;
};
typedef char AntiAliasedSavedState_size[sizeof(AntiAliasedSavedState) == 600 ? 1 : -1];
typedef char AntiAliasedSavedState_pRenderTarget_offset[offsetof(AntiAliasedSavedState, pRenderTarget) == 0 ? 1 : -1];
typedef char AntiAliasedSavedState_pZBuffer_offset[offsetof(AntiAliasedSavedState, pZBuffer) == 4 ? 1 : -1];
typedef char AntiAliasedSavedState_pTexture_offset[offsetof(AntiAliasedSavedState, pTexture) == 8 ? 1 : -1];
typedef char AntiAliasedSavedState_pPixelShader_offset[offsetof(AntiAliasedSavedState, pPixelShader) == 12 ? 1 : -1];
typedef char AntiAliasedSavedState_VertexShaderHandle_offset[offsetof(AntiAliasedSavedState, VertexShaderHandle) == 16 ? 1 : -1];
typedef char AntiAliasedSavedState_Viewport_offset[offsetof(AntiAliasedSavedState, Viewport) == 20 ? 1 : -1];
typedef char AntiAliasedSavedState_Stage1ColorOp_offset[offsetof(AntiAliasedSavedState, Stage1ColorOp) == 44 ? 1 : -1];
typedef char AntiAliasedSavedState_PSTextureModes_offset[offsetof(AntiAliasedSavedState, PSTextureModes) == 48 ? 1 : -1];
typedef char AntiAliasedSavedState_RenderState_offset[offsetof(AntiAliasedSavedState, RenderState) == 52 ? 1 : -1];
typedef char AntiAliasedSavedState_TextureState_offset[offsetof(AntiAliasedSavedState, TextureState) == 116 ? 1 : -1];
typedef char AntiAliasedSavedState_PixelShaderState_offset[offsetof(AntiAliasedSavedState, PixelShaderState) == 160 ? 1 : -1];
typedef char AntiAliasedSavedState_VertexShaders_offset[offsetof(AntiAliasedSavedState, VertexShaders) == 388 ? 1 : -1];
typedef char AntiAliasedSavedState_SrcTexture_offset[offsetof(AntiAliasedSavedState, SrcTexture) == 580 ? 1 : -1];
extern const DWORD g_AntiAliasedRenderStates[32];
extern const DWORD g_AntiAliasedTextureStates[22];
extern DWORD g_FlipAddress;
void WINAPI AntiAliasedSetState(
    AntiAliasedSavedState *saved,
    D3DSurface *destination,
    D3DSurface *source,
    DWORD filter);
void WINAPI AntiAliasedSaveState(
    AntiAliasedSavedState *saved);
void WINAPI AntiAliasedRestoreState(
    AntiAliasedSavedState *saved);
DWORD *WINAPI AntiAliasedFilterBlt(
    DWORD *push,
    DWORD xScale,
    DWORD yScale,
    const RECT *rectangle);
void WINAPI AntiAliasedCopy(
    const RECT *rectangle);
void WINAPI AliasedFlip(
    void);
}
#endif
