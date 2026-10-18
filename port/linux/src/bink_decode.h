/*
BINK_DECODE.H

A decoder for Bink 1 movies (revisions f to i, the Xbox disc's), written
for the port from the format's public descriptions and its published
behaviour, and plain C with no operating system calls, so every port can
share it. It takes the tables the format needs from the player's own Xbox
executable (bink_tables.c); none are in this source.

A movie is read one frame at a time through a read callback, so a movie
need not fit in memory. Frames are decoded in order. Everything in a movie
is untrusted: a bad movie makes the calls fail with a reason, and never
reads or writes out of bounds.
*/

#ifndef __HALO_BINK_DECODE_H
#define __HALO_BINK_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- tables (bink_tables.c) */

/* what the format needs besides its code: read from the executable. The
quantisation matrices are 16.16 fixed point, 16 quality levels of 64
coefficients in raster order */
struct bink_tables
{
	unsigned char scan[64];
	unsigned char patterns[16][64];
	unsigned char tree_bits[16][16];
	unsigned char tree_lengths[16][16];
	int intra_quantizer[16][64];
	int inter_quantizer[16][64];
	unsigned long critical_frequencies[24];
	unsigned char run_lengths[16];
};

/* finds the tables in the image of the Xbox executable (default.xbe) of
Halo (the disc's, or a build that links the same Bink library); returns 1,
or 0 with the reason in error */
int bink_tables_from_xbe(const unsigned char *image, unsigned long size, struct bink_tables *tables,
	char *error, int error_size);

/* ---------- decoder */

enum
{
	BINK_MAXIMUM_DIMENSION = 4096,
	BINK_MAXIMUM_FRAMES = 1 << 20
};

struct bink_decoder;

/* reads size bytes at offset of the movie's file; returns 1 if all were read */
typedef int (*bink_read_proc)(void *context, unsigned long offset, void *buffer, unsigned long size);

struct bink_info
{
	unsigned long width;
	unsigned long height;
	unsigned long frames;
	unsigned long frame_rate;
	unsigned long frame_rate_divisor;
	int revision; /* 'f' to 'i' */
	int has_alpha;
	/* the first audio track (sample_rate is 0 without one) */
	unsigned long sample_rate;
	int channels;
};

/* opens a movie of file_size bytes; the tables are kept (not copied) and
must outlive the decoder. Returns NULL with the reason in error */
struct bink_decoder *bink_open(const struct bink_tables *tables, bink_read_proc read, void *context,
	unsigned long file_size, char *error, int error_size);
void bink_close(struct bink_decoder *decoder);
const struct bink_info *bink_get_info(const struct bink_decoder *decoder);

/* the next frame to decode (from 0; frames in the movie when done) */
unsigned long bink_next_frame(const struct bink_decoder *decoder);

/* goes back to the start (the first frame is a key frame) */
void bink_rewind(struct bink_decoder *decoder);

/* decodes the next frame's video, and its audio (appended to what
bink_take_audio has not taken yet). Returns 1, or 0 with the reason in
error: the decoder is then finished, and only bink_rewind restarts it */
int bink_decode_frame(struct bink_decoder *decoder, char *error, int error_size);

/* takes up to max_frames frames of 16-bit audio (info.channels interleaved)
decoded so far; returns the number of frames taken */
unsigned long bink_take_audio(struct bink_decoder *decoder, short *samples, unsigned long max_frames);

/* the frames of audio decoded and not taken */
unsigned long bink_audio_available(const struct bink_decoder *decoder);

/* the last decoded frame as pixels of 4 bytes (blue, green, red, 0: the
Xbox's X8R8G8B8, little endian) into destination, a rows of pitch bytes.
The picture is copied at (x, y) and clipped to the destination's height
and pitch */
void bink_copy_to_buffer(const struct bink_decoder *decoder, void *destination, long pitch,
	unsigned long destination_height, unsigned long x, unsigned long y);

#ifdef __cplusplus
}
#endif

#endif
