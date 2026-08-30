/*
S3TC.H

header included in hcex build.
*/

#ifndef __S3TC_H
#define __S3TC_H
#pragma once

#include "cseries/cseries.h"

/* ---------- constants */

enum
{
	S3TC_BLOCK_PIXELS = 16,
	S3TC_ALPHA = 3,
};

/* ---------- macros */

/* ---------- structures */

struct s3tc_color
{
	byte rgba[4];
};

struct s3tc_block_rgb
{
	word color0;
	word color1;
	unsigned long bitmap;
};

struct s3tc_block_alpha3
{
	byte alpha0;
	byte alpha1;
	byte alpha_bitmap[6];
	struct s3tc_block_rgb rgb;
};

typedef char s3tc_color_size_assert[
	sizeof(struct s3tc_color) == 0x4 ? 1 : -1];
typedef char s3tc_block_rgb_size_assert[
	sizeof(struct s3tc_block_rgb) == 0x8 ? 1 : -1];
typedef char s3tc_block_alpha3_rgb_offset_assert[
	offsetof(struct s3tc_block_alpha3, rgb) == 0x8 ? 1 : -1];
typedef char s3tc_block_alpha3_size_assert[
	sizeof(struct s3tc_block_alpha3) == 0x10 ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

void DecodeBlockRGB(
	struct s3tc_block_rgb *source,
	struct s3tc_color colors[S3TC_BLOCK_PIXELS]);
void DecodeBlockAlpha3(
	struct s3tc_block_alpha3 *source,
	struct s3tc_color colors[S3TC_BLOCK_PIXELS]);

/* ---------- globals */

/* ---------- public code */

#endif // __S3TC_H
