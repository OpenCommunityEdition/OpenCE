#include "harness.h"

#include <stdlib.h>

typedef unsigned long long Uint64;
typedef unsigned int GLuint;
struct texture_entry
{
	struct texture_entry *next;
	GLuint texture;
	struct { unsigned long width, height; } description;
	unsigned long address, generation, last_used_frame;
	GLuint hud_upscale;
};
#include "config.inc"
static struct texture_entry *texture_buckets[TEXTURE_BUCKET_COUNT];
static struct { struct texture_entry *entry; } recent_textures[4];
static Uint64 clock_ms;
static Uint64 SDL_GetTicks(void) { return clock_ms; }
static void glDeleteTextures(int count, const GLuint *texture) { }
static void xgpu_gl_state_invalidate(void) { }
/* the high-res HUD's settings never change here */
static unsigned long hud_hires_settings_changed(void) { return 0; }
static int hud_hires_bitmap_affected(unsigned long address, long width, long height, unsigned long changed)
{
	return 0;
}
#include "under_test.inc"

/* the texture drawn every frame, and the one drawn until a time */
enum { ALWAYS = 1, BUCKET_ALWAYS = 7, SOMETIMES = 2, BUCKET_SOMETIMES = 9 };

static void add(GLuint texture, int bucket)
{
	struct texture_entry *entry = calloc(1, sizeof(*entry));

	entry->texture = texture;
	entry->last_used_frame = texture_frame;
	entry->next = texture_buckets[bucket];
	texture_buckets[bucket] = entry;
}

static struct texture_entry *find(GLuint texture, int bucket)
{
	struct texture_entry *entry;

	for (entry = texture_buckets[bucket]; entry; entry = entry->next)
	{
		if (entry->texture == texture)
			return entry;
	}
	return NULL;
}

/* frames at a rate for a time (seconds), drawing one texture every frame and
the other until seen_until: the seconds it was dropped after it was last
drawn (-1: kept) */
static double play(int rate, double seen_until, double seconds)
{
	long frame;

	add(ALWAYS, BUCKET_ALWAYS);
	add(SOMETIMES, BUCKET_SOMETIMES);
	for (frame = 0; frame < (long)(seconds * rate); frame++)
	{
		struct texture_entry *sometimes;

		clock_ms = (Uint64)frame * 1000 / rate;
		xgpu_texture_cache_begin_frame();
		CHECK(find(ALWAYS, BUCKET_ALWAYS), "the texture drawn every frame was dropped at %.1f s", clock_ms / 1000.0);
		find(ALWAYS, BUCKET_ALWAYS)->last_used_frame = texture_frame;
		sometimes = find(SOMETIMES, BUCKET_SOMETIMES);
		if (!sometimes)
			return clock_ms / 1000.0 - seen_until;
		if (clock_ms < seen_until * 1000)
			sometimes->last_used_frame = texture_frame;
	}
	return -1.0;
}

static void check_idle(double idle)
{
	/* (a sweep every 10 s drops what was not drawn since the sweep 30 s
	before it: 30 to 40 s after) */
	CHECK(idle >= 30.0 && idle <= 41.0, "dropped %.1f s after it was last drawn, not 30 to 40", idle);
}

int main(int argc, char **argv)
{
	const char *case_name = argv[1];

	CASE("fast")
	{
		/* at 360 frames a second, a texture out of sight for five seconds
		(1800 frames) is kept */
		check_idle(play(360, 1.0, 60.0));
	}
	CASE("steady")
	{
		/* at 60 frames a second, as it was in frames: half a minute */
		check_idle(play(60, 1.0, 80.0));
	}
	CASE("late")
	{
		/* round the sweeps more than once */
		check_idle(play(360, 25.0, 80.0));
	}
	return 0;
}
