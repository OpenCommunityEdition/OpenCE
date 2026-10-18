/* The Xbox rasterizer's Direct3D object/device creation boundary, backed by
   the native CGL/AGL device. Palette allocation and resource lifetime use
   the shared Xbox-resource implementation, with native contiguous memory. */
#include "engine_device_factory.h"
#include "engine_device_resources.h"
#include "engine_rasterizer_services.h"
#include "native_resource_memory.h"
#undef memset
#include <string.h>
#include <OpenGL/gl.h>

typedef char native_device_factory_is_ilp32[(sizeof(void *)==4&&sizeof(unsigned long)==4)?1:-1];

static unsigned char direct3d_object;
static unsigned direct3d_references;
static int native_device_created;
static int native_device_visible;

int mac_engine_device_factory_set_visible(int visible)
{
    if(native_device_created||mac_device_state_get()||(visible!=0&&visible!=1))return 0;
    native_device_visible=visible;
    return 1;
}

Direct3D *WINAPI Direct3DCreate8(UINT sdk_version)
{
    if(sdk_version!=D3D_SDK_VERSION||direct3d_references==(unsigned)-1)return NULL;
    ++direct3d_references;
    return (Direct3D *)&direct3d_object;
}

HRESULT WINAPI Direct3D_CreateDevice(UINT adapter,D3DDEVTYPE device_type,
    void *focus_window,DWORD behavior_flags,D3DPRESENT_PARAMETERS *parameters,
    D3DDevice **returned_device)
{
    unsigned long width,height;
    const unsigned long supported_intervals[]={D3DPRESENT_INTERVAL_DEFAULT,
        D3DPRESENT_INTERVAL_IMMEDIATE,D3DPRESENT_INTERVAL_ONE,D3DPRESENT_INTERVAL_TWO};
    unsigned i;
    if(returned_device)*returned_device=NULL;
    if(!direct3d_references||adapter!=D3DADAPTER_DEFAULT||
       device_type!=D3DDEVTYPE_HAL||focus_window||!returned_device||!parameters||
       behavior_flags!=D3DCREATE_HARDWARE_VERTEXPROCESSING||native_device_created)
        return E_INVALIDARG;
    width=parameters->BackBufferWidth;height=parameters->BackBufferHeight;
    if(!width||!height||width>2048||height>2048||parameters->Windowed||
       parameters->BackBufferFormat!=D3DFMT_A8R8G8B8||
       parameters->SwapEffect!=D3DSWAPEFFECT_DISCARD||
       parameters->MultiSampleType!=0||
       parameters->Flags!=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER||
       !parameters->EnableAutoDepthStencil||
       parameters->AutoDepthStencilFormat!=D3DFMT_D24S8||
       parameters->BackBufferCount>1)
        return E_INVALIDARG;
    for(i=0;i<sizeof(supported_intervals)/sizeof(supported_intervals[0]);++i)
        if(parameters->FullScreen_PresentationInterval==supported_intervals[i])break;
    if(i==sizeof(supported_intervals)/sizeof(supported_intervals[0]))return E_INVALIDARG;
    /* Rasterizer startup must never open a person's desktop. Its native
       default is a hidden, offscreen CGL context; visible hosting is explicit. */
    if(mac_device_state_get())return E_FAIL;
    if(!mac_device_gpu_start(width,height,native_device_visible,"PowerSpark"))return E_FAIL;
    *returned_device=mac_device_gpu_identity();
    if(!*returned_device) {
        mac_device_state_stop();
        return E_FAIL;
    }
    native_device_created=1;
    return S_OK;
}

int mac_engine_device_factory_get_caps(D3DDevice *device,D3DCAPS8 *caps)
{
    GLint max_texture_size,max_texture_units;
    if(!native_device_created||device!=mac_device_gpu_identity()||!caps)return 0;
    memset(caps,0,sizeof(*caps));
    max_texture_size=0;max_texture_units=0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS,&max_texture_units);
    if(glGetError()!=GL_NO_ERROR||max_texture_size<=0||max_texture_units<=0)return 0;
    caps->DeviceType=D3DDEVTYPE_HAL;
    caps->AdapterOrdinal=D3DADAPTER_DEFAULT;
    /* Texture size comes from the active GL implementation. The stage count,
       streams, and constants are bounded by the native device owner. Feature
       masks and shader versions remain clear until full contracts exist. */
    caps->MaxTextureWidth=(unsigned long)max_texture_size;
    caps->MaxTextureHeight=(unsigned long)max_texture_size;
    caps->MaxTextureBlendStages=(unsigned long)max_texture_units<D3DTSS_MAXSTAGES?
        (unsigned long)max_texture_units:D3DTSS_MAXSTAGES;
    caps->MaxSimultaneousTextures=caps->MaxTextureBlendStages;
    caps->MaxStreams=16;
    caps->MaxVertexShaderConst=192;
    return 1;
}

void WINAPI D3DDevice_GetDeviceCaps(D3DCAPS8 *caps)
{
    if(!mac_engine_device_factory_get_caps(mac_device_gpu_identity(),caps)&&caps)
        memset(caps,0,sizeof(*caps));
}

int mac_engine_device_factory_set_palette(D3DDevice *device,DWORD stage,
    D3DPalette *palette)
{
    if(!native_device_created||device!=mac_device_gpu_identity()||
       stage>=D3DTSS_MAXSTAGES||
       (palette&&((palette->Common&D3DCOMMON_TYPE_MASK)!=D3DCOMMON_TYPE_PALETTE||
        !(palette->Common&D3DCOMMON_D3DCREATED))))return 0;
    D3DDevice_SetPalette(stage,palette);
    return 1;
}

D3DPalette *mac_engine_device_factory_palette_for_stage(unsigned stage)
{
    const struct mac_device_state *state=mac_device_state_get();
    return state&&stage<D3DTSS_MAXSTAGES?state->palettes[stage]:NULL;
}

ULONG WINAPI D3DDevice_Release(void)
{
    if(!native_device_created)return 0;
    mac_engine_device_resources_stop();
    if(mac_device_state_get()) {
        unsigned i;
        for(i=0;i<D3DTSS_MAXSTAGES;++i)D3DDevice_SetPalette(i,NULL);
    }
    native_device_created=0;
    mac_device_state_stop();
    mac_engine_rasterizer_services_reset();
    return 0;
}
