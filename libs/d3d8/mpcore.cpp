/* Portions adapted with reference to RXDK-Libs (2026 Team Resurgent),
 * GPL-3.0-or-later. See libs/d3d8/PROVENANCE.md and libs/d3d8/LICENSE.GPL-3.0.
 * Original January target bytes and PDB remain the matching authority. */
/* January miniport helpers. Contracts and register accesses come from the
 * supplied target/PDB, not from a later miniport implementation. */
#include "hardware_internal.h"
#include "miniport_constants.h"
#include "display_internal.h"
#include "kernel_interrupt.h"
#include "kernel_clock.h"
#include <conio.h>
#pragma intrinsic(_outp)
#pragma code_seg("D3D")
namespace D3D
{
DWORD CMiniport::ReserveInstMem(
    DWORD paragraphs)
{
    DWORD address = m_FreeInstAddr;
    m_FreeInstAddr += paragraphs;
    return address;
}

int CMiniport::MapRegisters(
    void)
{
    /* NV2A fixed kernel MMIO mapping in this Xbox build. These are hardware
     * registers: volatility is required for the actual device transaction. */
    _HWREG *base = (_HWREG *)0xfd000000UL;
    m_RegisterBase = base;
    WriteGpuRegister(base, 0x1804, ReadGpuRegister(base, 0x1804) | 4);
    WriteGpuRegister(base, 0x600140, 0);
    WriteGpuRegister(base, 0x9140, 0);
    return TRUE;
}

void CMiniport::TmrDelay(
    DWORD nanoseconds)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD start = ReadGpuRegister(base, 0x9400);
    while (ReadGpuRegister(base, 0x9400) - start < nanoseconds)
    {
    }
    return;
}

void CMiniport::InitGammaRamp(
    DWORD bank)
{
    D3DGAMMARAMP *ramp = &m_GammaRamp[bank];
    for (DWORD value = 0; value < 256; ++value)
    {
        ramp->red[value] = (BYTE)value;
        ramp->green[value] = (BYTE)value;
        ramp->blue[value] = (BYTE)value;
    }
    return;
}
}

namespace D3D
{
int CMiniport::GetGeneralInfo(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    m_GenInfo.ChipId = (ReadGpuRegister(base, 0x1800) >> 16) & 0xfffc;
    m_GenInfo.MaskRevision = ReadGpuRegister(base, 0x1808) & 0xff;
    m_GenInfo.VideoRamSize = ReadGpuRegister(base, 0x10020c);
    m_GenInfo.CrystalFreq = 16666666;
    return TRUE;
}

void CMiniport::GetAddressInfo(
    void *linearAddress,
    void **address,
    DWORD *addressSpace,
    int isAlias)
{
    if (isAlias)
    {
        *address = (void *)(((DWORD)linearAddress & 0x0fffffffUL) | 0x40000000UL);
        *addressSpace = ADDR_AGPMEM;
    }
    else if (((DWORD)linearAddress & 0xf0000000UL) != 0x80000000UL)
    {
        *address = linearAddress;
        *addressSpace = ADDR_FBMEM;
    }
    else
    {
        *address = (void *)((DWORD)linearAddress & 0x03ffffffUL);
        *addressSpace = ADDR_SYSMEM;
    }
    return;
}

int CMiniport::BindToChannel(
    OBJECTINFO *object)
{
    DWORD handle = object->Handle;
    DWORD hash = (((handle >> 11) ^ handle) >> 11) ^ handle;
    HalFifoHashAdd(hash & 0x7ff, handle, m_HalInfo.FifoChID,
        object->Instance, object->Engine);
    return TRUE;
}

int CMiniport::InitDMAChannel(
    DWORD classNumber,
    OBJECTINFO *errorContext,
    OBJECTINFO *dataContext,
    DWORD offset,
    void **channel)
{
    /* The January routine uses fixed channel/class/offset values. The public
     * parameters remain because they are part of the recorded interface. */
    m_HalInfo.FifoChID = 0;
    m_HalInfo.FifoMode = 0;
    m_HalInfo.FifoAllocCount = 0;
    m_HalInfo.FifoObjectCount = 0;
    m_HalInfo.FifoInstance = ReserveInstMem(0x37f);
    HalFifoAllocDMA(0x80, 0x80, 8, dataContext);
    *channel = (BYTE *)m_RegisterBase + ((m_HalInfo.FifoChID + 0x80) << 16);
    return TRUE;
}

int CMiniport::CreateGrObject(
    DWORD handle,
    DWORD classNumber,
    OBJECTINFO *object)
{
    BOOL is3d = FALSE;
    DWORD instanceSize;
    switch (classNumber)
    {
    case NV03_NULL:
    case NV03_MEMORY_TO_MEMORY_FORMAT:
    case NV10_CONTEXT_SURFACES_2D:
    case NV12_IMAGE_BLIT:
        instanceSize = 16;
        break;
    case NV20_KELVIN_PRIMITIVE:
        instanceSize = 0x330;
        is3d = TRUE;
        break;
    default:
        instanceSize = classNumber;
        break;
    }
    DWORD instance = ReserveInstMem(instanceSize >> 4);
    if (is3d)
    {
        m_HalInfo.GrCurrentObjects3d[m_HalInfo.FifoChID] = instance;
        HalGrInit3d();
    }
    HalGrInitObjectContext(instance, classNumber);
    object->Init();
    object->Handle = handle;
    object->ClassNum = classNumber;
    object->Instance = instance;
    object->Engine = 1;
    BindToChannel(object);
    return TRUE;
}

int CMiniport::CreateCtxDmaObject(
    DWORD handle,
    DWORD classNumber,
    void *baseAddress,
    DWORD limit,
    OBJECTINFO *object)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    void *address = NULL;
    DWORD addressSpace = 0;
    GetAddressInfo(baseAddress, &address, &addressSpace, FALSE);
    DWORD instance = ReserveInstMem(1);
    DWORD page = (DWORD)address | 3;
    DWORD flags = 0;
    switch (classNumber)
    {
    case NV01_CONTEXT_DMA_FROM_MEMORY: flags = NV01_CONTEXT_DMA_FROM_MEMORY; break;
    case NV01_CONTEXT_DMA_TO_MEMORY: flags = NV01_CONTEXT_DMA_TO_MEMORY; break;
    case NV01_CONTEXT_DMA_IN_MEMORY: flags = NV01_CONTEXT_DMA_IN_MEMORY; break;
    }
    flags |= ((DWORD)address << 20) | 0x3000;
    switch (addressSpace)
    {
    case ADDR_FBMEM: break;
    case ADDR_AGPMEM: flags |= 0x30000; break;
    case ADDR_SYSMEM: flags |= 0x20000; break;
    }
    WriteGpuRegister(base, 0x700008 + (instance << 4), page);
    WriteGpuRegister(base, 0x70000c + (instance << 4), page);
    flags |= 0x8000;
    WriteGpuRegister(base, (0x70000 + instance) << 4, flags);
    WriteGpuRegister(base, 0x700004 + (instance << 4), limit);
    object->Init();
    object->Engine = 0;
    object->Handle = handle;
    object->ClassNum = classNumber;
    object->Instance = instance;
    return TRUE;
}
}

namespace D3D
{
int CMiniport::InitEngines(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    WriteGpuRegister(base, 0x1830, 0);
    WriteGpuRegister(base, 0x180c, 0xf800);
    HalMcControlInit();
    m_GenInfo.ChipIntrEn0 = 1;

    // Reduce the clock ratio before encoding the 16-bit timer fractions.
    // The original calculation uses signed clock intermediates and unsigned
    // range comparisons; real device clock rates are positive.
    long numerator = (long)m_DacInfo.NVClk;
    long denominator = 31250000;
    while (numerator % 2 == 0 && denominator % 2 == 0)
    {
        numerator /= 2;
        denominator /= 2;
    }
    while (numerator % 5 == 0 && denominator % 5 == 0)
    {
        numerator /= 5;
        denominator /= 5;
    }
    while ((DWORD)numerator > 0x1fffe || (DWORD)denominator > 0x1fffe)
    {
        numerator /= 2;
        denominator /= 2;
    }
    WriteGpuRegister(base, 0x9200, (numerator + 1) / 2);
    WriteGpuRegister(base, 0x9210, (denominator + 1) / 2);
    WriteGpuRegister(base, 0x9420, 0xffffffffUL);
    HalFbControlInit();
    HalDacControlInit();
    InitGammaRamp(0);
    HalVideoControlInit();
    m_HalInfo.GrChID = 2;
    HalGrControlInit();
    HalFifoControlInit();
    return TRUE;
}
}

namespace D3D
{
/* Drain the actual FIFO/graphics pipeline before modifying tile/context state.
 * Hardware status bits may change asynchronously; all polls are MMIO accesses. */
void CMiniport::TilingUpdateIdle(
    DWORD *savedPush)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    while (!(ReadGpuRegister(base, 0x3214) & 0x10) ||
           !(ReadGpuRegister(base, 0x2400) & 0x10) ||
            (ReadGpuRegister(base, 0x3220) & 0x10))
    {
        ServiceFifoInterrupt();
        if (ReadGpuRegister(base, 0x400100))
        {
            ServiceGrInterrupt();
        }
        if (ReadGpuRegister(base, 0x100) & 0x1000000)
        {
            VBlank();
        }
    }
    *savedPush = ReadGpuRegister(base, 0x3220);
    WriteGpuRegister(base, 0x3220, 0);
    while (ReadGpuRegister(base, 0x3220) & 0x10)
    {
    }
    return;
}

void CMiniport::ShutdownEngines(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    if (!AvGetSavedDataAddress())
    {
        AvSendTVEncoderOption(base, AV_OPTION_BLANK_SCREEN, TRUE, NULL);
    }
    for (int tile = 0; tile < 8; ++tile)
    {
        DestroyTile(tile, TRUE);
    }
    WriteGpuRegister(base, 0x2044, 0);
    // Shutdown has its own drain sequence: unlike TilingUpdateIdle it does not
    // retain the DMA push register for resuming later. This separation is also
    // present in the pinned mpcore reference, not manual helper expansion.
    while (!(ReadGpuRegister(base, 0x3214) & 0x10) ||
           !(ReadGpuRegister(base, 0x2400) & 0x10) ||
            (ReadGpuRegister(base, 0x3220) & 0x10))
    {
        ServiceFifoInterrupt();
        if (ReadGpuRegister(base, 0x400100))
        {
            ServiceGrInterrupt();
        }
        if (ReadGpuRegister(base, 0x100) & 0x1000000)
        {
            VBlank();
        }
    }
    WriteGpuRegister(base, 0x3220, 0);
    while (ReadGpuRegister(base, 0x3220) & 0x10)
    {
    }
    WriteGpuRegister(base, 0x2500, 0);
    WriteGpuRegister(base, 0x3000, 0);
    WriteGpuRegister(base, 0x3050, 0);
    WriteGpuRegister(base, 0x3200, 0);
    WriteGpuRegister(base, 0x3250, 0);
    HalFifoContextSwitch(1);
    WriteGpuRegister(base, 0x3210, 0);
    WriteGpuRegister(base, 0x3270, 0);

    DWORD fifoReassign = ReadGpuRegister(base, 0x2500);
    DWORD fifoPush = ReadGpuRegister(base, 0x3200);
    DWORD fifoPull = ReadGpuRegister(base, 0x3250);
    WriteGpuRegister(base, 0x2500, 0);
    WriteGpuRegister(base, 0x3200, 0);
    WriteGpuRegister(base, 0x3250, 0);
    for (DWORD channel = 0; channel < 2; ++channel)
    {
        if (m_HalInfo.FifoInUse)
        {
            WriteGpuRegister(base, m_HalInfo.FifoContextAddr1 + channel * 64 + 4,
                ReadGpuRegister(base, m_HalInfo.FifoContextAddr1 + channel * 64));
            WriteGpuRegister(base, m_HalInfo.FifoContextAddr1 + channel * 64 + 16, 0);
        }
    }
    WriteGpuRegister(base, 0x3250, fifoPull);
    WriteGpuRegister(base, 0x3200, fifoPush);
    WriteGpuRegister(base, 0x2500, fifoReassign);
    WriteGpuRegister(base, 0x2508, 0);
    WriteGpuRegister(base, 0x2140, 0);
    HalGrLoadChannelContext(2);
    WriteGpuRegister(base, 0x100200, m_HalInfo.FbSave0);
    WriteGpuRegister(base, 0x100204, m_HalInfo.FbSave1);
    WriteGpuRegister(base, 0x200, m_HalInfo.McSave);
    WriteGpuRegister(base, 0x140, m_HalInfo.McSaveIntrEn0);
    HalRegisterShutdownNotification(
        &m_ShutdownRegistration, FALSE);
    KeDisconnectInterrupt(&m_InterruptObject);
    return;
}
}

namespace D3D
{
/* Tile lifecycle reconstructed from January target accesses and corroborated
 * by pinned RXDK mpcore. Valid callers provide tile indices below eight and
 * aligned, GPU-addressable regions. Device readbacks can require retrying. */
int CMiniport::CreateTile(
    DWORD tile,
    DWORD offset,
    DWORD size,
    DWORD pitch,
    DWORD memoryType,
    DWORD zTag,
    DWORD zOffset)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD dmaPush;
    TilingUpdateIdle(&dmaPush);
    do
    {
        HalGrIdle();
        WriteGpuRegister(base, 0x100244 + tile * 16, offset + size - 1);
        WriteGpuRegister(base, 0x100248 + tile * 16, pitch);
        WriteGpuRegister(base, 0x400904 + tile * 16, offset + size - 1);
        WriteGpuRegister(base, 0x400908 + tile * 16, pitch);
        if (memoryType & D3DTILE_FLAGS_ZBUFFER)
        {
            WriteGpuRegister(base, 0x400900 + tile * 16, offset | 3);
            WriteGpuRegister(base, 0x100240 + tile * 16, offset | 3);
            WriteGpuRegister(base, 0x400750, 0xea0000 | ((tile * 4 + 0x10) & 0x1ffc));
            WriteGpuRegister(base, 0x400754, offset | 3);
        }
        else
        {
            WriteGpuRegister(base, 0x400900 + tile * 16, offset | 1);
            WriteGpuRegister(base, 0x100240 + tile * 16, offset | 1);
            WriteGpuRegister(base, 0x400750, 0xea0000 | ((tile * 4 + 0x10) & 0x1ffc));
            WriteGpuRegister(base, 0x400754, offset | 1);
        }
        WriteGpuRegister(base, 0x400750, 0xea0000 | ((tile * 4 + 0x30) & 0x1ffc));
        WriteGpuRegister(base, 0x400754, offset + size - 1);
        WriteGpuRegister(base, 0x400750, 0xea0000 | ((tile * 4 + 0x50) & 0x1ffc));
        WriteGpuRegister(base, 0x400754, pitch);
    } while (ReadGpuRegister(base, 0x100240 + tile * 16) != ReadGpuRegister(base, 0x400900 + tile * 16) ||
             ReadGpuRegister(base, 0x100248 + tile * 16) != ReadGpuRegister(base, 0x400908 + tile * 16) ||
             (ReadGpuRegister(base, 0x100244 + tile * 16) & ~0x3fffUL) !=
             (ReadGpuRegister(base, 0x400904 + tile * 16) & ~0x3fffUL));
    if (memoryType & D3DTILE_FLAGS_ZCOMPRESS)
    {
        DWORD zData = (zTag / 4) | 0x80000000UL;
        if (memoryType & D3DTILE_FLAGS_Z32BITS)
        {
            zData |= 0x04000000;
        }
        do
        {
            HalGrIdle();
            WriteGpuRegister(base, 0x100300 + tile * 4, zData);
            WriteGpuRegister(base, 0x400980 + tile * 4, zData);
            WriteGpuRegister(base, 0x400750, 0xea0000 | ((tile * 4 + 0x90) & 0x1ffc));
            WriteGpuRegister(base, 0x400754, zData);
        } while (ReadGpuRegister(base, 0x100300 + tile * 4) !=
                 ReadGpuRegister(base, 0x400980 + tile * 4));
        if (zOffset != 0)
        {
            do
            {
                DWORD zOffsetValue = zOffset | tile | 0x80000000UL;
                HalGrIdle();
                WriteGpuRegister(base, 0x100324, zOffsetValue);
                WriteGpuRegister(base, 0x4009a0, zOffsetValue);
            } while (ReadGpuRegister(base, 0x100324) != ReadGpuRegister(base, 0x4009a0));
        }
    }
    WriteGpuRegister(base, 0x3220, dmaPush);
    return TRUE;
}

int CMiniport::DestroyTile(
    DWORD tile,
    DWORD zOffset)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD dmaPush;
    TilingUpdateIdle(&dmaPush);
    do
    {
        HalGrIdle();
        WriteGpuRegister(base, 0x100240 + tile * 16, 0);
        WriteGpuRegister(base, 0x400900 + tile * 16, 0);
        WriteGpuRegister(base, 0x400750, 0xea0000 | ((tile * 4 + 0x10) & 0x1ffc));
        WriteGpuRegister(base, 0x400754, 0);
    } while (ReadGpuRegister(base, 0x100240 + tile * 16) !=
             ReadGpuRegister(base, 0x400900 + tile * 16));
    WriteGpuRegister(base, 0x100300 + tile * 4, 0);
    WriteGpuRegister(base, 0x400980 + tile * 4, 0);
    WriteGpuRegister(base, 0x400750, 0xea0000 | ((tile * 4 + 0x90) & 0x1ffc));
    WriteGpuRegister(base, 0x400754, 0);
    if (zOffset != 0)
    {
        WriteGpuRegister(base, 0x100324, 0);
        WriteGpuRegister(base, 0x4009a0, 0);
    }
    WriteGpuRegister(base, 0x3220, dmaPush);
    return TRUE;
}
}

namespace D3D
{
int CMiniport::LoadEngines(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD command = ReadGpuRegister(base, 0x184c);
    WriteGpuRegister(base, 0x184c, command & ~0x300UL);
    WriteGpuRegister(base, 0x184c, command);
    WriteGpuRegister(base, 0x200, 0xffffffffUL);
    EnableInterrupts();
    HalDacLoad();

    LARGE_INTEGER systemTime;
    _TIME_FIELDS fields;
    static const BYTE timerMonthDays[12] =
    {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    KeQuerySystemTime(&systemTime);
    RtlTimeToTimeFields(&systemTime, &fields);
    if (fields.Year > 1990)
    {
        fields.Year -= 1990;
    }
    DWORD days = fields.Year * 365 + (fields.Year + 1) / 4;
    if (!((fields.Year + 2) & 3) && fields.Month > 2)
    {
        ++days;
    }
    // Preserve the original timer's month indexing, rather than substituting
    // a corrected calendar conversion. Kernel TIME_FIELDS months are 1..12.
    while (--fields.Month)
    {
        days += timerMonthDays[fields.Month];
    }
    days += fields.Day;
    DWORD highTime = days * 20117 + fields.Hour * 838 +
        (fields.Minute * 257698 + fields.Second * 4295) / 1000;
    highTime += 146951526;
    WriteGpuRegister(base, 0x9400, 0);
    WriteGpuRegister(base, 0x9410, highTime);
    WriteGpuRegister(base, 0x400720, 0);
    HalGrControlLoad();
    WriteGpuRegister(base, 0x400100, 0xffffffffUL);
    WriteGpuRegister(base, 0x400140, 0xffffffffUL);
    HalFifoControlLoad();
    WriteGpuRegister(base, 0x2100, 0xffffffffUL);
    WriteGpuRegister(base, 0x2140, m_HalInfo.FifoIntrEn0);
    return TRUE;
}

int CMiniport::InitHardware(
    void)
{
    KeInitializeDpc(&m_Dpc, CMiniport::Dpc, this);
    KeInitializeEvent(&m_BusyBlockEvent, NotificationEvent, TRUE);
    KeInitializeEvent(&m_VerticalBlankEvent, NotificationEvent, TRUE);
    if (!MapRegisters())
    {
        return FALSE;
    }
    if (!GetGeneralInfo())
    {
        return FALSE;
    }
    BYTE interruptIrql;
    DWORD vector = HalGetInterruptVector(3, &interruptIrql);
    KeInitializeInterrupt(&m_InterruptObject, CMiniport::Isr, this,
        vector, interruptIrql, LevelSensitive, TRUE);
    if (!KeConnectInterrupt(&m_InterruptObject))
    {
        return FALSE;
    }
    m_ShutdownRegistration.NotificationRoutine = CMiniport::ShutdownNotification;
    m_ShutdownRegistration.Priority = 0;
    HalRegisterShutdownNotification(&m_ShutdownRegistration, TRUE);
    if (!InitEngines())
    {
        return FALSE;
    }
    for (DWORD bank = 0; bank < 2; ++bank)
    {
        D3DGAMMARAMP *ramp = &m_GammaRamp[bank];
        for (DWORD value = 0; value < 256; ++value)
        {
            ramp->red[value] = (BYTE)value;
            ramp->green[value] = (BYTE)value;
            ramp->blue[value] = (BYTE)value;
        }
    }
    _outp(0x80c0, 1);
    m_InterruptsEnabled = TRUE;
    if (!LoadEngines())
    {
        return FALSE;
    }
    return TRUE;
}
}
