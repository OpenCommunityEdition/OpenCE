/* Shared diagnostic BSP draw path. This uploads decoded retail base textures
   and lightmaps to a caller-owned compatibility GL context. It does not
   implement Xbox rasterizer state or complete shader semantics. */
#include "mac_bsp_renderer.h"
#include "retail_texture.h"
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct renderer_texture { uint32_t handle,index;GLuint object; };
struct mac_bsp_renderer {
    double lo[3],hi[3];unsigned long vertex_count;
    const struct mac_retail_cache *cache;FILE *bitmaps;
    uint32_t bitmap_bytes,lightmap_handle,texture_digest;
    uint32_t bsp_index;
    unsigned int texture_count,unsupported;
    struct renderer_texture textures[128];
};
static uint32_t read32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static GLuint texture_get(struct mac_bsp_renderer *r,uint32_t handle,uint32_t index)
{
    struct mac_retail_bitmap bitmap;unsigned char *raw,*rgba;size_t bytes,i;
    GLuint object;unsigned int j;
    for(j=0;j<r->texture_count;++j)if(r->textures[j].handle==handle&&r->textures[j].index==index)return r->textures[j].object;
    if(r->texture_count==128||!mac_retail_bitmap_get(r->cache,handle,index,&bitmap))return 0;
    bytes=(size_t)bitmap.width*bitmap.height*4;
    if(bitmap.external&&(!r->bitmaps||bitmap.file_offset>r->bitmap_bytes||bitmap.stored_bytes>r->bitmap_bytes-bitmap.file_offset))return 0;
    if(!bitmap.external&&(bitmap.file_offset>r->cache->file_size||bitmap.stored_bytes>r->cache->file_size-bitmap.file_offset))return 0;
    raw=(unsigned char *)malloc(bitmap.level_bytes);rgba=(unsigned char *)malloc(bytes);
    if(!raw||!rgba){free(raw);free(rgba);return 0;}
    if(bitmap.external) {
        if(fseek(r->bitmaps,(long)bitmap.file_offset,SEEK_SET)||
            fread(raw,1,bitmap.level_bytes,r->bitmaps)!=bitmap.level_bytes){free(raw);free(rgba);return 0;}
    } else memcpy(raw,r->cache->file+bitmap.file_offset,bitmap.level_bytes);
    if(!mac_retail_texture_decode(bitmap.format,bitmap.width,bitmap.height,raw,bitmap.level_bytes,rgba,bytes)){
        free(raw);free(rgba);return 0;
    }
    for(i=0;i<bytes;++i)r->texture_digest=(r->texture_digest^rgba[i])*UINT32_C(16777619);
    glGenTextures(1,&object);glBindTexture(GL_TEXTURE_2D,object);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,bitmap.width,bitmap.height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    free(raw);free(rgba);
    if(glGetError()!=GL_NO_ERROR){glDeleteTextures(1,&object);return 0;}
    r->textures[r->texture_count].handle=handle;r->textures[r->texture_count].index=index;
    r->textures[r->texture_count++].object=object;return object;
}
int mac_bsp_renderer_bind_model_texture(struct mac_bsp_renderer *r,uint32_t shader_handle,
    float *u_scale,float *v_scale)
{
    uint32_t bitmap_handle;GLuint texture;float u=1.0f,v=1.0f;
    if(u_scale)*u_scale=1.0f;if(v_scale)*v_scale=1.0f;
    if(!r)return 0;
    if(!mac_retail_model_base_bitmap(r->cache,shader_handle,&bitmap_handle,&u,&v)&&
       !mac_retail_chicago_base_bitmap(r->cache,shader_handle,&bitmap_handle,&u,&v)&&
       !mac_retail_base_bitmap(r->cache,shader_handle,&bitmap_handle))return 0;
    texture=texture_get(r,bitmap_handle,0);if(!texture)return 0;
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);
    glEnable(GL_TEXTURE_2D);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    if(glGetError()!=GL_NO_ERROR)return 0;
    if(u_scale)*u_scale=u;if(v_scale)*v_scale=v;
    return 1;
}
static int measure(void *context,const struct mac_retail_material *m)
{
    struct mac_bsp_renderer *r=(struct mac_bsp_renderer *)context;
    uint32_t i;unsigned int j;
    for(i=0;i<m->vertex_count;++i) {
        for(j=0;j<3;++j) {
            double v=m->vertices[i].position[j];
            if(!r->vertex_count||v<r->lo[j])r->lo[j]=v;
            if(!r->vertex_count||v>r->hi[j])r->hi[j]=v;
        }
        ++r->vertex_count;
    }
    return 1;
}
static int draw_material(void *context,const struct mac_retail_material *m)
{
    struct mac_bsp_renderer *r=(struct mac_bsp_renderer *)context;
    uint32_t i,handle;int textured=0,lit=0;GLuint base,light;
    glActiveTexture(GL_TEXTURE1);glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);glDisable(GL_TEXTURE_2D);
    if(r->bitmaps) {
        if(mac_retail_base_bitmap(r->cache,m->shader_handle,&handle)) {
            base=texture_get(r,handle,0);if(!base)return 0;
            glBindTexture(GL_TEXTURE_2D,base);glEnable(GL_TEXTURE_2D);
            glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);textured=1;
            if(m->lightmap_bitmap>=0&&m->light_vertex_count==m->vertex_count) {
                glActiveTexture(GL_TEXTURE1);
                light=texture_get(r,r->lightmap_handle,(uint32_t)m->lightmap_bitmap);if(!light)return 0;
                glBindTexture(GL_TEXTURE_2D,light);glEnable(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
                glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);lit=1;
                glActiveTexture(GL_TEXTURE0);
            }
        } else ++r->unsupported;
    }
    glBegin(GL_TRIANGLES);
    for(i=0;i<m->triangle_count*3;++i) {
        uint16_t index=m->indices[i];const float *p=m->vertices[index].position;
        if(textured) {
            glColor4f(1,1,1,1);glMultiTexCoord2fv(GL_TEXTURE0,m->vertices[index].uv);
            if(lit)glMultiTexCoord2fv(GL_TEXTURE1,m->light_vertices[index].uv);
        } else if(r->bitmaps)glColor3f(1,0,1);
        else {
            double h=((double)p[2]-r->lo[2])/(r->hi[2]-r->lo[2]);
            glColor3d(0.2+0.65*h,0.35+0.5*h,0.15+0.65*h);
        }
        glVertex3fv(p);
    }
    glEnd();return glGetError()==GL_NO_ERROR;
}
struct mac_bsp_renderer *mac_bsp_renderer_open(const struct mac_retail_cache *cache,
    uint32_t bsp_index,const char *bitmap_path,const char **error)
{
    struct mac_bsp_renderer *r;const struct mac_retail_bsp *b;const unsigned char *root;
    const char *geometry_error;long length;unsigned int i;
    if(error)*error=NULL;
    if(!cache||!cache->file||bsp_index>=cache->bsp_count) { if(error)*error="renderer arguments";return NULL; }
    r=(struct mac_bsp_renderer *)calloc(1,sizeof(*r));
    if(!r){if(error)*error="renderer allocation";return NULL;}
    r->cache=cache;r->bsp_index=bsp_index;r->texture_digest=UINT32_C(2166136261);
    b=&cache->bsps[bsp_index];
    if(b->root<b->base||b->root-b->base>b->size||16>b->size-(b->root-b->base)) {
        if(error)*error="BSP root range";free(r);return NULL;
    }
    root=cache->file+b->file_offset+(b->root-b->base);r->lightmap_handle=read32(root+12);
    if(!mac_retail_bsp_geometry(cache,bsp_index,measure,r,&geometry_error)) {
        if(error)*error=geometry_error;free(r);return NULL;
    }
    if(!r->vertex_count){if(error)*error="empty BSP geometry";free(r);return NULL;}
    for(i=0;i<3;++i)if(r->hi[i]<=r->lo[i])r->hi[i]=r->lo[i]+1;
    if(bitmap_path) {
        r->bitmaps=fopen(bitmap_path,"rb");
        if(!r->bitmaps||fseek(r->bitmaps,0,SEEK_END)) {
            if(error)*error="bitmap resource open";if(r->bitmaps)fclose(r->bitmaps);free(r);return NULL;
        }
        length=ftell(r->bitmaps);
        if(length<=0||(uint64_t)length>(uint64_t)UINT32_MAX) {
            if(error)*error="bitmap resource size";fclose(r->bitmaps);free(r);return NULL;
        }
        r->bitmap_bytes=(uint32_t)length;
    }
    return r;
}
int mac_bsp_renderer_draw_topdown(struct mac_bsp_renderer *r,unsigned width,unsigned height)
{
    double pad;
    if(!r||!width||!height)return 0;
    r->unsupported=0;
    glViewport(0,0,(GLsizei)width,(GLsizei)height);glDisable(GL_DITHER);glDisable(GL_CULL_FACE);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    pad=(r->hi[0]-r->lo[0]>r->hi[1]-r->lo[1]?r->hi[0]-r->lo[0]:r->hi[1]-r->lo[1])*0.03;
    glOrtho(r->lo[0]-pad,r->hi[0]+pad,r->lo[1]-pad,r->hi[1]+pad,-r->hi[2]-1,-r->lo[2]+1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    return mac_retail_bsp_geometry(r->cache,r->bsp_index,draw_material,r,NULL);
}
int mac_bsp_renderer_draw_camera(struct mac_bsp_renderer *r,unsigned width,unsigned height,
    const struct mac_bsp_camera *camera)
{
    double f[3],s[3],u[3],norm,near_plane=0.05,far_plane=500.0,top,right;
    double matrix[16];const double radians=0.017453292519943295;unsigned int i;
    if(!r||!camera||!width||!height||camera->pitch<-1.45||camera->pitch>1.45||
       camera->yaw!=camera->yaw||camera->pitch!=camera->pitch||
       camera->yaw>1000000.0||camera->yaw< -1000000.0)return 0;
    r->unsupported=0;
    for(i=0;i<3;++i)if(camera->position[i]!=camera->position[i]||
        camera->position[i]>1000000.0||camera->position[i]< -1000000.0)return 0;
    f[0]=cos(camera->yaw)*cos(camera->pitch);f[1]=sin(camera->yaw)*cos(camera->pitch);f[2]=sin(camera->pitch);
    s[0]=f[1];s[1]=-f[0];s[2]=0;norm=sqrt(s[0]*s[0]+s[1]*s[1]);if(norm==0)return 0;
    for(i=0;i<3;++i)s[i]/=norm;
    u[0]=s[1]*f[2]-s[2]*f[1];u[1]=s[2]*f[0]-s[0]*f[2];u[2]=s[0]*f[1]-s[1]*f[0];
    matrix[0]=s[0];matrix[4]=s[1];matrix[8]=s[2];matrix[12]=-(s[0]*camera->position[0]+s[1]*camera->position[1]+s[2]*camera->position[2]);
    matrix[1]=u[0];matrix[5]=u[1];matrix[9]=u[2];matrix[13]=-(u[0]*camera->position[0]+u[1]*camera->position[1]+u[2]*camera->position[2]);
    matrix[2]=-f[0];matrix[6]=-f[1];matrix[10]=-f[2];matrix[14]=f[0]*camera->position[0]+f[1]*camera->position[1]+f[2]*camera->position[2];
    matrix[3]=matrix[7]=matrix[11]=0;matrix[15]=1;
    glViewport(0,0,(GLsizei)width,(GLsizei)height);glDisable(GL_DITHER);glDisable(GL_CULL_FACE);
    glClearColor(0.08f,0.12f,0.2f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();top=near_plane*tan(60.0*radians/2.0);right=top*(double)width/(double)height;
    glFrustum(-right,right,-top,top,near_plane,far_plane);
    glMatrixMode(GL_MODELVIEW);glLoadMatrixd(matrix);
    return mac_retail_bsp_geometry(r->cache,r->bsp_index,draw_material,r,NULL);
}
void mac_bsp_renderer_get_stats(const struct mac_bsp_renderer *r,struct mac_bsp_renderer_stats *stats)
{
    if(!stats)return;memset(stats,0,sizeof(*stats));if(!r)return;
    stats->vertices=r->vertex_count;stats->texture_digest=r->texture_digest;
    stats->texture_count=r->texture_count;stats->unsupported_materials=r->unsupported;
}
void mac_bsp_renderer_close(struct mac_bsp_renderer *r)
{
    unsigned int i;if(!r)return;
    for(i=0;i<r->texture_count;++i)glDeleteTextures(1,&r->textures[i].object);
    if(r->bitmaps)fclose(r->bitmaps);free(r);
}
