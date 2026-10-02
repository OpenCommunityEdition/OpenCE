/* Exercise the actual private shader sources using the scene's existing
   platform stubs. The runner supplies its shimmed copy of raytrace_gl.c. */
#define main picture_scene_main
#include "raytrace_test.c"
#undef main
#include "raytrace_gl.c"
extern int host_rt_debug_read(int which, int x, int y, int count, float *values);


static GLuint rgba_texture(int size, const float *pixel)
{
 float *pixels = malloc((size_t)size * size * 4 * sizeof(float));
 if (!pixels) { fputs("Unable to allocate test texture\n", stderr); exit(1); }
 for (int i = 0; i < size * size; i++) memcpy(pixels + i * 4, pixel, 16);
 GLuint texture; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size, size, 0, GL_RGBA, GL_FLOAT, pixels);
 free(pixels);
 return texture;
}
static void sampler(GLuint program, const char *name, int unit, GLuint texture)
{
 glActiveTexture(GL_TEXTURE0 + unit); glBindTexture(GL_TEXTURE_2D, texture);
 glUniform1i(glGetUniformLocation(program, name), unit);
}
static int shader_check(int pass, const char *name)
{
 printf("%s: %s\n", pass ? "PASS" : "FAIL", name); return !pass;
}


/* Reference transfer functions keep the regression independent of the
   production shader's sRGB helpers and catch double material multiplication. */
static float test_srgb_decode(float value)
{
 return value <= 0.04045f ? value / 12.92f : powf((value + 0.055f) / 1.055f, 2.4f);
}
static float test_srgb_encode(float value)
{
 return value <= 0.0031308f ? value * 12.92f : 1.055f * powf(value, 1.0f / 2.4f) - 0.055f;
}
static int rgb_close(const float *actual, const float *expected)
{
 for (int channel = 0; channel < 3; channel++)
  if (!isfinite(actual[channel]) || fabsf(actual[channel] - expected[channel]) > 0.001f) return 0;
 return 1;
}
static int object_sun_composite_regressions(void)
{
 int failures = 0;
 GLuint program = link_with(vertex_source, composite_source, "object direct-sun composite regression");
 if (!program) return 1;
 ray.trace_scale = 2; use_ray_program(program);
 const float near_clip = 0.0625f, far_clip = 1024.0f;
 const float depth_value = 15728640.0f / 16777215.0f;
 const float z = near_clip * far_clip / (far_clip - depth_value * (far_clip - near_clip));
 const float ambient[4] = { 0.2f, 0.3f, 0.6f, 1.0f };
 const float sunlight[3] = { 0.08f, 0.02f, 0.01f };
 float expected[3], value[4];
 for (int channel = 0; channel < 3; channel++)
  expected[channel] = test_srgb_encode(test_srgb_decode(ambient[channel]) + sunlight[channel]);
 float u[16] = { near_clip,far_clip,0.5f,1, 0,0,4,4, 1,1,0,0, 0,4,4,0 };
 glUniform4fv(glGetUniformLocation(program,"u"),4,u);
 glUniform1i(glGetUniformLocation(program,"rt_enabled"),1);
 glUniform1i(glGetUniformLocation(program,"rt_objects_known"),1);
 glUniform1i(glGetUniformLocation(program,"correct"),1);
 glUniform1i(glGetUniformLocation(program,"light_split"),1);
 glUniform1i(glGetUniformLocation(program,"debug_mode"),0);
 GLuint scene = rgba_texture(4,ambient);
 GLuint effect = rgba_texture(2,(const float[4]){0,0,0,1});
 GLuint world = rgba_texture(2,(const float[4]){1,0,0,0});
 GLuint baked = rgba_texture(2,(const float[4]){0.05f,0.05f,0.05f,1});
 GLuint dynamic = rgba_texture(2,(const float[4]){0.4f,0.3f,0.8f,1});
 GLuint lit = rgba_texture(2,(const float[4]){-1,0,0,0});
 GLuint denoised = rgba_texture(2,(const float[4]){0,0,0,0});
 GLuint applied = rgba_texture(4,(const float[4]){0});
 GLuint objects = rgba_texture(4,(const float[4]){0,0,240.0f/255.0f,1});
 GLuint gbuffer = rgba_texture(2,(const float[4]){-z,0,0,-1});
 GLuint target = rgba_texture(4,(const float[4]){0});
 GLuint depth; glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 uint32_t depths[16];for(int i=0;i<16;i++) depths[i]=0xf0000000u;
 glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,4,4,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,depths);
 /* Texture creation changes the active binding; establish every sampler
    after all allocations, including the depth/type inputs. */
 sampler(program,"scene_texture",0,scene);sampler(program,"depth_texture",1,depth);
 sampler(program,"effect_texture",2,effect);sampler(program,"rt_texture",3,world);
 sampler(program,"baked_texture",4,baked);sampler(program,"lit_texture",5,dynamic);
 sampler(program,"lit_rt",6,lit);sampler(program,"denoised_texture",7,denoised);
 sampler(program,"applied_texture",8,applied);sampler(program,"objects_depth",9,objects);
 sampler(program,"gbuffer_now",10,gbuffer);
 GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);
 GLenum draw_buffer=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&draw_buffer);
 if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) return 1;
 glViewport(0,0,4,4);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(rgb_close(value,ambient),"object zero-direct sentinel preserves back-facing ambient lighting");

 glActiveTexture(GL_TEXTURE6);glBindTexture(GL_TEXTURE_2D,lit);
 float lit_pixels[16];for(int i=0;i<4;i++) { lit_pixels[i*4]=-1;memcpy(lit_pixels+i*4+1,sunlight,12); }
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_FLOAT,lit_pixels);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(rgb_close(value,expected),"front-lit object adds albedo-weighted sunlight once in linear space");

 const float flashlight[4]={0.15f,0.7f,0.1f,1};
 float flashlight_pixels[64];for(int i=0;i<16;i++) memcpy(flashlight_pixels+i*4,flashlight,16);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,scene);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,flashlight_pixels);
 for(int i=0;i<4;i++) { lit_pixels[i*4]=-1;lit_pixels[i*4+1]=lit_pixels[i*4+2]=lit_pixels[i*4+3]=0; }
 glActiveTexture(GL_TEXTURE6);glBindTexture(GL_TEXTURE_2D,lit);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_FLOAT,lit_pixels);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(rgb_close(value,flashlight),"blocked direct sun preserves the object's colored flashlight scene");

 /* Object and level have exactly equal depth. Only their explicit type
    separates the left object's sunlight from the adjacent right level. */
 float scene_pixels[64], object_pixels[64], gbuffer_pixels[16];
 for(int y=0;y<4;y++) for(int x=0;x<4;x++) {
  int at=(y*4+x)*4;memcpy(scene_pixels+at,ambient,16);
  object_pixels[at]=object_pixels[at+1]=x<2?0:1;
  object_pixels[at+2]=x<2?240.0f/255.0f:1;object_pixels[at+3]=1;
 }
 for(int y=0;y<2;y++) for(int x=0;x<2;x++) {
  int at=(y*2+x)*4;gbuffer_pixels[at]=x==0?-z:z;
  gbuffer_pixels[at+1]=gbuffer_pixels[at+2]=0;gbuffer_pixels[at+3]=-1;
  lit_pixels[at]=x==0?-1:1;
  for(int channel=0;channel<3;channel++) lit_pixels[at+1+channel]=x==0?sunlight[channel]:0;
 }
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,scene);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,scene_pixels);
 glActiveTexture(GL_TEXTURE9);glBindTexture(GL_TEXTURE_2D,objects);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,object_pixels);
 glActiveTexture(GL_TEXTURE10);glBindTexture(GL_TEXTURE_2D,gbuffer);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_FLOAT,gbuffer_pixels);
 glActiveTexture(GL_TEXTURE6);glBindTexture(GL_TEXTURE_2D,lit);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_FLOAT,lit_pixels);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(rgb_close(value,expected),"same-depth level taps do not dilute the object's traced sunlight");
 glReadPixels(2,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(rgb_close(value,ambient),"object sunlight does not bleed into adjacent same-depth level pixels");

 /* Both sides now belong to the same-depth object, but meet at an opposing
    normal crease: a front-lit face must not lend direct sun to its back. */
 for(int i=0;i<16;i++) {
  object_pixels[i*4]=object_pixels[i*4+1]=0;
  object_pixels[i*4+2]=240.0f/255.0f;object_pixels[i*4+3]=1;
 }
 for(int y=0;y<2;y++) for(int x=0;x<2;x++) {
  int at=(y*2+x)*4;gbuffer_pixels[at]=-z;
  gbuffer_pixels[at+1]=gbuffer_pixels[at+2]=0;gbuffer_pixels[at+3]=x==0?-1:1;
  lit_pixels[at]=-1;
  for(int channel=0;channel<3;channel++) lit_pixels[at+1+channel]=x==0?sunlight[channel]:0;
 }
 glActiveTexture(GL_TEXTURE9);glBindTexture(GL_TEXTURE_2D,objects);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,object_pixels);
 glActiveTexture(GL_TEXTURE10);glBindTexture(GL_TEXTURE_2D,gbuffer);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_FLOAT,gbuffer_pixels);
 glActiveTexture(GL_TEXTURE6);glBindTexture(GL_TEXTURE_2D,lit);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_FLOAT,lit_pixels);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(2,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(rgb_close(value,ambient),"back-facing object retains ambient without sunlight across its normal crease");
 glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(rgb_close(value,expected),"front-facing object's direct sunlight is not diluted by opposite object normals");
 return failures;
}

static int hybrid_base_regressions(void)
{
 int failures=0;
 GLuint inject=link_with(vertex_source,inject_source,"full-resolution Hybrid injection regression");
 GLuint composite=link_with(vertex_source,composite_source,"full-resolution Hybrid correction regression");
 if(!inject||!composite) return 1;
 const int scales[]={1,2,4,8};
 const float material[]={0.4f,0.7f,0.2f};
 const float near_clip=0.0625f,far_clip=1024;
 const float z=near_clip*far_clip/(far_clip-(15728640.0f/16777215.0f)*(far_clip-near_clip));
 for(int scale_index=0;scale_index<4;scale_index++) {
  int scale=scales[scale_index],size=16,grid=size/scale;ray.trace_scale=scale;
  float base_pixels[16*16*4],scene_pixels[16*16*4],applied_pixels[16*16*4],carrier_pixels[16*16*4],final_pixels[16*16*4];
  for(int y=0;y<size;y++) for(int x=0;x<size;x++) {
   int at=(y*size+x)*4;
   base_pixels[at]=0.04f+0.01f*x; /* A one-pixel ramp within every RT cell. */
   base_pixels[at+1]=(x+y)&1?0.18f:0.5f; /* Fine colored point-light detail. */
   base_pixels[at+2]=0.03f+0.015f*y;base_pixels[at+3]=1;
  }
  GLuint base=rgba_texture(size,(const float[4]){0});
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,size,size,GL_RGBA,GL_FLOAT,base_pixels);
  GLuint previous=rgba_texture(grid,(const float[4]){0.1f,0.1f,0.1f,1});
  GLuint current=rgba_texture(grid,(const float[4]){0.25f,0.25f,0.25f,1});
  GLuint geometry=rgba_texture(grid,(const float[4]){z,0,0,1});
  GLuint objects=rgba_texture(size,(const float[4]){1,1,1,1});
  GLuint carrier=rgba_texture(size,(const float[4]){0});
  GLuint applied=rgba_texture(size,(const float[4]){0});
  GLuint scene=rgba_texture(size,(const float[4]){0});
  GLuint output=rgba_texture(size,(const float[4]){0});
  GLuint world=rgba_texture(grid,(const float[4]){2,0,0,0});
  GLuint effect=rgba_texture(grid,(const float[4]){0,0,0,1});
  GLuint coarse_base=rgba_texture(grid,(const float[4]){0.03f,0.03f,0.03f,1});
  GLuint depth;glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  uint32_t depths[16*16];for(int i=0;i<size*size;i++) depths[i]=0xf0000000u;
  glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,size,size,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,depths);
  GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,carrier,0);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,applied,0);
  const GLenum attachments[]={GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};glDrawBuffers(2,attachments);
  glViewport(0,0,size,size);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
  glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  float u[16]={near_clip,far_clip,0.5f,1,0,0,size,size,0,0,0,0,0,size,size,0};
  float cameras[32]={0};cameras[3]=cameras[19]=0.5f;cameras[7]=cameras[23]=1;
  cameras[5]=cameras[10]=cameras[12]=cameras[21]=cameras[26]=cameras[28]=1;
  use_ray_program(inject);glUniform4fv(glGetUniformLocation(inject,"u"),4,u);
  glUniform4fv(glGetUniformLocation(inject,"cameras"),8,cameras);
  glUniform4fv(glGetUniformLocation(inject,"previous_grid"),1,(const float[4]){0,0,grid,grid});
  glUniform4fv(glGetUniformLocation(inject,"fallback"),1,(const float[4]){0,0,0,1});
  sampler(inject,"depth_texture",0,depth);sampler(inject,"irradiance_texture",1,previous);
  sampler(inject,"gbuffer_texture",2,geometry);sampler(inject,"objects_texture",3,objects);
  sampler(inject,"base_texture",4,base);
  glDrawArrays(GL_TRIANGLES,0,3);
  glReadBuffer(GL_COLOR_ATTACHMENT0);glReadPixels(0,0,size,size,GL_RGBA,GL_FLOAT,carrier_pixels);
  glReadBuffer(GL_COLOR_ATTACHMENT1);glReadPixels(0,0,size,size,GL_RGBA,GL_FLOAT,applied_pixels);
  int inject_ok=1;
  for(int i=0;i<size*size;i++) {
   for(int channel=0;channel<3;channel++) {
    float took=applied_pixels[i*4+channel]-1;
    inject_ok &= fabsf(took-carrier_pixels[i*4+channel]-base_pixels[i*4+channel])<1e-5f;
    scene_pixels[i*4+channel]=material[channel]*took;
   }
   scene_pixels[i*4+3]=1;
  }
  char name[128];snprintf(name,sizeof(name),"scale%d Hybrid carrier records each full-resolution base pixel",scale);
  failures+=shader_check(inject_ok,name);
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,scene);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,size,size,GL_RGBA,GL_FLOAT,scene_pixels);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,0,0);
  glDrawBuffers(1,attachments);glReadBuffer(GL_COLOR_ATTACHMENT0);
  use_ray_program(composite);glUniform4fv(glGetUniformLocation(composite,"u"),4,u);
  glUniform1i(glGetUniformLocation(composite,"correct"),2);glUniform1i(glGetUniformLocation(composite,"rt_enabled"),1);
  sampler(composite,"scene_texture",0,scene);sampler(composite,"depth_texture",1,depth);
  sampler(composite,"effect_texture",2,effect);sampler(composite,"rt_texture",3,world);
  sampler(composite,"baked_texture",4,coarse_base);sampler(composite,"lit_texture",5,coarse_base);
  sampler(composite,"lit_rt",6,current);sampler(composite,"denoised_texture",7,current);
  sampler(composite,"applied_texture",8,applied);sampler(composite,"objects_depth",9,objects);
  sampler(composite,"gbuffer_now",10,geometry);sampler(composite,"base_texture",11,base);
  glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,size,size,GL_RGBA,GL_FLOAT,final_pixels);
  int final_ok=1;
  for(int i=0;i<size*size;i++) for(int channel=0;channel<3;channel++)
   final_ok &= isfinite(final_pixels[i*4+channel])&&fabsf(final_pixels[i*4+channel]-material[channel]*(0.25f+base_pixels[i*4+channel]))<0.001f;
  snprintf(name,sizeof(name),"scale%d Hybrid preserves fine base ramp and colored light with exact material ratio",scale);
  failures+=shader_check(final_ok,name);
 }
 ray.trace_scale=2;
 return failures;
}


/* Additive cubemap/specular glints never contain the diffuse light carrier.
   Correcting the finished scene amplifies them when history was dark. */
static int additive_highlight_regressions(void)
{
 int failures=0;
 GLuint program=link_with(vertex_source,composite_source,"additive highlight isolation regression");
 if(!program) return 1;
 const int size=16,scales[]={1,2,4,8};
 const float near_clip=0.0625f,far_clip=1024;
 const float z=near_clip*far_clip/(far_clip-(15728640.0f/16777215.0f)*(far_clip-near_clip));
 const float albedo[]={0.4f,0.6f,0.8f}, levels[]={0.8f,0.0f,0.3f};
 for(int si=0;si<4;si++) for(int mode=1;mode<=2;mode++) {
  ray.trace_scale=scales[si];int grid=size/scales[si];
  const float base=mode==2?0.1f:0;
  float diffuse_pixels[16*16*4],scene_pixels[16*16*4],glints[16*16*4],values[16*16*4];
  for(int y=0;y<size;y++) for(int x=0;x<size;x++) {
   int at=(y*size+x)*4;
   for(int c=0;c<3;c++) {
    /* Fine coloured dots and a diagonal streak, deliberately smaller than RT cells. */
    glints[at+c]=((x+3*y)%7==0?0.02f*(c+1):0)+(x==y?0.08f:0);
    diffuse_pixels[at+c]=albedo[c]*(0.3f+base);
    scene_pixels[at+c]=diffuse_pixels[at+c]+glints[at+c];
   }
   diffuse_pixels[at+3]=scene_pixels[at+3]=1;
  }
  GLuint scene=rgba_texture(size,(const float[4]){0});
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,size,size,GL_RGBA,GL_FLOAT,scene_pixels);
  GLuint diffuse=rgba_texture(size,(const float[4]){0});
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,size,size,GL_RGBA,GL_FLOAT,diffuse_pixels);
  GLuint output=rgba_texture(size,(const float[4]){0});
  GLuint original_base=rgba_texture(size,(const float[4]){base,base,base,1});
  GLuint applied=rgba_texture(size,(const float[4]){1.3f+base,1.3f+base,1.3f+base,1});
  GLuint objects=rgba_texture(size,(const float[4]){1,1,1,1});
  GLuint geometry=rgba_texture(grid,(const float[4]){z,0,0,-1});
  GLuint world=rgba_texture(grid,(const float[4]){2,0,0,0});
  GLuint dark=rgba_texture(grid,(const float[4]){0,0,0,1});
  GLuint light=rgba_texture(grid,(const float[4]){0});
  GLuint depth;glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  uint32_t depths[16*16];for(int i=0;i<size*size;i++) depths[i]=0xf0000000u;
  glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,size,size,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,depths);
  GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
  const GLenum attachment=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&attachment);glReadBuffer(attachment);
  glViewport(0,0,size,size);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  use_ray_program(program);
  float u[16]={near_clip,far_clip,0.5f,1,0,0,size,size,0,0,0,0,0,size,size,0};
  glUniform4fv(glGetUniformLocation(program,"u"),4,u);
  glUniform1i(glGetUniformLocation(program,"correct"),mode);glUniform1i(glGetUniformLocation(program,"rt_enabled"),1);
  glUniform1i(glGetUniformLocation(program,"diffuse_known"),1);
  sampler(program,"scene_texture",0,scene);sampler(program,"depth_texture",1,depth);
  sampler(program,"effect_texture",2,dark);sampler(program,"rt_texture",3,world);
  sampler(program,"baked_texture",4,dark);sampler(program,"lit_texture",5,dark);
  sampler(program,"lit_rt",6,dark);sampler(program,"denoised_texture",7,light);
  sampler(program,"applied_texture",8,applied);sampler(program,"objects_depth",9,objects);
  sampler(program,"gbuffer_now",10,geometry);sampler(program,"base_texture",11,original_base);
  sampler(program,"diffuse_texture",12,diffuse);
  for(int li=0;li<3;li++) {
   float light_pixels[16*16*4];for(int i=0;i<grid*grid;i++) {
    light_pixels[i*4]=light_pixels[i*4+1]=light_pixels[i*4+2]=levels[li];light_pixels[i*4+3]=1;
   }
   glActiveTexture(GL_TEXTURE7);glBindTexture(GL_TEXTURE_2D,light);
   glTexSubImage2D(GL_TEXTURE_2D,0,0,0,grid,grid,GL_RGBA,GL_FLOAT,light_pixels);
   glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,size,size,GL_RGBA,GL_FLOAT,values);
   int ok=1;
   for(int i=0;i<size*size;i++) {
    for(int c=0;c<3;c++) ok &= isfinite(values[i*4+c])&&fabsf(values[i*4+c]-(albedo[c]*(levels[li]+base)+glints[i*4+c]))<0.001f;
    ok &= fabsf(values[i*4+3]-1)<0.001f;
   }
   char label[160];snprintf(label,sizeof(label),"scale%d %s GI %.2f preserves fine additive glints and streaks without amplification",scales[si],mode==1?"Path":"Hybrid",levels[li]);
   failures+=shader_check(ok,label);
  }
 }
 ray.trace_scale=2;
 return failures;
}

static int diffuse_capture_regression(void)
{
 GLuint saved_color=color_texture;
 /* The public capture uses the renderer's actual dimensions and format. */
 glGenTextures(1,&color_texture);glBindTexture(GL_TEXTURE_2D,color_texture);
 unsigned char *colour=malloc((size_t)WIDTH*HEIGHT*4);
 for(int i=0;i<WIDTH*HEIGHT;i++){colour[i*4]=51;colour[i*4+1]=102;colour[i*4+2]=153;colour[i*4+3]=191;}
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,WIDTH,HEIGHT,0,GL_RGBA,GL_UNSIGNED_BYTE,colour);free(colour);
 ray.initialized=1;ray.enabled=1;ray.failed=0;ray.gi=3;ray.hardware=1;
 size_textures(WIDTH,HEIGHT);ray.light_stages=7;
 halo_ray_traced_light_stage(3);
 GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,ray.diffuse_texture,0);
 const GLenum attachment=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&attachment);glReadBuffer(attachment);
 unsigned char captured[4];glReadPixels(3,3,1,1,GL_RGBA,GL_UNSIGNED_BYTE,captured);
 float pixel[4];for(int c=0;c<4;c++)pixel[c]=captured[c]/255.0f;
 int failures=shader_check((ray.light_stages&15)==15&&rgb_close(pixel,(const float[3]){0.2f,0.4f,0.6f}),"diffuse capture records full-resolution textured colour before additive passes");
 ray.light_stages=7;ray.gi=0;halo_ray_traced_light_stage(3);
 failures+=shader_check(ray.light_stages==7,"diffuse capture is invalid when GI is disabled");
 color_texture=saved_color;ray.gi=0;ray.light_stages=0;
 return failures;
}

static int object_sun_phase_regression(void)
{
 GLuint program=link_with(vertex_source,composite_source,"Eighth object sun phase regression");
 if(!program) return 1;
 ray.trace_scale=8;
 const float near_clip=0.0625f,far_clip=1024;
 const float z=near_clip*far_clip/(far_clip-(15728640.0f/16777215.0f)*(far_clip-near_clip));
 GLuint scene=rgba_texture(16,(const float[4]){0.2f,0.3f,0.4f,1});
 GLuint objects=rgba_texture(16,(const float[4]){0,0,240.0f/255.0f,1});
 GLuint output=rgba_texture(16,(const float[4]){0});
 GLuint geometry=rgba_texture(2,(const float[4]){-z,0,0,-1});
 GLuint effect=rgba_texture(2,(const float[4]){0,0,0,1});
 GLuint world=rgba_texture(2,(const float[4]){1,0,0,0});
 GLuint sunlight=rgba_texture(2,(const float[4]){-1,0,0,0});
 const float sunlight_pixels[16]={-1,0,0,0,-1,0.2f,0.04f,0.01f,-1,0,0,0,-1,0.2f,0.04f,0.01f};
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_FLOAT,sunlight_pixels);
 GLuint depth;glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 uint32_t depths[256];for(int i=0;i<256;i++) depths[i]=0xf0000000u;
 glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,16,16,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,depths);
 GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
 const GLenum attachment=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&attachment);glReadBuffer(attachment);
 glViewport(0,0,16,16);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
 use_ray_program(program);
 glUniform4fv(glGetUniformLocation(program,"u"),4,(const float[16]){near_clip,far_clip,0.5f,1,0,0,16,16,0,0,0,0,0,16,16,0});
 glUniform1i(glGetUniformLocation(program,"rt_enabled"),1);glUniform1i(glGetUniformLocation(program,"rt_objects_known"),1);
 sampler(program,"scene_texture",0,scene);sampler(program,"depth_texture",1,depth);
 sampler(program,"effect_texture",2,effect);sampler(program,"rt_texture",3,world);
 sampler(program,"baked_texture",4,effect);sampler(program,"lit_texture",5,effect);
 sampler(program,"lit_rt",6,sunlight);sampler(program,"denoised_texture",7,effect);
 sampler(program,"applied_texture",8,scene);sampler(program,"objects_depth",9,objects);
 sampler(program,"gbuffer_now",10,geometry);sampler(program,"base_texture",11,scene);
 glDrawArrays(GL_TRIANGLES,0,3);
 float pixels[8*4];glReadPixels(0,4,8,1,GL_RGBA,GL_FLOAT,pixels);
 int smooth=1;float first=test_srgb_decode(pixels[0]),previous=first;
 for(int x=1;x<8;x++) {
  float current=test_srgb_decode(pixels[x*4]);
  smooth &= isfinite(current)&&current>previous+1e-4f&&current-previous<0.025f;
  previous=current;
 }
 smooth &= previous-first>0.02f;
 ray.trace_scale=2;
 return shader_check(smooth,"Eighth object sun varies smoothly between full pixels inside one tracing cell");
}

int main(int argc, char **argv)
{
	if (argc != 2) return 2;
	char library[1024];
	snprintf(library, sizeof(library), "%s/libGLESv2.dylib", argv[1]);
	SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, library);
	snprintf(library, sizeof(library), "%s/libEGL.dylib", argv[1]);
	SDL_SetHint(SDL_HINT_EGL_LIBRARY, library);
	setenv("ANGLE_DEFAULT_PLATFORM", "metal", 0);
	if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_Window *window = SDL_CreateWindow("depth regressions", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
	SDL_GLContext context = window ? SDL_GL_CreateContext(window) : NULL;
	if (!context || !gl_functions_load()) return 1;
	printf("%s\n", (const char *)glGetString(GL_RENDERER));
	GLuint depth, target, fbo, vao;
	glGenTextures(1, &depth);
	glBindTexture(GL_TEXTURE_2D, depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	uint32_t clear = 0xffffff00u;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 1, 1, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &clear);
	glGenTextures(1, &target);
	glBindTexture(GL_TEXTURE_2D, target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glViewport(0, 0, 1, 1);
	float u[16] = { 0.0625f, 1024, 0.5f, 1, 0,0,1,1, 0,0,0,0, 0,1,1,0 };
	GLuint program = link_with(vertex_source, objects_source, "depth encoding regression");
	if (!program) return 1; use_ray_program(program);
	glUniform4fv(glGetUniformLocation(program, "u"), 4, u);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depth);
	glUniform1i(glGetUniformLocation(program, "depth_texture"), 0);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	unsigned char pixel[4];
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	int failures = pixel[0] != 255 || pixel[1] != 255 || pixel[2] != 255;
	printf("%s: clear depth packs as ff ff ff (got %02x %02x %02x)\n", failures ? "FAIL" : "PASS", pixel[0], pixel[1], pixel[2]);

	/* A real depth just short of the far plane must survive sky rejection. */
	GLuint packed = target;
	glGenTextures(1, &target);
	glBindTexture(GL_TEXTURE_2D, target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
	program = link_with(vertex_source, gbuffer_source, "far geometry regression");
	if (!program) return 1; use_ray_program(program);
	glUniform4fv(glGetUniformLocation(program, "u"), 4, u);
	glUniform1i(glGetUniformLocation(program, "depth_texture"), 0);
	glUniform1i(glGetUniformLocation(program, "objects_texture"), 1);
	glUniform1i(glGetUniformLocation(program, "objects_known"), 0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, packed);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, depth);
	uint32_t far_depth = 0xffff8000u;
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &far_depth);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	float g[4]; glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, g);
	int visible = g[0] > 880 && g[0] < 1024 && isfinite(g[0]);
	printf("%s: far geometry remains in gbuffer (depth %.3f)\n", visible ? "PASS" : "FAIL", g[0]);
	failures += !visible;

 /* Reprojected darkness must retain material colour for the current frame. */
 glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, depth);
 uint32_t depths[16]; for (int i = 0; i < 16; i++) depths[i] = 0xf0000000u;
 glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 4, 4, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, depths);
 float z = u[0] * u[1] / (u[1] - (240.0f / 256.0f) * (u[1] - u[0]));
 u[6] = u[7] = u[13] = u[14] = 4;
 GLuint dark = rgba_texture(4, (const float[4]){0,0,0,1});
 GLuint old_g = rgba_texture(4, (const float[4]){z,0,0,-1});
 GLuint objects = rgba_texture(4, (const float[4]){1,1,1,1});
 GLuint carrier = rgba_texture(4, (const float[4]){0});
 GLuint applied = rgba_texture(4, (const float[4]){0});
 glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, carrier, 0);
 glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, applied, 0);
 GLenum both[] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1}; glDrawBuffers(2,both);
 glViewport(0,0,4,4);
 program = link_with(vertex_source, inject_source, "zero history carrier regression");
 if (!program) return 1; use_ray_program(program); glUniform4fv(glGetUniformLocation(program,"u"),4,u);
 float cameras[32] = {0}; cameras[3]=cameras[19]=0.5; cameras[5]=cameras[8+2]=cameras[12]=1;
 cameras[7]=cameras[23]=1; cameras[21]=cameras[26]=cameras[28]=1;
 glUniform4fv(glGetUniformLocation(program,"cameras"),8,cameras);
 glUniform4fv(glGetUniformLocation(program,"previous_grid"),1,(const float[4]){0,0,2,2});
 glUniform4fv(glGetUniformLocation(program,"fallback"),1,(const float[4]){0});
 sampler(program,"depth_texture",0,depth); sampler(program,"irradiance_texture",1,dark);
 sampler(program,"gbuffer_texture",2,old_g); sampler(program,"objects_texture",3,objects);
 sampler(program,"base_texture",4,dark);
 glDrawArrays(GL_TRIANGLES,0,3); float value[4]; glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0] - 64.0f/255.0f) < 0.001f,"zero previous light retains a recoverable carrier");
 /* Reprojection must match the geometric plane, not only similar depth.
    A new wall at a corner must not borrow illumination from the old wall. */
 glReadBuffer(GL_COLOR_ATTACHMENT0);
 glUniform1i(glGetUniformLocation(program,"split"),2);
 float history_geometry[64];
 for(int i=0;i<16;i++) { history_geometry[i*4]=z;history_geometry[i*4+1]=1;history_geometry[i*4+2]=history_geometry[i*4+3]=0; }
 glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,old_g);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,history_geometry);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(2,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(value[1]<0.001f,"history injection rejects same-depth perpendicular wall lighting");
 for(int i=0;i<16;i++) { history_geometry[i*4]=z+0.06f;history_geometry[i*4+1]=history_geometry[i*4+2]=0;history_geometry[i*4+3]=-1; }
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,history_geometry);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(2,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(value[1]<0.001f,"history injection rejects parallel nearby geometry after disocclusion");
 for(int i=0;i<16;i++) history_geometry[i*4]=z;
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,history_geometry);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(2,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(value[1]>0.999f,"history injection retains matching wall illumination");
 glUniform1i(glGetUniformLocation(program,"split"),0);
 glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,0,0);
 glDrawBuffers(1,both);
 GLuint final = rgba_texture(4,(const float[4]){0});
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,final,0);
 GLuint scene = rgba_texture(4,(const float[4]){0.5f*64/255,0.5f*64/255,0.5f*64/255,1});
 GLuint bright = rgba_texture(4,(const float[4]){0.5,0.5,0.5,1});
 GLuint complete = rgba_texture(4,(const float[4]){2,0,0,0});
 program = link_with(vertex_source,composite_source,"dark-to-lit correction regression");
 if (!program) return 1; use_ray_program(program); glUniform4fv(glGetUniformLocation(program,"u"),4,u);
 glUniform1i(glGetUniformLocation(program,"correct"),1);
 glUniform1i(glGetUniformLocation(program,"rt_enabled"),1);
 sampler(program,"scene_texture",0,scene); sampler(program,"depth_texture",1,depth);
 sampler(program,"effect_texture",2,dark); sampler(program,"rt_texture",3,complete);
 sampler(program,"baked_texture",4,dark); sampler(program,"lit_texture",5,dark);
 sampler(program,"lit_rt",6,dark); sampler(program,"denoised_texture",7,bright);
 sampler(program,"applied_texture",8,applied); sampler(program,"objects_depth",9,objects);
 sampler(program,"gbuffer_now",10,old_g);
 glDrawArrays(GL_TRIANGLES,0,3); glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0]-0.25f)<0.01f,"current lighting recovers colour after black history");
 /* Model the game's RGBA8 scene, whose quantization must not be amplified
    into large colour steps when history is black. */
 glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,scene);
 unsigned char quantized[4*4*4];
 for(int i=0;i<16;i++) { quantized[i*4]=quantized[i*4+1]=quantized[i*4+2]=(unsigned char)lroundf(0.47f*64); quantized[i*4+3]=255; }
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,4,4,0,GL_RGBA,GL_UNSIGNED_BYTE,quantized);
 glDrawArrays(GL_TRIANGLES,0,3); glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0]-0.47f*0.5f)<0.005f,"black-history recovery preserves RGBA8 material precision");
 sampler(program,"denoised_texture",7,dark);
 glDrawArrays(GL_TRIANGLES,0,3); glReadPixels(1,1,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0])<0.001f,"carrier does not brighten genuinely dark current lighting");

 /* Even full-resolution pixels coincide with the samples that generated
    the half-grid. A ramp catches a half-pixel shift that constant light
    cannot reveal. Poisoned fallback data must not leak into complete GI. */
 float ramp[4*4*4];
 for(int y=0;y<4;y++) for(int x=0;x<4;x++) {
  int i=(y*4+x)*4; ramp[i]=ramp[i+1]=ramp[i+2]=x==0?0.2f:0.8f; ramp[i+3]=1;
 }
 glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D,bright);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,ramp);
 glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,scene);
 float scene_pixels[4*4*4];
 for(int i=0;i<16;i++) { scene_pixels[i*4]=scene_pixels[i*4+1]=scene_pixels[i*4+2]=0.5f*64/255; scene_pixels[i*4+3]=1; }
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,4,4,0,GL_RGBA,GL_FLOAT,scene_pixels);
 GLuint poison = rgba_texture(4,(const float[4]){3,3,3,0});
 sampler(program,"scene_texture",0,scene);sampler(program,"effect_texture",2,poison); sampler(program,"denoised_texture",7,bright);
 glDrawArrays(GL_TRIANGLES,0,3); glReadPixels(2,0,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0]-0.4f)<0.001f && fabsf(value[1]-0.4f)<0.001f,"complete GI aligns to sampled pixel centers and excludes fallback light");
 /* The complete-GI early return must retain its optional reflection. */
 glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D,complete);
 float reflective[4*4*4];
 for(int i=0;i<16;i++) { reflective[i*4]=2;reflective[i*4+1]=reflective[i*4+2]=0;reflective[i*4+3]=0.5f; }
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,4,4,GL_RGBA,GL_FLOAT,reflective);
 u[15]=1;glUniform4fv(glGetUniformLocation(program,"u"),4,u);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(2,0,1,1,GL_RGBA,GL_FLOAT,value);
 float reflected_expected=0.4f+(0.5f*64/255)*0.5f*(1-0.4f*0.5f);
 failures += shader_check(fabsf(value[0]-reflected_expected)<0.001f,"complete GI preserves reflected scene color");
 u[15]=0;

 /* The same physical samples are four pixels apart at quarter scale;
    pixel x=4 must receive sample1, with reflection and material intact. */
 ray.trace_scale=4;use_ray_program(program);
 GLuint quarter_scene=rgba_texture(8,(const float[4]){0.5f*64/255,0.5f*64/255,0.5f*64/255,1});
 GLuint quarter_applied=rgba_texture(8,(const float[4]){1+64.0f/255,1+64.0f/255,1+64.0f/255,1});
 GLuint quarter_objects=rgba_texture(8,(const float[4]){1,1,1,1});
 GLuint quarter_final=rgba_texture(8,(const float[4]){0});
 uint32_t quarter_depths[64];for(int i=0;i<64;i++) quarter_depths[i]=0xf0000000u;
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depth);
 glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,8,8,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,quarter_depths);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,quarter_final,0);
 glViewport(0,0,8,8);u[6]=u[7]=u[13]=u[14]=8;u[15]=1;
 glUniform4fv(glGetUniformLocation(program,"u"),4,u);
 sampler(program,"scene_texture",0,quarter_scene);sampler(program,"depth_texture",1,depth);
 sampler(program,"effect_texture",2,poison);sampler(program,"rt_texture",3,complete);
 sampler(program,"applied_texture",8,quarter_applied);sampler(program,"objects_depth",9,quarter_objects);
 sampler(program,"denoised_texture",7,bright);sampler(program,"gbuffer_now",10,old_g);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(4,0,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0]-reflected_expected)<0.001f&&fabsf(value[1]-reflected_expected)<0.001f,
  "quarter-resolution GI retains sample alignment, color and reflection");
 /* Eighth scale has eight physical pixels between lighting samples. */
 ray.trace_scale=8;use_ray_program(program);
 GLuint eighth_scene=rgba_texture(16,(const float[4]){0.5f*64/255,0.5f*64/255,0.5f*64/255,1});
 GLuint eighth_applied=rgba_texture(16,(const float[4]){1+64.0f/255,1+64.0f/255,1+64.0f/255,1});
 GLuint eighth_objects=rgba_texture(16,(const float[4]){1,1,1,1});
 GLuint eighth_final=rgba_texture(16,(const float[4]){0});
 uint32_t eighth_depths[256];for(int i=0;i<256;i++) eighth_depths[i]=0xf0000000u;
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depth);
 glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,16,16,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,eighth_depths);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,eighth_final,0);
 glViewport(0,0,16,16);u[6]=u[7]=u[13]=u[14]=16;
 glUniform4fv(glGetUniformLocation(program,"u"),4,u);
 sampler(program,"scene_texture",0,eighth_scene);sampler(program,"applied_texture",8,eighth_applied);
 sampler(program,"objects_depth",9,eighth_objects);
 sampler(program,"depth_texture",1,depth);sampler(program,"effect_texture",2,poison);
 sampler(program,"rt_texture",3,complete);sampler(program,"denoised_texture",7,bright);
 sampler(program,"gbuffer_now",10,old_g);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(8,0,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0]-reflected_expected)<0.001f&&fabsf(value[1]-reflected_expected)<0.001f,
  "eighth-resolution GI retains sample alignment, color and reflection");
 ray.trace_scale=2;u[6]=u[7]=u[13]=u[14]=4;u[15]=0;
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depth);
 glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,4,4,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,depths);

 /* A mature pixel with an isolated bright sample still needs filtering. */
 GLuint noisy = rgba_texture(7,(const float[4]){0.01,0.01,0.01,1});
 glTexSubImage2D(GL_TEXTURE_2D,0,3,3,1,1,GL_RGBA,GL_FLOAT,(const float[4]){4,4,4,1});
 GLuint floor_g = rgba_texture(7,(const float[4]){1,0,0,1});
 GLuint counts = rgba_texture(7,(const float[4]){32,0,0,0});
 GLuint filtered = rgba_texture(7,(const float[4]){0});
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,filtered,0);
 glViewport(0,0,7,7);
 program = link_with(vertex_source,denoise_source,"mature firefly regression");
 if (!program) return 1; use_ray_program(program); float grid[16]={0,0,7,7,4,0}; glUniform4fv(glGetUniformLocation(program,"u"),4,grid);
 sampler(program,"lights_texture",0,noisy); sampler(program,"results_texture",1,complete);
 sampler(program,"gbuffer_texture",2,floor_g); sampler(program,"counts_texture",3,counts);
 glDrawArrays(GL_TRIANGLES,0,3); glReadPixels(3,3,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(isfinite(value[0]) && value[0]<0.051f && value[0]>0.005f,"mature history rejects an isolated lighting firefly");

 /* Adjacent bright samples must not inflate the variance enough to
    protect one another. These are on the same dark plane, unlike the
    disconnected foreground surface tested below. Exercise all filter
    strides, including the mature-history shortcut. */
 float clustered[7*7*4];
 for(int extreme=0;extreme<=1;extreme++) {
 float cluster_brightness=extreme ? 4.0f : 0.25f;
 for(int cluster_size=2;cluster_size<=3;cluster_size++) {
  for(int i=0;i<49;i++) {
   clustered[i*4]=clustered[i*4+1]=clustered[i*4+2]=0.01f;
   clustered[i*4+3]=1;
  }
  const int cluster_x[3]={3,2,4};
  for(int i=0;i<cluster_size;i++) {
   int k=(3*7+cluster_x[i])*4;
   clustered[k]=clustered[k+1]=clustered[k+2]=cluster_brightness;
  }
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,noisy);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,7,7,GL_RGBA,GL_FLOAT,clustered);
  for(int stride=1;stride<=4;stride*=2) {
   grid[4]=(float)stride;
   glUniform4fv(glGetUniformLocation(program,"u"),4,grid);
   glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(3,3,1,1,GL_RGBA,GL_FLOAT,value);
   char label[96];
   snprintf(label,sizeof(label),"mature history rejects %d %s lighting outliers at stride %d",cluster_size,
    extreme ? "HDR clustered" : "moderate clustered",stride);
   failures += shader_check(isfinite(value[0])&&value[0]<0.051f&&value[0]>0.005f,label);
  }
 }
 }

 /* A coherent lit region and a hard lighting boundary on one plane
    have several bright neighbors. Outlier rejection must retain them,
    rather than treating all high-contrast illumination as noise. */
 for(int boundary=0;boundary<=1;boundary++) {
  for(int y=0;y<7;y++) for(int x=0;x<7;x++) {
   int k=(y*7+x)*4;
   int lit=boundary ? x>=3 : (x>=2&&x<=4&&y>=2&&y<=4);
   clustered[k]=clustered[k+1]=clustered[k+2]=lit ? 4.0f : 0.01f;
   clustered[k+3]=1;
  }
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,noisy);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,7,7,GL_RGBA,GL_FLOAT,clustered);
  grid[4]=1;glUniform4fv(glGetUniformLocation(program,"u"),4,grid);
  glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(3,3,1,1,GL_RGBA,GL_FLOAT,value);
  failures += shader_check(isfinite(value[0])&&value[0]>3.0f&&value[0]<=4.001f,
   boundary ? "denoiser retains the bright side of a high-contrast lighting boundary" : "denoiser retains a coherent bright lighting patch");
 }

 /* A sloped plane has rapidly changing depth, but zero distance from
    each neighbor to the center's tangent plane. It should still smooth
    modest Monte Carlo noise rather than preserve vertical stripes. */
 float plane_pixels[7*7*4], striped[7*7*4];
 const float inv_sqrt2=0.70710678118f;
 for(int y=0;y<7;y++) for(int x=0;x<7;x++) {
  int i=(y*7+x)*4;float ndc=(x+0.25f)/7*2-1;
  plane_pixels[i]=1/(1-ndc*0.5f);plane_pixels[i+1]=-inv_sqrt2;
  plane_pixels[i+2]=0;plane_pixels[i+3]=inv_sqrt2;
  striped[i]=striped[i+1]=striped[i+2]=(x&1)?0.15f:0.25f;striped[i+3]=1;
 }
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,noisy);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,7,7,GL_RGBA,GL_FLOAT,striped);
 glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,floor_g);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,7,7,GL_RGBA,GL_FLOAT,plane_pixels);
 grid[4]=1;grid[8]=0.5f;grid[9]=1;
 glUniform4fv(glGetUniformLocation(program,"u"),4,grid);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(3,3,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(isfinite(value[0])&&value[0]>0.175f&&value[0]<0.225f,"denoiser smooths coplanar sloped neighbors despite depth variation");

 /* A small bright foreground surface is not a firefly on the unrelated
    dark background. Both have the same normal to exercise plane rejection. */
 for(int i=0;i<49;i++) {
  plane_pixels[i*4]=4;plane_pixels[i*4+1]=plane_pixels[i*4+2]=0;plane_pixels[i*4+3]=1;
  striped[i*4]=striped[i*4+1]=striped[i*4+2]=0.01f;striped[i*4+3]=1;
 }
 plane_pixels[(3*7+3)*4]=1;
 striped[(3*7+3)*4]=striped[(3*7+3)*4+1]=striped[(3*7+3)*4+2]=2;
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,noisy);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,7,7,GL_RGBA,GL_FLOAT,striped);
 glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,floor_g);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,7,7,GL_RGBA,GL_FLOAT,plane_pixels);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(3,3,1,1,GL_RGBA,GL_FLOAT,value);
 failures += shader_check(fabsf(value[0]-2)<0.001f,"disconnected bright foreground is not clipped by dark background statistics");


 /* World tracing must establish its own sampler state, independently of
    the preceding game material or the screen-space fallback pass. */
 build_scene(0); initialize(); size_textures(4,4);
 GLuint stale_sampler; glGenSamplers(1,&stale_sampler);
 glSamplerParameteri(stale_sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
 glBindSampler(1,stale_sampler);
 glViewport(0,0,2,2); glDisable(GL_SCISSOR_TEST);
 const float eye[3]={0,0,0}, ahead[3]={0,1,0}, up[3]={0,0,1};
 GLuint world = world_rays(u,eye,ahead,up,4,4,depth);
 float shared_g[4]={0};
 int read = world && host_rt_debug_read(0,0,0,1,shared_g);
 failures += shader_check(read && fabsf(shared_g[0]-z)<0.01f,"world tracing clears inherited mipmapped depth sampler state");
 /* Use the public settings path: changing scale without changing the
    window must recreate lighting targets and reject old reprojection. */
 struct halo_ray_tracing_settings settings;halo_ray_tracing_get(&settings);
 int scale_change=1;
 const int scales[]={4,8,1,2};
 for(int change=0;change<4;change++) {
  settings.trace_scale=scales[change];
  ray.gi_previous=1;ray.previous_camera[12]=1;ray.gi_traced=1;
  halo_ray_tracing_set(&settings);
  scale_change &= ray.width==0&&ray.height==0&&!ray.gi_previous&&!ray.gi_traced&&ray.previous_camera[12]==0;
  size_textures(4,4);int grid=(4+TRACE_SCALE-1)/TRACE_SCALE;glViewport(0,0,grid,grid);
  world=world_rays(u,eye,ahead,up,4,4,depth);
  read=host_rt_debug_read(0,grid-1,grid-1,1,shared_g);
  scale_change &= world&&read&&ray.width==4&&ray.height==4&&fabsf(shared_g[0]-z)<0.01f;
 }
 failures += shader_check(scale_change,"full-half-quarter-eighth settings changes resize world targets and reject stale history");
 failures += shader_check(trace_scale_value(3)==2 && trace_scale_value(0)==2 && trace_scale_value(1)==1 && trace_scale_value(8)==8, "invalid RT scale falls back to Half while Full and Eighth are accepted");
 halo_ray_traced_projection(2,4);
 lighting_uniforms(u,0.1f,100,1.2f,(const int[4]){0,0,4,4},4,4);
 failures += shader_check(fabsf(u[2]-0.25f)<1e-6f && fabsf(u[3]-2)<1e-6f,"rays use the actual rasterizer projection scales");
 halo_ray_traced_projection(0,0);

 failures += object_sun_composite_regressions();
 failures += hybrid_base_regressions();
 failures += object_sun_phase_regression();
 failures += additive_highlight_regressions();
 failures += diffuse_capture_regression();

	GLenum error = glGetError();
	printf("GL error %#x\n", error);
	failures += error != GL_NO_ERROR;
	SDL_Quit();
	return failures ? 1 : 0;
}
