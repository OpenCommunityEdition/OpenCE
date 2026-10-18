/*
MENU_PC_ART.C (test)

port/linux/src/menu_pc_art.c as a whole: a PC cache (version 7) written here
with one bitmap tag, whose frame's pixels are in a bitmaps.map beside it, is
read as the player's install (paths.pc_data) and the frame decoded; caches
that are not one, and offsets that leave the files, give nothing.
*/

#include "harness.h"
#include <stdint.h>
#include <unistd.h>

static char install_folder[256];
static int exact_read_only;

char const *config_string(char const *name)
{
	return !strcmp(name, "paths.pc_data") ? install_folder : "";
}
char const *platform_data_root(void)
{
	return "/nonexistent";
}

#include "halo_menus.h"
#include "under_test.inc"

#define PNG "ce/shell/main_menu/multiplayer_type_select/mp_options__0.png"
#define TAG "ui\\shell\\main_menu\\multiplayer_type_select\\mp_options"
#define BASE 0x40440000UL

static void put32(uint8_t *at, uint32_t value)
{
	at[0] = (uint8_t)value; at[1] = (uint8_t)(value >> 8); at[2] = (uint8_t)(value >> 16); at[3] = (uint8_t)(value >> 24);
}
static void put16(uint8_t *at, uint32_t value)
{
	at[0] = (uint8_t)value; at[1] = (uint8_t)(value >> 8);
}

/* the install: version of the ui.map, where in bitmaps.map the pixels are claimed to be */
static void make_install(uint32_t version, uint32_t pixel_offset, uint32_t pixel_bytes)
{
	static uint8_t map[0x800 + 0x400];
	uint8_t *tags = map + 0x800;
	uint8_t *root = tags + 0x100, *frame = tags + 0x200;
	uint8_t bitmaps[64 + 16];
	char path[300];
	FILE *file;

	memset(map, 0, sizeof(map));
	memset(bitmaps, 0, sizeof(bitmaps));
	put32(map, 0x68656164); put32(map + 4, version); put32(map + 8, sizeof(map));
	put32(map + 16, 0x800); put32(map + 20, 0x400); put32(map + 0x7fc, 0x666f6f74);
	put32(tags, BASE + 40); put32(tags + 12, 1); put32(tags + 36, 0x74616773);
	put32(tags + 40, 0x6269746d);                  /* group */
	put32(tags + 40 + 16, BASE + 0x80);            /* name */
	put32(tags + 40 + 20, BASE + 0x100);           /* tag */
	strcpy((char *)tags + 0x80, TAG);
	put32(root + 0x60, 1); put32(root + 0x64, BASE + 0x200);
	put16(frame + 4, 4); put16(frame + 6, 4); put16(frame + 8, 1); put16(frame + 10, 0);
	put16(frame + 12, 15); put16(frame + 14, 0x183);
	put32(frame + 24, pixel_offset); put32(frame + 28, pixel_bytes);
	/* a DXT3 block: all alpha, red and blue endpoints, every texel the first */
	memset(bitmaps + 64, 0xff, 8);
	put16(bitmaps + 72, 0xf800); put16(bitmaps + 74, 0x001f); put32(bitmaps + 76, 0);
	snprintf(path, sizeof(path), "%s/ui.map", install_folder);
	file = fopen(path, "wb"); fwrite(map, 1, sizeof(map), file); fclose(file);
	snprintf(path, sizeof(path), "%s/bitmaps.map", install_folder);
	file = fopen(path, "wb"); fwrite(bitmaps, 1, sizeof(bitmaps), file); fclose(file);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	char folder[] = "/tmp/halo-pc-art-XXXXXX";
	long width = 0, height = 0;
	unsigned char *pixels;

	(void)exact_read_only;
	CHECK(mkdtemp(folder) != NULL, "no temporary folder");
	snprintf(install_folder, sizeof(install_folder), "%s", folder);

	CASE("decodes-a-frame")
	{
		make_install(7, 64, 16);
		pixels = halo_menus_pc_art(PNG, &width, &height);
		CHECK(pixels != NULL, "the frame was not found");
		CHECK(width == 4 && height == 4, "size %ldx%ld", width, height);
		CHECK(pixels[0] == 255 && pixels[1] == 0 && pixels[2] == 0 && pixels[3] == 255, "texel %d %d %d %d",
			pixels[0], pixels[1], pixels[2], pixels[3]);
	}
	CASE("other-pngs-have-none")
	{
		make_install(7, 64, 16);
		CHECK(halo_menus_pc_art("ce/shell/bitmaps/white__0.png", &width, &height) == NULL, "not a listed frame");
	}
	CASE("refuses-the-xbox-version")
	{
		make_install(5, 64, 16);
		CHECK(halo_menus_pc_art(PNG, &width, &height) == NULL, "version 5 was read");
	}
	CASE("refuses-pixels-outside-bitmaps-map")
	{
		make_install(7, 72, 16);
		CHECK(halo_menus_pc_art(PNG, &width, &height) == NULL, "pixels past the end were read");
		make_install(7, 0xfffffff0u, 16);
		CHECK(halo_menus_pc_art(PNG, &width, &height) == NULL, "a wrapping offset was read");
	}
	CASE("refuses-short-pixels")
	{
		make_install(7, 64, 8);
		CHECK(halo_menus_pc_art(PNG, &width, &height) == NULL, "8 bytes were taken for a 4x4 DXT3 frame");
	}
	return 0;
}
