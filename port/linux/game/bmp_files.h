/*
BMP_FILES.H

Reading of Windows bitmap (.bmp) files: the pictures players put next to
Custom Edition maps for the multiplayer menus (custom_edition_maps.c). Only
uncompressed 24-bit and 32-bit pictures are read, the kinds image editors
save.

The code is standalone C with fixed-width types: the same file is compiled
into the native game builds and into the host tool
port/tools/bmp_file_report.c, which the tests drive (tools/test_bmp_files.py).
A picture file is untrusted input: every field is read little-endian from the
file's bytes, and every size and offset is checked against the file before it
is used.
*/

#ifndef __BMP_FILES_H
#define __BMP_FILES_H

/* ---------- headers */

#include <stdint.h>

/* ---------- constants */

/* the widest and tallest picture read, beyond any screenshot */
#define BMP_FILE_MAXIMUM_DIMENSION 8192

enum bmp_file_status
{
	_bmp_file_status_ok,

	/* no BM signature, or too short for its headers */
	_bmp_file_status_not_bmp,
	/* an information header of a size no Windows version writes */
	_bmp_file_status_unsupported_header,
	/* a width or height of zero, negative width, or beyond
	BMP_FILE_MAXIMUM_DIMENSION */
	_bmp_file_status_bad_dimensions,
	/* not 24-bit or 32-bit uncompressed color in blue, green, red order */
	_bmp_file_status_unsupported_pixels,
	/* pixel rows that start inside the headers or end beyond the file */
	_bmp_file_status_bad_pixel_range,

	NUMBER_OF_BMP_FILE_STATUSES
};

/* ---------- structures */

/* where a file's pixels are: bmp_file_open fills it, and bmp_file_fit
reads the pixels through it */
struct bmp_file_picture
{
	uint32_t width;
	uint32_t height;
	uint16_t bits_per_pixel;
	/* the file offset of the first row stored, the bytes from one row to the
	next, and whether the top row is stored first (else the bottom row is) */
	uint32_t pixels_offset;
	uint32_t row_bytes;
	int top_down;
};

/* ---------- prototypes */

char const *bmp_file_status_describe(
	enum bmp_file_status status);

/* Checks the `file_size` bytes at `file` as a bmp file this module reads,
describing its picture in `picture` when it is one. */
enum bmp_file_status bmp_file_open(
	uint8_t const *file,
	uint32_t file_size,
	struct bmp_file_picture *picture);

/* Fills `pixels`, a picture `width` by `height` pixels large (each nonzero)
of 32-bit 0xFFRRGGBB values with the top row first, from the picture of the
file bmp_file_open checked: from the middle of it, as much as has the shape
`shape_width` by `shape_height` (all of it when either is zero). Each pixel
is the average of the file's pixels it covers, or the file's pixel under it
where the picture is enlarged. */
void bmp_file_fit(
	uint8_t const *file,
	struct bmp_file_picture const *picture,
	uint32_t shape_width,
	uint32_t shape_height,
	uint32_t width,
	uint32_t height,
	uint32_t *pixels);

#endif
