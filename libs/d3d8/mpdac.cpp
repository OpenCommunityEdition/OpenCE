/* January target/PDB reconstruction. Hardware writes are not executed here. */
#include <conio.h>
#include "hardware_internal.h"
#pragma intrinsic(_inp)
#pragma code_seg("D3D")
namespace D3D
{
void CMiniport::DacProgramGammaRamp(
    D3DGAMMARAMP *ramp)
{
    volatile BYTE *registers = (volatile BYTE *)m_RegisterBase;
    registers[0x6813c8] = 0;
    for (DWORD i = 0; i < 256; ++i)
    {
        registers[0x6813c9] = ramp->red[i];
        registers[0x6813c9] = ramp->green[i];
        registers[0x6813c9] = ramp->blue[i];
    }
    return;
}
}

namespace D3D
{
void CMiniport::DacProgramVideoStart(
    DWORD offset)
{
    DWORD oddField = ((DWORD)_inp(0x80c0) & 0x20UL) >> 5;
    volatile DWORD *registers = (volatile DWORD *)m_RegisterBase;
    DWORD mode = m_DisplayMode & 0xc0000000UL;
    if (mode == 0x40000000UL) oddField = !oddField;
    if (!m_FirstFlip)
    {
        if ((m_CurrentAvInfo & 0x01000000) && mode != 0x80000000UL && oddField)
        {
            offset -= m_SurfacePitch / 2;
        }
        else if (mode == 0x80000000UL && (m_CurrentAvInfo & 0x00200000) && oddField)
        {
            offset += m_SurfacePitch;
        }
        registers[0x600800 / sizeof(DWORD)] = offset;
    }
    return;
}
}
