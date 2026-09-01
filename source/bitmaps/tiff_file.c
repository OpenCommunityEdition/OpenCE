/*
TIFF_FILE.C

symbols in this file:
0006DBE0 0070:
	_tiff_get_bounds (0000)
0006DC50 0420:
	_tiff_export (0000)
0006E070 0540:
	_tiff_import (0000)
00255454 0002:
	??_C@_01KDCPPGHE@r?$AA@ (0000)
00255458 0014:
	??_C@_0BE@FPJPBLAF@failed?5to?5open?5tiff?$AA@ (0000)
0025546C 000e:
	??_C@_0O@BNNCBLEN@out?5of?5memory?$AA@ (0000)
0025547C 0019:
	??_C@_0BJ@EDBMJEAG@failed?5to?5write?5scanline?$AA@ (0000)
00255498 0029:
	??_C@_0CJ@CNFFKPJI@invalid?5bitmap?5encoding?5for?5tiff@ (0000)
002554C4 0023:
	??_C@_0CD@MEPHMAHO@c?3?2halo?2SOURCE?2bitmaps?2tiff_file@ (0000)
002554E8 0002:
	??_C@_01NOFIACDB@w?$AA@ (0000)
002554EC 0014:
	??_C@_0BE@DABJOKOK@file?5does?5not?5exist?$AA@ (0000)
00255500 0010:
	??_C@_0BA@GNLHBLFA@not?5a?5TIFF?5file?$AA@ (0000)
00255510 0030:
	??_C@_0DA@NCFNHNBH@unsupported?5TIFF?5orientation?5?$CImu@ (0000)
00255540 0036:
	??_C@_0DG@CEDAGLPJ@unsupported?5bits?5per?5sample?5?$CI?$CFd?$CJ@ (0000)
00255578 0033:
	??_C@_0DD@OCPJGOKH@unsupported?5TIFF?5photometric?0?5pl@ (0000)
002555AC 000f:
	??_C@_0P@HCMCPNNJ@TIFF?5too?5large?$AA@ (0000)
002555BC 001e:
	??_C@_0BO@DJPDCIEM@failed?5to?5read?5TIFF?5scan?5line?$AA@ (0000)
002555DC 0013:
	??_C@_0BD@NKJFFLKL@unsupported?5format?$AA@ (0000)
0031C320 0200:
	_bss_0031c320 (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/libtiff/tiffio.h"
#include "math/integer_math.h"
#include "tag_files/files.h"

/* ---------- constants */

enum
{
	_bitmap_format_a8 = 0,
	_bitmap_format_y8 = 1,
	_bitmap_format_ay8 = 2,
	_bitmap_format_r5g6b5 = 6,
	_bitmap_format_a1r5g5b5 = 8,
	_bitmap_format_a4r4g4b4 = 9,
	_bitmap_format_x8r8g8b8 = 10,
	_bitmap_format_a8r8g8b8 = 11,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void *bitmap_2d_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short mipmap_index);
struct bitmap_data *bitmap_2d_new(
	short width,
	short height,
	short mipmap_count,
	short format);
void bitmap_delete(
	struct bitmap_data *bitmap);

/* ---------- globals */

char error_message_buffer[512] = {0};

/* ---------- public code */

boolean
tiff_get_bounds(
	struct file_reference *file,
	unsigned long *width,
	unsigned long *height)
{
	char path[256];
	TIFF *tiff;
	boolean result = FALSE;

	tiff = TIFFOpen(
		file_reference_get_name(file, 0xD, path),
		"r");
	if (tiff)
	{
		TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, width);
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, height);
		TIFFClose(tiff);
		result = TRUE;
	}

	return result;
}

char const *
tiff_export(
	struct file_reference *file,
	struct bitmap_data *bitmap)
{
	char const *error_message = NULL;
	long tiff_format;
	short photometric;
	short samples_per_pixel;
	char path[256];
	TIFF *tiff;
	long row_size;
	byte *row_buffer;
	short y;

	switch (bitmap->format)
	{
	case _bitmap_format_a8:
	case _bitmap_format_y8:
	case _bitmap_format_ay8:
		tiff_format = _bitmap_format_a8;
		photometric = PHOTOMETRIC_MINISBLACK;
		samples_per_pixel = 1;
		break;
	case _bitmap_format_r5g6b5:
	case _bitmap_format_a1r5g5b5:
	case _bitmap_format_a4r4g4b4:
	case _bitmap_format_x8r8g8b8:
	case _bitmap_format_a8r8g8b8:
		tiff_format = _bitmap_format_a8r8g8b8;
		photometric = PHOTOMETRIC_RGB;
		samples_per_pixel = 4;
		break;
	default:
		return "invalid bitmap encoding for tiff export.";
	}

	tiff = TIFFOpen(
		file_reference_get_name(file, 0xD, path),
		"w");
	if (tiff)
	{
		row_size = (short)(bitmap_format_get_bits_per_pixel((short)tiff_format) * bitmap->width / 8);
		row_buffer = debug_malloc(row_size, FALSE, "c:\\halo\\SOURCE\\bitmaps\\tiff_file.c", 107);
		if (row_buffer)
		{
			TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, bitmap->width);
			TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, bitmap->height);
			TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
			TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric);
			TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
			TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);
			TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
			TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

			for (y = 0; y < bitmap->height; y++)
			{
				byte *source_row = bitmap_2d_address(bitmap, 0, (short)y, 0);
				short x;

				switch (bitmap->format)
				{
				case _bitmap_format_a1r5g5b5:
					for (x = 0; x < bitmap->width; x++)
					{
						word pixel = ((word *)source_row)[x];
						word green = pixel >> 5;
						row_buffer[x * 4 + 2] = (((byte)pixel & 0x1F) | ((byte)pixel << 1)) << 2;
						row_buffer[x * 4 + 1] = ((green & 0x1F) | (green << 1)) << 2;
						row_buffer[x * 4] = (((byte)(pixel >> 7) & 0xFB) | (byte)(pixel >> 8)) & 0xFC;
						row_buffer[x * 4 + 3] = 0xFF;
					}
					break;

				case _bitmap_format_r5g6b5:
					for (x = 0; x < bitmap->width; x++)
					{
						word pixel = ((word *)source_row)[x];
						row_buffer[x * 4 + 2] = ((byte)(pixel >> 2) & 7) | (byte)(pixel << 3);
						row_buffer[x * 4 + 1] = ((byte)(pixel >> 9) & 3) | ((byte)(pixel >> 5) << 2);
						row_buffer[x * 4] = ((byte)(pixel >> 8) & 0xF8) | (byte)(pixel >> 13);
						row_buffer[x * 4 + 3] = 0xFF;
					}
					break;

				case _bitmap_format_a4r4g4b4:
					for (x = 0; x < bitmap->width; x++)
					{
						word pixel = ((word *)source_row)[x];
						byte alpha = (byte)(pixel >> 12);
						byte red = (byte)(pixel >> 8);
						byte green = (byte)(pixel >> 4);
						byte blue = (byte)pixel;

						row_buffer[x * 4 + 3] = (alpha & 0xF) | (alpha << 4);
						row_buffer[x * 4 + 2] = (blue & 0xF) | (blue << 4);
						row_buffer[x * 4 + 1] = (green & 0xF) | (green << 4);
						row_buffer[x * 4] = (red & 0xF) | (red << 4);
					}
					break;

				case _bitmap_format_x8r8g8b8:
					for (x = 0; x < bitmap->width; x++)
					{
						unsigned long pixel = ((unsigned long *)source_row)[x];
						row_buffer[x * 4 + 2] = (byte)pixel;
						row_buffer[x * 4 + 1] = (byte)(pixel >> 8);
						row_buffer[x * 4] = (byte)(pixel >> 16);
						row_buffer[x * 4 + 3] = 0xFF;
					}
					break;

				case _bitmap_format_a8r8g8b8:
					for (x = 0; x < bitmap->width; x++)
					{
						unsigned long pixel = ((unsigned long *)source_row)[x];
						row_buffer[x * 4 + 3] = (byte)(pixel >> 24);
						row_buffer[x * 4 + 2] = (byte)pixel;
						row_buffer[x * 4 + 1] = (byte)(pixel >> 8);
						row_buffer[x * 4] = (byte)(pixel >> 16);
					}
					break;

				default:
					csmemcpy(row_buffer, source_row, row_size);
					break;
				}

				if (TIFFWriteScanline(tiff, row_buffer, y, 0) < 0)
				{
					error_message = "failed to write scanline";
					break;
				}
			}

			debug_free(row_buffer, "c:\\halo\\SOURCE\\bitmaps\\tiff_file.c", 231);
			TIFFClose(tiff);
		}
		else
		{
			error_message = "out of memory";
			TIFFClose(tiff);
		}
	}
	else
	{
		return "failed to open tiff";
	}

	return error_message;
}

char const *
tiff_import(
	struct file_reference *file,
	struct bitmap_data **bitmap_result,
	rectangle2d const *requested_bounds,
	short format)
{
	char const *error_message = NULL;
	char path[256];
	TIFF *tiff;
	long scanline_size;
	word bits_per_sample;
	word orientation;
	word samples_per_pixel;
	word planar_config;
	word photometric;
	unsigned long image_width;
	unsigned long image_height;
	rectangle2d bounds;
	short width;
	short height;
	struct bitmap_data *bitmap = NULL;
	byte *scanline = NULL;
	short y;

	if (file_exists(file))
	{
		tiff = TIFFOpen(file_reference_get_name(file, 0xD, path), "r");
		if (tiff)
		{
			scanline_size = TIFFScanlineSize(tiff);
			TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
			TIFFGetFieldDefaulted(tiff, TIFFTAG_ORIENTATION, &orientation);
			TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
			TIFFGetField(tiff, TIFFTAG_PLANARCONFIG, &planar_config);
			TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);
			TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &image_width);
			TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &image_height);

			if (requested_bounds)
				bounds = *requested_bounds;
			else
			{
				bounds.y0 = 0;
				bounds.x0 = 0;
				bounds.x1 = (short)image_width;
				bounds.y1 = (short)image_height;
			}

			if (orientation == ORIENTATION_TOPLEFT)
			{
				if (bits_per_sample != 8 ||
				(samples_per_pixel != 4 && samples_per_pixel != 3 &&
				samples_per_pixel != 2 && samples_per_pixel != 1))
				{
					_snprintf(
					error_message_buffer,
					NUMBEROF(error_message_buffer),
					"unsupported bits per sample (%d) or sample count (%d)",
					bits_per_sample,
					samples_per_pixel);
					error_message = error_message_buffer;
					goto cleanup;
				}

				if (format != NONE && format != _bitmap_format_a8r8g8b8)
				{
					error_message = "unsupported format";
					goto cleanup;
				}

				if (planar_config == PLANARCONFIG_CONTIG)
				{
					width = rectangle2d_width(&bounds);
					height = rectangle2d_height(&bounds);
					if (width < 0 || width > 30000 || height < 0 || height > 30000)
					{
						error_message = "TIFF too large";
						goto cleanup;
					}

					bitmap = bitmap_2d_new(width, height, 0, _bitmap_format_a8r8g8b8);
					scanline = debug_malloc(
						scanline_size,
						FALSE,
						"c:\\halo\\SOURCE\\bitmaps\\tiff_file.c",
						319);
					if (!bitmap || !scanline)
					{
						error_message = "out of memory";
						goto cleanup;
					}

					*bitmap_result = bitmap;
					for (y = bounds.y0; y < bounds.y1; y++)
					{
						long source_y;
						short x;

						source_y = y < 0 ? 0 : (y > image_height - 1 ? image_height - 1 : y);

						if (TIFFReadScanline(tiff, scanline, source_y, 0) < 0)
						{
							error_message = "failed to read TIFF scan line";
							goto cleanup;
						}

						switch (samples_per_pixel)
						{
						case 1:
							{
								unsigned long *destination = bitmap_2d_address(bitmap, 0, (short)(y - bounds.y0), 0);
								for (x = bounds.x0; x < bounds.x1; x++)
								{
									short source_x = x < 0 ? 0 : (x > image_width - 1 ? image_width - 1 : x);
									byte value = scanline[source_x];
									destination[x - bounds.x0] =
										(value << 24) | (value << 16) | (value << 8) | value;
								}
							}
							break;
						case 2:
							{
								unsigned long *destination = bitmap_2d_address(bitmap, 0, (short)(y - bounds.y0), 0);
								for (x = bounds.x0; x < bounds.x1; x++)
								{
									short source_x = x < 0 ? 0 : (x > image_width - 1 ? image_width - 1 : x);
									byte *pixel = scanline + source_x * 2;
									destination[x - bounds.x0] =
										(pixel[1] << 24) | (pixel[0] << 16) | (pixel[0] << 8) | pixel[0];
								}
							}
							break;
						case 3:
							{
								unsigned long *destination = bitmap_2d_address(bitmap, 0, (short)(y - bounds.y0), 0);
								for (x = bounds.x0; x < bounds.x1; x++)
								{
									short source_x = x < 0 ? 0 : (x > image_width - 1 ? image_width - 1 : x);
									byte *pixel = scanline + source_x * 3;
									destination[x - bounds.x0] =
										0xFF000000 | (pixel[0] << 16) | (pixel[1] << 8) | pixel[2];
								}
							}
							break;
						case 4:
							{
								unsigned long *destination = bitmap_2d_address(bitmap, 0, (short)(y - bounds.y0), 0);
								for (x = bounds.x0; x < bounds.x1; x++)
								{
									short source_x = x < 0 ? 0 : (x > image_width - 1 ? image_width - 1 : x);
									byte *pixel = scanline + source_x * 4;
									destination[x - bounds.x0] =
										(pixel[3] << 24) | (pixel[0] << 16) | (pixel[1] << 8) | pixel[2];
								}
							}
							break;
						default:
							display_assert(NULL, "c:\\halo\\SOURCE\\bitmaps\\tiff_file.c", 406, TRUE);
							system_exit(-1);
							break;
						}
					}

				cleanup:
					/*
					 * Original bug preserved: on a scan-line failure, bitmap_result still points
					 * at the bitmap deleted below. A non-matching safety fix would assign
					 * *bitmap_result = NULL after bitmap_delete().
					 */
					if (error_message && bitmap)
						bitmap_delete(bitmap);
					if (scanline)
						debug_free(scanline, "c:\\halo\\SOURCE\\bitmaps\\tiff_file.c", 422);
					TIFFClose(tiff);
					return error_message;
				}
				else
				{
					error_message = "unsupported TIFF photometric, planar configuration";
					goto cleanup;
				}
			}
			else
			{
				error_message = "unsupported TIFF orientation (must be top left)";
				goto cleanup;
			}
		}

		return "not a TIFF file";
	}

	return "file does not exist";
}

/* ---------- private code */
