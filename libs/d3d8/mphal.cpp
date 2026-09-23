/* Portions adapted with reference to RXDK-Libs (2026 Team Resurgent),
 * GPL-3.0-or-later. See libs/d3d8/PROVENANCE.md and libs/d3d8/LICENSE.GPL-3.0.
 * Original January target bytes and PDB remain the matching authority. */
/* NV2A hardware helpers reconstructed from the January target and its PDB.
 * Volatile accesses are actual MMIO, not optimizer steering. No hardware
 * code in this file has been executed against a live GPU by this campaign. */
#include "hardware_internal.h"
#include "miniport_constants.h"
#include "kernel_gpu.h"
#include "graphics_context_internal.h"
#pragma code_seg("D3D")
namespace D3D
{
void CMiniport::HalVideoControlInit(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    for (DWORD i = 0; i < 2; ++i)
    {
        WriteGpuRegister(base, 0x8910 + i * 4, 0x1000);
        WriteGpuRegister(base, 0x8918 + i * 4, 0x1000);
        WriteGpuRegister(base, 0x8938 + i * 4, 0x100000);
        WriteGpuRegister(base, 0x8940 + i * 4, 0x100000);
        WriteGpuRegister(base, 0x8930 + i * 4, 0);
        WriteGpuRegister(base, 0x8928 + i * 4, 0xffffffffUL);
    }
    return;
}

void CMiniport::HalGrControlInit(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD context = m_HalInfo.GrCtxTableBase;
    WriteGpuRegister(base, 0x400780, context & 0xffff);
    for (DWORD i = 0; i < 2; ++i)
    {
        WriteGpuRegister(base, ((context + 0x70000) << 4) + i * 4, 0);
    }
    return;
}

void CMiniport::HalGrIdle(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    while (ReadGpuRegister(base, 0x400700))
    {
        DWORD interrupts = ReadGpuRegister(base, 0x100);
        if (interrupts & 0x1000)
        {
            ServiceGrInterrupt();
        }
        if (interrupts & 0x1000000)
        {
            VBlank();
        }
    }
    return;
}

void CMiniport::HalGrUnloadChannelContext(
    DWORD channel)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    if (channel != 2)
    {
        WriteGpuRegister(base, 0x400784, m_HalInfo.GrCtxTable[channel] & 0xffff);
        WriteGpuRegister(base, 0x400788, 2);
        HalGrIdle();
        WriteGpuRegister(base, 0x400144, 0x10000000);
    }
    return;
}

void CMiniport::HalDacUnload(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    WriteGpuRegister(base, 0x600100, 1);
    WriteGpuRegister(base, 0x600140, 0);
    return;
}

void CMiniport::HalDacLoad(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    WriteGpuRegister(base, 0x600100, 1);
    WriteGpuRegister(base, 0x600140, 1);
    return;
}

void CMiniport::HalFifoHashAdd(
    DWORD index,
    DWORD handle,
    DWORD channel,
    DWORD instance,
    DWORD engine)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD entry = m_HalInfo.HashTableAddr + index * sizeof(HW_HASHENTRY);
    WriteGpuRegister(base, entry, handle);
    DWORD context = ((((channel | 0x80) << 8) | engine) << 16) | (instance & 0xffff);
    WriteGpuRegister(base, entry + 4, context);
    return;
}

void CMiniport::HalGrInitObjectContext(
    DWORD instance,
    DWORD classNumber)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD context = classNumber & 0xff;
    DWORD extra = 0;
    switch (classNumber)
    {
    case NV03_MEMORY_TO_MEMORY_FORMAT:
        context |= 0x1000000;
        break;
    case NV10_CONTEXT_SURFACES_2D:
        break;
    case NV20_KELVIN_PRIMITIVE:
        extra = 0xa00;
        break;
    }
    WriteGpuRegister(base, (0x70000 + instance) << 4, context);
    WriteGpuRegister(base, 0x700004 + (instance << 4), extra);
    WriteGpuRegister(base, 0x700008 + (instance << 4), 0);
    WriteGpuRegister(base, 0x70000c + (instance << 4), 0);
    return;
}
}

namespace D3D
{
void CMiniport::HalFifoControlInit(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    m_HalInfo.FifoRetryCount = 0xff;
    m_HalInfo.FifoUserBase = 0x800000;
    m_HalInfo.FifoIntrEn0 = 0x01111111;
    WriteGpuRegister(base, 0x3210, 0);
    WriteGpuRegister(base, 0x3270, 0);
    WriteGpuRegister(base, 0x3240, 0);
    WriteGpuRegister(base, 0x3244, 0);
    WriteGpuRegister(base, 0x3058, 0);
    WriteGpuRegister(base, 0x3258, 0);
    WriteGpuRegister(base, 0x2504, m_HalInfo.FifoMode);
    WriteGpuRegister(base, 0x2508, 0);
    WriteGpuRegister(base, 0x250c, 0);
    WriteGpuRegister(base, 0x3228, 0);
    WriteGpuRegister(base, 0x2410, 0);
    WriteGpuRegister(base, 0x2420, 0);
    return;
}

void CMiniport::HalFifoControlLoad(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    WriteGpuRegister(base, 0x3224, 0xf0078);
    WriteGpuRegister(base, 0x2044, 0x0101ffff);
    WriteGpuRegister(base, 0x2040, m_HalInfo.FifoRetryCount & 0x3ff);
    WriteGpuRegister(base, 0x2500, 0);
    WriteGpuRegister(base, 0x3000, 0);
    WriteGpuRegister(base, 0x3050, 0);
    WriteGpuRegister(base, 0x3200, 0);
    WriteGpuRegister(base, 0x3250, 0);
    WriteGpuRegister(base, 0x3220, 0);
    HalFifoContextSwitch(1);
    WriteGpuRegister(base, 0x3210, 0);
    WriteGpuRegister(base, 0x3270, 0);
    WriteGpuRegister(base, 0x3250, 1);
    WriteGpuRegister(base, 0x3200, 1);
    WriteGpuRegister(base, 0x2500, 1);
    WriteGpuRegister(base, 0x2500, 0);
    return;
}

void CMiniport::HalDacControlInit(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    WriteGpuRegister8(base, 0x6013d4, 0x1f);
    BYTE lock = ReadGpuRegister8(base, 0x6013d5);
    WriteGpuRegister8(base, 0x6013d4, 0x1f);
    WriteGpuRegister8(base, 0x6013d5, 0x57);
    WriteGpuRegister8(base, 0x6013d4, 0x52);
    BYTE value = ReadGpuRegister8(base, 0x6013d5);
    WriteGpuRegister8(base, 0x6013d4, 0x52);
    WriteGpuRegister8(base, 0x6013d5, (BYTE)(value + 4));
    WriteGpuRegister(base, 0x8088, (ReadGpuRegister(base, 0x8088) & 0xfffff43f) | 0x400);
    WriteGpuRegister(base, 0x8088, (ReadGpuRegister(base, 0x8088) & 0xf43fffff) | 0x4000000);
    WriteGpuRegister(base, 0x808c, (ReadGpuRegister(base, 0x808c) & 0xfffff40f) | 0x400);
    WriteGpuRegister(base, 0x808c, (ReadGpuRegister(base, 0x808c) & 0xf40fffff) | 0x4000000);
    WriteGpuRegister8(base, 0x6013d4, 0x20);
    WriteGpuRegister8(base, 0x6013d5, 0x29);
    WriteGpuRegister8(base, 0x6013d4, 0x1b);
    WriteGpuRegister8(base, 0x6013d5, 5);
    WriteGpuRegister(base, 0x600804, (ReadGpuRegister(base, 0x600804) & ~5UL) | 2);
    if (!lock)
    {
        WriteGpuRegister8(base, 0x6013d4, 0x1f);
        WriteGpuRegister8(base, 0x6013d5, 0x99);
    }
    return;
}

void CMiniport::HalMpControlInit(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    /* The initial read is a real hardware probe; its value is discarded by
     * the January implementation, not a temporary introduced for allocation. */
    (void)*(volatile DWORD *)((BYTE *)base + 0x200200);
    m_GenInfo.MpVIPSlavePresent = FALSE;
    WriteGpuRegister(base, 0x200140, 0);
    WriteGpuRegister(base, 0x200200, (ReadGpuRegister(base, 0x200200) & ~0x1fUL) | 0x20);
    WriteGpuRegister(base, 0x200200, ReadGpuRegister(base, 0x200200) | 0x80);
    WriteGpuRegister(base, 0x200200, (ReadGpuRegister(base, 0x200200) & ~0x800UL) | 0x700);
    WriteGpuRegister(base, 0x200428, 0);
    WriteGpuRegister(base, 0x20042c, 0);
    WriteGpuRegister(base, 0x200430, 0);
    WriteGpuRegister(base, 0x200458, 0);
    WriteGpuRegister(base, 0x20045c, 0);
    WriteGpuRegister(base, 0x200460, 0);
    WriteGpuRegister(base, 0x200488, 0);
    WriteGpuRegister(base, 0x20048c, 0);
    WriteGpuRegister(base, 0x200490, 0);
    WriteGpuRegister(base, 0x200350, 0);
    WriteGpuRegister(base, 0x200354, 0);
    WriteGpuRegister(base, 0x200358, 0);
    WriteGpuRegister(base, 0x200390, 0);
    WriteGpuRegister(base, 0x200394, 0);
    WriteGpuRegister(base, 0x200398, 0);
    WriteGpuRegister(base, 0x200400, 0);
    WriteGpuRegister(base, 0x200204, (ReadGpuRegister(base, 0x200204) & ~0xbUL) | 4);
    WriteGpuRegister(base, 0x200204, ReadGpuRegister(base, 0x200204) & ~0x300UL);
    WriteGpuRegister(base, 0x200208, (ReadGpuRegister(base, 0x200208) & ~0xaUL) | 5);
    WriteGpuRegister(base, 0x200208, ReadGpuRegister(base, 0x200208) & ~0x300UL);
    return;
}
}

namespace D3D
{
void CMiniport::HalMcControlInit(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    m_HalInfo.McSave = ReadGpuRegister(base, 0x200);
    if (m_HalInfo.McSave == 0)
    {
        WriteGpuRegister(base, 0x200, 0);
    }
    m_HalInfo.McSaveIntrEn0 = ReadGpuRegister(base, 0x140);
    WriteGpuRegister(base, 0x200, 0xffffffffUL);

    DWORD m = ReadGpuRegister(base, 0x680504) & 0xff;
    DWORD n = (ReadGpuRegister(base, 0x680504) >> 8) & 0xff;
    DWORD p = (ReadGpuRegister(base, 0x680504) >> 16) & 7;
    m_DacInfo.MPllM = m;
    m_DacInfo.MPllN = n;
    m_DacInfo.MPllO = 1;
    m_DacInfo.MPllP = p;
    m_DacInfo.MClk = m ? (m_GenInfo.CrystalFreq * n) / (1UL << p) / m : 0;

    m = ReadGpuRegister(base, 0x680508) & 0xff;
    n = (ReadGpuRegister(base, 0x680508) >> 8) & 0xff;
    p = (ReadGpuRegister(base, 0x680508) >> 16) & 7;
    m_DacInfo.VPllM = m;
    m_DacInfo.VPllN = n;
    m_DacInfo.VPllO = 1;
    m_DacInfo.VPllP = p;
    m_DacInfo.VClk = m ? (m_GenInfo.CrystalFreq * n) / (1UL << p) / m : 0;

    m = ReadGpuRegister(base, 0x680500) & 0xff;
    n = (ReadGpuRegister(base, 0x680500) >> 8) & 0xff;
    p = (ReadGpuRegister(base, 0x680500) >> 16) & 7;
    m_DacInfo.NVPllM = m;
    m_DacInfo.NVPllN = n;
    m_DacInfo.NVPllO = 1;
    m_DacInfo.NVPllP = p;
    m_DacInfo.NVClk = m ? (m_GenInfo.CrystalFreq * n) / (1UL << p) / m : 0;
    return;
}

void CMiniport::HalFbControlInit(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    m_HalInfo.FbSave0 = ReadGpuRegister(base, 0x100200);
    m_HalInfo.FbSave1 = ReadGpuRegister(base, 0x100204);
    m_GenInfo.VideoRamType = (ReadGpuRegister(base, 0x1218) & 0x100) == 0x100 ? 2 : 3;

    DWORD instancePadding;
    // The returned pointer marks the upper end of the reserved instance area.
    // Both it and its 20 KiB prefix lie in the kernel-owned instance mapping.
    m_InstMem = (BYTE *)MmClaimGpuInstanceMemory(20 * 1024, &instancePadding) - 20 * 1024;
    DWORD instance = 0x700000 + instancePadding;
    m_HalInfo.HashTableAddr = instance;
    WriteGpuRegister(base, 0x2210, ((instancePadding >> 8) & 0x1f0) | 0x03000000);

    instance += 0x1000;
    m_HalInfo.FifoContextAddr1 = instance;
    instance += 0x80;
    m_HalInfo.FifoContextAddr2 = instance;
    WriteGpuRegister(base, 0x2214,
        ((m_HalInfo.FifoContextAddr1 >> 8) & 0x1fc) |
        (((m_HalInfo.FifoContextAddr2 & 0x1fc00) | 0x200) << 7));
    instance += 0x20;
    m_FreeInstAddr = (instance - 0x700000) >> 4;
    WriteGpuRegister(base, 0x100214, ReadGpuRegister(base, 0x100214) & ~1UL);
    for (DWORD address = 0x700000; address < 0x705000; address += sizeof(DWORD))
    {
        WriteGpuRegister(base, address + instancePadding, 0);
    }
    m_HalInfo.GrCtxTableBase = ReserveInstMem(8);
    return;
}
}

namespace D3D
{
void CMiniport::HalGrLoadChannelContext(
    DWORD channel)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    if (ReadGpuRegister(base, 0x400100))
    {
        ServiceGrInterrupt();
    }
    DWORD savedFifo = ReadGpuRegister(base, 0x400720);
    WriteGpuRegister(base, 0x400720, 0);
    HalGrIdle();
    if (m_HalInfo.GrChID != channel)
    {
        HalGrUnloadChannelContext(m_HalInfo.GrChID);
    }
    m_HalInfo.GrChID = channel;
    if (channel == 2)
    {
        WriteGpuRegister(base, 0x400144, 0x10000100);
        WriteGpuRegister(base, 0x400764, 0x08000000);
        WriteGpuRegister(base, 0x400720, savedFifo | 1);
        return;
    }
    if (m_HalInfo.GrCurrentObjects3d[channel])
    {
        WriteGpuRegister(base, 0x400080, 0x70000);
        // Hardware readbacks bracket this reset; values are deliberately ignored.
        (void)*(volatile DWORD *)((BYTE *)base + 0x400080);
        WriteGpuRegister(base, 0x400080, 0);
        (void)*(volatile DWORD *)((BYTE *)base + 0x400080);
        WriteGpuRegister(base, 0x400750, 0x3d0000);
        for (DWORD i = 0; i < 15; ++i)
        {
            WriteGpuRegister(base, 0x400754, 0);
        }
    }
    WriteGpuRegister(base, 0x400084, ReadGpuRegister(base, 0x400084) | 0x01000000);
    WriteGpuRegister(base, 0x400148, (channel & 31) << 24);
    WriteGpuRegister(base, 0x400784, m_HalInfo.GrCtxTable[channel] & 0xffff);
    WriteGpuRegister(base, 0x400788, 1);
    HalGrIdle();
    WriteGpuRegister(base, 0x400148,
        (ReadGpuRegister(base, 0x400148) & 0xe0ffffffUL) | ((channel & 31) << 24));
    WriteGpuRegister(base, 0x400144, 0x10010100);
    WriteGpuRegister(base, 0x400764, ReadGpuRegister(base, 0x400764) & 0xcfffffffUL);
    return;
}

void CMiniport::HalGrControlLoad(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    WriteGpuRegister(base, 0x200, ReadGpuRegister(base, 0x200) & ~0x1000UL);
    WriteGpuRegister(base, 0x200, ReadGpuRegister(base, 0x200) | 0x1000UL);
    m_DebugRegister[6] = 0x45ead10e;
    m_DebugRegister[1] = 0x00118700;
    m_DebugRegister[2] = 0x0008c3ff;
    m_DebugRegister[3] = 0xf3de0479;
    m_DebugRegister[4] = 0;
    m_DebugRegister[5] = 4;
    m_DebugRegister[0] = 0;
    m_DebugRegister[6] = (ReadGpuRegister(base, 0x1218) & 0x100) == 0x100 ?
        0x45ead10f : 0x45ead10e;
    m_DebugRegister[7] = 0;
    m_DebugRegister[8] = 0x78;
    m_DebugRegister[9] = 0x40;
    m_DebugRegister[10] = 0;
    WriteGpuRegister(base, 0x400080, 0);
    WriteGpuRegister(base, 0x400084, m_DebugRegister[1]);
    WriteGpuRegister(base, 0x400880, m_DebugRegister[2]);
    WriteGpuRegister(base, 0x40008c, m_DebugRegister[3]);
    WriteGpuRegister(base, 0x400090, m_DebugRegister[4]);
    WriteGpuRegister(base, 0x400094, m_DebugRegister[5]);
    WriteGpuRegister(base, 0x400b80, m_DebugRegister[6]);
    WriteGpuRegister(base, 0x400b84, m_DebugRegister[7]);
    WriteGpuRegister(base, 0x400098, m_DebugRegister[8]);
    WriteGpuRegister(base, 0x40009c, m_DebugRegister[9]);
    WriteGpuRegister(base, 0x400b88, m_DebugRegister[10]);
    WriteGpuRegister(base, 0x400780, m_HalInfo.GrCtxTableBase & 0xffff);
    HalGrIdle();

    // Mirror each framebuffer tile's limit, pitch and base into PGRAPH and RDI.
    DWORD value;
    for (long tile = 0; tile < 8; ++tile)
    {
        value = ReadGpuRegister(base, 0x100244 + tile * 16);
        WriteGpuRegister(base, 0x400904 + tile * 16, value);
        WriteGpuRegister(base, 0x400750, 0xea0000 | ((0x30 + tile * 4) & 0x1ffc));
        WriteGpuRegister(base, 0x400754, value);
        value = ReadGpuRegister(base, 0x100248 + tile * 16);
        WriteGpuRegister(base, 0x400908 + tile * 16, value);
        WriteGpuRegister(base, 0x400750, 0xea0000 | ((0x50 + tile * 4) & 0x1ffc));
        WriteGpuRegister(base, 0x400754, value);
        value = ReadGpuRegister(base, 0x100240 + tile * 16);
        WriteGpuRegister(base, 0x400900 + tile * 16, value);
        WriteGpuRegister(base, 0x400750, 0xea0000 | ((0x10 + tile * 4) & 0x1ffc));
        WriteGpuRegister(base, 0x400754, value);
    }
    for (long tile = 0; tile < 8; ++tile)
    {
        value = ReadGpuRegister(base, 0x100300 + tile * 4);
        WriteGpuRegister(base, 0x400980 + tile * 4, value);
        WriteGpuRegister(base, 0x400750, 0xea0000 | ((0x90 + tile * 4) & 0x1ffc));
        WriteGpuRegister(base, 0x400754, value);
    }
    value = ReadGpuRegister(base, 0x100324);
    WriteGpuRegister(base, 0x4009a0, value);
    WriteGpuRegister(base, 0x400750, 0xea000c);
    WriteGpuRegister(base, 0x400754, value);
    value = ReadGpuRegister(base, 0x100200);
    WriteGpuRegister(base, 0x4009a4, value);
    WriteGpuRegister(base, 0x400750, 0xea0000);
    WriteGpuRegister(base, 0x400754, value);
    value = ReadGpuRegister(base, 0x100204);
    WriteGpuRegister(base, 0x4009a8, value);
    WriteGpuRegister(base, 0x400750, 0xea0004);
    WriteGpuRegister(base, 0x400754, value);
    WriteGpuRegister(base, 0x40014c, 0);
    WriteGpuRegister(base, 0x400150, 0);
    WriteGpuRegister(base, 0x400154, 0);
    WriteGpuRegister(base, 0x400158, 0);
    WriteGpuRegister(base, 0x400144, 0x10000000);
    WriteGpuRegister(base, 0x400764, 0x08000000);
    HalGrLoadChannelContext(m_HalInfo.GrChID);
    return;
}
}

namespace D3D
{
void CMiniport::HalFifoContextSwitch(
    DWORD channel)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD contextBase = m_HalInfo.FifoContextAddr1;
    DWORD fifoReassign = ReadGpuRegister(base, 0x2500);
    DWORD fifoPush = ReadGpuRegister(base, 0x3200);
    DWORD fifoPull = ReadGpuRegister(base, 0x3250);
    WriteGpuRegister(base, 0x2500, 0);
    WriteGpuRegister(base, 0x3200, 0);
    WriteGpuRegister(base, 0x3250, 0);

    DWORD oldChannel = ReadGpuRegister(base, 0x3204) & 31;
    volatile FifoChannelContext *context = (volatile FifoChannelContext *)
        ((BYTE *)base + contextBase + oldChannel * sizeof(FifoChannelContext));
    context->Put = ReadGpuRegister(base, 0x3240);
    context->Get = ReadGpuRegister(base, 0x3244);
    context->Ref = ReadGpuRegister(base, 0x3248);
    context->Instance = ReadGpuRegister(base, 0x322c);
    context->State = ReadGpuRegister(base, 0x3228);
    context->Fetch = ReadGpuRegister(base, 0x3224);
    context->Engine = ReadGpuRegister(base, 0x3280);
    context->Pull = ReadGpuRegister(base, 0x3254);
    context->AcquireValue = ReadGpuRegister(base, 0x3268);
    context->AcquireTimestamp = ReadGpuRegister(base, 0x3264);
    context->AcquireTimeout = ReadGpuRegister(base, 0x3260);
    context->Semaphore = ReadGpuRegister(base, 0x326c);
    context->Subroutine = ReadGpuRegister(base, 0x324c);

    if (ReadGpuRegister(base, 0x3204) & 0x100)
    {
        DWORD pending = ReadGpuRegister(base, 0x2508) & ~(1UL << oldChannel);
        if (ReadGpuRegister(base, 0x3240) != ReadGpuRegister(base, 0x3244))
        {
            pending |= 1UL << oldChannel;
        }
        WriteGpuRegister(base, 0x2508, pending);
    }
    WriteGpuRegister(base, 0x3204, channel & 31);
    if ((m_HalInfo.FifoMode & (1UL << channel)) && channel != 1)
    {
        WriteGpuRegister(base, 0x3204, ReadGpuRegister(base, 0x3204) | 0x100);
    }
    context = (volatile FifoChannelContext *)
        ((BYTE *)base + (contextBase + channel * sizeof(FifoChannelContext)));
    WriteGpuRegister(base, 0x3240, context->Put);
    WriteGpuRegister(base, 0x3244, context->Get);
    WriteGpuRegister(base, 0x3248, context->Ref);
    WriteGpuRegister(base, 0x322c, context->Instance);
    WriteGpuRegister(base, 0x3228, context->State);
    WriteGpuRegister(base, 0x3224, context->Fetch);
    WriteGpuRegister(base, 0x3280, context->Engine);
    WriteGpuRegister(base, 0x3254, context->Pull);
    WriteGpuRegister(base, 0x3268, context->AcquireValue);
    WriteGpuRegister(base, 0x3264, context->AcquireTimestamp);
    WriteGpuRegister(base, 0x3260, context->AcquireTimeout);
    WriteGpuRegister(base, 0x326c, context->Semaphore);
    WriteGpuRegister(base, 0x324c, context->Subroutine);
    if ((m_HalInfo.FifoMode & (1UL << channel)) && channel != 1)
    {
        WriteGpuRegister(base, 0x3220, 1);
    }
    WriteGpuRegister(base, 0x204c, 0x1fffff);
    WriteGpuRegister(base, 0x3250, fifoPull);
    WriteGpuRegister(base, 0x3200, fifoPush);
    WriteGpuRegister(base, 0x2500, fifoReassign);
    return;
}

void CMiniport::HalFifoAllocDMA(
    DWORD fetchTrigger,
    DWORD fetchSize,
    DWORD fetchRequests,
    OBJECTINFO *dataContext)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD channel = m_HalInfo.FifoChID;
    // Read the reassignment latch before disabling it.
    (void)*(volatile DWORD *)((BYTE *)base + 0x2500);
    WriteGpuRegister(base, 0x2500, 0);
    for (DWORD i = 0; i < 0xdfc; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (m_HalInfo.FifoInstance << 4) + i * 4, 0);
    }
    WriteGpuRegister(base, 0x700000 + (m_HalInfo.GrCtxTableBase << 4) + channel * 4,
        m_HalInfo.FifoInstance);
    m_HalInfo.GrCtxTable[channel] = m_HalInfo.FifoInstance;

    volatile FifoChannelContext *context = (volatile FifoChannelContext *)
        ((BYTE *)base + m_HalInfo.FifoContextAddr1 + channel * sizeof(FifoChannelContext));
    for (DWORD i = 0; i < sizeof(FifoChannelContext); i += sizeof(DWORD))
    {
        *(volatile DWORD *)((BYTE *)context + i) = 0;
    }
    context->Instance = dataContext->Instance;
    if (fetchTrigger < 8) fetchTrigger = 8;
    if (fetchTrigger > 256) fetchTrigger = 256;
    fetchTrigger = (fetchTrigger >> 3) - 1;
    if (fetchSize < 32) fetchSize = 32;
    if (fetchSize > 256) fetchSize = 256;
    fetchSize = (fetchSize >> 5) - 1;
    if (fetchRequests > 15) fetchRequests = 15;
    DWORD fetch = (((((fetchRequests & 31) << 3) | (fetchSize & 7)) << 10) |
        (fetchTrigger & 31)) << 3;
    context->Fetch = fetch;
    m_HalInfo.FifoMode |= 1UL << channel;
    WriteGpuRegister(base, 0x2504, m_HalInfo.FifoMode);
    WriteGpuRegister(base, 0x3200, 0);
    WriteGpuRegister(base, 0x3250, 0);
    WriteGpuRegister(base, 0x3204, channel & 31);
    if (m_HalInfo.FifoMode & (1UL << channel))
    {
        WriteGpuRegister(base, 0x3204, ReadGpuRegister(base, 0x3204) | 0x100);
    }
    WriteGpuRegister(base, 0x3240, 0);
    WriteGpuRegister(base, 0x3244, 0);
    WriteGpuRegister(base, 0x322c, dataContext->Instance);
    WriteGpuRegister(base, 0x3230, 0);
    WriteGpuRegister(base, 0x3228, 0);
    WriteGpuRegister(base, 0x3280, 0);
    WriteGpuRegister(base, 0x3224, fetch);
    if (m_HalInfo.FifoMode & (1UL << channel))
    {
        WriteGpuRegister(base, 0x3220, 1);
    }
    WriteGpuRegister(base, 0x3250, 1);
    WriteGpuRegister(base, 0x3200, 1);
    WriteGpuRegister(base, 0x2500, 1);
    m_HalInfo.FifoInUse |= 1UL << channel;
    return;
}
}

namespace D3D
{
/* Adapted from the NV2A context initialization layout described in RXDK-Libs
 * (GPL-3.0-or-later), pinned at 29e018674a3443f9df5b5cb4478bb9c54d8a2a3a.
 * January target bytes determine the actual values and ordering below.
 * Requires the reserved, mapped instance-memory context for FifoChID. */
void CMiniport::HalGrInit3d(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD grCtxBuffer = m_HalInfo.GrCtxTable[m_HalInfo.FifoChID];
    DWORD ctxUser = ReadGpuRegister(base, 0x700000 + (grCtxBuffer << 4));
    ctxUser |= 1;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4), ctxUser);

    DWORD offset = GRCTX_COMMON_DWORDS;
    DWORD i;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0xffff0000); /* AntiAliasing */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Blend */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BlendColor */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BorderColor0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BorderColor1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BorderColor2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BorderColor3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat00_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat00_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat00_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat01_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat01_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat01_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat10_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat10_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat10_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat11_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat11_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpMat11_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpOffset1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpOffset2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpOffset3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpScale1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpScale2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BumpScale3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0xfff0000); /* ClearRectX */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0xfff0000); /* ClearRectY */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ColorClearValue */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ColorKeyColor0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ColorKeyColor1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ColorKeyColor2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ColorKeyColor3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_4 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_5 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_6 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor0_7 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_4 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_5 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_6 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineFactor1_7 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI4 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI5 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI6 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaI7 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO4 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO5 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO6 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineAlphaO7 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI4 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI5 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI6 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorI7 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO4 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO5 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO6 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineColorO7 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x101); /* CombineCtl */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineSpecFog0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CombineSpecFog1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Control0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Control1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x111); /* Control2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Control3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* FogColor */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* FogParam0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* FogParam1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* PointSize */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x44400000); /* SetupRaster */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ShaderClipMode */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ShaderCtl */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ShaderProg */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SemaphoreOffset */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ShadowCtl */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ShadowZSlopeThreshold */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SpecFogFactor0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SpecFogFactor1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SurfaceClipX */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SurfaceClipY */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x30303); /* TexAddress0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x30303); /* TexAddress1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x30303); /* TexAddress2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x30303); /* TexAddress3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexCtl0_0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexCtl0_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexCtl0_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexCtl0_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80000); /* TexCtl1_0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80000); /* TexCtl1_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80000); /* TexCtl1_2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80000); /* TexCtl1_3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexCtl2_0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexCtl2_1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x1012000); /* TexFilter0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x1012000); /* TexFilter1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x1012000); /* TexFilter2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x1012000); /* TexFilter3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x105b8); /* TexFormat0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x105b8); /* TexFormat1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x105b8); /* TexFormat2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x105b8); /* TexFormat3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80008); /* TexImageRect0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80008); /* TexImageRect1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80008); /* TexImageRect2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x80008); /* TexImageRect3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexOffset0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexOffset1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexOffset2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexOffset3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexPalette0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexPalette1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexPalette2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* TexPalette3 */
    for (i = 0; i < 8; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x7ff0000); /* WindowClipX */
    }
    for (i = 0; i < 8; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x7ff0000); /* WindowClipY */
    }
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ZCompressOcclude */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ZStencilClearValue */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x4b7fffff); /* ZClipMax */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ZClipMin */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ContextDmaA */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ContextDmaB */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ContextDmaVtxA */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ContextDmaVtxB */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ZOffsetBias */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ZOffsetFactor */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Eyevec0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Eyevec1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Eyevec2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Shadow */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* FdData */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* FdSwatch */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* FdExtras */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Emission0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Emission1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Emission2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SceneAmb0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SceneAmb1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* SceneAmb2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* ContextDmaGetState */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x1); /* BeginPatch0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BeginPatch1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x4000); /* BeginPatch2 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BeginPatch3 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BeginCurve */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x1); /* BeginTrans0 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* BeginTrans1 */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x40000); /* Csv0_D */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x10000); /* Csv0_C */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Csv1_B */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* Csv1_A */
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0); /* CheopsOffset */

    // The pipeline banks immediately follow the 3D register context.
    for (i = 0; i < IDX_FMT_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < CAS0_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < CAS1_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < CAS2_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < ASSM_STATE_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < IDX_CACHE0_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < IDX_CACHE1_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < IDX_CACHE2_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < IDX_CACHE3_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < VTX_FILE0_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < VTX_FILE1_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < XL_P_COUNT; i += 4)
    {
        // Safe initial transform-program instruction, encoded for NV2A.
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x10700ff9);
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x436086c);
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0xc001b);
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    DWORD cheopsCtxOffset = offset;
    for (i = 0; i < XL_XFCTX_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    DWORD zoserCtxAoffset = offset;
    for (i = 0; i < XL_LTCTXA_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < XL_LTCTXB_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    DWORD zoserC0Offset = offset;
    for (i = 0; i < XL_LTC0_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    DWORD zoserC1Offset = offset;
    for (i = 0; i < XL_LTC1_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < XL_LTC2_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < XL_LTC3_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < XL_VAB_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < CRSTR_STIPP_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < FD_CTRL_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }
    for (i = 0; i < ROP_REGS_COUNT; ++i)
    {
        WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    }

    // eye position.
    offset = cheopsCtxOffset + 0xe0;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x3f800000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // transform constants 0.
    offset = cheopsCtxOffset + 0xf0;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x40000000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x3f800000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x3f000000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // transform constants 1.
    offset = cheopsCtxOffset + 0xf4;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x40000000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x3f800000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0xbf800000);

    // transform constants 2.
    offset = cheopsCtxOffset + 0xf8;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0xbf800000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // fog plane.
    offset = cheopsCtxOffset + 0xe4;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x3f800000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // transform constants 3.
    offset = cheopsCtxOffset + 0xfc;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // lighting zero vector.
    offset = zoserCtxAoffset + 0x58;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // lighting eye direction, 22-bit.
    offset = zoserCtxAoffset + 0x40;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0xfe000);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // front ambient.
    offset = zoserCtxAoffset + 0x44;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // color material.
    offset = zoserCtxAoffset + 0x4c;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // linear fog.
    offset = zoserCtxAoffset + 0x60;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0x3f8);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset++ << 2), 0);

    // Negative one and per-light range values use the 22-bit lighting format.
    offset = zoserC0Offset + 1;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0xbf800000UL >> 10);
    offset = zoserC1Offset + 4;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    offset = zoserC1Offset + 5;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    offset = zoserC1Offset + 6;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    offset = zoserC1Offset + 7;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    offset = zoserC1Offset + 8;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    offset = zoserC1Offset + 9;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    offset = zoserC1Offset + 10;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    offset = zoserC1Offset + 11;
    WriteGpuRegister(base, 0x700000 + (grCtxBuffer << 4) + (offset << 2), 0x7149f2caUL >> 10);
    return;
}
}

namespace D3D
{
/* PLL sequencing is reconstructed against January machine code. The related
 * pinned RXDK mphal source corroborates the PLL and refresh-period meanings.
 * Assumes valid nonzero clock divisors in the device's DAC information. */
void CMiniport::HalDacProgramMClk(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD mNew = m_DacInfo.MPllM;
    DWORD nNew = m_DacInfo.MPllN;
    DWORD pNew = m_DacInfo.MPllP;
    DWORD coeff = ReadGpuRegister(base, NV_PRAMDAC_MPLL_COEFF);
    DWORD newCoeff = (mNew & 255) | ((nNew & 255) << 8) | ((pNew & 7) << 16);
    if (coeff != newCoeff)
    {
        DWORD mOld = coeff & 255;
        DWORD nOld = (coeff >> 8) & 255;
        DWORD pOld = (coeff >> 16) & 7;
        if (pNew < pOld)
        {
            WriteGpuRegister(base, NV_PRAMDAC_MPLL_COEFF,
                (mNew & 255) | ((nNew & 255) << 8) | ((pOld & 7) << 16));
            for (DWORD loops = 100; loops; --loops)
            {
                if (((ReadGpuRegister(base, NV_PRAMDAC_PLL_TEST_COUNTER) >> 30) & 1) == 1)
                {
                    if (((ReadGpuRegister(base, NV_PRAMDAC_PLL_TEST_COUNTER) >> 30) & 1) == 1)
                    {
                        break;
                    }
                }
                KeStallExecutionProcessor(1000);
            }
        }
        WriteGpuRegister(base, NV_PRAMDAC_MPLL_COEFF, newCoeff);
        KeStallExecutionProcessor(64 * 1000);
        WriteGpuRegister(base, NV_PFB_EMRS,
            ReadGpuRegister(base, NV_PFB_EMRS) | 0x100000);
        WriteGpuRegister(base, NV_PFB_MRS,
            ReadGpuRegister(base, NV_PFB_MRS) | 0x100);

        DWORD oldMClk = nOld * m_GenInfo.CrystalFreq / (1UL << pOld) / mOld;
        DWORD oldCyclesPerRefresh = ((ReadGpuRegister(base, NV_PFB_TIMING2) >> 5) & 0x7ff) * 32;
        DWORD oldPeriod = oldCyclesPerRefresh * 1024 / (oldMClk / 1000);
        DWORD newCyclesPerRefresh = (m_DacInfo.MClk / 1000) * oldPeriod / 1024;
        WriteGpuRegister(base, NV_PFB_TIMING2,
            (ReadGpuRegister(base, NV_PFB_TIMING2) & ~0xffe0UL) |
            (((newCyclesPerRefresh / 32) & 0x7ff) << 5));
    }
    return;
}

void CMiniport::HalDacProgramNVClk(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD mNew = m_DacInfo.NVPllM;
    DWORD nNew = m_DacInfo.NVPllN;
    DWORD pNew = m_DacInfo.NVPllP;
    DWORD pOld = (ReadGpuRegister(base, NV_PRAMDAC_NVPLL_COEFF) >> 16) & 7;
    if (pNew < pOld)
    {
        WriteGpuRegister(base, NV_PRAMDAC_NVPLL_COEFF,
            (mNew & 255) | ((nNew & 255) << 8) | ((pOld & 7) << 16));
        for (DWORD loops = 100; loops; --loops)
        {
            if (((ReadGpuRegister(base, NV_PRAMDAC_PLL_TEST_COUNTER) >> 29) & 1) == 1)
            {
                if (((ReadGpuRegister(base, NV_PRAMDAC_PLL_TEST_COUNTER) >> 29) & 1) == 1)
                {
                    break;
                }
            }
            KeStallExecutionProcessor(1000);
        }
    }
    WriteGpuRegister(base, NV_PRAMDAC_NVPLL_COEFF,
        (mNew & 255) | ((nNew & 255) << 8) | ((pNew & 7) << 16));
    DWORD setting = 4;
    switch (m_DacInfo.NVClk)
    {
    case AGP_CLOCK_133_MHZ: setting = 15; break;
    case AGP_CLOCK_126_MHZ: setting = 14; break;
    case AGP_CLOCK_120_MHZ: setting = 13; break;
    case AGP_CLOCK_113_MHZ: setting = 12; break;
    case AGP_CLOCK_106_MHZ: setting = 11; break;
    case AGP_CLOCK_100_MHZ: setting = 10; break;
    case AGP_CLOCK_94_MHZ: setting = 9; break;
    case AGP_CLOCK_87_MHZ: setting = 8; break;
    case AGP_CLOCK_80_MHZ: setting = 7; break;
    case AGP_CLOCK_73_MHZ: setting = 6; break;
    case AGP_CLOCK_66_MHZ: setting = 5; break;
    }
    WriteGpuRegister(base, NV_PBUS_DEBUG_3,
        (ReadGpuRegister(base, NV_PBUS_DEBUG_3) & ~0xf0UL) | ((setting & 15) << 4));
    return;
}

void CMiniport::HalDacProgramPClk(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD mNew = m_DacInfo.VPllM;
    DWORD nNew = m_DacInfo.VPllN;
    DWORD pNew = m_DacInfo.VPllP;
    DWORD pOld = (ReadGpuRegister(base, NV_PRAMDAC_VPLL_COEFF) >> 16) & 7;
    if (pNew < pOld)
    {
        WriteGpuRegister(base, NV_PRAMDAC_VPLL_COEFF,
            (mNew & 255) | ((nNew & 255) << 8) | ((pOld & 7) << 16));
        for (DWORD loops = 100; loops; --loops)
        {
            // Pixel PLL requires two consecutive locked status samples.
            if (((ReadGpuRegister(base, NV_PRAMDAC_PLL_TEST_COUNTER) >> 31) & 1) == 1)
            {
                if (((ReadGpuRegister(base, NV_PRAMDAC_PLL_TEST_COUNTER) >> 31) & 1) == 1)
                {
                    break;
                }
            }
            KeStallExecutionProcessor(1000);
        }
    }
    WriteGpuRegister(base, NV_PRAMDAC_VPLL_COEFF,
        (mNew & 255) | ((nNew & 255) << 8) | ((pNew & 7) << 16));
    return;
}
}
