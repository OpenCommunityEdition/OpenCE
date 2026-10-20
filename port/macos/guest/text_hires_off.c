/* text_hires_off.c: the high-res text, off until it draws through the
Metal renderer (text_hires.c makes GL textures). The game draws the maps'
own fonts, as with display.high_res_text off. */

#include "text_hires.h"

#include <stddef.h>

int text_hires_enabled(void)
{
	return 0;
}

long text_hires_font(char const *tag_name, float cap_height, short const *letter_widths, float oversample)
{
	(void)tag_name;
	(void)cap_height;
	(void)letter_widths;
	(void)oversample;
	return -1;
}

int text_hires_covers(long font, unsigned long code)
{
	(void)font;
	(void)code;
	return 0;
}

int text_hires_glyph(long font, unsigned long code, struct text_hires_glyph *glyph)
{
	(void)font;
	(void)code;
	(void)glyph;
	return 0;
}

void text_hires_register_atlas(const unsigned long *texture, unsigned long width, unsigned long height)
{
	(void)texture;
	(void)width;
	(void)height;
}

void text_hires_set_reset_hook(void (*hook)(void))
{
	(void)hook;
}
