#include "harness.h"
#include <math.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(a,b,c) MIN(MAX(a,b),c)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
typedef struct
{
	unsigned long pixels[4096];
} IDirect3DVolumeTexture8;
typedef IDirect3DVolumeTexture8 D3DResource;
typedef struct
{
	void *pBits;
} D3DLOCKED_BOX;
enum
{
	D3DFMT_A8R8G8B8,
	D3DPOOL_MANAGED,
	_collision_test_structure_bit,
	_collision_test_front_facing_surfaces_bit,
	_collision_test_back_facing_surfaces_bit,
	_collision_test_objects_bit,
	_collision_test_objects_scenery_bit,
	_collision_test_objects_machines_bit,
	_collision_test_objects_vehicles_bit
};
struct collision_result
{
	real t;
};
struct rasterizer_light_submit_parameters
{
	real_point3d position;
	real radius;
};
static struct
{
	long light_count;
	struct rasterizer_light_submit_parameters lights[32];
} rasterizer_lights;
static int on = 1, queries, allocations, blocked = 1, door, fail_upload;
static unsigned long time_ms = 1000;
static int uploads, fail_unlock;
static real specular_scale = 1;
static int muzzle_on, emitter_on, emitter_occlusion, shield_on, shield_occlusion, vehicle_on;
static long last_ignore;
static unsigned long last_flags;
static int directional_on, vehicle_only;
static int prong_on;
struct plasma_collision_scene { int unused; };
int plasma_lighting_prong_cone(long r,float axis[3],float origin[3],float *radius)
{ axis[0]=1;axis[1]=axis[2]=0;origin[0]=origin[1]=origin[2]=0;*radius=.3f;return prong_on?3:0; }
unsigned long plasma_collision_scene_revision(struct plasma_collision_scene *scene,const real_point3d *point,float extent,int vehicles)
{
 static int last=-1; static unsigned long revision;
 int state=blocked+2*door+4*vehicle_only;
 if(state!=last){last=state;revision++;}
 return revision;
}
static float directional_axis[3]={-.8f,0,.6f}, directional_origin, directional_radius=1;
int muzzle_lighting_vehicle_occlusion(long light) { return muzzle_on && light==500; }
int covenant_emitter_lighting_cone(long r,float axis[3],float origin[3],float *radius)
{
 int i;if(!directional_on||r<0||r>=8)return 0;
 for(i=0;i<3;i++){axis[i]=directional_axis[i];origin[i]=directional_origin;}
 *radius=directional_radius;return directional_on;
}
long covenant_emitter_lighting_occlusion_ignore(long i) { return emitter_on && i == 502 ? 42 : NONE; }
int covenant_emitter_lighting_enabled(void) { return emitter_on; }
int covenant_emitter_lighting_selected(long i) { return emitter_on && i == 0; }
int muzzle_lighting_selected(long i) { return muzzle_on && i == 0; }
int shield_glow_enabled(void) { return shield_on; }
int shield_glow_selected(long i) { return shield_on && i == 0; }
void shield_glow_frame_end(void) {}
void shield_glow_window_begin(void) {}
void shield_glow_submitted(long l,long r) {}
float shield_glow_radius(long l,float r) { return r; }
int lighting_config_boolean(const char *name)
{
	if (!strcmp(name, "display.plasma_occlusion") && shield_on) return shield_occlusion;
	if (!strcmp(name, "display.muzzle_lighting")) return muzzle_on;
	if (!strcmp(name, "display.vehicle_lighting")) return vehicle_on;
	if (!strcmp(name, "display.plasma_occlusion") && emitter_on) return emitter_occlusion;
	return on;
}
static unsigned long GetTickCount(void)
{
	return time_ms;
}
float plasma_occlusion_specular_scale(long index)
{
	return specular_scale;
}
static int collision_test_vector(unsigned long flags, const real_point3d *p, const real_vector3d *v,
								 long ignore, struct collision_result *hit)
{
	float t, y;
	queries++;
	last_ignore = ignore;
	last_flags = flags;
	if (vehicle_only && !(flags & FLAG(_collision_test_objects_vehicles_bit))) return 0;
	if (!blocked || v->i == 0)
		return 0;
	t = (0.3f - p->x) / v->i;
	y = p->y + t * v->j;
	if (t < 0 || t >= 1 || (door && fabsf(y) < 0.3f))
		return 0;
	hit->t = t;
	return 1;
}
static void plasma_collision_begin(unsigned long flags, const real_point3d *p, long ignore) {}
static void plasma_collision_end(void) {}
static float plasma_collision_depth(unsigned long flags, const real_point3d *p, const real_vector3d *v, long ignore)
{
	struct collision_result hit;
	return collision_test_vector(flags, p, v, ignore, &hit) ? hit.t : 2.0f;
}
enum { PLASMA_COLLISION_RAYS = 1536 };
static void plasma_collision_depths(unsigned long flags, const real_point3d *p,
 const real_vector3d *vectors, int count, long ignore, float *depths)
{ int i;for(i=0;i<count;i++)depths[i]=plasma_collision_depth(flags,p,&vectors[i],ignore); }
static int D3DDevice_CreateVolumeTexture(int w, int h, int d, int levels, int usage, int format, int pool,
										 IDirect3DVolumeTexture8 **t)
{
	if (fail_upload)
		return -1;
	*t = calloc(1, sizeof(**t));
	allocations++;
	return 0;
}
static int IDirect3DVolumeTexture8_LockBox(IDirect3DVolumeTexture8 *t, int l, D3DLOCKED_BOX *box, void *b,
										   int f)
{
	uploads++;
	box->pBits = t->pixels;
	return 0;
}
static int IDirect3DVolumeTexture8_UnlockBox(IDirect3DVolumeTexture8 *t, int l)
{
	return fail_unlock ? -1 : 0;
}
static void D3DResource_Release(D3DResource *p)
{
	free(p);
	allocations--;
}
static void rasterizer_xbox_bitmap_swizzle3d_long(void *d, void *s, int w, int h, int depth)
{
	memcpy(d, s, 4096 * sizeof(long));
}
void plasma_occlusion_window(void);
int plasma_lighting_vehicle_occlusion(long i) { return vehicle_on && i == 500; }
int authored_emitter_lighting_enabled(void) { return vehicle_on; }
int covenant_emitter_lighting_vehicle_occlusion(long light) { return vehicle_on && light == 502; }
#include "under_test.inc"
/* Match the renderer's linear filtering between vertical texel centres. */
static float filtered_height(IDirect3DVolumeTexture8 *texture, float height, float radius)
{
	float z = PIN((height / radius + 1.0f) * 8.0f - 0.5f, 0.0f, 15.0f);
	int lower = (int)z, upper = MIN(lower + 1, 15);
	float a = (float)(texture->pixels[(lower * 16 + 8) * 16 + 8] & 255) / 255.0f;
	float b = (float)(texture->pixels[(upper * 16 + 8) * 16 + 8] & 255) / 255.0f;
	return a + (b - a) * (z - lower);
}

int main(int argc, char **argv)
{
	struct occlusion_field *f;
	real previous;
	int n, cap, step;
	real temporal[4];
	float transform[4];
	rasterizer_lights.light_count = 1;
	rasterizer_lights.lights[0].radius = 1;
	plasma_occlusion_window();
	plasma_occlusion_submit(1, 0);
	f = field_for(0);
	CHECK(f, "admitted light");
	CHECK(sample_field(f->visibility, 0.9f, 0.5f, 0.5f) < 0.01f, "wall blocks clearly hidden receiver");
	CHECK(sample_field(f->visibility, 0.4f, 0.5f, 0.5f) > 0.99f, "near side stays lit");
	n = queries;
	time_ms++;
	CHECK(field_for(0) == f && queries == n, "one bounded update per window, not per model draw");
	n = queries;
	time_ms += 4;
	plasma_occlusion_window();
	plasma_occlusion_submit(1, 0);
	f = field_for(0);
	CHECK(queries == n, "collision depths are briefly reused while render projection updates");
	rasterizer_lights.lights[0].position.x = 0.2f;
	plasma_occlusion_window();
	plasma_occlusion_submit(1, 0);
	f = field_for(0);
	CHECK(queries == n + 1536, "fast movement invalidates cached depths immediately");
	rasterizer_lights.lights[0].position.x = 0;
	plasma_occlusion_window();
	plasma_occlusion_submit(1, 0);
	f = field_for(0);
	blocked = 0;
	previous = f->visibility[4095];
	time_ms += 16;
	plasma_occlusion_window();
	plasma_occlusion_submit(1, 0);
	f = field_for(0);
	CHECK(f->visibility[4095] > previous && f->visibility[4095] < 0.5f,
		  "clearing a corner fades rather than popping");
	time_ms += 200;
	plasma_occlusion_window();
	plasma_occlusion_submit(1, 0);
	f = field_for(0);
	CHECK(f->visibility[4095] > 0.9f, "clear view converges");
	on = 0;
	n = queries;
	CHECK(!field_for(0) && n == queries, "disabled makes no queries");
	plasma_occlusion_reset();
	CHECK(!allocations && !submitted_count, "map cleanup releases fields");
	on = 1;
	blocked = 1;
	door = 1;
	plasma_occlusion_window();
	plasma_occlusion_submit(2, 0);
	f = field_for(0);
	CHECK(sample_field(f->visibility, 0.9f, 0.5f, 0.5f) > 0.99f, "door opening admits light");
	CHECK(sample_field(f->visibility, 0.9f, 0.95f, 0.5f) < 0.35f,
		  "filtered doorway edge remains predominantly blocked");
	CHECK(sample_field(f->visibility, 0.9f, 0.99f, 0.5f) < 0.01f,
		  "receiver beyond filtered edge is fully blocked");
	door = 0;
	time_ms += 16;
	plasma_occlusion_window();
	plasma_occlusion_submit(2, 0);
	f = field_for(0);
	CHECK(sample_field(f->visibility, 0.9f, 0.5f, 0.5f) < 0.3f,
		  "closing door blocks within bounded refresh and fade");
	time_ms += 64;
	plasma_occlusion_window();
	plasma_occlusion_submit(2, 0);
	f = field_for(0);
	CHECK(sample_field(f->visibility, 0.9f, 0.9f, 0.5f) < 0.01f,
		  "thin blocker remains closed across cube face seam");
	door = 1;
	time_ms += 16;
	plasma_occlusion_window();
	plasma_occlusion_submit(2, 0);
	f = field_for(0);
	CHECK(sample_field(f->visibility, 0.9f, 0.5f, 0.5f) > 0.3f &&
			  sample_field(f->visibility, 0.9f, 0.5f, 0.5f) < 0.8f,
		  "opening door clears gradually");
	plasma_occlusion_reset();
	door = 0;
	rasterizer_lights.light_count = 9;
	for (n = 0; n < 9; n++)
	{
		rasterizer_lights.lights[n].radius = 1;
		plasma_occlusion_submit(10 + n, n);
	}
	queries = 0;
	for (n = 0; n < 8; n++)
		CHECK(field_for(n), "eight fields admitted");
	CHECK(!field_for(8) && queries == 8 * 1536, "ninth light falls back without evicting active fields");
	CHECK(allocations == 0, "visibility updates allocate no unused textures");
	for (n = 0; n < 8; n++)
		CHECK(plasma_occlusion_texture(n, 0), "diffuse texture requested");
	CHECK(allocations == 8 && uploads == 8, "only consumed passes allocate and upload");
	CHECK(plasma_occlusion_texture(0, 0) && uploads == 8, "repeated pass reuses upload");
	time_ms += 16;
	plasma_occlusion_window();
	for (n = 0; n < 9; n++)
		plasma_occlusion_submit(10 + n, n);
	queries = 0;
	CHECK(!field_for(8), "overflow cannot replace an active field");
	for (n = 0; n < 8; n++)
		CHECK(field_for(n), "rejected overflow does not consume an admitted light's update budget");
	CHECK(queries == 0, "unchanged stationary fields reuse exact collision inputs without retracing");

	time_ms += 120;
	plasma_occlusion_window();
	plasma_occlusion_submit(18, 8);
	f = field_for(8);
	CHECK(f && f->fading, "previous overflow is admitted with a fade");
	CHECK(field_texture(f, 0, 0) && f->texture[0]->pixels[4095] == 0xffffffffUL, "admission starts at previous unoccluded appearance");
	time_ms += 40;
	plasma_occlusion_window();
	plasma_occlusion_submit(18, 8);
	f = field_for(8);
	CHECK(field_texture(f, 0, 0) && f->texture[0]->pixels[4095] != 0xffffffffUL && f->texture[0]->pixels[4095] != 0,
		  "admission blends instead of snapping");
	plasma_occlusion_submit(10, 0);
	plasma_occlusion_model_slot(0, 0);
	CHECK(plasma_occlusion_model_texture(0, transform), "model uses selected field");
	CHECK(!plasma_occlusion_model_texture(-1, transform), "invalid model slot rejected");
	plasma_occlusion_window();
	CHECK(!plasma_occlusion_model_texture(0, transform), "view transition clears model slots");
	plasma_occlusion_reset();
	CHECK(!allocations, "all fields released");
	fail_upload = 1;
	plasma_occlusion_submit(30, 0);
	CHECK(!plasma_occlusion_texture(0, 0), "allocation failure falls back");
	fail_upload = 0;
	CHECK(!plasma_occlusion_texture(0, 0), "failed pass does not retry every draw in the same view");
	time_ms += 16;
	plasma_occlusion_window();
	plasma_occlusion_submit(30, 0);
	CHECK(plasma_occlusion_texture(0, 0), "failed upload retries on the next view");
	plasma_occlusion_reset();
	fail_upload = 0;
	time_ms = 1000;
	for (cap = 0; cap < 4; cap++)
	{
		blocked = 1;
		door = 0;
		plasma_occlusion_submit(40 + cap, 0);
		f = field_for(0);
		blocked = 0;
		for (step = 1; step <= (3 << cap); step++)
		{
			unsigned long next = 1000 + (unsigned long)(100.0f * step / (3 << cap));
			if (step == 1)
				time_ms = 1000;
			time_ms = next;
			plasma_occlusion_window();
			plasma_occlusion_submit(40 + cap, 0);
			f = field_for(0);
		}
		temporal[cap] = f->visibility[4095];
		plasma_occlusion_reset();
		time_ms = 1000;
	}
	CHECK(fabsf(temporal[0] - temporal[1]) < 0.025f && fabsf(temporal[1] - temporal[2]) < 0.025f && fabsf(temporal[2] - temporal[3]) < 0.025f,
		  "30/60/120/240 response agrees within the bounded collision refresh interval");
	blocked = 0;
	plasma_occlusion_submit(99, 0);
	f = field_for(0);
	time_ms += 4;
	rasterizer_lights.lights[0].position.x = 0.1f;
	plasma_occlusion_window();
	plasma_occlusion_submit(99, 0);
	f = field_for(0);
	CHECK(f->visibility[4095] > 0.99f, "a clear cached ray is not a wall at the old field boundary");
	plasma_occlusion_reset();
	/* Compare every output texel with the original attenuation expression,
	   including different specular extents and partially admitted lights. */
	for (cap = 0; cap < 3; cap++)
	{
		int pass, x, y, z;
		specular_scale = cap == 0 ? 0.5f : cap == 1 ? 1.0f : 2.0f;
		plasma_occlusion_submit(101 + cap, 0);
		f = field_for(0);
		for (n = 0; n < 4096; n++) f->visibility[n] = (n % 23) / 22.0f;
		f->admission = 0.37f;
		for (pass = 0; pass < 3; pass++)
		{
			IDirect3DVolumeTexture8 *t = field_texture(f, pass, 0);
			CHECK(t, "requested pass uploaded");
			if (pass == 2)
				CHECK((t == f->texture[1]) == (specular_scale == 1.0f),
					"only equal diffuse and specular extents share the same upload");
			for (z=0; z<16; z++) for (y=0; y<16; y++) for (x=0; x<16; x++)
			{
				real dx=2.0f*(x+0.5f)/16-1, dy=2.0f*(y+0.5f)/16-1, dz=2.0f*(z+0.5f)/16-1;
				real scale=(pass==2 ? f->specular : 1.0f)*f->radius/f->extent;
				real remaining=MAX(0.0f,1-dx*dx-dy*dy-dz*dz);
				real value=pass ? sample_field(f->visibility,0.5f+dx*scale*0.5f,0.5f+dy*scale*0.5f,0.5f+dz*scale*0.5f)*remaining*remaining : f->visibility[(z*16+y)*16+x];
				int expected, actual;
				value=pass ? (1-f->admission)*remaining*remaining+f->admission*value : 1-f->admission+f->admission*value;
				expected=(int)(255*value+0.5f); actual=(int)(t->pixels[(z*16+y)*16+x]&255);
				CHECK(abs(expected-actual)<=1,"lazy texture agrees with original within one quantization level");
			}
		}
		if (specular_scale == 1.0f)
		{
			n = uploads;
			CHECK(field_texture(f, 2, 0) == field_texture(f, 1, 0) && uploads == n,
				"shared specular pass does not rebuild or upload a duplicate texture");
		}
		n=uploads; time_ms+=16; plasma_occlusion_window(); plasma_occlusion_submit(101+cap,0);
		CHECK(plasma_occlusion_texture(0,0) && uploads==n+1,"new view invalidates each pass independently");
		plasma_occlusion_reset();
		CHECK(!allocations,"all lazy passes released");
	}
	/* Exact texture reuse is independent of visibility/history updates. */
	specular_scale=1;plasma_occlusion_submit(700,0);f=field_for(0);
	CHECK(field_texture(f,0,0),"initial model mask upload");n=uploads;
	f->texture_mask=0;
	CHECK(field_texture(f,0,0)&&uploads==n,"identical texels skip backing-memory and GPU work");
	f->visibility[0]=.37f;f->texture_mask=0;
	CHECK(field_texture(f,0,0)&&uploads==n+1,"changed texel still uploads");
	f->visibility[0]=.61f;f->texture_mask=0;fail_unlock=1;
	CHECK(!field_texture(f,0,0),"failed changed upload falls back");
	f->visibility[0]=.37f;f->texture_mask=0;fail_unlock=0;
	CHECK(field_texture(f,0,0)&&uploads==n+3,"failure invalidates byte cache even when old pixels return");
	for(n=0;n<OCC_COUNT;n++) f->visibility[n]=1;
	f->texture_mask=0;directional_on=1;
	CHECK((field_texture(f,0,0) && (f->texture[0]->pixels[(8*16+8)*16]&255)>200),
		"occluded counter mask lights its facing side");
	directional_axis[0]=.8f;
	CHECK((field_texture(f,0,0) && (f->texture[0]->pixels[(8*16+8)*16]&255)==0),
		"rotating a counter invalidates cached texels even with identical visibility");
	directional_axis[0]=-.8f;directional_on=0;
	plasma_occlusion_reset();CHECK(!allocations,"cached upload textures disposed");
	on=0; muzzle_on=1; time_ms+=200;
	plasma_occlusion_window(); plasma_occlusion_submit(500,0);
	CHECK(plasma_occlusion_texture(0,0), "muzzle occlusion works with projectile lighting off");
	plasma_occlusion_reset();
	plasma_occlusion_window(); plasma_occlusion_submit(501,1);
	CHECK(!plasma_occlusion_texture(1,0), "muzzle option cannot enable unrelated projectile occlusion");
	muzzle_on=0; plasma_occlusion_reset();
	emitter_on=1; emitter_occlusion=1; time_ms+=200;
	plasma_occlusion_window(); plasma_occlusion_submit(502,0);
	CHECK(plasma_occlusion_texture(0,0), "emitter occlusion works with projectile lighting off");
	CHECK(last_ignore==42, "luminous source is excluded from its own visibility rays");
	emitter_occlusion=0; plasma_occlusion_reset();
	plasma_occlusion_window(); plasma_occlusion_submit(503,0);
	CHECK(!plasma_occlusion_texture(0,0), "emitter occlusion remains independently optional");
	/* A counter remains directional with wall occlusion disabled or full. */
	directional_on=1;plasma_occlusion_reset();
	{
	 float axis[3]={-.8f,0,.6f},tr[4];
	 IDirect3DVolumeTexture8 *t;
	 CHECK(ammo_counter_falloff_factor(axis,-.8f,0,.6f)==1,"screen-facing strength preserved");
	 CHECK(ammo_counter_falloff_factor(axis,0,0,-1)==0,"floor behind display is dark");
	 CHECK(ammo_counter_falloff_factor(axis,.8f,0,-.6f)==0,"back hemisphere is dark");
	 CHECK(ammo_counter_falloff_factor(axis,0,0,1)==1,"face above screen retains brightness");
	 CHECK(needler_falloff_factor(0)==1,"needles spill equally in every horizontal direction");
	 CHECK(needler_falloff_factor(-.05f)==1,"nearby hands and housing below needles retain full spill");
	 CHECK(needler_falloff_factor(-.09f)>.49f&&needler_falloff_factor(-.09f)<.51f,"lower spill fades softly with distance");
	 CHECK(needler_falloff_factor(-.25f)==0,"distant floor spill suppressed");
	 CHECK(needler_falloff_factor(1)==1,"exposed tips retain full intensity");
	 plasma_occlusion_model_slot(0,0);t=plasma_occlusion_model_texture(0,tr);
	 CHECK(t&&tr[3]==.5f,"model directional mask independent of collision option");
	 CHECK((t->pixels[(0*16+8)*16+8]&255)==0,"model floor mask dark");
	 t=plasma_occlusion_texture(0,0);
	 CHECK(t&&(t->pixels[(0*16+8)*16+8]&255)==0,"world floor mask dark");
	 n=uploads;CHECK(plasma_occlusion_texture(0,1)==t&&uploads==n,"specular reuses angular radial mask");
	 CHECK(!plasma_occlusion_texture(20,0),"unrelated lights retain stock fallback");
	 plasma_occlusion_window();
	 CHECK(plasma_occlusion_texture(0,0)==t&&uploads==n,"unchanged view reuses directional upload");
	 directional_origin=3;directional_radius=2;
	 plasma_occlusion_window();
	 CHECK(ammo_counter_falloff_texture(0,1,tr)==t&&uploads==n&&tr[0]==3&&tr[3]==.25f,"translation and radius update transform without rebuilding mask");
	 directional_axis[0]=.8f;
	 plasma_occlusion_window();
	 CHECK(ammo_counter_falloff_texture(0,1,tr)==t&&uploads==n+1,"rotation refreshes radial mask");
	 CHECK(ammo_counter_falloff_texture(0,0,tr)&&uploads==n+2,"rotation refreshes angular mask independently");
	 directional_on=2;
	 plasma_occlusion_window();
	 CHECK(ammo_counter_falloff_texture(0,1,tr)&&uploads==n+3,"profile change refreshes mask");
	 directional_axis[0]=0;directional_axis[1]=1;directional_axis[2]=0;
	 plasma_occlusion_window();
	 CHECK(ammo_counter_falloff_texture(0,1,tr)&&uploads==n+3,"radial Needler spill does not rotate with aim");
	 specular_scale=2;
	 t=plasma_occlusion_texture(0,1);
	 CHECK(t&&(t->pixels[(7*16+8)*16+8]&255)==0,"unoccluded Needler specular uses physical lower fade");
	 t=plasma_occlusion_texture(0,0);
	 CHECK(t&&(t->pixels[(7*16+8)*16+8]&255)>0,"specular cache does not overwrite diffuse mask");
	 specular_scale=1;
	 CHECK(plasma_occlusion_texture(0,1)==t,"equal radii share diffuse and specular mask");
	 specular_scale=.5f;
	 t=plasma_occlusion_texture(0,1);
	 CHECK(t&&(t->pixels[(7*16+8)*16+8]&255)>128,"changed specular radius refreshes its mask");
	 specular_scale=1;
	 directional_axis[0]=-.8f;directional_axis[1]=0;directional_axis[2]=.6f;
	 directional_on=1;directional_origin=0;directional_radius=1;
	 plasma_occlusion_reset();CHECK(!allocations,"directional textures released on reset");
	}
	on=1;emitter_on=1;emitter_occlusion=1;blocked=0;
	plasma_occlusion_submit(502,0);
	{
	 IDirect3DVolumeTexture8 *t=plasma_occlusion_texture(0,0);
	 CHECK(t&&(t->pixels[(0*16+8)*16+8]&255)==0,"combined collision and direction mask stays dark behind screen");
	}
	/* Model visibility spans the largest specular radius. The Needler's lower
	   fade must still be measured against its actual diffuse radius. */
	plasma_occlusion_reset();directional_on=2;specular_scale=2;
	rasterizer_lights.lights[0].radius=1;
	plasma_occlusion_submit(502,0);f=field_for(0);
	{
	 IDirect3DVolumeTexture8 *t=field_texture(f,0,0);
	 CHECK(t&&(t->pixels[(7*16+8)*16+8]&255)==0,"model lower fade uses physical source radius");
	 t=field_texture(f,1,0);
	 CHECK(t&&(t->pixels[(7*16+8)*16+8]&255)>0,"nearby diffuse receiver retains lower glow");
	 t=field_texture(f,2,0);
	 CHECK(t&&(t->pixels[(7*16+8)*16+8]&255)==0,"specular lower fade uses physical source radius");
	}
	/* Captured crouched-player and short-Grunt source heights. Check filtered
	   output, not just the analytic mask: specular uses twice the volume radius. */
	for (cap = 0; cap < 2; cap++)
	{
		IDirect3DVolumeTexture8 *t;
		plasma_occlusion_reset();
		on = emitter_occlusion = cap;
		directional_on = 2;
		directional_radius = rasterizer_lights.lights[0].radius = 1.5f;
		specular_scale = 2;
		plasma_occlusion_submit(502, 0);
		t = plasma_occlusion_texture(0, 0);
		CHECK(t && filtered_height(t, -.38f, 1.5f) == 0, "crouched holder has no diffuse foot pool");
		CHECK(filtered_height(t, -.25f, 1.5f) < .2f, "short holder has only faint near-contact diffuse spill");
		CHECK(filtered_height(t, -.05f, 1.5f) > .9f, "diffuse spill immediately below needles stays bright");
		t = plasma_occlusion_texture(0, 1);
		CHECK(t && filtered_height(t, -.25f, 3.0f) == 0, "short holder has no specular foot pool");
		CHECK(filtered_height(t, -.38f, 3.0f) == 0, "crouched holder has no specular foot pool");
	}
	directional_radius = rasterizer_lights.lights[0].radius = 1;
	specular_scale=1;
	plasma_occlusion_reset();on=0;
	directional_on=0;emitter_on=0;muzzle_on=1;vehicle_only=blocked=1;
	plasma_occlusion_submit(500,0);f=field_for(0);
	CHECK(f&&(last_flags&FLAG(_collision_test_objects_vehicles_bit)),"turret rays include chassis");
	CHECK(f->visibility[4095]<.1f,"vehicle blocks receiver behind hull");
	plasma_occlusion_reset();plasma_occlusion_submit(501,0);f=field_for(0);
	CHECK(f&&!(last_flags&FLAG(_collision_test_objects_vehicles_bit)),"other muzzle profiles retain original blockers");
	CHECK(f->visibility[4095]>.99f,"vehicle-only blocker does not change unrelated lights");
	plasma_occlusion_reset();
	muzzle_on=0;vehicle_on=1;
	for(n=0;n<2;n++)
	{
		plasma_occlusion_reset();time_ms+=200;plasma_occlusion_window();
		plasma_occlusion_submit(n?502:500,0);f=field_for(0);
		CHECK(f&&(last_flags&FLAG(_collision_test_objects_vehicles_bit)),"headlights and engine emitters include chassis with general occlusion off");
		CHECK(f->visibility[4095]<.1f,"vehicle emitter cannot illuminate through its hull");
		plasma_occlusion_submit(501,1);CHECK(!field_for(1),"vehicle switch cannot select unrelated lights");
	}
	vehicle_on=0;n=queries;
	CHECK(!plasma_occlusion_texture(0,0)&&queries==n,"vehicle disable skips GPU texture and collision work");
	plasma_occlusion_reset();
	on = muzzle_on = emitter_on = directional_on = 0; shield_on = shield_occlusion = 1;
	plasma_occlusion_window(); plasma_occlusion_submit(600, 0);
	CHECK(field_for(0), "shield-only lighting receives occlusion");
	shield_occlusion = 0; n = queries;
	plasma_occlusion_window(); plasma_occlusion_submit(600, 0);
	CHECK(!field_for(0) && queries == n, "shield occlusion obeys its independent toggle");
	plasma_occlusion_reset();
	on=1;shield_on=shield_occlusion=emitter_on=directional_on=vehicle_only=0;
	prong_on=1;blocked=0;rasterizer_lights.lights[0].radius=.3f;specular_scale=2;
	queries=0;plasma_occlusion_submit(800,0);f=field_for(0);
	CHECK(f&&queries==96,"short prongs use the smaller ray grid only");
	{
		float axis[3]={1,0,0};
		IDirect3DVolumeTexture8 *t=field_texture(f,0,0);
		CHECK(plasma_prong_falloff_factor(axis,1,0,0)==1,"outward prong spill remains full");
		CHECK(plasma_prong_falloff_factor(axis,-1,0,0)==0,"prongs do not illuminate their rear hemisphere");
		CHECK(plasma_prong_falloff_factor(axis,0,1,0)>0&&plasma_prong_falloff_factor(axis,0,1,0)<.3f,"side spill fades broadly and softly");
		CHECK(t&&(t->pixels[(8*16+8)*16]&255)==0&&(t->pixels[(8*16+8)*16+15]&255)>240,"actual model texture carries the outward mask");
	}
	blocked=1;time_ms+=16;plasma_occlusion_window();plasma_occlusion_submit(800,0);f=field_for(0);
	CHECK(f&&queries==192&&f->visibility[4095]<.3f,"small prong grid still blocks a nearby wall");
	plasma_occlusion_reset();CHECK(!allocations,"prong reset releases GPU resources");
	/* A permanently blocked cell must settle to black, not retain subnormal
	   fade history forever at high frame rates. Reopening still fades normally. */
	prong_on=directional_on=emitter_on=muzzle_on=vehicle_on=shield_on=0;
	on=blocked=1;specular_scale=1;rasterizer_lights.lights[0].radius=1;
	for(cap=0;cap<4;cap++)
	{
		int rates[4]={30,60,120,240};
		plasma_occlusion_reset();
		blocked=0;time_ms+=100;plasma_occlusion_window();plasma_occlusion_submit(900,0);f=field_for(0);
		blocked=1;
		for(n=0;n<rates[cap]*3;n++)
		{
			time_ms+=1000/rates[cap];plasma_occlusion_window();plasma_occlusion_submit(900,0);
			f=field_for(0);
		}
		CHECK(f->visibility[4095]==0,"blocked history settles without denormal residue");
		blocked=0;time_ms+=16;plasma_occlusion_window();plasma_occlusion_submit(900,0);f=field_for(0);
		CHECK(f->visibility[4095]>0&&f->visibility[4095]<.5f,"settled cells reopen with the normal fade");
		blocked=1;
	}
	plasma_occlusion_reset();
	return 0;
}
