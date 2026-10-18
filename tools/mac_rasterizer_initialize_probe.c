/* Exercise the genuine Xbox rasterizer initializer on the bounded hidden
   native GL device. This test never opens a desktop window or game save. */
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "engine_arena.h"
#include "engine_device_state.h"
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#define MAC_INPUT_NO_CARBON
#include "mac_window.h"
#undef MAC_INPUT_NO_CARBON
#include "engine_rasterizer_services.h"
#include "engine_texture_lifecycle.h"
#include "engine_physical_memory.h"
#include "mac_gpu_textures.h"
#include "cache/physical_memory_map.h"
#include "saved games/game_state.h"
#include "math/real_math.h"
#include <stdio.h>

extern D3DDevice *global_d3d_device;

static unsigned checks,failures;
#define CHECK(x) do { ++checks; if(!(x)) { ++failures; fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__); } } while(0)

static void check_default_rectangle(D3DTexture *texture)
{
    D3DLOCKED_RECT locked;
    unsigned char *row;
    unsigned x,y;
    int locked_ok;
    locked_ok=IDirect3DTexture8_LockRect(texture,0,&locked,NULL,0)>=0;
    CHECK(locked_ok);
    if(!locked_ok)return;
    CHECK(locked.pBits!=NULL&&locked.Pitch>=8);
    if(locked.pBits&&locked.Pitch>=8) {
        for(y=0;y<4;y++) {
            row=(unsigned char *)locked.pBits+y*locked.Pitch;
            for(x=0;x<4;x++) {
                if((x&1)==0) {
                    CHECK(row[x*2]==0x00&&row[x*2+1]==0x0f);
                } else {
                    CHECK(row[x*2]==0xf0&&row[x*2+1]==0xf0);
                }
            }
        }
    }
    CHECK(IDirect3DTexture8_UnlockRect(texture,0)>=0);
}

static void check_default_volume(D3DVolumeTexture *texture)
{
    D3DLOCKED_BOX locked;
    unsigned char *row;
    unsigned x,y,z;
    int locked_ok;
    locked_ok=IDirect3DVolumeTexture8_LockBox(texture,0,&locked,NULL,0)>=0;
    CHECK(locked_ok);
    if(!locked_ok)return;
    CHECK(locked.pBits!=NULL&&locked.RowPitch>=8&&locked.SlicePitch>=32);
    if(locked.pBits&&locked.RowPitch>=8&&locked.SlicePitch>=32) {
        for(z=0;z<4;z++)for(y=0;y<4;y++) {
            row=(unsigned char *)locked.pBits+z*locked.SlicePitch+y*locked.RowPitch;
            for(x=0;x<4;x++) {
                if(((z*16+y*4+x)&1)==0) {
                    CHECK(row[x*2]==0x00&&row[x*2+1]==0x0f);
                } else {
                    CHECK(row[x*2]==0xf0&&row[x*2+1]==0xf0);
                }
            }
        }
    }
    CHECK(IDirect3DVolumeTexture8_UnlockBox(texture,0)>=0);
}

static void check_default_cube(D3DCubeTexture *texture)
{
    D3DLOCKED_RECT locked;
    unsigned char *row;
    unsigned face,x,y;
    int locked_ok;
    for(face=0;face<6;face++) {
        locked_ok=IDirect3DCubeTexture8_LockRect(texture,(D3DCUBEMAP_FACES)face,0,&locked,NULL,0)>=0;
        CHECK(locked_ok);
        if(!locked_ok)continue;
        CHECK(locked.pBits!=NULL&&locked.Pitch>=8);
        if(locked.pBits&&locked.Pitch>=8) {
            for(y=0;y<4;y++) {
                row=(unsigned char *)locked.pBits+y*locked.Pitch;
                for(x=0;x<4;x++) {
                    if((x&1)==0) {
                        CHECK(row[x*2]==0x00&&row[x*2+1]==0x0f);
                    } else {
                        CHECK(row[x*2]==0xf0&&row[x*2+1]==0xf0);
                    }
                }
            }
        }
        CHECK(IDirect3DCubeTexture8_UnlockRect(texture,(D3DCUBEMAP_FACES)face,0)>=0);
    }
}

static void check_uploaded_pixel(const unsigned char *rgba, int first)
{
    if (first) CHECK(rgba[0]==255&&rgba[1]==0&&rgba[2]==0&&rgba[3]==0);
    else CHECK(rgba[0]==0&&rgba[1]==255&&rgba[2]==0&&rgba[3]==255);
}

static void check_default_gpu_uploads(void)
{
    const void *textures[D3DTSS_MAXSTAGES];
    unsigned char sampler[D3DTSS_MAXSTAGES];
    float scale[D3DTSS_MAXSTAGES][4];
    DWORD saved_states[D3DTSS_MAXSTAGES][D3DTSS_MAX];
    struct mac_gpu_texture_scope scope;
    GLuint sentinels[4];
    GLint bound;
    GLuint draw_bindings[4];
    GLenum targets[4]={GL_TEXTURE_2D,GL_TEXTURE_CUBE_MAP,GL_TEXTURE_3D,GL_TEXTURE_2D};
    GLenum faces[6]={GL_TEXTURE_CUBE_MAP_POSITIVE_X,GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
    unsigned char pixels[4*4*4*4];
    const char *error=NULL;
    unsigned stage,face;
    GLint binding_query[4]={GL_TEXTURE_BINDING_2D,GL_TEXTURE_BINDING_CUBE_MAP,
        GL_TEXTURE_BINDING_3D,GL_TEXTURE_BINDING_2D};

    memcpy(saved_states,D3D__TextureState,sizeof(saved_states));
    textures[0]=rasterizer_globals.default_2d_hardware_format;
    textures[1]=rasterizer_globals.default_cm_hardware_format;
    textures[2]=rasterizer_globals.default_3d_hardware_format;
    textures[3]=rasterizer_globals.default_2d_hardware_format;
    for(stage=0;stage<D3DTSS_MAXSTAGES;stage++) {
        D3D__TextureState[stage][D3DTSS_ADDRESSU]=D3DTADDRESS_WRAP;
        D3D__TextureState[stage][D3DTSS_ADDRESSV]=D3DTADDRESS_WRAP;
        D3D__TextureState[stage][D3DTSS_ADDRESSW]=D3DTADDRESS_WRAP;
        D3D__TextureState[stage][D3DTSS_MINFILTER]=D3DTEXF_POINT;
        D3D__TextureState[stage][D3DTSS_MAGFILTER]=D3DTEXF_POINT;
        D3D__TextureState[stage][D3DTSS_MIPFILTER]=D3DTEXF_NONE;
        D3D__TextureState[stage][D3DTSS_MAXANISOTROPY]=1;
    }
    glGenTextures(4,sentinels);
    for(stage=0;stage<4;stage++) {
        glActiveTextureARB(GL_TEXTURE0_ARB+stage);
        glBindTexture(targets[stage],sentinels[stage]);
    }
    {
        int bind_ok=mac_gpu_textures_bind(&scope,textures,
            0x01U | (0x03U<<5) | (0x02U<<10) | (0x01U<<15),sampler,scale,&error);
        CHECK(bind_ok);
        if(!bind_ok) {
            if(error)fprintf(stderr,"texture upload error: %s\n",error);
            glDeleteTextures(4,sentinels);
            memcpy(D3D__TextureState,saved_states,sizeof(saved_states));
            return;
        }
    }
    if(error)fprintf(stderr,"texture upload error: %s\n",error);
    CHECK(sampler[0]==1&&sampler[1]==3&&sampler[2]==2&&sampler[3]==1);
    for(stage=0;stage<4;stage++) {
        glActiveTextureARB(GL_TEXTURE0_ARB+stage);
        glGetIntegerv(binding_query[stage],&bound);
        draw_bindings[stage]=(GLuint)bound;
        CHECK(bound!=0&&bound!=sentinels[stage]);
        if(stage==0||stage==3) {
            CHECK(glGetError()==GL_NO_ERROR);
            glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
            check_uploaded_pixel(pixels,1);
            check_uploaded_pixel(pixels+4,0);
        } else if(stage==1) {
            for(face=0;face<6;face++) {
                glGetTexImage(faces[face],0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
                check_uploaded_pixel(pixels,1);
                check_uploaded_pixel(pixels+4,0);
            }
        } else {
            glGetTexImage(GL_TEXTURE_3D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
            check_uploaded_pixel(pixels,1);
            check_uploaded_pixel(pixels+4,0);
            check_uploaded_pixel(pixels+16*4,1);
        }
    }
    CHECK(draw_bindings[0]!=draw_bindings[3]);
    mac_gpu_textures_end(&scope);
    CHECK(!scope.active);
    for(stage=0;stage<4;stage++) {
        glActiveTextureARB(GL_TEXTURE0_ARB+stage);
        glGetIntegerv(binding_query[stage],&bound);
        CHECK((GLuint)bound==sentinels[stage]);
    }
    CHECK(glGetError()==GL_NO_ERROR);
    glDeleteTextures(4,sentinels);
    memcpy(D3D__TextureState,saved_states,sizeof(saved_states));
}

static void check_no_sampler_modes(void)
{
    const void *textures[D3DTSS_MAXSTAGES]={NULL,NULL,NULL,NULL};
    unsigned char sampler[D3DTSS_MAXSTAGES];
    float scale[D3DTSS_MAXSTAGES][4];
    struct mac_gpu_texture_scope scope;
    const char *error=NULL;
    int ok;
    ok=mac_gpu_textures_bind(&scope,textures,
        0x0aU | (0x11U<<5) | (0x04U<<10) | (0x05U<<15),sampler,scale,&error);
    CHECK(ok);
    CHECK(!error&&sampler[0]==0&&sampler[1]==0&&sampler[2]==0&&sampler[3]==0);
    if(ok)mac_gpu_textures_end(&scope);
    error=NULL;
    ok=mac_gpu_textures_bind(&scope,textures,0x13U,sampler,scale,&error);
    CHECK(!ok&&error!=NULL);
}

int main(void)
{
    struct mac_rasterizer_display_options display;
    const struct mac_device_state *device;
    int physical=0,allocated=0,state=0,math=0,initialized=0;
    CHECK(mac_engine_physical_memory_start());
    if(failures)goto done;
    physical=1;
    physical_memory_allocate();
    allocated=1;
    physical_memory_verify();
    game_state_initialize();
    state=1;
    real_math_initialize();
    math=1;
    CHECK(!mac_device_state_get());
    CHECK(halo_screen_width()==640);
    initialized=rasterizer_initialize();
    CHECK(initialized);
    CHECK(rasterizer_globals.initialized);
    CHECK(global_d3d_device==mac_device_gpu_identity());
    CHECK(mac_device_state_get()!=NULL);
    device=mac_device_state_get();
    CHECK(device&&device->shader_constant_mode==D3DSCM_192CONSTANTS);
    CHECK(mac_engine_rasterizer_services_get_display_options(&display));
    printf("native rasterizer display request: push=%lu kickoff=%lu flicker=%lu soft=%d\n",
        display.push_buffer_size,display.kick_off_size,display.flicker_filter,
        display.soft_display_filter);
    CHECK(display.push_buffer_size==768UL*1024UL);
    CHECK(display.kick_off_size==32UL*1024UL);
    CHECK(display.flicker_filter==5&&display.soft_display_filter==FALSE);
    CHECK(mac_engine_texture_lifecycle_ready());
    if(initialized) {
        CHECK(rasterizer_globals.default_2d_hardware_format!=NULL);
        CHECK(rasterizer_globals.default_3d_hardware_format!=NULL);
        CHECK(rasterizer_globals.default_cm_hardware_format!=NULL);
        check_default_rectangle((D3DTexture *)rasterizer_globals.default_2d_hardware_format);
        check_default_volume((D3DVolumeTexture *)rasterizer_globals.default_3d_hardware_format);
        check_default_cube((D3DCubeTexture *)rasterizer_globals.default_cm_hardware_format);
        check_default_gpu_uploads();
        check_no_sampler_modes();
    }
    if(initialized) {
        rasterizer_dispose();
        initialized=0;
        CHECK(global_d3d_device==NULL);
        CHECK(mac_device_state_get()==NULL);
        CHECK(!mac_engine_texture_lifecycle_ready());
    }
done:
    if(initialized)rasterizer_dispose();
    if(state)game_state_dispose();
    if(allocated)physical_memory_free();
    if(physical)mac_engine_physical_memory_stop();
    if(math)real_math_dispose();
    printf("native original rasterizer initialization: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
