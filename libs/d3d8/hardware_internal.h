#ifndef D3D8_HARDWARE_INTERNAL_H
#define D3D8_HARDWARE_INTERNAL_H
#include "device_layout.h"
#include "xmetal_memory_internal.h"
/* January PDB layout. Volatile register accesses describe mapped device
 * storage, rather than selecting an otherwise unnecessary instruction. */
struct Nv206eControl
{
    DWORD Ignored00[16];
    DWORD Put;
    DWORD Get;
    DWORD Reference;
    DWORD Ignored01[2029];
};
typedef char nv_control_size[sizeof(Nv206eControl) == 8192 ? 1 : -1];
typedef char nv_control_get_offset[offsetof(Nv206eControl, Get) == 0x44 ? 1 : -1];
/* NvNotification's field layout is decoded from the January PDB. */
struct NvNotification
{
    struct
    {
        DWORD nanoseconds[2];
    } timeStamp;
    DWORD info32;
    WORD info16;
    WORD status;
};
typedef char nv_notification_size_check[sizeof(NvNotification) == 16 ? 1 : -1];
namespace D3D
{
/* Original NV2A display/graphics status and framebuffer debug access. */
enum
{
    NV2A_RASTER_POSITION = 0x600808,
    NV2A_RASTER_POSITION_MASK = 0x7ff,
    NV2A_GRAPHICS_STATUS = 0x400700,
    NV2A_TAG_INDEX = 0x1000f0,
    NV2A_TAG_DATA = 0x100100,
    NV2A_TAG_PARTITION_SELECT = 0x130,
    NV2A_TAG_SELECT_MASK = 0x1ff,
    NV2A_TAG_ADDRESS_MASK = 0x7f
};

/* Hardware RAMFC context: the 13 saved DMA/FIFO registers occupy the first
 * 52 bytes of a 64-byte channel slot. Offsets follow the January save/restore
 * and initialization routines, not an unrelated later SDK layout. */
struct FifoChannelContext
{
    DWORD Put;
    DWORD Get;
    DWORD Ref;
    DWORD Instance;
    DWORD State;
    DWORD Fetch;
    DWORD Engine;
    DWORD Pull;
    DWORD AcquireValue;
    DWORD AcquireTimestamp;
    DWORD AcquireTimeout;
    DWORD Semaphore;
    DWORD Subroutine;
    DWORD Reserved[3];
};
typedef char fifo_channel_context_size[sizeof(FifoChannelContext) == 64 ? 1 : -1];
struct _HWREG
{
    volatile DWORD Reg032[1];
};
/* Address the complete mapped MMIO region, not an out-of-bounds element of
 * the one-element marker array in the historical declaration. */
static __inline DWORD WINAPI REG_RD32(
    void *base,
    DWORD offset)
{
    return *(volatile DWORD *)((BYTE *)base + offset);
}
static __inline void WINAPI REG_WR32(
    void *base,
    DWORD offset,
    DWORD value)
{
    *(volatile DWORD *)((BYTE *)base + offset) = value;
    return;
}
/* Byte-wide indexed CRTC and palette registers have distinct access widths. */
static __inline BYTE WINAPI REG_RD08(
    void *base,
    DWORD offset)
{
    return *(volatile BYTE *)((BYTE *)base + offset);
}
static __inline void WINAPI REG_WR08(
    void *base,
    DWORD offset,
    BYTE value)
{
    *(volatile BYTE *)((BYTE *)base + offset) = value;
    return;
}
__inline void CMiniport::EnableInterrupts(
    void)
{
    REG_WR32((_HWREG *)m_RegisterBase, 0x140, m_GenInfo.ChipIntrEn0);
    return;
}
/* Original inline method, also present as a 16-byte target in dxgcreate. */
__inline void CMiniport::DisableInterrupts(
    void)
{
    REG_WR32((_HWREG *)m_RegisterBase, 0x140, 0);
    return;
}
__inline DWORD *CDevice::HwGet(
    void)
{
    return (DWORD *)XMETAL_MapToContiguousAddress(m_pControlDma->Get);
}
__inline DWORD CDevice::GpuTime(
    void)
{
    return *m_pGpuTime;
}
__inline DWORD CDevice::Age(
    DWORD time)
{
    return m_CpuTime - time;
}
__inline BOOL CDevice::IsTimePending(
    DWORD time)
{
    return Age(time) < Age(GpuTime());
}
}
#endif
