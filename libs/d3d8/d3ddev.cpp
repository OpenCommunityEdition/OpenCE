/* Device entry points reconstructed from the original executable/PDB. */
#include "device_layout.h"
#include "push_internal.h"
#pragma code_seg("D3D")
using namespace D3D;
void WINAPI Direct3D_SetPushBufferSize(
    DWORD pushBufferSize,
    DWORD pushSegmentSize)
{
    CDevice::m_PushBufferSize = pushBufferSize;
    CDevice::m_PushSegmentSize = pushSegmentSize;
    return;
}
HRESULT WINAPI Direct3D_CreateDevice(
    UINT adapter,
    D3DDEVTYPE deviceType,
    void *unused,
    DWORD behaviorFlags,
    D3DPRESENT_PARAMETERS *parameters,
    D3DDevice **returnedDevice)
{
    if (CDevice::m_PushBufferSize == 0)
        CDevice::m_PushBufferSize = 512 * 1024;
    if (CDevice::m_PushSegmentSize == 0)
        CDevice::m_PushSegmentSize = 32 * 1024;
    g_pDevice = &g_Device;
    g_pPushBuffer = &g_Device.m_Pusher;
    g_Device.m_cRefs = 1;
    g_Device.m_StateFlags |= behaviorFlags & D3DCREATE_PUREDEVICE;
    HRESULT result = g_Device.Init(parameters);
    if (FAILED(result))
    {
        g_Device.UnInit();
        return result;
    }
    if (returnedDevice)
        *returnedDevice = &g_Device;
    return S_OK;
}
