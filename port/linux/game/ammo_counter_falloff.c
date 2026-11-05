/* A screen emits into its forward hemisphere. Keep this mask independent of
   the optional collision field and its admission budget. */
#include "cseries.h"
#include <xtl.h>
#include "rasterizer/rasterizer_swizzle.h"
#include "covenant_emitter_lighting.h"
#include "ammo_counter_falloff.h"
#include "plasma_occlusion.h"
#include "plasma_lighting.h"
#include <math.h>
#include <string.h>

enum { COUNTER_SIZE = 16, COUNTER_COUNT = 4096, COUNTER_LIGHTS = 8 };
static struct counter_texture
{
	long rasterizer;
	int used, mask, profile;
	float axis[3];
	float specular_scale;
	IDirect3DVolumeTexture8 *texture[3];
} counters[COUNTER_LIGHTS];

float ammo_counter_falloff_factor(const float axis[3], float x, float y, float z)
{
	float length = sqrtf(x*x + y*y + z*z);
	float t;
	if (length < 0.00001f)
		return 0.0f;
	t = PIN((axis[0]*x + axis[1]*y + axis[2]*z) / (length * 0.6f), 0.0f, 1.0f);
	return t*t*(3.0f - 2.0f*t);
}

/* Exposed needles have radial spill, including immediately below the source.
   Fade only farther down in world space to keep hands/housing lit without a
   floor pool. End the fade before the next lower volume-texture slice: its
   linear filtering otherwise carries spill down to crouched/short holders'
   feet, especially with the larger specular radius. Height is relative to
   the source, divided by the light radius. */
float needler_falloff_factor(float height)
{
	float t = PIN((-height - 0.06f) / 0.06f, 0.0f, 1.0f);
	return 1.0f - t*t*(3.0f - 2.0f*t);
}

/* Exposed prongs illuminate a broad forward lobe, with a soft transition
   just behind the emitter plane rather than the counter's focused mask. */
float plasma_prong_falloff_factor(const float axis[3], float x, float y, float z)
{
	float length = sqrtf(x*x + y*y + z*z);
	float t;
	if (length < 0.00001f) return 0.0f;
	t = PIN(((axis[0]*x + axis[1]*y + axis[2]*z) / length + 0.2f) / 0.7f, 0.0f, 1.0f);
	return t*t*(3.0f - 2.0f*t);
}

void *ammo_counter_falloff_texture(long rasterizer, int radial, float transform[4])
{
	float axis[3], origin[3], radius, scale = 1.0f;
	struct counter_texture *counter = NULL;
	D3DLOCKED_BOX box;
	unsigned long pixels[COUNTER_COUNT];
	int i, x, y, z;
	int profile = covenant_emitter_lighting_cone(rasterizer, axis, origin, &radius);
	if (!profile) profile = plasma_lighting_prong_cone(rasterizer, axis, origin, &radius);
	if (!profile || radius <= 0.0f)
		return NULL;
	if (transform)
	{
		for (i = 0; i < 3; i++) transform[i] = origin[i];
		transform[3] = 0.5f / radius;
	}
	for (i = 0; i < COUNTER_LIGHTS; i++)
		if (counters[i].used && counters[i].rasterizer == rasterizer)
			counter = &counters[i];
	if (!counter)
		for (i = 0; i < COUNTER_LIGHTS; i++)
			if (!counters[i].used)
			{
				counter = &counters[i];
				counter->used = TRUE;
				counter->rasterizer = rasterizer;
				break;
			}
	if (!counter)
		return NULL;
	/* Translation and radius change the transform, not the normalized mask. */
	if (counter->profile != profile || (profile != 2 && (counter->axis[0] != axis[0] ||
		counter->axis[1] != axis[1] || counter->axis[2] != axis[2])))
	{
		counter->mask = 0;
		counter->profile = profile;
		memcpy(counter->axis, axis, sizeof(axis));
	}
	/* Angular AR masks are scale-independent. The radial Needler's lower fade
	   needs a separate variant when specular receivers use another radius. */
	if (profile == 2 && radial == 2)
		scale = plasma_occlusion_specular_scale(rasterizer);
	radial = scale != 1.0f ? 2 : radial != 0;
	if (radial == 2 && counter->specular_scale != scale)
	{
		counter->mask &= ~(1 << 2);
		counter->specular_scale = scale;
	}
	if (counter->mask & (1 << radial))
		return counter->texture[radial];
	if (!counter->texture[radial] && D3DDevice_CreateVolumeTexture(COUNTER_SIZE, COUNTER_SIZE,
		COUNTER_SIZE, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &counter->texture[radial]) < 0)
		return NULL;
	if (IDirect3DVolumeTexture8_LockBox(counter->texture[radial], 0, &box, NULL, 0) < 0 || !box.pBits)
		return NULL;
	for (z = 0; z < COUNTER_SIZE; z++)
		for (y = 0; y < COUNTER_SIZE; y++)
			for (x = 0; x < COUNTER_SIZE; x++)
			{
				float dx = 2.0f*(x+0.5f)/COUNTER_SIZE - 1.0f;
				float dy = 2.0f*(y+0.5f)/COUNTER_SIZE - 1.0f;
				float dz = 2.0f*(z+0.5f)/COUNTER_SIZE - 1.0f;
				float value = profile == 3 ? plasma_prong_falloff_factor(axis, dx, dy, dz) : profile == 2 ? needler_falloff_factor(dz * scale) :
					ammo_counter_falloff_factor(axis, dx, dy, dz);
				float remaining = MAX(0.0f, 1.0f - dx*dx - dy*dy - dz*dz);
				if (radial) value *= remaining*remaining;
				pixels[(z*COUNTER_SIZE+y)*COUNTER_SIZE+x] = (unsigned long)(255.0f*value+0.5f)*0x01010101UL;
			}
	rasterizer_xbox_bitmap_swizzle3d_long(box.pBits, pixels, COUNTER_SIZE, COUNTER_SIZE, COUNTER_SIZE);
	if (IDirect3DVolumeTexture8_UnlockBox(counter->texture[radial], 0) < 0)
		return NULL;
	counter->mask |= 1 << radial;
	return counter->texture[radial];
}

void ammo_counter_falloff_window(void)
{
	int i;
	for (i = 0; i < COUNTER_LIGHTS; i++)
		counters[i].used = 0;
}

void ammo_counter_falloff_dispose(void)
{
	int i, j;
	for (i = 0; i < COUNTER_LIGHTS; i++)
		for (j = 0; j < 3; j++)
			if (counters[i].texture[j])
				D3DResource_Release((D3DResource *)counters[i].texture[j]);
	memset(counters, 0, sizeof(counters));
}
