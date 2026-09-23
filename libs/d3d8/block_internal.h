/* State-block record layout from the January PDB (StateBlock, 0x40 bytes).
 * Serialized records follow the 0x3c-byte counter header in one allocation. */
#ifndef D3D8_BLOCK_INTERNAL_H
#define D3D8_BLOCK_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
enum StateBlockFlags
{
    STATE_RECORDBLOCK = 0x20,
    LIGHT_SET = 1,
    LIGHT_STATEBLOCK_SET = 2,
    LIGHT_STATEBLOCK_ENABLED = 4
};
struct StateBlock
{
    DWORD D0_Textures;
    BOOL D1_PixelShader;
    BOOL D2_VertexShader;
    BOOL D3_Index;
    DWORD D4_Streams;
    DWORD D6_PixelShaderConstants;
    DWORD D7_VertexShaderConstants;
    DWORD D8_RenderStates;
    DWORD D9_TextureStates;
    DWORD D10_Transforms;
    BOOL D11_Viewport;
    BOOL D12_Material;
    BOOL D13_BackMaterial;
    DWORD D14_Lights;
    DWORD D15_LightEnables;
    DWORD Data[1];
};
typedef char StateBlock_size_check[sizeof(StateBlock) == 0x40 ? 1 : -1];
typedef char StateBlock_data_check[offsetof(StateBlock, Data) == 0x3c ? 1 : -1];
typedef char StateBlock_D0_Textures_offset[offsetof(StateBlock, D0_Textures) == 0 ? 1 : -1];
typedef char StateBlock_D1_PixelShader_offset[offsetof(StateBlock, D1_PixelShader) == 4 ? 1 : -1];
typedef char StateBlock_D2_VertexShader_offset[offsetof(StateBlock, D2_VertexShader) == 8 ? 1 : -1];
typedef char StateBlock_D3_Index_offset[offsetof(StateBlock, D3_Index) == 12 ? 1 : -1];
typedef char StateBlock_D4_Streams_offset[offsetof(StateBlock, D4_Streams) == 16 ? 1 : -1];
typedef char StateBlock_D6_PixelShaderConstants_offset[offsetof(StateBlock, D6_PixelShaderConstants) == 20 ? 1 : -1];
typedef char StateBlock_D7_VertexShaderConstants_offset[offsetof(StateBlock, D7_VertexShaderConstants) == 24 ? 1 : -1];
typedef char StateBlock_D8_RenderStates_offset[offsetof(StateBlock, D8_RenderStates) == 28 ? 1 : -1];
typedef char StateBlock_D9_TextureStates_offset[offsetof(StateBlock, D9_TextureStates) == 32 ? 1 : -1];
typedef char StateBlock_D10_Transforms_offset[offsetof(StateBlock, D10_Transforms) == 36 ? 1 : -1];
typedef char StateBlock_D11_Viewport_offset[offsetof(StateBlock, D11_Viewport) == 40 ? 1 : -1];
typedef char StateBlock_D12_Material_offset[offsetof(StateBlock, D12_Material) == 44 ? 1 : -1];
typedef char StateBlock_D13_BackMaterial_offset[offsetof(StateBlock, D13_BackMaterial) == 48 ? 1 : -1];
typedef char StateBlock_D14_Lights_offset[offsetof(StateBlock, D14_Lights) == 52 ? 1 : -1];
typedef char StateBlock_D15_LightEnables_offset[offsetof(StateBlock, D15_LightEnables) == 56 ? 1 : -1];
/* The cursor addresses the allocation's serialized tail, rather than indexing
 * beyond the PDB's one-element placeholder array. Callers require a valid,
 * fully populated state-block allocation. Counts are not untrusted input. */
inline DWORD *StateBlockData(
    StateBlock *block)
{
    return (DWORD *)((BYTE *)block + offsetof(StateBlock, Data));
}
void WINAPI ClearStateBlockFlags(
    void);
HRESULT WINAPI RecordStateBlock(
    DWORD *token);
extern const BYTE g_PixelRenderStates[];
extern const BYTE g_PixelTextureStates[];
extern const BYTE g_VertexRenderStates[];
extern const BYTE g_VertexTextureStates[];
}
#endif
