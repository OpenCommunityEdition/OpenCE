/*
VM_HIRES_STUBS.C

The high-res HUD (port/linux/src/hud_hires.c), the high-res text's atlas
(text_hires.c) and the menus' art (menu_files.c) as the device's tests (vm_textures.c, vm_draw.c) need them:
none, so that every texture is the game's own.
*/

#include "xgpu.h"
#include "hud_hires.h"
#include "menu_files.h"
#include "text_hires.h"

long hud_hires_override_find(unsigned long address, unsigned long width, unsigned long height,
	unsigned long level0_size)
{
	(void)address;
	(void)width;
	(void)height;
	(void)level0_size;
	return -1;
}

unsigned int hud_hires_override_texture(long asset, unsigned long *levels)
{
	(void)asset;
	(void)levels;
	return 0;
}

unsigned long hud_hires_settings_changed(void)
{
	return 0;
}

int hud_hires_bitmap_affected(unsigned long address, long width, long height, unsigned long changed)
{
	(void)address;
	(void)width;
	(void)height;
	(void)changed;
	return 0;
}

int hud_hires_override_coverage(long asset)
{
	(void)asset;
	return 0;
}

int hud_hires_override_point_threshold(long asset)
{
	(void)asset;
	return 0;
}

unsigned int hud_hires_placeholder_texture(unsigned long data, unsigned long *levels)
{
	(void)data;
	(void)levels;
	return 0;
}

long hud_hires_upscale_at(unsigned long address, long width, long height)
{
	(void)address;
	(void)width;
	(void)height;
	return -1;
}

unsigned int hud_hires_upscale_texture(const unsigned char *bgra, unsigned long width, unsigned long height,
	long kind, unsigned long *levels)
{
	(void)bgra;
	(void)width;
	(void)height;
	(void)kind;
	(void)levels;
	return 0;
}

unsigned int menu_art_texture(unsigned long data, unsigned long *levels)
{
	(void)data;
	(void)levels;
	return 0;
}


unsigned int text_hires_atlas_texture(unsigned long data)
{
	(void)data;
	return 0;
}
