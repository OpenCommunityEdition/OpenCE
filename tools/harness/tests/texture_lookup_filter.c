/*
TEXTURE_LOOKUP_FILTER.C (test)

The menus' bitmaps (menu_files.c) and the high-res HUD's placeholders
(hud_hires.c), which the renderer asks after for each texture it binds: with
their filters, a texture is found exactly when it is registered, through
registrations, replacements and forgetting. test_texture_lookup_filter.py
takes the code (under_test.inc) from the sources.
*/

#include "harness.h"

/* what the code calls */
#define MAXIMUM_ART 1024
#define platform_log(...) ((void)0)
#define xgpu_gl_texture_deleted(texture) ((void)0)
#define glDeleteTextures(count, textures) ((void)0)
static unsigned int next_texture = 1;
static const unsigned char *file_data(const char *name, unsigned long *size)
{
	*size = 3;
	return (const unsigned char *)name;
}
#define hud_hires_png_texture(png, size, levels) (*(levels) = 1, next_texture++)
#define hud_hires_override_texture(asset, levels) (*(levels) = 1, (unsigned int)(asset) + 100)
#define render_thread_running() 0
static char *copy_of(const char *text)
{
	char *copy = malloc(strlen(text) + 1);

	return copy ? strcpy(copy, text) : NULL;
}
#define strdup copy_of

#include "under_test.inc"

/* the model: what is registered */
enum { ART = 160, ASSETS = 24, QUERIES = 4096 };
static unsigned long art_data[ART], placeholder_data[ASSETS];
static unsigned int art_texture[ART];
static unsigned long seed = 12345;

static unsigned long random_data(void)
{
	seed = seed * 1103515245UL + 12345UL;
	/* texel data: 128-byte aligned addresses */
	return (seed >> 1 & 0x3FFFFF80UL) + 0x80UL;
}

static void register_art(long slot, unsigned long data)
{
	unsigned long texture[2] = { 0, data };

	art_register(texture, "menu.png");
	art_data[slot] = data;
	art_texture[slot] = 0;
}

static void register_placeholder(long asset, unsigned long data)
{
	unsigned long texture[2] = { 0, data };

	placeholder_register(asset, data ? texture : NULL);
	placeholder_data[asset] = data;
}

/* every registered texture found (the same one each time), and nothing else */
static int same(unsigned long const *queries, long count)
{
	long query, slot;

	for (query = 0; query < count; query++)
	{
		unsigned long data = queries[query], levels = 0;
		unsigned int art = menu_art_texture(data, &levels), placeholder = hud_hires_placeholder_texture(data, &levels);
		unsigned int expected_placeholder = 0;

		for (slot = 0; slot < ART && art_data[slot] != data; slot++)
			;
		if (slot < ART && data)
		{
			if (!art || (art_texture[slot] && art != art_texture[slot]))
				return 0;
			art_texture[slot] = art;
		}
		else if (art)
		{
			return 0;
		}
		for (slot = 0; slot < ASSETS; slot++)
		{
			if (data && placeholder_data[slot] == data)
				expected_placeholder = (unsigned int)slot + 100;
		}
		if (placeholder != expected_placeholder)
			return 0;
	}
	return 1;
}

/* the registered textures, then as many others */
static long queries_of(unsigned long *queries)
{
	long count = 0, slot;

	for (slot = 0; slot < ART; slot++)
		queries[count++] = art_data[slot];
	for (slot = 0; slot < ASSETS; slot++)
		queries[count++] = placeholder_data[slot];
	while (count < QUERIES)
		queries[count++] = random_data();
	return count;
}

static void register_all(void)
{
	long slot;

	for (slot = 0; slot < ART; slot++)
		register_art(slot, random_data());
	for (slot = 0; slot < ASSETS; slot++)
		register_placeholder(slot, random_data());
}

static double seconds_of_lookups(unsigned long const *queries, long count)
{
	clock_t start = clock();
	unsigned long sum = 0, levels;
	long round, query;

	for (round = 0; round < 2000; round++)
	{
		for (query = 0; query < count; query++)
			sum += menu_art_texture(queries[query], &levels) + hud_hires_placeholder_texture(queries[query], &levels);
	}
	if (sum == 1)
		printf("-");
	return (double)(clock() - start) / CLOCKS_PER_SEC;
}

int main(int argc, char **argv)
{
	static unsigned long queries[QUERIES];
	const char *case_name = argc > 1 ? argv[1] : "";
	long count;

	CASE("found")
	{
		register_all();
		count = queries_of(queries);
		CHECK(same(queries, count), "a texture was found that is not registered, or one registered was not");
		return 0;
	}
	/* a texture's slot taken by another, a placeholder moved or forgotten */
	CASE("replaced")
	{
		long slot;

		register_all();
		for (slot = 0; slot < ASSETS; slot += 2)
			register_placeholder(slot, slot % 4 ? random_data() : 0);
		for (slot = 0; slot < ART; slot += 3)
		{
			unsigned long texture[2] = { 0, art_data[slot] };

			art_register(texture, "another.png");
			art_texture[slot] = 0;
		}
		count = queries_of(queries);
		CHECK(same(queries, count), "the textures found differ after replacements");
		return 0;
	}
	/* the menus' bitmaps forgotten (a new map), then others registered */
	CASE("forgotten")
	{
		long slot;

		register_all();
		count = queries_of(queries);
		art_forget();
		memset(art_data, 0, sizeof(art_data));
		CHECK(same(queries, count), "a forgotten bitmap was still found");
		for (slot = 0; slot < ART / 2; slot++)
			register_art(slot, random_data());
		count = queries_of(queries);
		CHECK(same(queries, count), "the textures found differ after forgetting");
		return 0;
	}
	/* not a check: how long the renderer's questions take */
	CASE("benchmark")
	{
		register_all();
		/* (as in a game: almost every texture bound is neither) */
		count = 0;
		while (count < QUERIES)
			queries[count++] = random_data();
		printf("%.1f ns a texture\n", 1e9 * seconds_of_lookups(queries, count) / (2000.0 * count));
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
