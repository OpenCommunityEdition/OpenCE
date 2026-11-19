/*
MENU_ART_AHEAD.C (test)

The menus' pictures decoded ahead (menu_files.c), on its own thread: a
picture asked for ahead is drawn from its texels without decoding its PNG
again; forgetting the art waits for a PNG being decoded; what is held stays
within MAXIMUM_AHEAD_BYTES, and is counted down as it is drawn; one drawn
while still queued is decoded once, there; one that does not decode is not
drawn. test_menu_art_ahead.py takes the code (under_test.inc) from the
sources.
*/

#include "harness.h"

#include <pthread.h>

#ifdef _WIN32
/* what the Windows build's POSIX layer (win32_posix.c) calls at startup */
int crash_reporter_process(void) { return 1; }
void mcc_memory_log_startup(void) { }
#endif

/* what the code calls */
#define MAXIMUM_ART 1024
#define platform_log(...) ((void)0)
#define xgpu_gl_ready() 1
#define xgpu_gl_texture_deleted(texture) ((void)0)
#define glDeleteTextures(count, textures) ((void)0)
static char *copy_of(const char *text)
{
	char *copy = malloc(strlen(text) + 1);

	return copy ? strcpy(copy, text) : NULL;
}
#define strdup copy_of

/* the pictures: "<width>x<height>" (a PNG's signature and header, enough for
its size), or "broken" (it does not decode) */
enum { PICTURES = 48 };
static unsigned char png_files[PICTURES][24];
static char png_names[PICTURES][32];

static const unsigned char *file_data(const char *name, unsigned long *size)
{
	long index;

	for (index = 0; index < PICTURES; index++)
	{
		if (!strcmp(png_names[index], name))
		{
			*size = sizeof(png_files[index]);
			return png_files[index];
		}
	}
	return NULL;
}

static unsigned long header_long(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] << 24 | (unsigned long)bytes[1] << 16 | (unsigned long)bytes[2] << 8 | bytes[3];
}

/* the decoder: how often each way, what is in flight (on the thread), and
a gate that holds the thread's decoding until opened */
static pthread_mutex_t test_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t test_changed = PTHREAD_COND_INITIALIZER;
static int gate_open = 1, in_flight, thread_decodes, drawn_decodes, held_texels;
static int decodes_of[PICTURES];

static long picture_of(const unsigned char *png)
{
	return (long)(((const unsigned char *)png - &png_files[0][0]) / sizeof(png_files[0]));
}

/* (texels: a block standing for them, tracked until let go) */
static void *texels[PICTURES * 2];

static unsigned char *texels_new(void)
{
	unsigned char *block = malloc(16);
	long index;

	pthread_mutex_lock(&test_lock);
	for (index = 0; texels[index]; index++)
		;
	texels[index] = block;
	held_texels++;
	pthread_mutex_unlock(&test_lock);
	return block;
}

static void test_free(void *pointer)
{
	long index;

	pthread_mutex_lock(&test_lock);
	for (index = 0; pointer && index < PICTURES * 2; index++)
	{
		if (texels[index] == pointer)
		{
			texels[index] = NULL;
			held_texels--;
		}
	}
	pthread_mutex_unlock(&test_lock);
	free(pointer);
}

static unsigned char *hud_hires_png_pixels(const void *png, unsigned long size, unsigned long *width,
	unsigned long *height)
{
	long picture = picture_of(png);
	unsigned char *texels;

	(void)size;
	pthread_mutex_lock(&test_lock);
	while (!gate_open)
		pthread_cond_wait(&test_changed, &test_lock);
	in_flight++;
	thread_decodes++;
	decodes_of[picture]++;
	pthread_mutex_unlock(&test_lock);
	{
		/* (a while: the time a decode takes, in which forgetting must wait) */
		volatile unsigned long spin;

		for (spin = 0; spin < 2000000; spin++)
			;
	}
	*width = header_long((const unsigned char *)png + 16);
	*height = header_long((const unsigned char *)png + 20);
	texels = strcmp(png_names[picture], "broken") ? texels_new() : NULL;
	pthread_mutex_lock(&test_lock);
	in_flight--;
	pthread_cond_broadcast(&test_changed);
	pthread_mutex_unlock(&test_lock);
	return texels;
}

static unsigned int next_texture = 1;

static unsigned int hud_hires_pixels_texture(unsigned char *pixels, unsigned long width, unsigned long height,
	unsigned long *levels)
{
	(void)width;
	(void)height;
	if (!pixels)
		return 0;
	test_free(pixels);
	*levels = 1;
	return next_texture++;
}

static unsigned int hud_hires_png_texture(const void *png, unsigned long size, unsigned long *levels)
{
	long picture = picture_of(png);

	(void)size;
	pthread_mutex_lock(&test_lock);
	drawn_decodes++;
	decodes_of[picture]++;
	pthread_mutex_unlock(&test_lock);
	if (!strcmp(png_names[picture], "broken"))
		return 0;
	*levels = 1;
	return next_texture++;
}

#define free test_free
#include "under_test.inc"
#undef free

static unsigned long art_textures[PICTURES][2];

static void picture_add(long index, const char *name, unsigned long width, unsigned long height)
{
	unsigned char *png = png_files[index];

	memset(png, 0, sizeof(png_files[index]));
	png[16] = (unsigned char)(width >> 24);
	png[17] = (unsigned char)(width >> 16);
	png[18] = (unsigned char)(width >> 8);
	png[19] = (unsigned char)width;
	png[20] = (unsigned char)(height >> 24);
	png[21] = (unsigned char)(height >> 16);
	png[22] = (unsigned char)(height >> 8);
	png[23] = (unsigned char)height;
	snprintf(png_names[index], sizeof(png_names[index]), "%s", name);
	art_textures[index][0] = 0;
	art_textures[index][1] = 0x1000UL + (unsigned long)index * 0x80UL;
	halo_menus_art_register(art_textures[index], png_names[index]);
}

/* square pictures of this side, named by their index */
static void pictures_add(long count, unsigned long side)
{
	long index;

	for (index = 0; index < count; index++)
	{
		char name[32];

		snprintf(name, sizeof(name), "%ld.png", index);
		picture_add(index, name, side, side);
	}
}

static unsigned int drawn(long index)
{
	unsigned long levels = 0;

	return menu_art_texture(art_textures[index][1], &levels);
}

/* the thread done with all it was given */
static void decoded_all(void)
{
	long index;

	for (;;)
	{
		int busy = 0;

		pthread_mutex_lock(&art_lock);
		for (index = 0; index < art_count; index++)
			busy |= art[index].ahead == _ahead_queued || art[index].ahead == _ahead_decoding;
		pthread_mutex_unlock(&art_lock);
		if (!busy)
			return;
		pthread_mutex_lock(&test_lock);
		pthread_cond_wait(&test_changed, &test_lock);
		pthread_mutex_unlock(&test_lock);
	}
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long index;

	/* each asked for ahead, then drawn: from its texels, decoded once, on the thread */
	CASE("ahead")
	{
		pictures_add(8, 256);
		for (index = 0; index < 8; index++)
			halo_menus_art_ahead(art_textures[index]);
		decoded_all();
		CHECK(thread_decodes == 8, "%d of 8 decoded ahead", thread_decodes);
		for (index = 0; index < 8; index++)
			CHECK(drawn(index), "picture %ld not drawn", index);
		CHECK(drawn_decodes == 0, "%d decoded again as drawn", drawn_decodes);
		CHECK(held_texels == 0 && ahead_bytes == 0, "%d texels, %lu bytes still held", held_texels, ahead_bytes);
		/* (asked again once drawn: nothing) */
		for (index = 0; index < 8; index++)
			halo_menus_art_ahead(art_textures[index]);
		decoded_all();
		CHECK(thread_decodes == 8, "a drawn picture decoded ahead again");
		return 0;
	}
	/* forgotten while decoding: the thread finished first, its texels let go */
	CASE("forgotten")
	{
		pictures_add(16, 256);
		for (index = 0; index < 16; index++)
			halo_menus_art_ahead(art_textures[index]);
		pthread_mutex_lock(&test_lock);
		while (!in_flight)
			pthread_cond_wait(&test_changed, &test_lock);
		pthread_mutex_unlock(&test_lock);
		halo_menus_art_forget();
		pthread_mutex_lock(&test_lock);
		CHECK(in_flight == 0, "forgotten with a decode in flight");
		pthread_mutex_unlock(&test_lock);
		CHECK(held_texels == 0 && ahead_bytes == 0, "%d texels, %lu bytes held after forgetting", held_texels,
			ahead_bytes);
		CHECK(thread_decodes < 16, "all were decoded: forgetting did not stop the thread");
		return 0;
	}
	/* held within the bytes allowed; drawing gives room back */
	CASE("bounded")
	{
		/* (each 16 MB: four allowed in 64 MB) */
		pictures_add(8, 2048);
		for (index = 0; index < 8; index++)
			halo_menus_art_ahead(art_textures[index]);
		decoded_all();
		CHECK(ahead_bytes <= MAXIMUM_AHEAD_BYTES, "%lu bytes held", ahead_bytes);
		CHECK(thread_decodes == 4, "%d decoded ahead, not 4", thread_decodes);
		for (index = 0; index < 8; index++)
			CHECK(drawn(index), "picture %ld not drawn", index);
		CHECK(drawn_decodes == 4, "%d decoded as drawn, not 4", drawn_decodes);
		CHECK(ahead_bytes == 0 && held_texels == 0, "%lu bytes still counted", ahead_bytes);
		return 0;
	}
	/* drawn while still queued: decoded there, once, and not by the thread */
	CASE("drawn-queued")
	{
		pictures_add(4, 256);
		pthread_mutex_lock(&test_lock);
		gate_open = 0;
		pthread_mutex_unlock(&test_lock);
		for (index = 0; index < 4; index++)
			halo_menus_art_ahead(art_textures[index]);
		/* (the thread holds the first, at the gate; the last is still queued) */
		CHECK(drawn(3), "the queued picture not drawn");
		pthread_mutex_lock(&test_lock);
		gate_open = 1;
		pthread_cond_broadcast(&test_changed);
		pthread_mutex_unlock(&test_lock);
		decoded_all();
		for (index = 0; index < 4; index++)
		{
			CHECK(drawn(index), "picture %ld not drawn", index);
			CHECK(decodes_of[index] == 1, "picture %ld decoded %d times", index, decodes_of[index]);
		}
		CHECK(ahead_bytes == 0 && held_texels == 0, "%lu bytes still counted", ahead_bytes);
		return 0;
	}
	/* one that does not decode: not drawn, and not tried again */
	CASE("broken")
	{
		picture_add(0, "broken", 64, 64);
		halo_menus_art_ahead(art_textures[0]);
		decoded_all();
		CHECK(!drawn(0), "a broken picture drawn");
		CHECK(!drawn(0), "a broken picture drawn the second time");
		CHECK(decodes_of[0] == 1, "decoded %d times", decodes_of[0]);
		CHECK(ahead_bytes == 0, "%lu bytes still counted", ahead_bytes);
		return 0;
	}
	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
