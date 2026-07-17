/*
TEXTURE_PAGE.C

symbols in this file:
0010F530 0040:
	_texture_page_fraction_used (0000)
0010F570 0060:
	_code_0010f570 (0000)
0010F5D0 00d0:
	_texture_page_new (0000)
0010F6A0 0030:
	_texture_page_delete (0000)
0010F6D0 0040:
	_texture_page_textures_begin (0000)
0010F710 0020:
	_texture_page_texture_get (0000)
0010F730 0060:
	_code_0010f730 (0000)
0010F790 02e0:
	_code_0010f790 (0000)
0010FA70 00f0:
	_texture_page_texture_new (0000)
0010FB60 00c0:
	_texture_page_textures_cancel (0000)
0010FC20 0070:
	_texture_page_textures_end (0000)
0010FC90 0030:
	_texture_page_texture_delete (0000)
0010FCC0 0060:
	_texture_page_resize (0000)
0027EAA0 0030:
	??_C@_0DA@DKBKMAEG@texture_page?9?$DOwidth?$DO0?5?$CG?$CG?5texture@ (0000)
0027EAD0 000d:
	??_C@_0N@NODJNJNK@texture_page?$AA@ (0000)
0027EAE0 0025:
	??_C@_0CF@GEHOPDBO@c?3?2halo?2SOURCE?2memory?2texture_pa@ (0000)
0027EB08 0016:
	??_C@_0BG@FAEHOIGE@texture?5page?5textures?$AA@ (0000)
0027EB20 001e:
	??_C@_0BO@PCBMOGEN@page_width?$DO0?5?$CG?$CG?5page_height?$DO0?$AA@ (0000)
0027EB40 002a:
	??_C@_0CK@JEDIHMHK@?$CBtexture_page?9?$DOcontains_unsorted@ (0000)
0027EB6C 0027:
	??_C@_0CH@KHJKPFFE@channel_count?$DMMAXIMUM_TEXTURE_CH@ (0000)
0027EB98 0059:
	??_C@_0FJ@FLDCNMDK@texture_page?9?$DOspacing?$DN?$DN0?5?$HM?$HM?5ceil@ (0000)
0027EBF4 0036:
	??_C@_0DG@MMDMAONB@immediate?5?$HM?$HM?5texture_page?9?$DOconta@ (0000)
0027EC2C 0011:
	??_C@_0BB@KPBIDJAC@resort_succeeded?$AA@ (0000)
0027EC40 0029:
	??_C@_0CJ@JJIPICBJ@texture_page?9?$DOcontains_unsorted_@ (0000)
00456628 0004:
	_bss_00456628 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "data.h"
#include "texture_page.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

static void code_0010f570(
	struct texture_page *texture_page);
boolean code_0010f790(
	struct texture_page *texture_page);

/* ---------- globals */

struct texture_page *bss_00456628 = NULL;

/* ---------- public code */

float texture_page_fraction_used(
	struct texture_page *texture_page,
	boolean use_sorted_pixel_count)
{
	long pixel_count = use_sorted_pixel_count ? texture_page->used_pixel_count : texture_page->texture_pixel_count;

	return (float)pixel_count/(texture_page->width*texture_page->height);
}

struct texture_page *texture_page_new(
	void *user_data,
	short page_width,
	short page_height,
	short spacing)
{
	struct texture_page *texture_page = match_malloc(
		"c:\\halo\\SOURCE\\memory\\texture_page.c",
		29,
		sizeof(*texture_page));

	match_assert("c:\\halo\\SOURCE\\memory\\texture_page.c", 31, page_width>0 && page_height>0);
	if (texture_page)
	{
		csmemset(texture_page, 0, sizeof(*texture_page));
		texture_page->width = page_width;
		texture_page->height = page_height;
		texture_page->spacing = spacing;
		texture_page->user_data = user_data;
		texture_page->texture_pixel_count = 0;
		texture_page->used_pixel_count = 0;
		texture_page->contains_unsorted_textures = FALSE;
		texture_page->textures = data_new(
			"texture page textures",
			MAXIMUM_TEXTURES_PER_PAGE,
			sizeof(struct texture_page_texture));
		if (texture_page->textures)
		{
			data_make_valid(texture_page->textures);
			code_0010f570(texture_page);
			return texture_page;
		}

		{
			struct data_array *textures = texture_page->textures;
			texture_page = NULL;
			match_free("c:\\halo\\SOURCE\\memory\\texture_page.c", 56, textures);
		}
	}

	return texture_page;
}

void texture_page_delete(
	struct texture_page *texture_page)
{
	code_0010f570(texture_page);
	data_dispose(texture_page->textures);
	match_free("c:\\halo\\SOURCE\\memory\\texture_page.c", 69, texture_page);
	return;
}

void texture_page_textures_begin(
	struct texture_page *texture_page)
{
	code_0010f570(texture_page);
	match_assert("c:\\halo\\SOURCE\\memory\\texture_page.c", 78, !texture_page->contains_unsorted_textures);
	texture_page->contains_unsorted_textures = TRUE;
	return;
}

struct texture_page_texture *texture_page_texture_get(
	struct texture_page *texture_page,
	long texture_index)
{
	code_0010f570(texture_page);
	return datum_get(texture_page->textures, texture_index);
}

void texture_page_texture_delete(
	struct texture_page *texture_page,
	long texture_index)
{
	code_0010f570(texture_page);
	datum_delete(texture_page->textures, texture_index);
	code_0010f790(texture_page);
	return;
}

boolean texture_page_resize(
	struct texture_page *texture_page,
	short width,
	short height)
{
	short old_width;
	short old_height;

	code_0010f570(texture_page);
	old_width = texture_page->width;
	old_height = texture_page->height;
	texture_page->width = width;
	texture_page->height = height;
	if (code_0010f790(texture_page))
		return TRUE;

	texture_page->width = old_width;
	texture_page->height = old_height;
	code_0010f790(texture_page);
	return FALSE;
}

/* ---------- private code */

static void code_0010f570(
	struct texture_page *texture_page)
{
	match_assert("c:\\halo\\SOURCE\\memory\\texture_page.c", 260, texture_page);
	match_assert("c:\\halo\\SOURCE\\memory\\texture_page.c", 261, texture_page->width>0 && texture_page->height>0);
	data_verify(texture_page->textures);
	return;
}
