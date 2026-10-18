#include "engine_device_factory.h"
#include "engine_device_resources.h"
#include "engine_rasterizer_services.h"
#include "native_resource_memory.h"
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#undef malloc
#undef calloc
#undef free
#undef memset
#undef memcpy
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

static int initialize_device(D3DDevice **device)
{
    Direct3D *d3d=Direct3DCreate8(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS parameters;
    memset(&parameters,0,sizeof(parameters));
    parameters.BackBufferWidth=640;parameters.BackBufferHeight=480;
    parameters.BackBufferFormat=D3DFMT_A8R8G8B8;
    parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;
    parameters.EnableAutoDepthStencil=TRUE;
    parameters.AutoDepthStencilFormat=D3DFMT_D24S8;
    parameters.Flags=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    parameters.FullScreen_PresentationInterval=D3DPRESENT_INTERVAL_DEFAULT;
    return d3d&&IDirect3D8_CreateDevice(d3d,D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
        NULL,D3DCREATE_HARDWARE_VERTEXPROCESSING,&parameters,device)==S_OK;
}

struct render_target_test {
    D3DDevice *device;
    D3DSurface color;
    int passed;
};

static void draw_test_triangle(float z,float red,float green,float blue)
{
    glColor3f(red,green,blue);
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.0f,-1.0f,z);glVertex3f(1.0f,-1.0f,z);
    glVertex3f(0.0f,1.0f,z);
    glEnd();
}

static int render_target_frame(void *opaque,const struct mac_input_frame *input,
    unsigned width,unsigned height)
{
    struct render_target_test *test=(struct render_target_test *)opaque;
    const struct mac_device_state *state=mac_device_state_get();
    unsigned framebuffer,target_depth;
    GLint bound,depth_object;
    unsigned char pixel[4];
    float depth=1.0f;
    CHECK(input!=NULL&&width==640&&height==480&&state!=NULL);
    if(!state)return 0;
    IDirect3DDevice8_SetRenderTarget(test->device,&test->color,
        (D3DSurface *)&state->depth_buffer);
    CHECK(mac_engine_device_resources_target_info(&test->color,&framebuffer,NULL,
        NULL,NULL));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&bound);
    CHECK((unsigned)bound==framebuffer);
    glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&depth_object);
    CHECK(mac_engine_device_resources_target_depth(&test->color,&target_depth));
    CHECK((unsigned)depth_object==target_depth&&target_depth!=0);
    glViewport(0,0,(GLsizei)width,(GLsizei)height);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);
    IDirect3DDevice8_Clear(test->device,0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
        0xff000000UL,1.0f,0);
    draw_test_triangle(0.5f,1.0f,0.0f,0.0f);
    draw_test_triangle(-0.5f,0.0f,1.0f,0.0f);
    draw_test_triangle(0.75f,0.0f,0.0f,1.0f);
    glReadPixels((GLint)(width/2),(GLint)(height/2),1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    glReadPixels((GLint)(width/2),(GLint)(height/2),1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&depth);
    CHECK(glGetError()==GL_NO_ERROR);
    CHECK(pixel[0]<8&&pixel[1]>245&&pixel[2]<8&&pixel[3]>245);
    CHECK(depth>0.24f&&depth<0.26f);
    glDisable(GL_DEPTH_TEST);
    IDirect3DDevice8_SetRenderTarget(test->device,(D3DSurface *)&state->back_buffer,
        (D3DSurface *)&state->depth_buffer);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&bound);
    CHECK((unsigned)bound==state->gpu_default_framebuffer);
    test->passed=1;
    return 1;
}

int main(void)
{
    D3DDevice *device=NULL;
    D3DTexture *texture=NULL,*target=NULL;
    D3DTexture *frame_target=NULL;
    D3DCubeTexture *cube=NULL;
    D3DVolumeTexture *volume=NULL;
    D3DSurface surface;
    struct render_target_test frame_test;
    D3DLOCKED_RECT locked;
    D3DLOCKED_BOX locked_box;
    struct mac_rasterizer_display_options display_options;
    unsigned long allocations;
    unsigned fbo,gl_texture,target_width,target_height;
    GLint old_fbo,bound_fbo;
    D3DSurface wrong_depth;
    unsigned char *pixel;
    CHECK(mac_engine_device_create_texture(4,4,1,0,D3DFMT_A4R4G4B4,
        D3DPOOL_MANAGED,&texture)==E_INVALIDARG&&texture==NULL);
    CHECK(initialize_device(&device));
    CHECK(device==mac_device_gpu_identity()&&device!=NULL);
    Direct3D_SetPushBufferSize(512UL*1024UL,32UL*1024UL);
    D3DDevice_SetFlickerFilter(5);
    D3DDevice_SetSoftDisplayFilter(FALSE);
    CHECK(mac_engine_rasterizer_services_get_display_options(&display_options));
    CHECK(display_options.push_buffer_size==512UL*1024UL&&
        display_options.kick_off_size==32UL*1024UL&&
        display_options.flicker_filter==5&&!display_options.soft_display_filter);
    D3DDevice_SetShaderConstantMode(D3DSCM_192CONSTANTS);
    CHECK(mac_device_state_get()->shader_constant_mode==D3DSCM_192CONSTANTS);
    allocations=mac_resource_allocation_count();
    CHECK(IDirect3DDevice8_CreateTexture(device,4,4,1,0,D3DFMT_A4R4G4B4,
        D3DPOOL_MANAGED,&texture)==S_OK&&texture!=NULL);
    CHECK(texture&&mac_resource_allocation_count()==allocations+1);
    CHECK(IDirect3DTexture8_LockRect(texture,0,&locked,NULL,0)==S_OK);
    CHECK(locked.pBits!=NULL&&locked.Pitch>=8);
    pixel=(unsigned char *)locked.pBits;
    if(pixel)pixel[0]=0x12;
    CHECK(pixel&&pixel[0]==0x12);
    CHECK(IDirect3DTexture8_UnlockRect(texture,0)==S_OK);
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)texture)==0);
    texture=NULL;
    CHECK(mac_resource_allocation_count()==allocations);

    CHECK(IDirect3DDevice8_CreateCubeTexture(device,4,1,0,D3DFMT_A4R4G4B4,
        D3DPOOL_MANAGED,&cube)==S_OK&&cube!=NULL);
    CHECK(IDirect3DCubeTexture8_LockRect(cube,D3DCUBEMAP_FACE_NEGATIVE_Z,0,
        &locked,NULL,0)==S_OK&&locked.pBits!=NULL);
    CHECK(IDirect3DCubeTexture8_UnlockRect(cube,D3DCUBEMAP_FACE_NEGATIVE_Z,0)==S_OK);
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)cube)==0);
    cube=NULL;

    CHECK(IDirect3DDevice8_CreateVolumeTexture(device,4,4,4,1,0,
        D3DFMT_A4R4G4B4,D3DPOOL_MANAGED,&volume)==S_OK&&volume!=NULL);
    CHECK(IDirect3DVolumeTexture8_LockBox(volume,0,&locked_box,NULL,0)==S_OK&&
        locked_box.pBits!=NULL&&locked_box.RowPitch>=8&&locked_box.SlicePitch>=32);
    CHECK(IDirect3DVolumeTexture8_UnlockBox(volume,0)==S_OK);
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)volume)==0);
    volume=NULL;

    CHECK(IDirect3DDevice8_CreateTexture(device,16,16,1,D3DUSAGE_RENDERTARGET,
        D3DFMT_LIN_A8R8G8B8,D3DPOOL_DEFAULT,&target)==S_OK&&target!=NULL);
    CHECK(target&&((target->Format&D3DFORMAT_FORMAT_MASK)>>D3DFORMAT_FORMAT_SHIFT)==
        D3DFMT_LIN_A8R8G8B8);
    memset(&surface,0,sizeof(surface));
    surface.Parent=(D3DBaseTexture *)target;surface.Data=target->Data;
    CHECK(mac_engine_device_resources_target_info(&surface,&fbo,&gl_texture,
        &target_width,&target_height));
    CHECK(fbo!=0&&gl_texture!=0&&target_width==16&&target_height==16);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&old_fbo);
    CHECK(mac_engine_device_resources_bind_target(&surface,NULL));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&bound_fbo);
    CHECK((unsigned)bound_fbo==fbo);
    memset(&wrong_depth,0,sizeof(wrong_depth));
    wrong_depth.Format=D3DFMT_D24S8;
    wrong_depth.Size=surface.Size;
    CHECK(!mac_engine_device_resources_bind_target(&surface,&wrong_depth));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&bound_fbo);
    CHECK((unsigned)bound_fbo==fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,(GLuint)old_fbo);
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)target)==0);
    target=NULL;
    CHECK(IDirect3DDevice8_CreateTexture(device,8,8,2,D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&target)==S_OK&&target!=NULL);
    memset(&surface,0,sizeof(surface));surface.Parent=(D3DBaseTexture *)target;
    surface.Data=target->Data;
    CHECK(mac_engine_device_resources_target_info(&surface,&fbo,&gl_texture,
        &target_width,&target_height)&&target_width==8&&target_height==8);
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)target)==0);
    target=NULL;
    CHECK(IDirect3DDevice8_CreateTexture(device,8,8,1,D3DUSAGE_RENDERTARGET,
        D3DFMT_R5G6B5,D3DPOOL_DEFAULT,&target)==S_OK&&target!=NULL);
    memset(&surface,0,sizeof(surface));surface.Parent=(D3DBaseTexture *)target;
    surface.Data=target->Data;
    CHECK(mac_engine_device_resources_target_info(&surface,&fbo,&gl_texture,
        &target_width,&target_height)&&target_width==8&&target_height==8);
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)target)==0);
    target=NULL;

    CHECK(IDirect3DDevice8_CreateTexture(device,640,480,1,D3DUSAGE_RENDERTARGET,
        D3DFMT_LIN_A8R8G8B8,D3DPOOL_DEFAULT,&frame_target)==S_OK&&frame_target!=NULL);
    memset(&frame_test,0,sizeof(frame_test));frame_test.device=device;
    frame_test.color.Parent=(D3DBaseTexture *)frame_target;
    frame_test.color.Data=frame_target->Data;frame_test.color.Format=frame_target->Format;
    frame_test.color.Size=frame_target->Size;
    CHECK(mac_device_gpu_frame(render_target_frame,&frame_test));
    CHECK(frame_test.passed);
    CHECK(IDirect3DBaseTexture8_Release((D3DBaseTexture *)frame_target)==0);
    frame_target=NULL;

    CHECK(IDirect3DDevice8_CreateTexture(device,0,4,1,0,D3DFMT_A4R4G4B4,
        D3DPOOL_MANAGED,&texture)==E_INVALIDARG&&texture==NULL);
    CHECK(IDirect3DDevice8_CreateTexture(device,4,4,1,D3DUSAGE_RENDERTARGET,
        D3DFMT_DXT1,D3DPOOL_DEFAULT,&texture)==E_INVALIDARG&&texture==NULL);
    CHECK(IDirect3DDevice8_CreateTexture(device,4,4,1,D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture)==E_INVALIDARG&&texture==NULL);
    CHECK(IDirect3DDevice8_CreateTexture(device,4,4,1,0,D3DFMT_A4R4G4B4,
        D3DPOOL_DEFAULT,&texture)==E_INVALIDARG&&texture==NULL);
    CHECK(IDirect3DDevice8_CreateTexture(device,4,4,20,0,D3DFMT_A4R4G4B4,
        D3DPOOL_MANAGED,&texture)==E_INVALIDARG&&texture==NULL);
    CHECK(IDirect3DDevice8_CreateCubeTexture(device,4,1,D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&cube)==E_INVALIDARG&&cube==NULL);
    CHECK(IDirect3DDevice8_CreateVolumeTexture(device,4,4,4,2,0,
        D3DFMT_A4R4G4B4,D3DPOOL_MANAGED,&volume)==E_INVALIDARG&&volume==NULL);
    CHECK(mac_resource_allocation_count()==allocations);
    CHECK(IDirect3DDevice8_Release(device)==0);
    CHECK(mac_device_state_get()==NULL);
    CHECK(!mac_engine_device_resources_target_info(&surface,&fbo,&gl_texture,
        &target_width,&target_height));
    printf("native resource checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
