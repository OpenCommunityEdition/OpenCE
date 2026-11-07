"""Render MCC meter conversion through the real GPU shader, without map assets.

Uses the existing hidden-GL test builder. Covers all 256 authored shape values,
fill thresholds, empty/full/partial meters and fractional placement at 1440p HUD
scale. Run directly in an environment with SDL3, clang, NumPy, Pillow and SciPy.
"""
from pathlib import Path
import test_hud_ammo_thresholds as gpu
from harness import function

root = Path(__file__).resolve().parents[1]
channels = function((root / 'port/linux/game/mcc_bitmaps.c').read_text(), 'mcc_channels')
prefix = gpu.HARNESS[:gpu.HARNESS.index('int main(')].replace(
    'CHECK(ok);glDeleteShader',
    'if(!ok){char log[8192];glGetProgramInfoLog(p,sizeof(log),NULL,log);fprintf(stderr,"%s\\n",log);}CHECK(ok);glDeleteShader')
setup = gpu.HARNESS[gpu.HARNESS.index('int main('):gpu.HARNESS.index(' struct meter_parameters meter=')]
setup = setup.replace('1040*272', '1040*800').replace('1040,272', '1040,800')
checks = r'''
 struct meter_parameters meter={.gradient_min_color=0xffffffff,.gradient_max_color=0xffffff,
  .flash_color=0xff000000,.background_color=0xff000000,.tint_color=0xffffff,.gradient=1};
 struct nv2a_pixel_shader_key key=meter_key(&meter,TRUE);
 GLuint fixed=program(&key);
 /* A synthetic Gearbox bitmap: luminance is coverage, alpha is fill order. */
 for(int y=0;y<2048;y++)for(int x=0;x<2048;x++) {
  unsigned c=mcc_channels(((unsigned)(x%256)<<24)|((unsigned)(y%256)<<16),4);
  int at=4*(y*2048+x);
  atlas[at]=(c>>16)&255;atlas[at+1]=(c>>8)&255;atlas[at+2]=c&255;atlas[at+3]=c>>24;
 }
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2048,2048,0,GL_RGBA,GL_UNSIGNED_BYTE,atlas);
 glGenerateMipmap(GL_TEXTURE_2D);
 int rect[4]={0,0,256,256},cutoffs[]={0,1,64,128,254,255};int renders=0;
 float scales[]={1,1.65f,3};
 for(int size=0;size<3;size++)for(int offset=0;offset<2;offset++)for(int c=0;c<6;c++) {
  int q=cutoffs[c],w=(int)(256*scales[size]),h=w;float shift=offset?0.37f:0;
  meter.gradient_min_color=(q<<24)|0xffffff;meter.flash_color=q<<24;
  key=meter_key(&meter,TRUE);draw(fixed,&key,rect,w,h,shift,out);
  for(int y=2;y<h-2;y++)for(int x=2;x<w-2;x++) {
   float px=shift+(x+0.5f)*256/w,py=shift+(y+0.5f)*256/h;
   /* Ignore exact sampler ties only; interpolated coverage is independent
      of the nearest fill-order texel. */
   if(fabsf(px-roundf(px))<0.001f)continue;
   int threshold=(int)floorf(px),at=4*(y*w+x),row=(int)floorf(py-0.5f);
   float fraction=py-0.5f-row;
   float shape=((row%256)*(1-fraction)+((row+1)%256)*fraction)/255.0f;
   int background[3]={32,48,64};
   for(int channel=0;channel<3;channel++) {
    float expected=background[channel];
    if(threshold<q)expected=255*shape+background[channel]*(1-shape);
    if(fabsf(out[at+channel]-expected)>2.1f) {
     fprintf(stderr,"q=%d scale=%f shift=%f x=%d y=%d channel=%d actual=%d expected=%f\n",q,scales[size],shift,x,y,channel,out[at+channel],expected);return 1;
    }
   }
  }
  renders++;
 }
 printf("PASS: %d MCC meter GPU renders, coverage edges, fill thresholds and native HUD scale\n",renders);
 glDeleteTextures(1,&texture);glDeleteTextures(1,&target);glDeleteFramebuffers(1,&fbo);
 free(atlas);free(out);free(reference);SDL_GL_DestroyContext(context);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
'''
def main():
    # Reuse the hidden-context builder; leave its stock test harness intact.
    original = gpu.HARNESS
    try:
        gpu.HARNESS = prefix + '\n' + channels + '\n' + setup + checks
        gpu.main()
    finally:
        gpu.HARNESS = original


if __name__ == '__main__':
    main()
