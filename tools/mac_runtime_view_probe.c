#include "../port/mac/mac_runtime_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_file(const char *path,unsigned char **bytes,size_t *size)
{
    FILE *file;long length;unsigned char *data;
    *bytes=NULL;*size=0;file=fopen(path,"rb");if(!file)return 0;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<0||
       length>0x30000000L||fseek(file,0,SEEK_SET)){fclose(file);return 0;}
    data=(unsigned char *)malloc((size_t)length);
    if(!data||fread(data,1,(size_t)length,file)!=(size_t)length) {
        free(data);fclose(file);return 0;
    }
    fclose(file);*bytes=data;*size=(size_t)length;return 1;
}
static int input_is_neutral(const struct mac_input_frame *input)
{
    return input->forward==0.0f&&input->left==0.0f&&input->yaw==0.0f&&input->pitch==0.0f&&
        input->held==0&&input->pressed==0&&input->released==0;
}
int main(int argc,char **argv)
{
    unsigned char *map=NULL;size_t map_size=0;struct mac_retail_cache cache;
    struct mac_runtime_view view,invalid;struct mac_runtime_view_frame first,last;
    struct mac_bsp_camera start,end;const char *error=NULL;int failures=0;
    memset(&view,0,sizeof(view));memset(&invalid,0,sizeof(invalid));
    memset(&first,0,sizeof(first));memset(&last,0,sizeof(last));
    if(argc!=3) { fprintf(stderr,"usage: %s bloodgulch.map bitmaps.map\n",argv[0]);return 2; }
    if(!read_file(argv[1],&map,&map_size)||!mac_retail_open(map,map_size,&cache,&error)) {
        fprintf(stderr,"retail map: %s\n",error?error:"read failed");free(map);return 1;
    }
    if(mac_runtime_view_open(&invalid,&cache,0,argv[2],640,480,(enum mac_runtime_view_mode)99,"invalid mode",&error)||invalid.state)
        ++failures;
    if(!mac_runtime_view_open(&view,&cache,0,argv[2],640,480,MAC_RUNTIME_VIEW_HIDDEN,
        "OpenCE runtime view diagnostic",&error)) {
        fprintf(stderr,"runtime view open: %s\n",error?error:"failed");free(map);return 1;
    }
    if(mac_runtime_view_is_visible(&view)||!mac_runtime_view_is_accelerated(&view))++failures;
    start.position[0]=98.4934;start.position[1]=-157.6390;start.position[2]=2.3547;
    start.yaw=1.32828;start.pitch=0.0;
    end.position[0]=101.614693;end.position[1]=-156.074493;end.position[2]=0.648732722;
    end.yaw=0.75000006;end.pitch=-0.124999993;
    if(!mac_runtime_view_draw(&view,&start,&first,&error)) {
        fprintf(stderr,"start view frame: %s\n",error?error:"failed");++failures;
    }
    if(!mac_runtime_view_draw(&view,&end,&last,&error)) {
        fprintf(stderr,"end view frame: %s\n",error?error:"failed");++failures;
    }
    if(first.frame_number!=1||last.frame_number!=2||first.scene_pixels<32||last.scene_pixels<32||
       !input_is_neutral(&first.input)||!input_is_neutral(&last.input))++failures;
    if(!*mac_runtime_view_renderer(&view))++failures;
    printf("GL_RENDERER=%s\n",mac_runtime_view_renderer(&view));
    printf("CGL_accelerated=%d visible=%d\n",mac_runtime_view_is_accelerated(&view),mac_runtime_view_is_visible(&view));
    printf("runtime view frames=%u,%u scene_pixels=%u,%u input=valid failures=%d\n",
        first.frame_number,last.frame_number,first.scene_pixels,last.scene_pixels,failures);
    mac_runtime_view_close(&view);free(map);
    if(view.state)++failures;
    return failures?1:0;
}
