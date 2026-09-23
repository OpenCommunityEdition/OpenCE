/* Portions adapted with reference to RXDK-Libs (2026 Team Resurgent),
 * GPL-3.0-or-later. See libs/d3d8/PROVENANCE.md and libs/d3d8/LICENSE.GPL-3.0.
 * Original January target bytes and PDB remain the matching authority. */
#ifndef D3D8_GRAPHICS_CONTEXT_INTERNAL_H
#define D3D8_GRAPHICS_CONTEXT_INTERNAL_H
#include "hardware_internal.h"
namespace D3D
{
/* NV2A saved graphics context layout. These lengths are fixed hardware banks;
 * January HalGrInit3d supplies the offsets, extents, write order and values.
 * Field/bank names are corroborated by the pinned RXDK-Libs mphal reference.
 * This is instance-memory initialization, not emitted CPU instruction bytes. */
enum GraphicsContextLayout
{
    GRCTX_COMMON_DWORDS = 0x33c / sizeof(DWORD),
    GRCTX_3D_DWORDS = 0x2f0 / sizeof(DWORD),
    IDX_FMT_COUNT = 35,
    CAS0_COUNT = 29,
    CAS1_COUNT = 29,
    CAS2_COUNT = 29,
    ASSM_STATE_COUNT = 2,
    IDX_CACHE0_COUNT = 128,
    IDX_CACHE1_COUNT = 128,
    IDX_CACHE2_COUNT = 128,
    IDX_CACHE3_COUNT = 128,
    VTX_FILE0_COUNT = 192,
    VTX_FILE1_COUNT = 480,
    XL_P_COUNT = 544,
    XL_XFCTX_COUNT = 768,
    XL_LTCTXA_COUNT = 104,
    XL_LTCTXB_COUNT = 208,
    XL_LTC0_COUNT = 4,
    XL_LTC1_COUNT = 20,
    XL_LTC2_COUNT = 15,
    XL_LTC3_COUNT = 14,
    XL_VAB_COUNT = 68,
    CRSTR_STIPP_COUNT = 32,
    FD_CTRL_COUNT = 15,
    ROP_REGS_COUNT = 1,
};
}
#endif
