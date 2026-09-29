/*
BMP_FILES.C

Windows bitmap files (bmp_files.h). The layouts are those of Microsoft's
Win32 documentation of BITMAPFILEHEADER, BITMAPINFOHEADER and its later
versions (wingdi.h): a 14-byte file header, then an information header whose
first field is its size, then the pixel rows, each padded to four bytes.
*/

/* ---------- headers */

#include "bmp_files.h"

#include <stddef.h>

/* ---------- constants */

#define FILE_HEADER_BYTES 14
/* BITMAPINFOHEADER; its later versions only add fields after it */
#define INFORMATION_HEADER_BYTES 40
/* the bit masks after a BITMAPINFOHEADER when it says BI_BITFIELDS */
#define BIT_MASKS_BYTES 12

/* the information header sizes Windows writes: BITMAPINFOHEADER, its V2 and
V3 extensions (with bit masks), BITMAPV4HEADER and BITMAPV5HEADER */
static uint32_t const information_header_sizes[] = { 40, 52, 56, 108, 124 };

/* biCompression: none, or colors placed by the bit masks */
#define COMPRESSION_NONE 0
#define COMPRESSION_BIT_FIELDS 3

/* the only masks read: blue, green and red bytes, as uncompressed 32-bit
pixels hold them */
#define RED_MASK 0x00FF0000UL
#define GREEN_MASK 0x0000FF00UL
#define BLUE_MASK 0x000000FFUL

#define OPAQUE_ALPHA 0xFF000000UL

static char const *const bmp_file_status_descriptions[NUMBER_OF_BMP_FILE_STATUSES] =
{
	"ok",
	"not a bmp file, or too short for its headers",
	"a bmp information header of an unknown size",
	"a picture of no size, a negative width, or too large",
	"not 24-bit or 32-bit uncompressed color",
	"pixel rows outside the file",
};

/* ---------- private code */

static uint16_t read_u16(
	uint8_t const *bytes)
{
	return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

static uint32_t read_u32(
	uint8_t const *bytes)
{
	return (uint32_t)bytes[0] |
		((uint32_t)bytes[1] << 8) |
		((uint32_t)bytes[2] << 16) |
		((uint32_t)bytes[3] << 24);
}

static int information_header_size_known(
	uint32_t size)
{
	size_t index;

	for (index = 0; index < sizeof(information_header_sizes) / sizeof(information_header_sizes[0]); index++)
	{
		if (information_header_sizes[index] == size)
		{
			return 1;
		}
	}

	return 0;
}

/* The file's pixel (x, y), counting rows from the top, as 0xFFRRGGBB. */
static uint32_t picture_pixel(
	uint8_t const *file,
	struct bmp_file_picture const *picture,
	uint32_t x,
	uint32_t y)
{
	uint32_t row = picture->top_down ? y : picture->height - 1 - y;
	uint8_t const *pixel = file + picture->pixels_offset +
		(size_t)row * picture->row_bytes +
		(size_t)x * (picture->bits_per_pixel / 8);

	return OPAQUE_ALPHA | ((uint32_t)pixel[2] << 16) | ((uint32_t)pixel[1] << 8) | pixel[0];
}

/* Where output pixel `index` of `count` starts and ends in a span of the
picture `length` pixels long starting at `start`: at least one pixel. */
static void span_cover(
	uint32_t start,
	uint32_t length,
	uint32_t index,
	uint32_t count,
	uint32_t *first,
	uint32_t *end)
{
	*first = start + (uint32_t)((uint64_t)index * length / count);
	*end = start + (uint32_t)((uint64_t)(index + 1) * length / count);
	if (*end <= *first)
	{
		*end = *first + 1;
	}

	return;
}

/* ---------- public code */

char const *bmp_file_status_describe(
	enum bmp_file_status status)
{
	return status >= 0 && status < NUMBER_OF_BMP_FILE_STATUSES ?
		bmp_file_status_descriptions[status] :
		"unknown status";
}

enum bmp_file_status bmp_file_open(
	uint8_t const *file,
	uint32_t file_size,
	struct bmp_file_picture *picture)
{
	uint32_t header_size;
	uint32_t headers_end;
	uint32_t raw_width;
	uint32_t raw_height;
	uint32_t compression;
	uint64_t pixels_end;

	if (file_size < FILE_HEADER_BYTES + INFORMATION_HEADER_BYTES || file[0] != 'B' || file[1] != 'M')
	{
		return _bmp_file_status_not_bmp;
	}
	header_size = read_u32(file + FILE_HEADER_BYTES);
	if (!information_header_size_known(header_size))
	{
		return _bmp_file_status_unsupported_header;
	}
	headers_end = FILE_HEADER_BYTES + header_size;
	if (headers_end > file_size)
	{
		return _bmp_file_status_not_bmp;
	}

	/* a negative height stores the top row first; a negative width means
	nothing, nor does the most negative height have a size */
	raw_width = read_u32(file + 18);
	raw_height = read_u32(file + 22);
	if (raw_width == 0 || raw_width > BMP_FILE_MAXIMUM_DIMENSION ||
		raw_height == 0 || raw_height == 0x80000000UL)
	{
		return _bmp_file_status_bad_dimensions;
	}
	picture->width = raw_width;
	picture->top_down = (raw_height & 0x80000000UL) != 0;
	picture->height = picture->top_down ? 0 - raw_height : raw_height;
	if (picture->height > BMP_FILE_MAXIMUM_DIMENSION)
	{
		return _bmp_file_status_bad_dimensions;
	}

	picture->bits_per_pixel = read_u16(file + 28);
	compression = read_u32(file + 30);
	if (read_u16(file + 26) != 1 ||
		(picture->bits_per_pixel != 24 && picture->bits_per_pixel != 32))
	{
		return _bmp_file_status_unsupported_pixels;
	}
	if (compression == COMPRESSION_BIT_FIELDS && picture->bits_per_pixel == 32)
	{
		/* the masks follow a BITMAPINFOHEADER, and are the first fields
		after it in the later versions */
		if (header_size == INFORMATION_HEADER_BYTES)
		{
			headers_end += BIT_MASKS_BYTES;
			if (headers_end > file_size)
			{
				return _bmp_file_status_not_bmp;
			}
		}
		if (read_u32(file + 54) != RED_MASK ||
			read_u32(file + 58) != GREEN_MASK ||
			read_u32(file + 62) != BLUE_MASK)
		{
			return _bmp_file_status_unsupported_pixels;
		}
	}
	else if (compression != COMPRESSION_NONE)
	{
		return _bmp_file_status_unsupported_pixels;
	}

	picture->row_bytes = (picture->width * picture->bits_per_pixel + 31) / 32 * 4;
	picture->pixels_offset = read_u32(file + 10);
	pixels_end = (uint64_t)picture->pixels_offset + (uint64_t)picture->row_bytes * picture->height;
	if (picture->pixels_offset < headers_end || pixels_end > file_size)
	{
		return _bmp_file_status_bad_pixel_range;
	}

	return _bmp_file_status_ok;
}

void bmp_file_fit(
	uint8_t const *file,
	struct bmp_file_picture const *picture,
	uint32_t shape_width,
	uint32_t shape_height,
	uint32_t width,
	uint32_t height,
	uint32_t *pixels)
{
	uint32_t crop_x = 0;
	uint32_t crop_y = 0;
	uint32_t crop_width = picture->width;
	uint32_t crop_height = picture->height;
	uint32_t y;

	/* the middle of the picture with the shape asked for */
	if (shape_width && shape_height)
	{
		if ((uint64_t)picture->width * shape_height > (uint64_t)picture->height * shape_width)
		{
			crop_width = (uint32_t)((uint64_t)picture->height * shape_width / shape_height);
			if (crop_width == 0)
			{
				crop_width = 1;
			}
			crop_x = (picture->width - crop_width) / 2;
		}
		else
		{
			crop_height = (uint32_t)((uint64_t)picture->width * shape_height / shape_width);
			if (crop_height == 0)
			{
				crop_height = 1;
			}
			crop_y = (picture->height - crop_height) / 2;
		}
	}

	for (y = 0; y < height; y++)
	{
		uint32_t first_row;
		uint32_t end_row;
		uint32_t x;

		span_cover(crop_y, crop_height, y, height, &first_row, &end_row);
		for (x = 0; x < width; x++)
		{
			uint32_t first_column;
			uint32_t end_column;
			uint32_t row;
			uint64_t red = 0;
			uint64_t green = 0;
			uint64_t blue = 0;
			uint64_t count;

			span_cover(crop_x, crop_width, x, width, &first_column, &end_column);
			for (row = first_row; row < end_row; row++)
			{
				uint32_t column;

				for (column = first_column; column < end_column; column++)
				{
					uint32_t pixel = picture_pixel(file, picture, column, row);

					red += (pixel >> 16) & 0xFF;
					green += (pixel >> 8) & 0xFF;
					blue += pixel & 0xFF;
				}
			}
			count = (uint64_t)(end_row - first_row) * (end_column - first_column);
			pixels[(size_t)y * width + x] = OPAQUE_ALPHA |
				((uint32_t)((red + count / 2) / count) << 16) |
				((uint32_t)((green + count / 2) / count) << 8) |
				(uint32_t)((blue + count / 2) / count);
		}
	}

	return;
}
