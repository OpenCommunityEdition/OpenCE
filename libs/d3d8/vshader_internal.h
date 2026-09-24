/* Shader state layouts from the supplied January PDB. */
#ifndef D3D8_VSHADER_INTERNAL_H
#define D3D8_VSHADER_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
struct VertexShaderSlot
{
    unsigned long StreamIndex; /* +0x0 */
    unsigned long Offset; /* +0x4 */
    unsigned long SizeAndType; /* +0x8 */
    unsigned char Flags; /* +0xc */
    unsigned char Source; /* +0xd */
};
typedef char VertexShaderSlot_size_check[sizeof(VertexShaderSlot) == 16 ? 1 : -1];
struct VertexShader
{
    unsigned long RefCount; /* +0x0 */
    unsigned long Flags; /* +0x4 */
    unsigned long MaxSlot; /* +0x8 */
    unsigned long TextureCount; /* +0xc */
    unsigned long ProgramSize; /* +0x10 */
    unsigned long ProgramAndConstantsDwords; /* +0x14 */
    unsigned long Dimensionality[4]; /* +0x18 */
    D3D::VertexShaderSlot Slot[20]; /* +0x28 */
    unsigned long ProgramAndConstants[1]; /* +0x168 */
};
typedef char VertexShader_size_check[sizeof(VertexShader) == 364 ? 1 : -1];
struct Stream
{
    unsigned long Stride; /* +0x0 */
    unsigned long Offset; /* +0x4 */
    D3DVertexBuffer *pVertexBuffer; /* +0x8 */
};
typedef char Stream_size_check[sizeof(Stream) == 12 ? 1 : -1];
extern const DWORD g_PassthruProgramSpecularFog[48];
extern const DWORD g_PassthruProgramZFog[44];
extern const DWORD g_PassthruProgramWFog[48];
extern VertexShader g_FvfVertexShader;
extern Stream g_Stream[16];
extern const DWORD g_TextureCount[4];
extern const DWORD g_FixedFunctionPipelineConstants[12];
extern const BYTE g_UnitsOfElement[];
extern const BYTE g_BytesPerUnit[];
enum
{
    VERTEXSHADER_WRITE = 1,
    VERTEXSHADER_PASSTHROUGH = 2,
    VERTEXSHADER_PROGRAM = 4,
    VERTEXSHADER_STATE = 8,
    VERTEXSHADER_HASDIFFUSE = 0x400,
    VERTEXSHADER_HASSPECULAR = 0x800,
    VERTEXSHADER_HASBACKDIFFUSE = 0x1000,
    VERTEXSHADER_HASBACKSPECULAR = 0x2000,
    SIZEANDTYPE_DISABLED = 2,
    VSHADER_BATCH_LIMIT = 8,
    VSHADER_PROGRAM_SLOTS = 136,
    VSHADER_CONSTANT_BIAS = 96,
    VERTEXSHADER_SLOTS = 20,
    SLOT_POSITION = 0,
    SLOT_WEIGHT = 1,
    SLOT_NORMAL = 2,
    SLOT_DIFFUSE = 3,
    SLOT_SPECULAR = 4,
    SLOT_TEXTURE0 = 16
};
void WINAPI InitializeVertexShaderFromFvf(
    VertexShader *shader,
    DWORD fvf);
void WINAPI ShadowVertexShaderState(
    VertexShader *shader,
    DWORD address);
DWORD *WINAPI ParseProgram(
    DWORD *push,
    const DWORD *program,
    DWORD dwords);
void WINAPI ParseDeclarationStream(
    VertexShader *shader,
    const DWORD *declaration,
    BOOL fixedFunction);
DWORD WINAPI ParseDeclarationConstants(
    const DWORD *declaration,
    DWORD *push);
void WINAPI RestoreVertexShaders(
    const DWORD *program,
    DWORD dwords);
void __fastcall CommonSetPassthroughProgram(
    CDevice *device);
}
#endif
