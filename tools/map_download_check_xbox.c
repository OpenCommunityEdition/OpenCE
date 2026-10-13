/*
MAP_DOWNLOAD_CHECK_XBOX.C

Stand-ins for tools/map_download_check.c's link (tools/linux_build.py). Every
platform unit carries the Xbox headers' D3DDevice_SetRenderState
(port/include/xdk/xdk_d3d8.h) as a weak inline function, which the map
downloads' units never call but which refers to the renderer's functions
and state table (port/linux/src/d3d8_gl.c). Without link-time optimisation,
which drops it, the tool links against these instead of the renderer. Built
with the platform layer's POSIX flags, so none of the headers' declarations
are seen; none of these is ever called.
*/

#define STAND_IN(name) void name(void) {}

STAND_IN(D3DDevice_SetRenderState_BackFillMode)
STAND_IN(D3DDevice_SetRenderState_CullMode)
STAND_IN(D3DDevice_SetRenderState_Deferred)
STAND_IN(D3DDevice_SetRenderState_DoNotCullUncompressed)
STAND_IN(D3DDevice_SetRenderState_Dxt1NoiseEnable)
STAND_IN(D3DDevice_SetRenderState_EdgeAntiAlias)
STAND_IN(D3DDevice_SetRenderState_FillMode)
STAND_IN(D3DDevice_SetRenderState_FogColor)
STAND_IN(D3DDevice_SetRenderState_FrontFace)
STAND_IN(D3DDevice_SetRenderState_LineWidth)
STAND_IN(D3DDevice_SetRenderState_LogicOp)
STAND_IN(D3DDevice_SetRenderState_MultiSampleAntiAlias)
STAND_IN(D3DDevice_SetRenderState_MultiSampleMask)
STAND_IN(D3DDevice_SetRenderState_MultiSampleType)
STAND_IN(D3DDevice_SetRenderState_NormalizeNormals)
STAND_IN(D3DDevice_SetRenderState_OcclusionCullEnable)
STAND_IN(D3DDevice_SetRenderState_PSTextureModes)
STAND_IN(D3DDevice_SetRenderState_RopZCmpAlwaysRead)
STAND_IN(D3DDevice_SetRenderState_RopZRead)
STAND_IN(D3DDevice_SetRenderState_ShadowFunc)
STAND_IN(D3DDevice_SetRenderState_Simple)
STAND_IN(D3DDevice_SetRenderState_StencilCullEnable)
STAND_IN(D3DDevice_SetRenderState_StencilEnable)
STAND_IN(D3DDevice_SetRenderState_StencilFail)
STAND_IN(D3DDevice_SetRenderState_TextureFactor)
STAND_IN(D3DDevice_SetRenderState_TwoSidedLighting)
STAND_IN(D3DDevice_SetRenderState_VertexBlend)
STAND_IN(D3DDevice_SetRenderState_YuvEnable)
STAND_IN(D3DDevice_SetRenderState_ZBias)
STAND_IN(D3DDevice_SetRenderState_ZEnable)

/* (the render states' table: D3DRS_MAX entries is far fewer than this) */
unsigned long D3D__RenderState[1024];
