/* Hidden first-person BSP presentation check. This uses an actual retail
   spawn plus a diagnostic camera pose; it does not run biped/gameplay code. */
#include "../port/mac/mac_window.h"
#include "../port/mac/mac_bsp_renderer.h"
#include "../port/mac/retail_spawns.h"
#include <OpenGL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VIEW_WIDTH 640
#define VIEW_HEIGHT 480

struct view_state {
    struct mac_bsp_renderer *renderer;
    struct mac_bsp_camera camera;
    unsigned char *pixels;
    unsigned width,height,geometry_pixels;
    int gl_error;
};
static int finite_number(double value)
{ return value==value&&value>-1000000.0&&value<1000000.0; }
static int render_frame(void *context,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    struct view_state *view=(struct view_state *)context;size_t i;unsigned char r,g,b;unsigned count=0;
    (void)input;
    if(width!=view->width||height!=view->height||!mac_bsp_renderer_draw_camera(view->renderer,width,height,&view->camera))return 0;
    glFinish();glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,(GLsizei)width,(GLsizei)height,GL_RGBA,GL_UNSIGNED_BYTE,view->pixels);
    if(glGetError()!=GL_NO_ERROR){view->gl_error=1;return 0;}
    r=20;g=31;b=51;
    for(i=0;i<(size_t)width*height;++i) {
        const unsigned char *p=view->pixels+i*4;
        if(abs((int)p[0]-r)>4||abs((int)p[1]-g)>4||abs((int)p[2]-b)>4)++count;
    }
    view->geometry_pixels=count;return 1;
}
static int save_ppm(const char *path,const unsigned char *pixels,unsigned width,unsigned height)
{
    FILE *file;unsigned y;
    file=fopen(path,"wb");if(!file)return 0;
    if(fprintf(file,"P6\n%u %u\n255\n",width,height)<0){fclose(file);return 0;}
    for(y=height;y>0;--y) {
        unsigned x;unsigned char rgb[VIEW_WIDTH*3];
        for(x=0;x<width;++x)memcpy(rgb+x*3,pixels+((size_t)(y-1)*width+x)*4,3);
        if(fwrite(rgb,1,width*3,file)!=width*3){fclose(file);return 0;}
    }
    return fclose(file)==0;
}
int main(int argc,char **argv)
{
    FILE *file=NULL;long length;unsigned char *map=NULL;struct mac_retail_cache cache;
    struct mac_retail_spawn spawns[MAC_RETAIL_MAX_SPAWNS];uint32_t spawn_count=0,spawn_index=0;
    const char *error=NULL;struct mac_bsp_renderer *renderer=NULL;struct mac_bsp_renderer_stats stats;
    struct mac_window window;struct view_state view;double eye_height=0.65,yaw_offset=0,pitch=0;
    char *end;int result=1,absolute_pose=argc==10 && !strcmp(argv[4],"--camera");
    double pose[5];unsigned pose_index;
    memset(&window,0,sizeof(window));memset(&view,0,sizeof(view));
    if(argc<4||(argc>8 && !absolute_pose)) {
        fprintf(stderr,"usage: %s map bitmaps.map output.ppm [spawn-index [eye-height [yaw-offset [pitch]]]] OR --camera x y z yaw pitch\n",argv[0]);return 2;
    }
    if(!absolute_pose && argc>4){unsigned long parsed=strtoul(argv[4],&end,10);if(*end||parsed>=MAC_RETAIL_MAX_SPAWNS){fprintf(stderr,"invalid spawn index\n");return 2;}spawn_index=(uint32_t)parsed;}
    if(!absolute_pose && argc>5){eye_height=strtod(argv[5],&end);if(*end||!finite_number(eye_height)){fprintf(stderr,"invalid eye height\n");return 2;}}
    if(!absolute_pose && argc>6){yaw_offset=strtod(argv[6],&end);if(*end||!finite_number(yaw_offset)){fprintf(stderr,"invalid yaw offset\n");return 2;}}
    if(!absolute_pose && argc>7){pitch=strtod(argv[7],&end);if(*end||!finite_number(pitch)){fprintf(stderr,"invalid pitch\n");return 2;}}
    if(absolute_pose)for(pose_index=0;pose_index<5;++pose_index) {
        pose[pose_index]=strtod(argv[5+pose_index],&end);
        if(end==argv[5+pose_index] || *end || !finite_number(pose[pose_index])) {
            fprintf(stderr,"invalid absolute camera pose\n");return 2;
        }
    }
    if(absolute_pose)pitch=pose[4];
    file=fopen(argv[1],"rb");if(!file||fseek(file,0,SEEK_END))goto done;
    length=ftell(file);if(length<0x800||length>0x30000000L||fseek(file,0,SEEK_SET))goto done;
    map=(unsigned char *)malloc((size_t)length);if(!map||fread(map,1,(size_t)length,file)!=(size_t)length)goto done;
    fclose(file);file=NULL;
    if(!mac_retail_open(map,(size_t)length,&cache,&error)||!mac_retail_spawns(&cache,spawns,&spawn_count,&error)) {
        fprintf(stderr,"map/spawns: %s\n",error?error:"decode failed");goto done;
    }
    if(spawn_index>=spawn_count){fprintf(stderr,"spawn index %lu outside %lu actual starts\n",(unsigned long)spawn_index,(unsigned long)spawn_count);goto done;}
    if(spawns[spawn_index].bsp<0){fprintf(stderr,"selected spawn has no concrete BSP index\n");goto done;}
    if(pitch<-1.45||pitch>1.45){fprintf(stderr,"pitch outside diagnostic view limits\n");goto done;}
    view.width=VIEW_WIDTH;view.height=VIEW_HEIGHT;
    view.camera.position[0]=spawns[spawn_index].position[0];
    view.camera.position[1]=spawns[spawn_index].position[1];
    view.camera.position[2]=(double)spawns[spawn_index].position[2]+eye_height;
    view.camera.yaw=(double)spawns[spawn_index].facing+yaw_offset;view.camera.pitch=pitch;
    if(absolute_pose) {
        for(pose_index=0;pose_index<3;++pose_index)view.camera.position[pose_index]=pose[pose_index];
        view.camera.yaw=pose[3];view.camera.pitch=pose[4];
    }
    view.pixels=(unsigned char *)malloc((size_t)view.width*view.height*4);if(!view.pixels)goto done;
    if(!mac_window_open(&window,view.width,view.height,0,"OpenCE hidden BSP camera diagnostic")) {
        fprintf(stderr,"hidden native GL drawable unavailable\n");goto done;
    }
    printf("GL_renderer=%s\n",glGetString(GL_RENDERER));
    renderer=mac_bsp_renderer_open(&cache,(uint32_t)spawns[spawn_index].bsp,argv[2],&error);
    if(!renderer){fprintf(stderr,"BSP renderer: %s\n",error?error:"open failed");goto done;}
    view.renderer=renderer;
    if(!mac_window_frame(&window,render_frame,&view)||view.gl_error||view.geometry_pixels<32) {
        fprintf(stderr,"camera draw failed: scene_pixels=%u gl_error=%d\n",view.geometry_pixels,view.gl_error);goto done;
    }
    if(!save_ppm(argv[3],view.pixels,view.width,view.height)){fprintf(stderr,"could not write output image\n");goto done;}
    mac_bsp_renderer_get_stats(renderer,&stats);
    printf("hidden_BSP_camera=pass spawn=%lu position=(%.4f,%.4f,%.4f) yaw=%.5f pitch=%.5f scene_pixels=%u vertices=%lu textures=%u unsupported=%u\n",
        (unsigned long)spawn_index,view.camera.position[0],view.camera.position[1],view.camera.position[2],
        view.camera.yaw,view.camera.pitch,view.geometry_pixels,stats.vertices,stats.texture_count,stats.unsupported_materials);
    result=0;
done:
    if(file)fclose(file);
    if(window.state) {
        /* Renderer-owned GL textures must be deleted while its context is current. */
        if(renderer)mac_bsp_renderer_close(renderer);
        mac_window_close(&window);
    } else if(renderer)mac_bsp_renderer_close(renderer);
    free(view.pixels);free(map);return result;
}
