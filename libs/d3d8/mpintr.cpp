/* January-PDB miniport interrupt helpers; actual MMIO read/write ordering.
 * No hardware or original game image is executed by this reconstruction. */
#include "hardware_internal.h"
#include "kernel_interrupt.h"
#include "display_internal.h"
#include "miniport_constants.h"
#include "kernel_debug.h"
#include "memory_internal.h"
#include "pusher_state.h"
#include <conio.h>
#pragma intrinsic(_inp)
#pragma code_seg("D3D")
namespace D3D
{
DWORD CMiniport::ServiceMediaPortInterrupt(
    void)
{
    return 0;
}
void CMiniport::GrDone(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    while (REG_RD32(base, 0x400700))
    {
    }
    return;
}
DWORD CMiniport::ServiceVideoInterrupt(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    if (REG_RD32(base, 0x8100) & 1)
    {
        REG_WR32(base, 0x8100, 1);
    }
    return 0;
}
}

namespace D3D
{
void WINAPI CMiniport::ShutdownNotification(
    _HAL_SHUTDOWN_REGISTRATION *registration)
{
    CMiniport *miniport = CONTAINING_RECORD(registration,
        CMiniport, m_ShutdownRegistration);
    miniport->DisableInterrupts();
    miniport->ShutdownEngines();
    return;
}

void WINAPI CMiniport::Dpc(
    _KDPC *dpc,
    void *context,
    void *argument1,
    void *argument2)
{
    CMiniport *miniport = (CMiniport *)context;
    _HWREG *base = (_HWREG *)miniport->m_RegisterBase;
    DWORD pending;
    do
    {
        DWORD interrupts = REG_RD32(base, 0x100);
        pending = 0;
        if (interrupts & 0x100000)
        {
            REG_WR32(base, 0x9100, 1);
            pending = REG_RD32(base, 0x9100);
        }
        if (interrupts & 0x01000000)
        {
            pending |= miniport->VBlank();
        }
        if (interrupts & 0x1000)
        {
            pending |= miniport->ServiceGrInterrupt();
        }
        if ((interrupts & 0x100) || (REG_RD32(base, 0x2080) & 1))
        {
            pending |= miniport->ServiceFifoInterrupt();
        }
        if (interrupts & 0x10000)
        {
            pending |= miniport->ServiceVideoInterrupt();
        }
    } while (pending);
    miniport->EnableInterrupts();
    return;
}

void CMiniport::FixupPushBuffer(
    PUSHBUFFERFIXUPINFO *fixup,
    DWORD method)
{
    BYTE *start = fixup->pStart;
    const BYTE *record = (const BYTE *)fixup->pFixup;
    if (record)
    {
        DWORD length;
        memcpy(&length, record, sizeof(length));
        while (length != 0xffffffffUL)
        {
            DWORD destination;
            memcpy(&destination, record + sizeof(DWORD), sizeof(destination));
            record += 2 * sizeof(DWORD);
            memcpy(start + destination, record, length);
            record += length;
            memcpy(&length, record, sizeof(length));
        }
    }
    DWORD returnAddress = (DWORD)fixup->ReturnAddress & 0x0fffffffUL;
    // Snapshot the destination before patching memory: the write must not
    // cause a later reload of the fixup record's original return offset.
    DWORD returnOffset = fixup->ReturnOffset;
    DWORD jump = returnAddress + 1;
    memcpy(start + returnOffset, &jump, sizeof(jump));
    if (method == 0x308)
    {
        REG_WR32((_HWREG *)m_RegisterBase, 0x324c, returnAddress);
        m_PusherGetRunSize += returnOffset;
    }
    return;
}
}

namespace D3D
{
DWORD g_Adjusts;
DWORD g_VideoOffset;

int CMiniport::IsFlipPending(
    void)
{
    return m_VBlankFlips[m_VBlankFlipNext].Pending;
}

BYTE WINAPI CMiniport::Isr(
    _KINTERRUPT *interrupt,
    void *context)
{
    CMiniport *miniport = (CMiniport *)context;
    _HWREG *base = (_HWREG *)miniport->m_RegisterBase;
    if (miniport->m_InterruptsEnabled && REG_RD32(base, 0x140))
    {
        DWORD pending = REG_RD32(base, 0x100);
        if (pending)
        {
            if (pending & 0x01000000)
            {
                if (miniport->m_VBlankFlips[miniport->m_VBlankFlipNext].Pending == TRUE &&
                    (miniport->m_PresentationInterval != D3DPRESENT_INTERVAL_TWO ||
                     miniport->IsOddField()))
                {
                    g_VideoOffset = miniport->m_VBlankFlips[miniport->m_VBlankFlipNext].Offset;
                    miniport->DacProgramVideoStart(g_VideoOffset);
                }
                else if ((miniport->m_DisplayMode & 0xc0000000UL) == 0x80000000UL &&
                         (miniport->m_CurrentAvInfo & 0x00200000))
                {
                    miniport->DacProgramVideoStart(g_VideoOffset);
                }
            }
            miniport->DisableInterrupts();
            KeInsertQueueDpc(&miniport->m_Dpc, NULL, NULL);
            return TRUE;
        }
    }
    return FALSE;
}

DWORD CMiniport::VBlank(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    BYTE crtcIndex = REG_RD08(base, 0x6013d4);
    ++m_VBlankCount;
    if (m_VBlankFlips[m_VBlankFlipNext].Pending == TRUE &&
        (m_PresentationInterval != D3DPRESENT_INTERVAL_TWO || IsOddField()))
    {
        DacProgramVideoStart(m_VBlankFlips[m_VBlankFlipNext].Offset);
        m_VBlankFlips[m_VBlankFlipNext].Pending = FALSE;
        DWORD bank = m_VBlankFlipCount & 1;
        if (m_GammaUpdated[bank] == TRUE)
        {
            DacProgramGammaRamp(&m_GammaRamp[bank]);
            m_GammaUpdated[bank] = FALSE;
        }
        REG_WR32(base, 0x40071c, REG_RD32(base, 0x40071c) | 2);
        if (++m_VBlankFlipNext == 5)
        {
            m_VBlankFlipNext = 0;
        }
        ++m_VBlankFlipCount;
    }
    do
    {
        REG_WR32(base, 0x600100, 1);
    } while (REG_RD32(base, 0x100) & 0x01000000);
    KeSetEvent(&m_VerticalBlankEvent, 1, FALSE);
    if (m_pVerticalBlankCallback)
    {
        m_pVerticalBlankCallback(m_VBlankCount);
    }
    REG_WR08(base, 0x6013d4, crtcIndex);
    return 0;
}
}

namespace D3D
{
void CMiniport::SoftwareMethod(
    DWORD method,
    DWORD data)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    switch (method)
    {
    case SoftwareFlipImmediate:
        {
            DacProgramVideoStart(data);
            REG_WR32(base, 0x40071c, REG_RD32(base, 0x40071c) | 2);
            DWORD bank = m_VBlankFlipCount & 1;
            if (m_GammaUpdated[bank] == TRUE)
            {
                DacProgramGammaRamp(&m_GammaRamp[bank]);
                m_GammaUpdated[bank] = FALSE;
            }
            ++m_VBlankFlipCount;
        }
        break;
    case SoftwareFlipQueued:
        m_VBlankFlips[m_VBlankQueueNext].Pending = TRUE;
        m_VBlankFlips[m_VBlankQueueNext].Offset = data;
        if (++m_VBlankQueueNext == 5)
        {
            m_VBlankQueueNext = 0;
        }
        break;
    case SoftwareRunPushBuffer:
    case SoftwareFixupPushBuffer:
        {
            PUSHBUFFERFIXUPINFO fixup = *(PUSHBUFFERFIXUPINFO *)data;
            FixupPushBuffer(&fixup, method);
            FlushWCCache();
        }
        break;
    case SoftwareFence:
        KeSetEvent(&m_BusyBlockEvent, 1, FALSE);
        break;
    case SoftwareReadCallback:
    case SoftwareWriteCallback:
        ((D3DCALLBACK)data)(REG_RD32(base, 0x40186c));
        break;
    case SoftwareDxt1NoiseEnable:
        REG_WR32(base, 0x400750, 0xe00050);
        REG_WR32(base, 0x400754, data);
        REG_WR32(base, 0x400750, 0xdf0008);
        REG_WR32(base, 0x400754, data);
        break;
    case SoftwareWriteRegister:
        REG_WR32(base, data, REG_RD32(base, 0x40186c));
        break;
    case SoftwareSetSurfacePitchAndOffset:
        {
            DWORD offset = REG_RD32(base, 0x40186c);
            if (offset != REG_RD32(base, 0x400828))
            {
                DbgPrint("D3D: Adjusting offset\n");
                REG_WR32(base, 0x400828, offset);
                ++g_Adjusts;
            }
            if (data != REG_RD32(base, 0x400858))
            {
                DbgPrint("D3D: Adjusting pitch\n");
                REG_WR32(base, 0x400858, data);
                ++g_Adjusts;
            }
        }
        break;
    }
    return;
}

DWORD CMiniport::ServiceGrInterrupt(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    REG_WR32(base, 0x400720, 0);
    DWORD interrupts = REG_RD32(base, 0x400100);
    DWORD method = REG_RD32(base, 0x400704);
    DWORD notifySource = REG_RD32(base, 0x400108);
    DWORD channel = (method >> 20) & 0x1f;
    method &= 0x1ffc;
    if (interrupts & 0x1000)
    {
        REG_WR32(base, 0x400100, 0x1000);
        GrDone();
        HalGrLoadChannelContext(channel);
    }
    if (interrupts & 0x10)
    {
        REG_WR32(base, 0x400100, 0x10);
    }
    if (((interrupts & 1) || (interrupts & 0x100000)) && (notifySource & 0x40))
    {
        REG_WR32(base, 0x400100, (interrupts & 1) ? 1 : 0x100000);
    }
    interrupts = REG_RD32(base, 0x400100);
    if (interrupts)
    {
        REG_WR32(base, 0x400100, interrupts);
        if (interrupts != 0x1000 && interrupts != 0x01000000)
        {
            if (interrupts & 0x10)
            {
                GrDone();
            }
            if (notifySource && ((interrupts & 1) || (interrupts & 0x100000)))
            {
                DWORD objectClass = REG_RD32(base, 0x40014c) & 0xff;
                DWORD data = REG_RD32(base, 0x400708);
                if (!(notifySource & 0x40))
                {
                    if (method == 0x100)
                    {
                        SoftwareMethod(data, REG_RD32(base, 0x401a88));
                    }
                    else
                    {
                        DbgPrint("MP: GRAPHICS HARDWARE ERROR\nMP: Source: %08x\nMP:   ChID: %d\nMP:  Class: %x\nMP: Offset: %08x\nMP:   Data: %08x\n", notifySource, channel, objectClass, method, data);
                        __debugbreak();
                    }
                }
            }
            if (interrupts & 0x10000)
            {
                GrDone();
            }
        }
    }
    REG_WR32(base, 0x400720, 1);
    return REG_RD32(base, 0x400100);
}

DWORD CMiniport::ServiceFifoInterrupt(
    void)
{
    _HWREG *base = (_HWREG *)m_RegisterBase;
    DWORD interrupts = REG_RD32(base, 0x2100);
    if (interrupts & 0x100000)
    {
        REG_WR32(base, 0x2100, 0x100000);
    }
    if (interrupts & 0x01000000)
    {
        REG_WR32(base, 0x2100, 0x01000000);
    }
    interrupts = REG_RD32(base, 0x2100);
    if (interrupts & 1)
    {
        DWORD pull = REG_RD32(base, 0x3250);
        REG_WR32(base, 0x2500, 0);
        REG_WR32(base, 0x3250, 0);
        REG_WR32(base, 0x2100, 1);
        DWORD retries = 0xffff;
        do
        {
            if (!(pull & 0x1000))
            {
                break;
            }
            pull = REG_RD32(base, 0x3250);
        } while (--retries);
        REG_WR32(base, 0x3258, 0);
        REG_WR32(base, 0x3250, 1);
        REG_WR32(base, 0x2500, 1);
    }
    if (interrupts & 0x1000)
    {
        DbgPrint("SW PUT=%x\n", g_pDevice->m_Pusher.m_pPut);
        DbgPrint("HW PUT=%x\n", *(volatile DWORD *)0xfd003240UL);
        DbgPrint("HW GET=%x\n", *(volatile DWORD *)0xfd003244UL);
        REG_WR32(base, 0x2100, 0x1000);
        REG_WR32(base, 0x3228, 0);
        if (REG_RD32(base, 0x3240) != REG_RD32(base, 0x3244))
        {
            REG_WR32(base, 0x3244, REG_RD32(base, 0x3244) + 4);
        }
    }
    if (REG_RD32(base, 0x3220) & 0x1000)
    {
        while (!(REG_RD32(base, 0x3214) & 0x10) && !REG_RD32(base, 0x2100))
        {
            if (REG_RD32(base, 0x400100))
            {
                ServiceGrInterrupt();
            }
            if (REG_RD32(base, 0x100) & 0x01000000)
            {
                VBlank();
            }
        }
        if (REG_RD32(base, 0x3214) & 0x10)
        {
            while (REG_RD32(base, 0x2500) & 0x10)
            {
            }
            REG_WR32(base, 0x3220, REG_RD32(base, 0x3220) & ~0x1000UL);
        }
    }
    if (!REG_RD32(base, 0x2100))
    {
        REG_WR32(base, 0x3250, 1);
        REG_WR32(base, 0x2500, 1);
    }
    return REG_RD32(base, 0x2100) | (REG_RD32(base, 0x2080) & 1);
}
}
