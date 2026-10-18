#include "engine_device_factory.h"
#include "engine_rasterizer_services.h"
#include "native_resource_memory.h"
#include <OpenGL/gl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#undef malloc
#undef calloc
#undef free
#undef memset
#undef memcpy
#undef memcmp
#undef printf
#undef fprintf
#undef fopen
#undef fclose
#undef fread
#undef fseek
#undef ftell
#include <stdlib.h>
#include <string.h>

static unsigned checks,failures;
#define CHECK(v) do {++checks;if(!(v)){++failures;fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#v);}} while(0)
void platform_log(const char *format,...)
{
    va_list arguments;
    va_start(arguments,format);vfprintf(stderr,format,arguments);va_end(arguments);
    fputc('\n',stderr);
}

int main(void)
{
    Direct3D *direct3d;
    D3DDevice *device=NULL;
    D3DPRESENT_PARAMETERS parameters;
    D3DCAPS8 caps;
    D3DPalette *palette=NULL;
    D3DCOLOR *colors=NULL;
    const struct mac_device_state *state;
    const char *renderer;
    unsigned i;
    unsigned long allocations_before;
    GLint gl_max_texture_size,gl_max_texture_units;

    CHECK(!mac_engine_device_factory_set_visible(2));
    CHECK(mac_engine_device_factory_set_visible(0));
    CHECK(Direct3DCreate8(D3D_SDK_VERSION+1)==NULL);
    direct3d=Direct3DCreate8(D3D_SDK_VERSION);
    CHECK(direct3d!=NULL);
    memset(&parameters,0,sizeof(parameters));
    parameters.BackBufferWidth=640;
    parameters.BackBufferHeight=480;
    parameters.BackBufferFormat=D3DFMT_A8R8G8B8;
    parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;
    parameters.EnableAutoDepthStencil=TRUE;
    parameters.AutoDepthStencilFormat=D3DFMT_D24S8;
    parameters.Flags=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    parameters.FullScreen_PresentationInterval=D3DPRESENT_INTERVAL_DEFAULT;

    parameters.Windowed=TRUE;
    CHECK(IDirect3D8_CreateDevice(direct3d,D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
        NULL,D3DCREATE_HARDWARE_VERTEXPROCESSING,&parameters,&device)==E_INVALIDARG);
    CHECK(device==NULL&&mac_device_state_get()==NULL);
    parameters.Windowed=FALSE;
    CHECK(IDirect3D8_CreateDevice(direct3d,1,D3DDEVTYPE_HAL,NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,&parameters,&device)==E_INVALIDARG);
    CHECK(device==NULL&&mac_device_state_get()==NULL);
    CHECK(IDirect3D8_CreateDevice(direct3d,D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
        NULL,D3DCREATE_HARDWARE_VERTEXPROCESSING,&parameters,&device)==S_OK);
    CHECK(device==mac_device_gpu_identity()&&device!=NULL);
    CHECK(!mac_engine_device_factory_set_visible(1));
    state=mac_device_state_get();
    CHECK(state&&state->gpu&&!state->headless&&!state->gpu_frame_active);
    CHECK(state&&state->gpu_width==640&&state->gpu_height==480);
    renderer=(const char *)glGetString(GL_RENDERER);
    CHECK(renderer!=NULL&&renderer[0]!=0);
    if(renderer)printf("native factory renderer=%s\n",renderer);

    memset(&caps,0xa5,sizeof(caps));
    CHECK(IDirect3DDevice8_GetDeviceCaps(device,&caps)==S_OK);
    CHECK(caps.DeviceType==D3DDEVTYPE_HAL&&caps.AdapterOrdinal==D3DADAPTER_DEFAULT);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&gl_max_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS,&gl_max_texture_units);
    CHECK(caps.MaxTextureWidth==(unsigned long)gl_max_texture_size&&
        caps.MaxTextureHeight==(unsigned long)gl_max_texture_size);
    CHECK(caps.MaxTextureBlendStages==(unsigned long)gl_max_texture_units<D3DTSS_MAXSTAGES?
        caps.MaxTextureBlendStages==(unsigned long)gl_max_texture_units:
        caps.MaxTextureBlendStages==D3DTSS_MAXSTAGES);
    CHECK(caps.MaxSimultaneousTextures==caps.MaxTextureBlendStages);
    CHECK(caps.MaxStreams==16&&caps.MaxVertexShaderConst==192);
    D3DDevice_GetDeviceCaps(&global_d3d_caps);
    CHECK(!memcmp(&caps,&global_d3d_caps,sizeof(caps)));
    CHECK(caps.Caps==0&&caps.RasterCaps==0&&caps.TextureCaps==0&&
        caps.VertexShaderVersion==0&&caps.PixelShaderVersion==0);
    CHECK(!mac_engine_device_factory_get_caps(NULL,&caps));

    allocations_before=mac_resource_allocation_count();
    CHECK(IDirect3DDevice8_CreatePalette(device,D3DPALETTE_256,&palette)==S_OK);
    CHECK(palette!=NULL&&mac_resource_allocation_count()==allocations_before+1);
    CHECK((palette->Common&D3DCOMMON_TYPE_MASK)==D3DCOMMON_TYPE_PALETTE);
    CHECK((palette->Common&D3DCOMMON_REFCOUNT_MASK)==1);
    CHECK(((D3DCOLOR *)PLATFORM_PHYSICAL_TO_VIRTUAL(palette->Data))[0]==0);
    CHECK(IDirect3DPalette8_Lock(palette,&colors,0)==S_OK);
    CHECK(colors==(D3DCOLOR *)PLATFORM_PHYSICAL_TO_VIRTUAL(palette->Data));
    if(colors)for(i=0;i<256;++i)colors[i]=0xff000000UL|i;
    CHECK(colors&&colors[255]==0xff0000ffUL);
    CHECK(IDirect3DPalette8_Unlock(palette)==S_OK);
    for(i=0;i<D3DTSS_MAXSTAGES;++i) {
        CHECK(IDirect3DDevice8_SetPalette(device,i,palette)==S_OK);
        CHECK(mac_engine_device_factory_palette_for_stage(i)==palette);
    }
    CHECK(!mac_engine_device_factory_set_palette(device,D3DTSS_MAXSTAGES,palette));
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)palette)==0);
    CHECK(mac_resource_allocation_count()==allocations_before);
    CHECK(IDirect3DDevice8_Release(device)==0);
    CHECK(mac_device_state_get()==NULL);
    printf("native factory checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
