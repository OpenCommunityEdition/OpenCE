/*
VM_NV2A_STUBS.C

What the shader translators' objects reach and the VM driver
(vm_nv2a.c) does not run: the render state setters of the Xbox SDK
stand-ins' inline functions (port/include/xdk/xdk_d3d8.h), which the
translators' objects carry uncalled, and the render state they read.
*/

unsigned long D3D__RenderState[256];

void D3DDevice_SetRenderState_BackFillMode(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_CullMode(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_Deferred(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_DoNotCullUncompressed(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_Dxt1NoiseEnable(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_EdgeAntiAlias(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_FillMode(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_FogColor(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_FrontFace(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_LineWidth(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_LogicOp(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_MultiSampleAntiAlias(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_MultiSampleMask(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_MultiSampleType(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_NormalizeNormals(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_OcclusionCullEnable(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_PSTextureModes(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_RopZCmpAlwaysRead(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_RopZRead(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_ShadowFunc(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_Simple(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_StencilCullEnable(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_StencilEnable(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_StencilFail(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_TextureFactor(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_TwoSidedLighting(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_VertexBlend(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_YuvEnable(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_ZBias(unsigned long value) { (void)value; }
void D3DDevice_SetRenderState_ZEnable(unsigned long value) { (void)value; }
