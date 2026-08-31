/*
S3TC.C

symbols in this file:
0005EB30 0060:
	_code_0005eb30 (0000)
0005EB90 0050:
	_code_0005eb90 (0000)
0005EBE0 0040:
	_code_0005ebe0 (0000)
0005EC20 0060:
	_code_0005ec20 (0000)
0005EC80 00a0:
	_code_0005ec80 (0000)
0005ED20 0210:
	_code_0005ed20 (0000)
0005EF30 0140:
	_code_0005ef30 (0000)
0005F070 0170:
	_code_0005f070 (0000)
0005F1E0 0890:
	_EncodeBlockRGBColorKey (0000)
0005FA70 01c0:
	_DecodeBlockRGB (0000)
0005FC30 01f0:
	_DecodeBlockRGB__single_pixel (0000)
0005FE20 0090:
	_DecodeBlockAlpha4 (0000)
0005FEB0 0050:
	_DecodeBlockAlpha4__single_pixel (0000)
0005FF00 0260:
	_DecodeBlockAlpha3 (0000)
00060160 01b0:
	_DecodeBlockAlpha3__single_pixel (0000)
00060310 0020:
	_EncodeBlockRGB (0000)
00060330 0070:
	_EncodeBlockAlpha4 (0000)
000603A0 0270:
	_EncodeBlockAlpha3 (0000)
002510C8 0004:
	__real@3b808081 (0000)
002510CC 0004:
	__real@c7c34f80 (0000)
002510D0 000d:
	??_C@_0N@NBFAEMLN@v?$DO?$DN0?5?$CG?$CG?5v?$DM?$DN4?$AA@ (0000)
002510E0 000d:
	??_C@_0N@CLDKFKKD@u?$DO?$DN0?5?$CG?$CG?5u?$DM?$DN4?$AA@ (0000)
002510F0 0023:
	??_C@_0CD@CADFKEEP@c?3?2halo?2SOURCE?2bitmaps?2s3tc?2s3tc@ (0000)
002DB64C 002c:
	_data_002db64c (0000)
*/

/* ---------- headers */

#include "bitmaps/s3tc/s3tc.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

/* ---------- private code */

void DecodeBlockAlpha3(
	struct s3tc_block_alpha3 *source,
	struct s3tc_color colors[S3TC_BLOCK_PIXELS])
{
	long pixel;
	long alpha[8];
	unsigned long bitmap = 0;

	DecodeBlockRGB(&source->rgb, colors);

	alpha[0] = source->alpha0;
	alpha[1] = source->alpha1;

	if (alpha[0] > alpha[1])
	{
		alpha[2] = (6 * alpha[0] + 1 * alpha[1]) / 7;
		alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;
		alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;
		alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;
		alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;
		alpha[7] = (1 * alpha[0] + 6 * alpha[1]) / 7;
	}
	else
	{
		alpha[2] = (4 * alpha[0] + 1 * alpha[1]) / 5;
		alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;
		alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;
		alpha[5] = (1 * alpha[0] + 4 * alpha[1]) / 5;
		alpha[6] = 0;
		alpha[7] = 255;
	}

	for (pixel = 0; pixel < S3TC_BLOCK_PIXELS; ++pixel)
	{
		if ((pixel & 7) == 0)
		{
			if (pixel == 0)
			{
				bitmap = source->alpha_bitmap[2];
				bitmap <<= 8;
				bitmap |= source->alpha_bitmap[1];
				bitmap <<= 8;
				bitmap |= source->alpha_bitmap[0];
			}
			else
			{
				bitmap = source->alpha_bitmap[5];
				bitmap <<= 8;
				bitmap |= source->alpha_bitmap[4];
				bitmap <<= 8;
				bitmap |= source->alpha_bitmap[3];
			}
		}

		colors[pixel].rgba[S3TC_ALPHA] = (byte)alpha[bitmap & 7];
		bitmap >>= 3;
	}

	return;
}

void DecodeBlockAlpha3__single_pixel(
	struct s3tc_block_alpha3 const *source,
	struct s3tc_color *color,
	short u,
	short v)
{
	word alpha[8];
	unsigned long bitmap;
	long shift;

	DecodeBlockRGB__single_pixel(&source->rgb, color, u, v);

	alpha[0] = source->alpha0;
	alpha[1] = source->alpha1;

	if (alpha[0] > alpha[1])
	{
		alpha[2] = (6 * alpha[0] + alpha[1]) / 7;
		alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;
		alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;
		alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;
		alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;
		alpha[7] = (alpha[0] + 6 * alpha[1]) / 7;
	}
	else
	{
		alpha[2] = (4 * alpha[0] + alpha[1]) / 5;
		alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;
		alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;
		alpha[5] = (alpha[0] + 4 * alpha[1]) / 5;
		alpha[6] = 0;
		alpha[7] = 255;
	}

	if (v < 2)
	{
		bitmap = source->alpha_bitmap[2];
		bitmap <<= 8;
		bitmap |= source->alpha_bitmap[1];
		bitmap <<= 8;
		bitmap |= source->alpha_bitmap[0];
		shift = 3 * (4 * v + u);
	}
	else
	{
		bitmap = source->alpha_bitmap[5];
		bitmap <<= 8;
		bitmap |= source->alpha_bitmap[4];
		bitmap <<= 8;
		bitmap |= source->alpha_bitmap[3];
		shift = 3 * (4 * (v - 2) + u);
	}

	color->rgba[S3TC_ALPHA] = (byte)alpha[(bitmap >> shift) & 7];

	return;
}
