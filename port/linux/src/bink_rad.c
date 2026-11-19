/*
BINK_RAD.C

The Bink video SDK's entry points that bink_playback.c uses (RAD Game Tools'
SDK, which the Xbox game linked), over the port's own decoder
(bink_decoder.c), so that the Xbox's movies play: the intro before the
first main menu, the attract mode's three after a while there, and the
credits at the campaign's end. They are read from the bink folder next to
maps (d:\bink), which importing a disc fills (xiso.c); a movie that is not
there is skipped, as before.

- BinkOpen reads the file's header and frame index. BinkDoFrame reads the
  current frame from the file, decodes its video and queues its audio on a
  DirectSound stream (dsound_sdl.c), which plays it at once: a Bink file
  carries its audio ahead of its video, so the stream is never short.
  BinkNextFrame moves to the next frame; BinkWait says whether the time of
  the next frame, by the movie's frame rate from the first frame's, has
  come; BinkCopyToBuffer draws the frame into the game's 32-bit texture.
- FrameNum counts from 1, as the SDK's: the game stops a movie when it
  reaches Frames - 1.
- The game hands the SDK its allocator (RADSetMemory), a pool it takes from
  the texture cache (4 MB, 16 MB for the intro), of which its own frame
  buffer takes 1.2 MB. The decoder needs more than the rest (its bundles,
  two frames), so it allocates its own memory.
- Only the first audio track plays, and only Bink audio's RDFT variant,
  which every Xbox movie uses; a movie with another plays without sound.
*/

#include "platform.h"
#include "bink_decoder.h"

#include <stdlib.h>
#include <string.h>

typedef void *(__stdcall *rad_memory_allocate_proc)(unsigned long size);
typedef void (__stdcall *rad_memory_free_proc)(void *memory);
typedef void *(__stdcall *bink_sound_system_open_proc)(unsigned long param);

/* the surface type the game asks BinkCopyToBuffer for (bink_playback.c) */
#define BINKSURFACE32 3
#define BINKSURFACEMASK 0x0F

/* a packet's bytes the bit readers may read past its end */
#define BINK_PACKET_PADDING 8
/* (the largest frame a file may have: the Xbox's largest is 127 KB) */
#define BINK_MAXIMUM_FRAME_BYTES (32UL * 1024 * 1024)

/* the fields bink_playback.c reads come first, as the SDK's */
typedef struct BINK
{
	unsigned long Width;
	unsigned long Height;
	unsigned long Frames;
	unsigned long FrameNum;
	unsigned long LastFrameNum;

	HANDLE file;
	struct bink_header header;
	/* where each frame starts, and the end of the last */
	unsigned long *index;
	unsigned char *packet;
	unsigned long packet_capacity;
	struct bink_video *video;
	struct bink_audio *audio;
	short *samples;
	unsigned long sample_capacity;
	LPDIRECTSOUNDSTREAM stream;
	/* when the first frame was decoded (GetTickCount), and whether it has */
	unsigned long start;
	int started;
	unsigned long frames_decoded;
} BINK, *HBINK;

/* the DirectSound the game gave for its movies' sound (BinkSetSoundSystem) */
static LPDIRECTSOUND bink_direct_sound;

static unsigned long read32(const unsigned char *data)
{
	return (unsigned long)data[0] | ((unsigned long)data[1] << 8) | ((unsigned long)data[2] << 16) |
		((unsigned long)data[3] << 24);
}

static int file_read_at(HANDLE file, unsigned long offset, void *buffer, unsigned long size)
{
	DWORD read = 0;

	if (SetFilePointer(file, (LONG)offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return 0;
	return ReadFile(file, buffer, size, &read, NULL) && read == size;
}

void __stdcall RADSetMemory(rad_memory_allocate_proc allocate, rad_memory_free_proc release)
{
	(void)allocate;
	(void)release;
}

/* (BinkSetSoundSystem's open procedure: param is the game's IDirectSound) */
void *__stdcall BinkOpenDirectSound(unsigned long param)
{
	bink_direct_sound = (LPDIRECTSOUND)param;
	return bink_direct_sound;
}

long __stdcall BinkSetSoundSystem(bink_sound_system_open_proc open, unsigned long param)
{
	return open && open(param) != NULL;
}

void __stdcall BinkSetIOSize(unsigned long io_size)
{
	(void)io_size;
}

void __stdcall BinkClose(HBINK bink)
{
	if (!bink)
		return;
	if (bink->stream)
	{
		IDirectSoundStream_Flush(bink->stream);
		IDirectSoundStream_Release(bink->stream);
	}
	if (bink->file != INVALID_HANDLE_VALUE)
		CloseHandle(bink->file);
	bink_video_delete(bink->video);
	bink_audio_delete(bink->audio);
	free(bink->index);
	free(bink->packet);
	free(bink->samples);
	free(bink);
}

static void stream_open(HBINK bink)
{
	WAVEFORMATEX format;
	DSSTREAMDESC description;
	int channels = bink_audio_channels(bink->audio);

	if (!bink_direct_sound)
		return;
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = (unsigned short)channels;
	format.nSamplesPerSec = bink->header.audio_sample_rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = (unsigned short)(channels * 2);
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	memset(&description, 0, sizeof(description));
	description.dwMaxAttachedPackets = 64;
	description.lpwfxFormat = &format;
	if (IDirectSound_CreateSoundStream(bink_direct_sound, &description, &bink->stream, NULL) != DS_OK)
		bink->stream = NULL;
}

HBINK __stdcall BinkOpen(const char *name, unsigned long flags)
{
	unsigned char start[BINK_HEADER_MINIMUM_SIZE + 256 * 12];
	unsigned char *table;
	HBINK bink;
	DWORD size_high = 0, file_size, read = 0;
	unsigned long frame, entries;

	(void)flags;
	if (!name || !*name)
		return NULL;
	bink = calloc(1, sizeof(*bink));
	if (!bink)
		return NULL;
	bink->file = CreateFileA(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (bink->file == INVALID_HANDLE_VALUE)
		goto fail;
	file_size = GetFileSize(bink->file, &size_high);
	if (!ReadFile(bink->file, start, sizeof(start), &read, NULL) || !bink_header_read(start, read, &bink->header))
	{
		platform_log("bink: \"%s\" is not a Bink 1 movie this port plays", name);
		goto fail;
	}
	/* (the file as large as its header says, its index within it) */
	entries = bink->header.frame_count + 1;
	if (size_high || bink->header.file_size > file_size ||
		bink->header.index_offset + entries * 4 > bink->header.file_size)
		goto fail;
	bink->index = malloc(entries * sizeof(*bink->index));
	table = malloc(entries * 4);
	if (!bink->index || !table || !file_read_at(bink->file, bink->header.index_offset, table, entries * 4))
	{
		free(table);
		goto fail;
	}
	for (frame = 0; frame < bink->header.frame_count; frame++)
		bink->index[frame] = read32(table + frame * 4) & ~1UL;
	bink->index[bink->header.frame_count] = bink->header.file_size;
	free(table);
	for (frame = 0; frame < bink->header.frame_count; frame++)
	{
		unsigned long size = bink->index[frame + 1] - bink->index[frame];

		if (bink->index[frame + 1] <= bink->index[frame] || size > BINK_MAXIMUM_FRAME_BYTES)
			goto fail;
		if (size > bink->packet_capacity)
			bink->packet_capacity = size;
	}
	bink->packet = malloc(bink->packet_capacity + BINK_PACKET_PADDING);
	bink->video = bink_video_new(&bink->header);
	if (!bink->packet || !bink->video)
		goto fail;
	bink->audio = bink_audio_new(&bink->header);
	if (bink->audio)
		stream_open(bink);
	else if (bink->header.audio_track_count)
		platform_log("bink: \"%s\" has audio this port does not play", name);
	bink->Width = bink->header.width;
	bink->Height = bink->header.height;
	bink->Frames = bink->header.frame_count;
	bink->FrameNum = 1;
	platform_log("bink: playing \"%s\" (%lux%lu, %lu frames)", name, bink->Width, bink->Height, bink->Frames);
	return bink;

fail:
	platform_log("bink: cannot play \"%s\"", name);
	BinkClose(bink);
	return NULL;
}

static void audio_queue(HBINK bink, const unsigned char *data, unsigned long size)
{
	unsigned long maximum = bink_audio_maximum_samples(bink->audio, size), count;
	XMEDIAPACKET packet;

	if (maximum > bink->sample_capacity)
	{
		short *samples = realloc(bink->samples, maximum * sizeof(short));

		if (!samples)
			return;
		bink->samples = samples;
		bink->sample_capacity = maximum;
	}
	count = bink_audio_decode(bink->audio, data, size, bink->samples, bink->sample_capacity);
	if (!count || !bink->stream)
		return;
	/* (the stream copies the samples as it takes the packet) */
	memset(&packet, 0, sizeof(packet));
	packet.pvBuffer = bink->samples;
	packet.dwMaxSize = count * sizeof(short);
	IDirectSoundStream_Process(bink->stream, &packet, NULL);
}

long __stdcall BinkDoFrame(HBINK bink)
{
	unsigned long frame, at, end, track;

	if (!bink || !bink->FrameNum || bink->FrameNum > bink->Frames)
		return 0;
	frame = bink->FrameNum - 1;
	at = bink->index[frame];
	end = bink->index[frame + 1];
	if (!file_read_at(bink->file, at, bink->packet, end - at))
		return 0;
	memset(bink->packet + (end - at), 0, BINK_PACKET_PADDING);
	if (!bink->started)
	{
		bink->start = GetTickCount();
		bink->started = 1;
	}
	/* each audio track's packet, then the video's */
	at = 0;
	end -= bink->index[frame];
	for (track = 0; track < bink->header.audio_track_count; track++)
	{
		unsigned long size;

		if (end - at < 4)
			return 0;
		size = read32(bink->packet + at);
		at += 4;
		if (size > end - at)
			return 0;
		if (track == 0 && bink->audio && size >= 4)
			audio_queue(bink, bink->packet + at, size);
		at += size;
	}
	bink_video_decode(bink->video, bink->packet + at, end - at);
	bink->frames_decoded++;
	return 0;
}

void __stdcall BinkNextFrame(HBINK bink)
{
	if (!bink)
		return;
	bink->LastFrameNum = bink->FrameNum;
	bink->FrameNum = bink->FrameNum >= bink->Frames ? 1 : bink->FrameNum + 1;
}

/* nonzero while the time of the frame to decode next has not come */
long __stdcall BinkWait(HBINK bink)
{
	unsigned long elapsed, due;

	if (!bink || !bink->started)
		return 0;
	elapsed = GetTickCount() - bink->start;
	/* (the frames since the first, which is due at 0; a loop's restart is
	timed from it too) */
	due = (unsigned long)((double)bink->frames_decoded * 1000.0 * bink->header.frame_rate_divisor /
		bink->header.frame_rate);
	if (elapsed >= due)
		return 0;
	/* (the game asks again at once while this says wait) */
	if (due - elapsed > 2)
		Sleep(1);
	return 1;
}

/* the frame shown, BT.601 at video levels, into X8R8G8B8 */
long __stdcall BinkCopyToBuffer(HBINK bink, void *destination, long destination_pitch,
	unsigned long destination_height, unsigned long destination_x, unsigned long destination_y,
	unsigned long flags)
{
	const unsigned char *y_plane, *u_plane, *v_plane;
	long luma_stride, chroma_stride;
	unsigned long row, column, rows, columns;

	if (!bink || !destination || (flags & BINKSURFACEMASK) != BINKSURFACE32)
		return 0;
	bink_video_planes(bink->video, &y_plane, &u_plane, &v_plane, &luma_stride, &chroma_stride);
	rows = bink->Height < destination_height ? bink->Height : destination_height;
	columns = bink->Width;
	for (row = 0; row < rows; row++)
	{
		unsigned long *out = (unsigned long *)((unsigned char *)destination +
			(row + destination_y) * destination_pitch) + destination_x;
		const unsigned char *luma = y_plane + row * luma_stride;
		const unsigned char *u = u_plane + (row >> 1) * chroma_stride;
		const unsigned char *v = v_plane + (row >> 1) * chroma_stride;

		for (column = 0; column < columns; column++)
		{
			long c = ((long)luma[column] - 16) * 298;
			long d = (long)u[column >> 1] - 128;
			long e = (long)v[column >> 1] - 128;
			long red = (c + 409 * e + 128) >> 8;
			long green = (c - 100 * d - 208 * e + 128) >> 8;
			long blue = (c + 516 * d + 128) >> 8;

			red = red < 0 ? 0 : red > 255 ? 255 : red;
			green = green < 0 ? 0 : green > 255 ? 255 : green;
			blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
			out[column] = 0xFF000000UL | ((unsigned long)red << 16) | ((unsigned long)green << 8) |
				(unsigned long)blue;
		}
	}
	return 0;
}

/* (debug_bink's statistics: none are kept) */
void __stdcall BinkGetSummary(HBINK bink, void *summary)
{
	(void)bink;
	memset(summary, 0, 0x7C);
}

void __stdcall BinkGetRealtime(HBINK bink, void *realtime, unsigned long frame_count)
{
	(void)frame_count;
	memset(realtime, 0, 0x38);
	/* (its Frames, which the game divides by) */
	((unsigned long *)realtime)[3] = 1;
	if (bink)
		((unsigned long *)realtime)[0] = bink->FrameNum;
}
