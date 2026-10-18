#include "engine_text.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static int checks,failures,resets;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);}}while(0)
static void reset(void){++resets;}
static uint32_t digest(const unsigned char *p,size_t size)
{
    uint32_t result=2166136261U;while(size--){result^=*p++;result*=16777619U;}return result;
}
int main(void)
{
    short widths[TEXT_HIRES_FIT_LETTERS];
    struct text_hires_glyph a,b;
    const unsigned char *pixels;
    size_t bytes,i,coverage;
    unsigned int width,height;
    unsigned long placeholder[2]={0,0x1234};
    long font,small;
    uint32_t sum;
    for(i=0;i<TEXT_HIRES_FIT_LETTERS;++i)widths[i]=12;
    CHECK(text_hires_embedded_count==3);
    CHECK(((const unsigned char*)text_hires_embedded[0].data)[0]==0 && ((const unsigned char*)text_hires_embedded[0].data)[1]==1);
    CHECK(text_hires_font(NULL,20,widths,1)==-1);
    CHECK(text_hires_font("ui\\large_ui",0,widths,1)==-1);
    CHECK(text_hires_font("ui\\large_ui",20,NULL,1)==-1);
    CHECK(text_hires_font("unknown",20,widths,1)==-1);
    CHECK(!mac_text_set_pixel_scale(0));CHECK(!mac_text_set_pixel_scale(33));CHECK(mac_text_set_pixel_scale(1));
    font=text_hires_font("ui\\large_ui",20,widths,1);CHECK(font>=0);
    CHECK(text_hires_font("UI\\LARGE_UI",20,widths,1)==font);
    small=text_hires_font("ui\\small_ui",12,widths,1);CHECK(small>=0 && small!=font);
    CHECK(text_hires_covers(font,'A'));CHECK(!text_hires_covers(-1,'A'));
    CHECK(!text_hires_covers(font,0x10ffff));
    CHECK(!text_hires_glyph(font,'A',&a));
    text_hires_register_atlas(placeholder,256,256);text_hires_set_reset_hook(reset);
    CHECK(text_hires_glyph(font,'A',&a));CHECK(a.right>a.left && a.bottom>a.top && a.advance>0);
    CHECK(text_hires_glyph(font,'A',&b) && memcmp(&a,&b,sizeof(a))==0);
    CHECK(!text_hires_glyph(font,' ',&b));
    for(i='B';i<='Z';++i)CHECK(text_hires_glyph(font,(unsigned long)i,&b));
    CHECK(text_hires_glyph(small,'g',&b));
    pixels=mac_text_atlas(&bytes,&width,&height);CHECK(pixels && width==2048 && height==2048 && bytes==4194304);
    coverage=0;for(i=0;i<bytes;++i)coverage+=pixels[i]!=0;CHECK(coverage>1000);
    sum=digest(pixels,bytes);
    CHECK(mac_text_set_pixel_scale(2));CHECK(text_hires_glyph(font,'A',&b));CHECK(resets==1);
    CHECK(b.right>b.left && b.bottom>b.top);
    text_hires_register_atlas(NULL,0,0);CHECK(!text_hires_glyph(font,'A',&b));
    mac_text_dispose();CHECK(resets==2);CHECK(mac_text_atlas(&bytes,&width,&height)==NULL && bytes==0);
    CHECK(text_hires_font("ui\\large_ui",20,widths,1)>=0);mac_text_dispose();
    printf("native shared text checks=%d failures=%d digest=%08lx coverage=%lu\n",checks,failures,(unsigned long)sum,(unsigned long)coverage);
    return failures?1:0;
}
