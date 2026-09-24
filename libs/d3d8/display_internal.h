#ifndef D3D8_DISPLAY_INTERNAL_H
#define D3D8_DISPLAY_INTERNAL_H
#include "device_layout.h"
#include <conio.h>
#pragma intrinsic(_inp)
extern "C" DWORD D3D__AvInfo;
extern "C" __declspec(dllimport) void *WINAPI AvGetSavedDataAddress(
    void);
extern "C" __declspec(dllimport) void WINAPI AvSendTVEncoderOption(
    void *registerBase,
    DWORD option,
    DWORD parameter,
    DWORD *result);
extern "C" __declspec(dllimport) DWORD WINAPI AvSetDisplayMode(
    void *registerBase,
    DWORD step,
    DWORD displayMode,
    DWORD sourceColorFormat,
    DWORD pitch,
    DWORD frameBuffer);
extern "C" __declspec(dllimport) void WINAPI AvSetSavedDataAddress(
    void *address);
namespace D3D
{
/* Field widths and stride from the original PDB. AV masks are independently
 * corroborated by the target enumeration instructions. */
struct DISPLAYMODE
{
    DWORD AvInfo;
    WORD Width;
    WORD Height;
    DWORD DisplayMode;
};
typedef char DisplayModeSize[sizeof(DISPLAYMODE) == 12 ? 1 : -1];
BOOL WINAPI CheckDisplayFormat(
    D3DFORMAT format);
const DISPLAYMODE *WINAPI GetTableForCurrentAvInfo(
    void);
extern const D3DADAPTER_IDENTIFIER8 g_AdapterIdentifier;
extern const DISPLAYMODE g_DisplayModes[191];
extern const DWORD g_DisplayModeCount;
__inline DWORD CMiniport::GetRefreshRate(
    void)
{
    return (m_CurrentAvInfo & 0x400000) ? 60 : 50;
}
/* The January odd-field GPIO reports the inverse of bit 5. This method is
 * shared by actual ISR and vblank callers; no artificial emission anchor. */
__inline int CMiniport::IsOddField(
    void)
{
    return (~((DWORD)_inp(0x80c0) >> 5)) & 1;
}
enum
{
    AV_OPTION_BLANK_SCREEN = 9,
    AV_OPTION_FLICKER_FILTER = 11,
    AV_OPTION_SOFT_DISPLAY_FILTER = 14,
    AV_PACK_MASK = 0xff,
    AV_PACK_NONE = 0,
    AV_STANDARD_MASK = 0xff00,
    AV_FLAGS_INTERLACED = 0x00200000,
    AV_FLAGS_FIELD = 0x01000000,
    AV_FLAGS_10x11PAR = 0x02000000,
    AV_FLAGS_50Hz = 0x00800000,
    AV_PACK_HDTV = 4,
    AV_FLAGS_WIDESCREEN = 0x10000,
    AV_FLAGS_HDTV_720p = 0x20000,
    AV_FLAGS_HDTV_1080i = 0x40000,
    AV_HDTV_MODE_MASK = 0xe0000,
    AV_REFRESH_MASK = 0xc00000,
    AV_FLAGS_60Hz = 0x400000
};
}
#endif
