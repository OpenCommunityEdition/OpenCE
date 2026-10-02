/* Numeric regressions exercise the game's actual Metal kernels on the GPU.
   Include the host implementation to inspect private resources without adding
   diagnostic entry points to the guest ABI. No ANGLE context is required. */
#include "../host/host_metal_rt.m"
#include <stdarg.h>
#include <stdio.h>

static int failures;
void host_logf(int priority, const char *format, ...)
{
	(void)priority;
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	puts("");
}
static void finish_noop(void) {}
static void check(int condition, const char *name)
{
	printf("%s: %s\n", condition ? "PASS" : "FAIL", name);
	failures += !condition;
}
static void read_texel(id<MTLTexture> texture, int x, int y, float *out)
{
	if (texture.storageMode == MTLStorageModePrivate)
	{
		id<MTLBuffer> buffer = [rt.device newBufferWithLength:256 options:MTLResourceStorageModeShared];
		id<MTLCommandBuffer> commands = [rt.queue commandBuffer];
		id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
		[blit copyFromTexture:texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(x,y,0)
			sourceSize:MTLSizeMake(1,1,1) toBuffer:buffer destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:256];
		[blit endEncoding]; [commands commit]; [commands waitUntilCompleted];
		if (texture.pixelFormat == MTLPixelFormatRG16Float)
		{
			const __fp16 *half = buffer.contents; out[0] = half[0]; out[1] = half[1]; out[2] = out[3] = 0;
		}
		else memcpy(out, buffer.contents, 16);
		return;
	}
	if (texture.pixelFormat == MTLPixelFormatRG16Float)
	{
		__fp16 half[2]; [texture getBytes:half bytesPerRow:4 fromRegion:MTLRegionMake2D(x,y,1,1) mipmapLevel:0];
		out[0]=half[0];out[1]=half[1];out[2]=out[3]=0;
	}
	else [texture getBytes:out bytesPerRow:16 fromRegion:MTLRegionMake2D(x,y,1,1) mipmapLevel:0];
}
static void read_pixel(id<MTLTexture> texture, float *out) { read_texel(texture,0,0,out); }
static void shared_history_metadata(int width, int height)
{
	MTLTextureDescriptor *d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRG16Float width:width height:height mipmapped:NO];
	d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;d.storageMode=MTLStorageModeShared;
	for(int i=0;i<2;i++) rt.history_metadata[i]=[rt.device newTextureWithDescriptor:d];
}
static void resize_inputs(int width)
{
	for (int i = 0; i < 3; i++)
	{
		MTLTextureDescriptor *d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:width height:width mipmapped:NO];
		d.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite; d.storageMode = MTLStorageModeShared;
		rt.textures[i] = [rt.device newTextureWithDescriptor:d]; rt.widths[i] = rt.heights[i] = width;
	}
}
static void input(float depth, float nx, float ny, float nz)
{
	float pixel[] = { depth, nx, ny, nz };
	[rt.textures[0] replaceRegion:MTLRegionMake2D(0, 0, 1, 1) mipmapLevel:0 withBytes:pixel bytesPerRow:16];
}
static void fixture(uint32_t generation, const float *vertices, int vertex_count, const uint32_t *indices,
	int triangle_count, int cutout_start)
{
	float *uv = calloc((size_t)vertex_count * 2, sizeof(float));
	uint32_t *materials = calloc((size_t)triangle_count, sizeof(uint32_t));
	const float material[] = { 0.5, 0.5, 0.5, cutout_start == 0 ? 16 : 0, 0, 0, 0, 0 };
	const unsigned char page[] = { 128, 128, 128, 255 };
	host_rt_set_world(generation, vertices, vertex_count, indices, triangle_count);
	host_rt_set_level(generation, vertices, uv, uv, vertex_count, indices, materials, triangle_count, cutout_start);
	host_rt_set_level_materials(material, 1);
	host_rt_set_level_page(0, 1, 1, page);
	free(uv);
	free(materials);
}
static int trace(float *camera)
{
	camera[44]++;
	return host_rt_trace(camera, 1, 1);
}
static void history_check(float *camera, float *surface, const char *name)
{
	/* Corrupt just the old surface to model a disocclusion at equal depth. */
	[rt.history_surfaces[rt.history_index] replaceRegion:MTLRegionMake2D(0, 0, 1, 1)
		mipmapLevel:0 withBytes:surface bytesPerRow:16];
	trace(camera);
	float value[4];
	read_pixel(rt.history[rt.history_index], value);
	check(value[3] == 1.0f, name);
}
static void primary_normal_slopes(void)
{
	const float floor[]={-10,-10,0,10,-10,0,0,10,0};
	const uint32_t indices[]={0,1,2};
	fixture(32,floor,3,indices,1,1);
	memcpy(rt.drawn_base_texcoords.contents,(const float[6]){0,0,1,0,.5,1},24);
	host_rt_set_surface_properties((const float[4]){0,0,0,1},1);
	float camera[96]={0},out[4],surface[4];
	camera[2]=1;camera[5]=-1;camera[7]=camera[9]=1;
	camera[12]=.01;camera[13]=100;camera[14]=.5;camera[15]=1;
	camera[18]=camera[19]=camera[23]=camera[35]=camera[45]=camera[46]=1;
	camera[24]=.8;camera[26]=.6;camera[36]=camera[37]=camera[38]=1;
	camera[42]=2;camera[61]=camera[63]=camera[92]=camera[93]=1;
	const float uv_scale[]={1,1};
	host_rt_set_normal(1,0,1,1,(const unsigned char[4]){230,128,204,255},uv_scale);
	input(1,0,0,-1);trace(camera);read_pixel(rt.textures[2],out);
	float bright=out[1];read_pixel(rt.history_surfaces[rt.history_index],surface);
	int geometric=surface[0]==0&&surface[1]==0&&surface[3]==1;
	host_rt_set_normal(2,0,1,1,(const unsigned char[4]){25,128,204,255},uv_scale);
	trace(camera);read_pixel(rt.textures[2],out);
	read_pixel(rt.history_surfaces[rt.history_index],surface);
	check(bright>.98f&&out[1]<.01f&&geometric&&surface[0]==0&&surface[1]==0&&surface[3]==1,
		"linear bump atlas opposite slopes change direct lighting while preserving geometric history normal");
}
static void reflection_hit_depth(void)
{
	/* A synthetic grazing receiver reflects toward a real BSP wall at
	   camera depth3, projected onto pixel(2,2). Only that wall's visible
	   colour is valid: a screen-space foreground or background must fail. */
	const float wall[]={-10,3,-10,10,3,-10,0,3,10};
	const uint32_t indices[]={0,1,2};
	fixture(36,wall,3,indices,1,1);resize_inputs(4);
	host_rt_set_objects(wall,(const unsigned char[1]){2},NULL,0,1);
	/* Exercise the geometry reflection path without diffuse GI or the
	   authored-material gate: the receiving point comes from the G-buffer. */
	id<MTLBuffer> saved_properties=rt.surface_data;
	rt.surface_data=nil;
	float camera[96]={0},out[4];
	camera[4]=camera[8]=camera[9]=1;
	camera[12]=.01;camera[13]=100;camera[14]=.5;camera[15]=1;
	camera[18]=camera[19]=4;camera[22]=40;camera[23]=camera[46]=camera[94]=1;
	camera[95]=.5;
	const float depths[]={3,3.03f,6,1,0,NAN};
	const char *names[]={
		"reflection retains the visible surface at the traced hit depth",
		"reflection tolerates small rasterized versus traced depth differences",
		"reflection rejects unrelated background behind the traced hit",
		"reflection rejects foreground covering the traced hit",
		"reflection rejects empty projected screen depth",
		"reflection rejects nonfinite projected screen depth"};
	for(int i=0;i<6;i++)
	{
		float pixels[64]={0};
		/* Receiver at(-.125,1,-.125), normal+X; its reflected hit is near
		   (.147,3,-.375), comfortably inside the wall and the viewport. */
		memcpy(pixels+(2*4+1)*4,(const float[4]){1,1,0,0},16);
		memcpy(pixels+(2*4+2)*4,(const float[4]){depths[i],0,0,-1},16);
		[rt.textures[0] replaceRegion:MTLRegionMake2D(0,0,4,4) mipmapLevel:0
			withBytes:pixels bytesPerRow:64];
		camera[44]++;host_rt_trace(camera,4,4);
		[rt.textures[1] getBytes:out bytesPerRow:16 fromRegion:MTLRegionMake2D(1,2,1,1) mipmapLevel:0];
		check(i<2?out[3]>.1f&&out[1]>.5f&&out[2]>.5f:out[3]==0,names[i]);
	}
	rt.surface_data=saved_properties;
	resize_inputs(1);
}
static void runtime_bounce_lights(void)
{
	/* Lamp is behind the camera. A roof blocks its direct route to the probe,
	   while the wall beyond the roof can receive and reflect its red light. */
	const float scene[] = {
		-10,1,-10,10,1,-10,0,1,10,
		-.3,-.6,.5,.3,-.6,.5,.3,0,.5,
		-.3,-.6,.5,.3,0,.5,-.3,0,.5 };
	const uint32_t indices[] = {0,1,2,3,4,5,6,7,8};
	fixture(31,scene,9,indices,3,3);
	host_rt_set_level_page(0,1,1,(const unsigned char[4]){0,0,0,255});
	host_rt_set_surface_properties((const float[4]){0,0,1,0},1);
	host_rt_set_surface(31,0,1,1,(const unsigned char[4]){200,100,40,255});
	float camera[96]={0},probe[10],point[]={0,0,0};
	camera[4]=camera[8]=camera[9]=1;
	camera[12]=.01;camera[13]=100;camera[14]=.5;camera[15]=1;
	camera[18]=camera[19]=camera[23]=camera[43]=camera[46]=1;
	camera[61]=camera[63]=camera[92]=camera[93]=1;camera[32]=1;
	float lamp[]={0,-.5,1,5,0,1,0,-2,0,1,.5,.25};
	/* A closed mesh surrounding the lamp shadows it at every wall hit, but
	   probes themselves trace the BSP, so this cannot hide the bounce wall. */
	const float box[8][3]={{-.1,-.6,.9},{.1,-.6,.9},{.1,-.4,.9},{-.1,-.4,.9},
		{-.1,-.6,1.1},{.1,-.6,1.1},{.1,-.4,1.1},{-.1,-.4,1.1}};
	const unsigned faces[36]={0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,
		3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5};
	float occluder[108];unsigned char groups[12];
	for(int t=0;t<12;t++) { groups[t]=2;for(int v=0;v<3;v++) memcpy(occluder+t*9+v*3,box[faces[t*3+v]],12); }
	for(int mode=0;mode<2;mode++)
	{
		camera[42]=mode?1:3;
		host_rt_set_objects(occluder,groups,NULL,0,1);host_rt_set_lights(lamp,1);
		host_rt_set_probes(point,1);input(0,0,0,-1);trace(camera);
		int ready=host_rt_probe_results(probe,1)==1;float red=probe[3];
		char name[160];snprintf(name,sizeof(name),"%s probe receives off-frustum lamp bounce tinted by brown wall texture",mode?"Hybrid":"Path");
		check(ready&&red>.001f&&isfinite(red)&&probe[3]>probe[4]*5&&probe[4]>probe[5]*5,name);
		host_rt_set_objects(occluder,groups,NULL,12,1);
		host_rt_set_probes(point,1);trace(camera);ready=host_rt_probe_results(probe,1)==1;
		snprintf(name,sizeof(name),"%s bounced lamp respects opaque model shadow occluders",mode?"Hybrid":"Path");
		check(ready&&probe[3]<1e-6f&&probe[4]<1e-6f&&probe[5]<1e-6f,name);
		host_rt_set_objects(occluder,groups,NULL,0,1);
		lamp[7]=.95f;lamp[5]=-1;host_rt_set_lights(lamp,1);
		host_rt_set_probes(point,1);trace(camera);host_rt_probe_results(probe,1);
		snprintf(name,sizeof(name),"%s spotlight aimed away from wall contributes no bounce",mode?"Hybrid":"Path");
		check(probe[3]<1e-6f&&probe[4]<1e-6f&&probe[5]<1e-6f,name);
		lamp[7]=-2;lamp[5]=1;host_rt_set_lights(lamp,1);camera[61]=0;
		host_rt_set_probes(point,1);trace(camera);host_rt_probe_results(probe,1);
		snprintf(name,sizeof(name),"%s bounced runtime lights obey indirect gain",mode?"Hybrid":"Path");
		check(probe[3]<1e-6f&&probe[4]<1e-6f&&probe[5]<1e-6f,name);
		camera[61]=1;
		lamp[3]=.2f;host_rt_set_lights(lamp,1);
		host_rt_set_probes(point,1);trace(camera);host_rt_probe_results(probe,1);
		snprintf(name,sizeof(name),"%s runtime lamp outside wall range contributes no bounce",mode?"Hybrid":"Path");
		check(probe[3]<1e-6f&&probe[4]<1e-6f&&probe[5]<1e-6f,name);
		lamp[3]=5;host_rt_set_lights(NULL,0);
		host_rt_set_emitters((const float[8]){0,-.5,1,5,1,.5,.25,1.5},1);
		host_rt_set_probes(point,1);trace(camera);ready=host_rt_probe_results(probe,1)==1;
		snprintf(name,sizeof(name),"%s attached colored emitter contributes textured indirect light",mode?"Hybrid":"Path");
		check(ready&&probe[3]>.001f&&probe[3]>probe[4]*5&&probe[4]>probe[5]*5,name);
		host_rt_set_emitters(NULL,0);
	}
	host_rt_set_lights(NULL,0);
}
static void oriented_bsp_emitters(void)
{
	float camera[96]={0},out[4],probe[10];
	camera[5]=-1;camera[7]=camera[9]=1;
	camera[12]=.01;camera[13]=100;camera[14]=.5;camera[15]=1;
	camera[18]=camera[19]=camera[23]=camera[46]=camera[47]=camera[61]=camera[63]=camera[92]=camera[93]=1;
	camera[42]=2;
	const uint32_t indices[]={0,1,2,3,4,5},materials[]={0,1};
	const float uv[12]={0},surface[]={.5,.5,.5,0,0,0,0,0, .5,.5,.5,0,1,.5,.2,0};
	for(int back=0;back<2;back++)
	{
		float z=back?2:0;
		const float scene[]={-10,-10,z,10,-10,z,0,10,z,
			-.4,-.4,1,0,.4,1,.4,-.4,1}; /* Lamp normal points down. */
		host_rt_set_world(34+back,scene,6,indices,2);
		host_rt_set_level(34+back,scene,uv,uv,6,indices,materials,2,2);
		host_rt_set_level_materials(surface,2);
		host_rt_set_level_page(0,1,1,(const unsigned char[4]){0,0,0,255});
		camera[0]=.125;camera[1]=-.125;camera[2]=z+.5;
		input(.5,0,0,-1);trace(camera);read_pixel(rt.textures[2],out);
		check(back?fabsf(out[1])+fabsf(out[2])+fabsf(out[3])<1e-6f:out[1]>.001f&&out[1]>out[2]&&out[2]>out[3],
			back?"opaque BSP emitter back produces no primary area light":"opaque BSP emitter front retains colored primary area light");
		host_rt_set_probes((const float[3]){0,0,z},1);trace(camera);
		int ready=host_rt_probe_results(probe,1)==1;
		check(ready&&(back?fabsf(probe[3])+fabsf(probe[4])+fabsf(probe[5])<1e-6f:probe[3]>.001f&&probe[3]>probe[4]&&probe[4]>probe[5]),
			back?"opaque BSP emitter back produces no model probe area light":"opaque BSP emitter front retains colored model probe area light");
	}
	host_rt_set_probes(NULL,0);
}
static void seed_planar_history(float value, float count)
{
	float light[16],surface[16]; __fp16 metadata[8];
	for(int i=0;i<4;i++)
	{
		memcpy(light+i*4,(const float[4]){value,value,value,count},16);
		memcpy(surface+i*4,(const float[4]){0,-1,1,1},16);
		metadata[i*2]=0;metadata[i*2+1]=1.0f/count;
	}
	[rt.history[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:light bytesPerRow:32];
	[rt.history_surfaces[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:surface bytesPerRow:32];
	[rt.history_metadata[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:metadata bytesPerRow:8];
}
static void cadence_history_changes(void)
{
	const float wall[]={-100,1,-100,100,1,-100,0,1,100};
	const uint32_t indices[]={0,1,2};
	fixture(33,wall,3,indices,1,1);resize_inputs(2);
	host_rt_set_objects(wall,(const unsigned char[1]){2},NULL,0,1);
	float gbuffer[16];for(int i=0;i<4;i++) memcpy(gbuffer+i*4,(const float[4]){1,0,0,-1},16);
	[rt.textures[0] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:gbuffer bytesPerRow:32];
	MTLTextureDescriptor *hd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:2 height:2 mipmapped:NO];
	hd.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;hd.storageMode=MTLStorageModeShared;
	for(int i=0;i<2;i++) { rt.history[i]=[rt.device newTextureWithDescriptor:hd];rt.history_surfaces[i]=[rt.device newTextureWithDescriptor:hd]; }
	shared_history_metadata(2,2);
	const int periods[]={1,4,16};
	for(int change=0;change<2;change++) for(int cadence=0;cadence<3;cadence++)
	{
		float camera[96]={0},out[4];int period=periods[cadence];
		camera[4]=camera[8]=camera[9]=1;
		camera[12]=.01;camera[13]=100;camera[14]=.5;camera[15]=1;
		camera[18]=camera[19]=2;camera[23]=camera[46]=camera[60]=camera[61]=1;
		camera[42]=3;camera[62]=.03;camera[63]=period;camera[92]=camera[93]=1;camera[95]=.25;
		float before=change?.1f:.4f,after=change?.4f:0;
		camera[39]=camera[40]=camera[41]=after;
		rt.history_valid=0;camera[44]++;host_rt_trace(camera,2,2);seed_planar_history(before,64);
		/* Exactly one fresh update occurs across each complete cadence.
		   The constant sky means path samples have zero Monte Carlo variance. */
		for(int frame=0;frame<period;frame++) { camera[44]++;host_rt_trace(camera,2,2); }
		read_pixel(rt.history[rt.history_index],out);
		float blend=1-powf(.97f,period),expected=before+(after-before)*blend;
		char name[144];snprintf(name,sizeof(name),"period%d %s preserves per-frame history response and statistical sample count",period,change?"light increase":"shadow decrease");
		check(out[3]==65&&fabsf(out[0]-expected)<.0001f&&fabsf(out[1]-out[0])+fabsf(out[2]-out[0])<.0001f,name);
		if(change&&period==16)
		{
			seed_planar_history(.1f,4);camera[44]++;host_rt_trace(camera,2,2);read_pixel(rt.history[rt.history_index],out);
			check(out[3]==5&&fabsf(out[0]-.16f)<.0001f,
				"sparse cadence bootstrap samples every frame and preserves one-over-N weighting");
		}
	}
	resize_inputs(1);
}
/* This exact screen trajectory missed 129 updates in the old hash schedule.
   Keep a world point in view while translating the camera one pixel/frame. */
static void moving_history_refresh(void)
{
	const int width=512,height=24,period=16,steps=96;
	const float wall[]={-100,1,-100,100,1,-100,0,1,100};
	const uint32_t indices[]={0,1,2};
	fixture(36,wall,3,indices,1,1);
	host_rt_set_objects(wall,(const unsigned char[1]){2},NULL,0,1);
	MTLTextureDescriptor *d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:width height:height mipmapped:NO];
	d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;d.storageMode=MTLStorageModeShared;
	for(int i=0;i<3;i++) { rt.textures[i]=[rt.device newTextureWithDescriptor:d];rt.widths[i]=width;rt.heights[i]=height; }
	for(int i=0;i<2;i++) { rt.history[i]=[rt.device newTextureWithDescriptor:d];rt.history_surfaces[i]=[rt.device newTextureWithDescriptor:d]; }
	shared_history_metadata(width,height);
	float *pixels=malloc((size_t)width*height*16);
	__fp16 *metadata=malloc((size_t)width*height*4);
	for(int i=0;i<width*height;i++) memcpy(pixels+i*4,(const float[4]){1,0,0,-1},16);
	[rt.textures[0] replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:pixels bytesPerRow:width*16];
	float camera[96]={0},out[4],confidence[4],meta[4];
	camera[4]=camera[8]=camera[9]=1;
	camera[12]=.01;camera[13]=100;camera[14]=.5;camera[15]=(float)width/height;
	camera[18]=width;camera[19]=height;camera[23]=camera[46]=camera[60]=camera[61]=1;
	camera[42]=3;camera[62]=.03;camera[63]=period;camera[92]=camera[93]=1;camera[95]=.5;
	camera[39]=camera[40]=camera[41]=.4;
	rt.history_valid=0;camera[44]=287;host_rt_trace(camera,width,height);
	for(int i=0;i<width*height;i++) memcpy(pixels+i*4,(const float[4]){.1,.1,.1,64},16);
	[rt.history[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:pixels bytesPerRow:width*16];
	for(int i=0;i<width*height;i++) memcpy(pixels+i*4,(const float[4]){0,-1,1,1},16);
	[rt.history_surfaces[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:pixels bytesPerRow:width*16];
	for(int i=0;i<width*height;i++) { metadata[i*2]=0;metadata[i*2+1]=1.0f/64; }
	[rt.history_metadata[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:metadata bytesPerRow:width*4];
	free(pixels);free(metadata);
	int bounded=1,stale=0,longest=0,updates=0;float previous_count=64;
	for(int frame=0;frame<steps;frame++)
	{
		camera[0]=-frame*(2*camera[14]*camera[15]/width);camera[44]=288+frame;
		host_rt_trace(camera,width,height);
		read_texel(rt.history[rt.history_index],288+frame,20,out);
		read_texel(rt.history_metadata[rt.history_index],288+frame,20,meta);
		if(out[3]>previous_count) { updates++;stale=0; } else stale++;
		longest=MAX(longest,stale);bounded&=out[3]>=previous_count&&meta[0]<period;
		previous_count=out[3];
	}
	check(bounded&&longest<period&&updates>=steps/period&&out[3]>=70,
		"moving mature plane refreshes within sixteen frames instead of starving for 129 frames");
	read_texel(rt.textures[2],288+steps-1,20,confidence);
	float blend=1-powf(.97f,period),squares=1.0f/64;
	for(int i=0;i<updates;i++) squares=(1-blend)*(1-blend)*squares+blend*blend;
	check(confidence[0]>4&&confidence[0]<4.5f&&fabsf(confidence[0]-1/squares)<.02f&&out[3]>=70,
		"sparse moving history exposes four effective samples while retaining lifetime sequence count");
	/* A reset must discard age and confidence along with radiance. */
	rt.history_valid=0;camera[44]++;host_rt_trace(camera,width,height);
	read_texel(rt.history[rt.history_index],288+steps-1,20,out);
	read_texel(rt.history_metadata[rt.history_index],288+steps-1,20,meta);
	read_texel(rt.textures[2],288+steps-1,20,confidence);
	check(out[3]==1&&meta[0]==0&&meta[1]==1&&confidence[0]==1,
		"history reset invalidates fresh-sample age and denoiser confidence");
	resize_inputs(1);
}
static void moving_history_centres(void)
{
	const float wall[] = { -100,1,-100, 100,1,-100, 0,1,100 };
	const uint32_t indices[] = { 0,1,2 };
	fixture(30,wall,3,indices,1,1);
	resize_inputs(2);
	host_rt_set_objects(wall,(const unsigned char[]){2},NULL,0,1);
	const float centres[] = { 0.25f,0.125f,0.0625f };
	for(int grid=0;grid<3;grid++)
	{
		float camera[96]={0},out[4];
		camera[4]=camera[8]=camera[9]=1;
		camera[12]=0.01;camera[13]=100;camera[14]=0.5;camera[15]=1;
		camera[18]=camera[19]=2;camera[23]=camera[46]=1;
		camera[42]=3;camera[60]=1;camera[62]=0.001;
		camera[63]=camera[92]=camera[93]=1;camera[95]=centres[grid];
		float gbuffer[16];
		for(int i=0;i<4;i++) memcpy(gbuffer+i*4,(const float[4]){1,0,0,-1},16);
		[rt.textures[0] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:gbuffer bytesPerRow:32];
		MTLTextureDescriptor *hd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:2 height:2 mipmapped:NO];
		hd.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;hd.storageMode=MTLStorageModeShared;
		for(int i=0;i<2;i++) { rt.history[i]=[rt.device newTextureWithDescriptor:hd];rt.history_surfaces[i]=[rt.device newTextureWithDescriptor:hd]; }
	shared_history_metadata(2,2);
		rt.history_valid=0;camera[44]++;host_rt_trace(camera,2,2);
		/* Distinct radiance on the same plane/primitive isolates coordinate
		   lookup from geometric rejection, sampling noise, and sun shadows. */
		float old_light[16],old_surface[16];
		for(int y=0;y<2;y++) for(int x=0;x<2;x++)
		{
			int at=(y*2+x)*4;float value=x?0.8f:0.2f;
			memcpy(old_light+at,(const float[4]){value,value,value,32},16);
			memcpy(old_surface+at,(const float[4]){0,-1,1,1},16);
		}
		[rt.history[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:old_light bytesPerRow:32];
		[rt.history_surfaces[rt.history_index] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:old_surface bytesPerRow:32];
		/* A0.3 world translation projects pixel0 to centre+0.6 in old grid
		   units. Its nearest sampled centre is index1 at all three scales,
		   although floor(ps) still returns index0. New irradiance is zero. */
		camera[0]=0.3f;camera[44]++;host_rt_trace(camera,2,2);read_pixel(rt.history[rt.history_index],out);
		char name[128];snprintf(name,sizeof(name),"scale%d moved planar history chooses nearest actual G-buffer sample centre",2<<grid);
		check(out[3]==33&&fabsf(out[0]-0.8f*32/33)<0.001f&&fabsf(out[1]-out[0])<1e-5f&&fabsf(out[2]-out[0])<1e-5f,name);
	}
	resize_inputs(1);
}
int main(void)
{
	@autoreleasepool
	{
		rt.device = MTLCreateSystemDefaultDevice();
		if (!rt.device.supportsRaytracing) { puts("Metal ray tracing unavailable"); return 1; }
		printf("GPU: %s\n", rt.device.name.UTF8String);
		rt.queue = [rt.device newCommandQueue];
		rt.available = 1;
		rt.glFinish = finish_noop;
		NSError *error = nil;
		id<MTLLibrary> library = [rt.device newLibraryWithSource:kernel_source options:nil error:&error];
		rt.pipeline = library ? [rt.device newComputePipelineStateWithFunction:[library newFunctionWithName:@"trace"] error:&error] : nil;
		rt.probe_pipeline = library ? [rt.device newComputePipelineStateWithFunction:[library newFunctionWithName:@"probes"] error:&error] : nil;
		if (!rt.pipeline || !rt.probe_pipeline) { puts(error.localizedDescription.UTF8String); return 1; }
		setenv("HALO_RT_EXPOSURE", "1", 1);
		setenv("HALO_RT_WHITE_BALANCE", "0", 1);
		for (int i = 0; i < 3; i++)
		{
			MTLTextureDescriptor *d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:1 height:1 mipmapped:NO];
			d.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
			d.storageMode = MTLStorageModeShared;
			rt.textures[i] = [rt.device newTextureWithDescriptor:d];
			rt.widths[i] = rt.heights[i] = 1;
		}
		float camera[96] = { 0 }, out[4];
		camera[0] = 0.25; camera[1] = -0.25; camera[2] = 1;
		camera[5] = -1; camera[7] = camera[9] = 1;
		camera[12] = 0.01; camera[13] = 100; camera[14] = 0.5; camera[15] = 1;
		camera[18] = camera[19] = 1; camera[21] = 2; camera[22] = 40; camera[23] = 1;
		camera[26] = camera[27] = camera[34] = camera[35] = 1;
		camera[36] = camera[37] = camera[38] = 1;
		camera[42] = camera[45] = camera[46] = camera[61] = camera[92] = camera[93] = 1;
		const float cutout[] = { -10, -10, 1, 10, -10, 1, 0, 10, 1 };
		const uint32_t indices[] = { 0, 2, 1 };
		/* A real receiving surface distinguishes transparent shadows from
		   an unmatched synthetic depth, which must retain the game's light. */
		const float receiver_and_cutout[] = {
			-10,-10,0, 10,-10,0, 0,10,0,
			-10,-10,1, 10,-10,1, 0,10,1 };
		const uint32_t receiver_indices[] = { 0,1,2, 3,5,4 }, receiver_materials[] = { 0,1 };
		const float receiver_uv[12] = { 0 }, receiving_materials[] = {
			0.5,0.5,0.5,0, 0,0,0,0,
			0.5,0.5,0.5,16, 0,0,0,0 };
		const unsigned char receiving_page[] = { 128,128,128,255 };
		host_rt_set_world(1, receiver_and_cutout, 6, receiver_indices, 2);
		host_rt_set_level(1, receiver_and_cutout, receiver_uv, receiver_uv, 6, receiver_indices, receiver_materials, 2, 1);
		host_rt_set_level_materials(receiving_materials, 2);
		host_rt_set_level_page(0, 1, 1, receiving_page);
		const unsigned char empty[] = { 0, 0, 0, 0 };
		host_rt_set_mask(1, 0, 2, 2, empty);
		const float light[] = { 0, 0, 2, 5, 0, 0, 1, -2, 0, 1, 1, 1 };
		host_rt_set_lights(light, 1);
		input(1, 0, 0, -1);
		check(trace(camera), "cutout trace completes");
		read_pixel(rt.textures[2], out);
		check(fabsf(out[1] - 1.54f) < 0.01f, "transparent cutout passes sunlight and point light");
		host_rt_set_lights(NULL, 0);
		camera[42] = camera[27] = camera[35] = 0;
		input(-1, 0, 0, -1);
		trace(camera); read_pixel(rt.textures[1], out);
		check(fabsf(out[0] - 1) < 1e-5f, "transparent cutout creates no occlusion");
		const unsigned char solid[] = { 255, 255, 255, 255 };
		host_rt_set_mask(1, 0, 2, 2, solid);
		trace(camera); read_pixel(rt.textures[1], out);
		check(out[0] < 0.9f, "solid cutout still occludes");
		host_rt_set_level(2, cutout, (const float[6]){0}, (const float[6]){0}, 3, indices, (const uint32_t[1]){0}, 1, 0);
		check(!rt.atlas && !rt.drawn_pages && !rt.drawn_materials && !rt.history_valid, "BSP reset invalidates pages, materials and history");
		check(host_rt_probe_results(out, -1) == 0 && host_rt_probe_results(NULL, 64) == 0, "probe readback rejects invalid bounds");

		/* A far wall exercises raw depth and history sample counts beyond 1024. */
		const float wall[] = { -10000, 2048, -10000, 10000, 2048, -10000, 0, 2048, 10000 };
		const uint32_t wall_indices[] = { 0, 1, 2 };
		fixture(3, wall, 3, wall_indices, 1, 1);
		camera[0] = camera[1] = camera[2] = 0;
		camera[3] = camera[5] = camera[6] = camera[7] = 0; camera[4] = camera[8] = 1;
		camera[13] = 4096; camera[42] = 1; camera[60] = 1;
		camera[63] = 1;
		camera[39] = camera[40] = camera[41] = 0.2;
		input(2048, 0, 0, -1);
		/* Shared storage only for diagnostic inspection. */
		MTLTextureDescriptor *hd = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:1 height:1 mipmapped:NO];
		hd.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite; hd.storageMode = MTLStorageModeShared;
		for (int i = 0; i < 2; i++) { rt.history[i] = [rt.device newTextureWithDescriptor:hd]; rt.history_surfaces[i] = [rt.device newTextureWithDescriptor:hd]; }
		for (int i = 0; i < 260; i++) trace(camera);
		read_pixel(rt.history[rt.history_index], out);
		check(out[3] == 255, "distant history retains 255 samples without packed-float overflow");
		float surface[4]; read_pixel(rt.history_surfaces[rt.history_index], surface);
		check(fabsf(surface[2] - 2048) < 0.01 && surface[3] == 1, "history stores raw depth and triangle identity");
		surface[3] = 2; history_check(camera, surface, "equal-depth different triangle rejects history");
		read_pixel(rt.history_surfaces[rt.history_index], surface); surface[0] = 0; surface[1] = 1;
		history_check(camera, surface, "equal-depth different normal rejects history");
		input(0, 0, 0, -1); trace(camera); input(2048, 0, 0, -1); trace(camera);
		read_pixel(rt.history[rt.history_index], out);
		check(out[3] == 1, "background invalidates history before geometry returns");
		moving_history_centres();
		cadence_history_changes();
		moving_history_refresh();
		oriented_bsp_emitters();
		runtime_bounce_lights();
		primary_normal_slopes();

		/* Sun is hidden at the probe, but illuminates the wall it sees. */
		const float bounce_scene[] = {
			-10,1,-10, 10,1,-10, 0,1,10,
			-0.3,-0.6,0.5, 0.3,-0.6,0.5, 0.3,0,0.5,
			-0.3,-0.6,0.5, 0.3,0,0.5, -0.3,0,0.5 };
		const uint32_t bounce_indices[] = { 0,1,2, 3,4,5, 6,7,8 };
		fixture(4, bounce_scene, 9, bounce_indices, 3, 3);
		camera[42] = 3; camera[39] = camera[40] = camera[41] = 0;
		camera[25] = -0.5; camera[26] = sqrtf(0.75); camera[35] = 1;
		const float point[] = { 0,0,0 };
		host_rt_set_probes(point, 1); input(1, 0, 0, -1); trace(camera);
		float probe[10];
		check(host_rt_probe_results(probe, 1) == 1 && probe[3] > 0.02f && isfinite(probe[3]), "path probe receives bounced sun from wall while direct sun is blocked");
		check(host_rt_probe_results(probe,1)==0,"completed probe batch is consumed instead of replayed as fresh lighting");
		float grey_bounce = probe[3];
		/* Identical geometry/material average, different hit UVs: the
		   receiving probe must see brown or blue bleeding from the texture. */
		const unsigned char brown_blue[] = { 200,100,40,255, 40,100,200,255 };
		host_rt_set_surface_properties((const float[4]){0,0,1,0},1);
		host_rt_set_surface(1,0,2,1,brown_blue);
		float *base_uv = rt.drawn_base_texcoords.contents;
		for(int i=0;i<9;i++) { base_uv[i*2]=0.25f;base_uv[i*2+1]=0.5f; }
		host_rt_set_probes(point,1);trace(camera);host_rt_probe_results(probe,1);
		float brown_red = probe[3];
		check(probe[3]>probe[4]*3&&probe[4]>probe[5]*3,
			"diffuse bounce carries sampled brown texture color to the receiver");
		for(int i=0;i<9;i++) base_uv[i*2]=0.75f;
		host_rt_set_probes(point,1);trace(camera);host_rt_probe_results(probe,1);
		check(probe[5]>probe[4]*3&&probe[4]>probe[3]*3,
			"same material with different hit UVs produces blue instead of brown bleeding");
		for(int i=0;i<9;i++) base_uv[i*2]=-0.75f;
		host_rt_set_probes(point,1);trace(camera);host_rt_probe_results(probe,1);
		check(fabsf(probe[3]-brown_red)<0.001f,"surface atlas repeats negative texture coordinates without tile bleeding");
		host_rt_set_surface(2,0,1,1,(const unsigned char[4]){128,128,128,255});
		host_rt_set_probes(point,1);trace(camera);host_rt_probe_results(probe,1);
		check(fabsf(probe[3]/grey_bounce-0.43172f)<0.01f,
			"sRGB diffuse texture decodes once to linear reflectance before bouncing");
		/* Opaque meshes keep their UVs in the all-object suffix, beyond the
		   alpha-only prefix. A second group makes the triangle offset nonzero. */
		const float textured_objects[] = {
			50,0.5,-10, 70,0.5,-10, 60,0.5,10,
			-10,0.5,-10, 10,0.5,-10, 0,0.5,10 };
		const unsigned char textured_groups[] = { (1<<3)|2, (5<<3)|2 };
		float object_attributes[] = {
			0.75,0.5, 0.75,0.5, 0.75,0.5, -1,0,
			0.25,0.5, 0.25,0.5, 0.25,0.5, -1,0 };
		const uint32_t object_color = 0x02808080u; /* Tile0; identical linear fallback. */
		memcpy(object_attributes+7,&object_color,4); memcpy(object_attributes+15,&object_color,4);
		host_rt_set_surface(3,0,2,1,brown_blue);
		camera[32]=1; camera[39]=camera[40]=camera[41]=0.2;
		host_rt_set_objects(textured_objects,textured_groups,object_attributes,2,1);
		trace(camera);read_pixel(rt.textures[1],out);
		check(out[0]>out[1]*3&&out[1]>out[2]*3,
			"opaque model samples brown UVs from its packed all-object attribute suffix");
		for(int i=0;i<3;i++) object_attributes[8+i*2]=0.75f;
		host_rt_set_objects(textured_objects,textured_groups,object_attributes,2,1);
		trace(camera);read_pixel(rt.textures[1],out);
		check(out[2]>out[1]*3&&out[1]>out[0]*3,
			"opaque model UV change selects blue with unchanged packed average albedo");
		host_rt_set_objects(textured_objects,textured_groups,object_attributes,0,1);
		camera[32]=0;camera[39]=camera[40]=camera[41]=0;

		/* Direct model sunlight is separate from ambient and point lights.
		   Deliberately wrong depth-derived normals require exact mesh recovery. */
		const float distant_level[] = { -10,1000,-10, 10,1000,-10, 0,1000,10 };
		fixture(6,distant_level,3,wall_indices,1,1);
		host_rt_set_surface_properties((const float[4]){0,0,1,0},1);
		host_rt_set_surface(4,0,2,1,brown_blue);
		float self_shadow_objects[18] = { -100,0,-100, 100,0,-100, 0,0,100 };
		const unsigned char self_shadow_groups[] = { (1<<3)|2, (2<<3)|2 };
		float self_shadow_attributes[] = {
			0.25,0.5, 0.25,0.5, 0.25,0.5, -1,0,
			0,0, 0,0, 0,0, -1,0 };
		memcpy(self_shadow_attributes+7,&object_color,4);
		const uint32_t blocker_color = 0x01808080u;
		memcpy(self_shadow_attributes+15,&blocker_color,4);
		memset(camera,0,sizeof(camera));
		camera[4]=camera[8]=camera[9]=1;
		camera[12]=0.01;camera[13]=2000;camera[14]=0.5;camera[15]=1;
		camera[18]=camera[19]=camera[23]=camera[35]=camera[46]=1;
		camera[21]=0.6;camera[22]=40;camera[24]=0.6;camera[25]=-0.8;
		camera[27]=1;camera[36]=1;camera[37]=0.8;camera[38]=0.6;camera[45]=0.7;
		camera[39]=0.2;camera[40]=0.3;camera[41]=0.4;
		camera[42]=3;camera[61]=camera[63]=camera[92]=camera[93]=1;
		const float model_depths[] = { 1,64 };
		const float model_centres[] = { 0.25f,0.125f,0.0625f };
		const char *model_grids[] = { "Half","Quarter","Eighth" };
		const float linear_brown[] = { 0.57758045f,0.12743768f,0.02121901f };
		int model_front=1,model_back=1,ambient_visibility=1,model_ao=1;
		for(int grid=0;grid<3;grid++)
		{
			camera[95]=model_centres[grid];
			for(int distance=0;distance<2;distance++)
			{
				float depth=model_depths[distance];camera[1]=-depth;
				host_rt_set_objects(self_shadow_objects,self_shadow_groups,self_shadow_attributes,1,1);
				input(-depth,1,0,0);trace(camera);read_pixel(rt.textures[2],out);
				model_front &= out[0]==-1;
				for(int channel=0;channel<3;channel++)
					model_front &= fabsf(out[channel+1]-linear_brown[channel]*0.8f*camera[36+channel]*camera[45])<0.005f;
				read_pixel(rt.textures[1],out);
				model_ao &= isfinite(out[0])&&out[0]>=0&&out[0]<=1;
				camera[24]=-0.6;camera[25]=0.8;
				trace(camera);read_pixel(rt.textures[2],out);
				model_back &= out[0]==-1&&fabsf(out[1])+fabsf(out[2])+fabsf(out[3])<1e-5f;
				camera[24]=0.6;camera[25]=-0.8;
				/* Move the narrow blocker with the physical grid sample. It
				   lies 0.04 along the sun ray, clear of the primary camera ray. */
				float px=-depth*(0.5f-camera[95]),pz=-px;
				float bias=0.005f+fmaxf(fabsf(px),fabsf(pz))*1e-6f;
				float bx=px+0.024f,by=-bias-0.032f;
				const float blocker[] = {
					bx-0.005f,by,pz-0.005f, bx+0.005f,by,pz-0.005f, bx,by,pz+0.005f };
				memcpy(self_shadow_objects+9,blocker,sizeof(blocker));
				host_rt_set_objects(self_shadow_objects,self_shadow_groups,self_shadow_attributes,2,1);
				trace(camera);read_pixel(rt.textures[2],out);
				char blocker_name[128];
				snprintf(blocker_name,sizeof(blocker_name),"%s %s model retains self-shadow blocker only 0.04 units away",
					model_grids[grid],distance==0?"near":"far");
				check(out[0]==-1&&fabsf(out[1])+fabsf(out[2])+fabsf(out[3])<1e-5f,blocker_name);
				read_pixel(rt.textures[1],out);float blocked_visibility=out[0];
				model_ao &= isfinite(out[0])&&out[0]>=0&&out[0]<=1;
				camera[27]=0;trace(camera);read_pixel(rt.textures[1],out);
				ambient_visibility &= fabsf(out[0]-blocked_visibility)<1e-5f;
				model_ao &= isfinite(out[0])&&out[0]>=0&&out[0]<=1;
				camera[27]=1;
			}
		}
		check(model_front,"Half Quarter Eighth matched models retain geometric normal and linear cosine-weighted sun");
		check(model_back,"Half Quarter Eighth model backs receive no direct sunlight while retaining ambient lighting");
		check(model_ao,"Half Quarter Eighth model AO remains finite and bounded with reduced ray counts");
		check(ambient_visibility,"traced model sun shadows do not multiply ambient occlusion visibility");
		/* Smooth normals are an attribute of the same packed triangle, not
		   another mesh. Interleaved groups and cutouts exercise repacking. */
		camera[1]=-1;camera[95]=0.5;camera[24]=0.6;camera[25]=-0.8;camera[27]=0;
		input(-1,1,0,0);
		const float smooth_objects[] = {
			50,0,-10, 70,0,-10, 60,0,10,
			-100,0,-100, 100,0,-100, 0,0,100,
			50,2,-10, 70,2,-10, 60,2,10 };
		const unsigned char smooth_groups[] = { (5<<3)|2, (1<<3)|2, (5<<3)|2 };
		float smooth_attributes[] = {
			0.75,0.5,0.75,0.5,0.75,0.5,-1,0,
			0.25,0.5,0.25,0.5,0.25,0.5,0,0,
			0.75,0.5,0.75,0.5,0.75,0.5,0,0 };
		for(int i=0;i<3;i++) memcpy(smooth_attributes+i*8+7,&object_color,4);
		float smooth_normals[] = {
			0,-1,0, 0,-1,0, 0,-1,0,
			1,0,0, 1,0,0, 0,-1,0,
			0,-1,0, 0,-1,0, 0,-1,0 };
		host_rt_set_objects(smooth_objects,smooth_groups,smooth_attributes,3,1);
		host_rt_set_object_normals(smooth_normals,3);
		trace(camera);read_pixel(rt.textures[2],out);
		float smooth_cosine=(0.6f+0.8f)*sqrtf(0.5f);int smooth_color=out[0]==-1;
		for(int channel=0;channel<3;channel++)
			smooth_color &= fabsf(out[channel+1]-linear_brown[channel]*smooth_cosine*camera[36+channel]*camera[45])<0.005f;
		check(smooth_color,"interleaved opaque/cutout model normals retain barycentric smooth cosine and textured colour");
		/* Change only normal attributes, leaving all positions and BLAS
		   unchanged. Attributes must still upload/repack every new pose. */
		for(int corner=0;corner<3;corner++) memcpy(smooth_normals+9+corner*3,(const float[3]){0,-1,0},12);
		host_rt_set_object_normals(smooth_normals,3);trace(camera);read_pixel(rt.textures[2],out);
		int normal_pose=out[0]==-1;
		for(int channel=0;channel<3;channel++)
			normal_pose &= fabsf(out[channel+1]-linear_brown[channel]*0.8f*camera[36+channel]*camera[45])<0.005f;
		check(normal_pose,"normals-only animation updates lighting while geometry positions stay unchanged");
		for(int corner=0;corner<3;corner++) memcpy(smooth_normals+9+corner*3,(const float[3]){1,-0.01f,0},12);
		host_rt_set_object_normals(smooth_normals,3);camera[24]=0.6;camera[25]=0.8;
		trace(camera);read_pixel(rt.textures[2],out);
		check(out[0]==-1&&fabsf(out[1])+fabsf(out[2])+fabsf(out[3])<1e-5f,
			"smooth model normals cannot light a geometric back face");
		camera[24]=0.6;camera[25]=-0.8;
		host_rt_set_object_normals(smooth_normals,2);trace(camera);read_pixel(rt.textures[2],out);
		check(rt.object_normal_count==0&&fabsf(out[1]-linear_brown[0]*0.8f*camera[36]*camera[45])<0.005f,
			"mismatched normal count rejects stale pose attributes and falls back to geometric shading");
		host_rt_set_object_normals(smooth_normals,3);
		host_rt_set_objects(smooth_objects,smooth_groups,smooth_attributes,3,1);trace(camera);read_pixel(rt.textures[2],out);
		check(rt.object_normal_count==0&&fabsf(out[1]-linear_brown[0]*0.8f*camera[36]*camera[45])<0.005f,
			"new mesh upload invalidates prior normals until the matching pose arrives");
		for(int corner=0;corner<3;corner++) memcpy(smooth_normals+9+corner*3,(const float[3]){NAN,0,0},12);
		host_rt_set_object_normals(smooth_normals,3);trace(camera);read_pixel(rt.textures[2],out);
		check(isfinite(out[1])&&fabsf(out[1]-linear_brown[0]*0.8f*camera[36]*camera[45])<0.005f,
			"non-finite authored model normals use geometric fallback in the actual hit shader");
		host_rt_set_objects(self_shadow_objects,self_shadow_groups,self_shadow_attributes,2,1);
		camera[27]=1;
		/* Probes sample only distant sky here; direct sun cannot enter the
		   irradiance that legacy model lighting redistributes into ambient. */
		input(0,0,0,-1);host_rt_set_probes(point,1);trace(camera);
		int sky_probe_ready=host_rt_probe_results(probe,1)==1;
		float sky_ambient[3]={probe[3],probe[4],probe[5]};
		camera[35]=0;host_rt_set_probes(point,1);trace(camera);
		sky_probe_ready &= host_rt_probe_results(probe,1)==1;
		check(sky_probe_ready&&sky_ambient[0]>0.1f&&sky_ambient[1]>0.1f&&sky_ambient[2]>0.1f&&
			fabsf(probe[3]-sky_ambient[0])+fabsf(probe[4]-sky_ambient[1])+fabsf(probe[5]-sky_ambient[2])<1e-5f,
			"nonzero sky ambient probe excludes primary direct sun without losing indirect light");
		host_rt_set_objects(self_shadow_objects,self_shadow_groups,self_shadow_attributes,0,1);

		/* The same sunlit ground must stay lit when the camera looks down.
		   A second floor one unit below it catches a wrong primary hit: both
		   face up, so lighting alone cannot distinguish the two surfaces. */
		const float floors[] = {
			-100,-100,0, 100,-100,0, 100,100,0, -100,100,0,
			-100,-100,-1, 100,-100,-1, 100,100,-1, -100,100,-1 };
		const uint32_t floor_indices[] = { 0,1,2, 0,2,3, 4,5,6, 4,6,7 };
		fixture(5, floors, 8, floor_indices, 4, 4);
		host_rt_set_probes(NULL, 0);
		memset(camera, 0, sizeof(camera));
		camera[2] = 2; camera[9] = 1;
		camera[12] = 0.01; camera[13] = 100; camera[14] = 0.5; camera[15] = 1;
		camera[18] = camera[19] = 1; camera[21] = 0.6; camera[22] = 40;
		camera[23] = camera[26] = camera[35] = camera[45] = camera[46] = 1;
		camera[36] = camera[37] = camera[38] = 1;
		camera[42] = 2; camera[61] = camera[63] = camera[92] = camera[93] = 1;
		const float pitches[] = { 30, 45, 60, 85 };
		int floor_lit = 1, nearest_floor = 1;
		for (unsigned int angle = 0; angle < sizeof(pitches) / sizeof(pitches[0]); angle++)
		{
			float pitch = pitches[angle] * 3.14159265f / 180.0f;
			camera[4] = cosf(pitch); camera[5] = -sinf(pitch);
			camera[7] = sinf(pitch); camera[8] = cosf(pitch);
			/* The one-pixel gbuffer samples window pixel zero, whose center
			   maps to (0.25,0.25) on the half-resolution tracing grid. */
			float depth = camera[2] / (sinf(pitch) - 0.25f * cosf(pitch));
			input(depth, 0, -cosf(pitch), -sinf(pitch));
			trace(camera);
			read_pixel(rt.textures[1], out);
			floor_lit &= out[0] > 1.5f;
			read_pixel(rt.textures[2], out);
			for (int channel = 1; channel < 4; channel++)
				floor_lit &= isfinite(out[channel]) && fabsf(out[channel] - 1.0f) < 0.01f;
			read_pixel(rt.history_surfaces[rt.history_index], surface);
			nearest_floor &= fabsf(surface[2] - depth) < 0.01f &&
				(surface[3] == 1.0f || surface[3] == 2.0f) &&
				fabsf(surface[0]) < 1e-5f && fabsf(surface[1]) < 1e-5f;
		}
		check(floor_lit, "sunlit floor stays equally lit at downward camera pitches");
		check(nearest_floor, "primary recovery selects nearest floor and its upward normal");
		/* A depth reconstructed below the true surface, beyond the match
		   tolerance, must leave the game's light intact instead of replacing
		   it with black from rays launched under the floor. */
		float valid_depth = camera[2] / (-camera[5] - 0.25f * camera[8]);
		input(valid_depth + 0.5f, 0, -camera[8], camera[5]);
		trace(camera); read_pixel(rt.textures[1], out);
		check(isfinite(out[0]) && out[0] <= 1.5f, "unmatched depth does not authoritatively replace level lighting");
		/* Quarter-resolution samples use pixel .5/4=.125 on the ray grid.
		   Reusing the old .25 would reject the exact floor at these pitches. */
		camera[95] = 0.125f;
		int quarter_lit = 1;
		for (unsigned int angle = 0; angle < sizeof(pitches) / sizeof(pitches[0]); angle++)
		{
			float pitch = pitches[angle] * 3.14159265f / 180.0f;
			camera[4] = cosf(pitch); camera[5] = -sinf(pitch);
			camera[7] = sinf(pitch); camera[8] = cosf(pitch);
			float depth = camera[2] / (sinf(pitch) - 0.375f * cosf(pitch));
			input(depth, 0, -cosf(pitch), -sinf(pitch)); trace(camera);
			read_pixel(rt.textures[1], out); quarter_lit &= out[0] > 1.5f;
			read_pixel(rt.textures[2], out);
			quarter_lit &= fabsf(out[1] - 1.0f) < 0.01f;
		}
		check(quarter_lit, "quarter-resolution pitched floors retain valid primary depth and sun");
		camera[95] = 0.0625f;
		int eighth_lit=1;
		for(unsigned int angle=0;angle<sizeof(pitches)/sizeof(pitches[0]);angle++) {
			float pitch=pitches[angle]*3.14159265f/180;
			camera[4]=cosf(pitch);camera[5]=-sinf(pitch);camera[7]=sinf(pitch);camera[8]=cosf(pitch);
			input(camera[2]/(sinf(pitch)-0.4375f*cosf(pitch)),0,-cosf(pitch),-sinf(pitch));
			trace(camera);read_pixel(rt.textures[1],out);eighth_lit &= out[0]>1.5f;
			read_pixel(rt.textures[2],out);eighth_lit &= fabsf(out[1]-1)<0.01f;
		}
		check(eighth_lit,"eighth-resolution pitched floors retain valid depth and sunlight");
		/* Count actual traced reflection segments; matte surfaces must skip
		   the ray itself, while authored shiny surfaces retain it. */
		camera[33]=camera[94]=1;
		host_rt_set_surface_properties((const float[4]){0,0,0,0},1);
		trace(camera);read_pixel(rt.textures[1],out);
		float segments[15*8];int segment_count=host_rt_probe(segments,15),matte=out[3]==0;
		for(int i=0;i<segment_count;i++) matte &= segments[i*8+3]!=2;
		check(matte,"authored matte sand does not trace a mirror reflection at grazing angles");
		host_rt_set_surface_properties((const float[4]){0.5,0.8,0,0},1);
		trace(camera);segment_count=host_rt_probe(segments,15);int shiny=0;
		for(int i=0;i<segment_count;i++) shiny |= segments[i*8+3]==2;
		check(shiny,"authored shiny surface retains its traced reflection ray");
		camera[42]=0;trace(camera);segment_count=host_rt_probe(segments,15);shiny=0;
		for(int i=0;i<segment_count;i++) shiny |= segments[i*8+3]==2;
		check(shiny,"material reflection gate works with diffuse GI disabled");
		camera[42]=2;camera[33]=camera[94]=0;
		/* Keep a physical 4x4 viewport, then change its lighting grid 2x2
		   to 1x1 and back. Host history allocation must reject the old grid. */
		camera[4] = camera[8] = 0; camera[5] = -1; camera[7] = 1; camera[60] = 1;
		int resize_history = 1;
		const int grid_sizes[] = { 2,1,2 };
		for (int change = 0; change < 3; change++)
		{
			int side = grid_sizes[change];resize_inputs(side);
			camera[18] = camera[19] = side;camera[95] = side == 1 ? 0.125f : 0.25f;
			float pixels[16];for(int i=0;i<side*side;i++) { pixels[i*4]=2;pixels[i*4+1]=pixels[i*4+2]=0;pixels[i*4+3]=-1; }
			[rt.textures[0] replaceRegion:MTLRegionMake2D(0,0,side,side) mipmapLevel:0 withBytes:pixels bytesPerRow:side*16];
			camera[44]++;host_rt_trace(camera,side,side);
			read_pixel(rt.history[rt.history_index], out);
			resize_history &= rt.history[rt.history_index].width == side && out[3] == 1.0f;
			read_pixel(rt.textures[1], out);resize_history &= out[0] > 1.5f;
			read_pixel(rt.textures[2], out);resize_history &= fabsf(out[1]-1.0f)<0.01f;
		}
		check(resize_history, "half-quarter grid changes resize and reject history without darkening floor");
		resize_inputs(1);
		/* Units have their own mesh, while scenery shares group31. A rock
		   beyond its first8192 triangles must still enter flashlight queries. */
		const int scenery_triangles = HOST_RT_GROUP_TRIANGLES + 1;
		const int crowded_count = scenery_triangles + 1;
		float *crowded = malloc((size_t)crowded_count * 9 * sizeof(float));
		unsigned char *crowded_groups = malloc((size_t)crowded_count);
		const float distant_triangle[] = { 50,-1,1, 52,-1,1, 51,1,1 };
		const float rock_blocker[] = { -10,-10,1, 10,-10,1, 0,10,1 };
		for (int i = 0; i < crowded_count; i++)
		{
			memcpy(crowded + i * 9, distant_triangle, sizeof(distant_triangle));
			crowded_groups[i] = (unsigned char)(((HOST_RT_GROUPS - 1) << 3) | 2);
		}
		crowded_groups[0] = (1 << 3) | 2; /* Preserve an actor's original group. */
		memset(camera, 0, sizeof(camera));
		camera[2] = 2; camera[5] = -1; camera[7] = camera[9] = 1;
		camera[12] = 0.01; camera[13] = 100; camera[14] = 0.5; camera[15] = 1;
		camera[18] = camera[19] = camera[23] = camera[34] = camera[46] = 1;
		camera[21] = 0.6; camera[22] = 40;
		const float flashlight[] = { 0,0,3,10, 0,0,-1,-2, 0,1,1,1 };
		host_rt_set_lights(flashlight, 1);
		host_rt_set_objects(crowded, crowded_groups, NULL, crowded_count, 1);
		input(2, 0, 0, -1); trace(camera); read_pixel(rt.textures[2], out);
		float unblocked_flashlight = out[1];
		check(unblocked_flashlight > 0.1f, "crowded scenery fixture receives unblocked flashlight");
		memcpy(crowded + (crowded_count - 1) * 9, rock_blocker, sizeof(rock_blocker));
		host_rt_set_objects(crowded, crowded_groups, NULL, crowded_count, 1);
		trace(camera); read_pixel(rt.textures[2], out);
		check(rt.body_counts[1][0] == 1 && rt.body_counts[2][0] == 1 &&
			rt.body_counts[HOST_RT_GROUPS - 1][0] == HOST_RT_GROUP_TRIANGLES,
			"scenery overflow uses an unused mesh without moving the actor group");
		check(isfinite(out[1]) && out[1] < unblocked_flashlight * 0.05f,
			"rock after8192 scenery triangles still shadows the flashlight");
		/* Motion must update TLAS bounds even when topology stays unchanged. */
		int moving_shadow = 1;
		for (int frame = 0; frame < 12; frame++)
		{
			float *rock = crowded + (crowded_count - 1) * 9;
			for (int i = 0; i < 9; i++)
				rock[i] = rock_blocker[i] + (i % 3 == 0 && (frame & 1) ? 0.05f : 0.0f);
			host_rt_set_objects(crowded, crowded_groups, NULL, crowded_count, 1);
			trace(camera); read_pixel(rt.textures[2], out);
			moving_shadow &= isfinite(out[1]) && out[1] < unblocked_flashlight * 0.05f;
		}
		check(moving_shadow && rt.body_age[2] > 0, "refitted moving model keeps its flashlight shadow");
		crowded_groups[crowded_count - 1] = (2 << 3) | 4;
		host_rt_set_objects(crowded, crowded_groups, NULL, crowded_count, 1);
		trace(camera); read_pixel(rt.textures[2], out);
		check(out[1] > unblocked_flashlight * 0.95f, "changed instance mask rebuilds scene visibility");
		free(crowded); free(crowded_groups);
		reflection_hit_depth();
		printf("%d failures\n", failures);
		return failures ? 1 : 0;
	}
}
