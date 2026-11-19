/*
BINK_DECODER.H

Bink 1 video and Bink audio (RDFT) decoding: bink_decoder.c, adapted from
FFmpeg's decoders and demuxer (LGPL 2.1 or later; see that file).
*/

#ifndef BINK_DECODER_H
#define BINK_DECODER_H

/* the bytes a file's header needs before its frame index */
#define BINK_HEADER_MINIMUM_SIZE 48

struct bink_header
{
	unsigned long revision;           /* the signature's fourth letter: 'f' to 'k' */
	unsigned long file_size;          /* in bytes */
	unsigned long frame_count;
	unsigned long largest_frame_size;
	unsigned long width;
	unsigned long height;
	unsigned long frame_rate;         /* frame_rate / frame_rate_divisor frames a second */
	unsigned long frame_rate_divisor;
	unsigned long video_flags;
	unsigned long audio_track_count;
	/* the first audio track's (none: 0) */
	unsigned long audio_sample_rate;
	unsigned long audio_flags;
	/* where the frame index starts */
	unsigned long index_offset;
};

/* the audio flags */
#define BINK_AUDIO_STEREO 0x2000
#define BINK_AUDIO_USE_DCT 0x1000

/* reads a file's header from its first size bytes: nonzero when it is a Bink
1 file this decoder plays (revisions f to k, a size it can hold) */
int bink_header_read(const unsigned char *data, unsigned long size, struct bink_header *header);

struct bink_video;

struct bink_video *bink_video_new(const struct bink_header *header);
void bink_video_delete(struct bink_video *video);
/* decodes a frame's video packet; negative when it is damaged, and the frame
shown stays the last one decoded. data must have 8 readable bytes past
size. */
int bink_video_decode(struct bink_video *video, const unsigned char *data, unsigned long size);
/* the frame shown: its luma and the two chroma planes, each half the size */
void bink_video_planes(const struct bink_video *video, const unsigned char **y, const unsigned char **u,
	const unsigned char **v, long *luma_stride, long *chroma_stride);

struct bink_audio;

/* NULL for audio this decoder does not play (the DCT variant) */
struct bink_audio *bink_audio_new(const struct bink_header *header);
void bink_audio_delete(struct bink_audio *audio);
/* the channels and samples a second of what bink_audio_decode writes */
int bink_audio_channels(const struct bink_audio *audio);
/* the most samples (each channel's counted) one call can write for a packet
of this size */
unsigned long bink_audio_maximum_samples(const struct bink_audio *audio, unsigned long size);
/* decodes a frame's packet of the first audio track into 16-bit samples,
the channels interleaved: the number of samples written. data must have 8
readable bytes past size. */
unsigned long bink_audio_decode(struct bink_audio *audio, const unsigned char *data, unsigned long size,
	short *samples, unsigned long capacity);

#endif
