/*
BINK_SDK.C

The Bink video SDK entry points bink_playback.c uses, over the port's own
decoder (bink_decode.c). The RAD SDK is proprietary and is not part of the
port; the game opens a movie, asks it to decode a frame, copies the frame
into a texture and waits for the next frame's time, as with the SDK.

A movie plays if

 - game.movies is on,
 - the data folder has default.xbe, the Halo executable of the Xbox disc,
   whose Bink library holds the format's tables (bink_tables.c; they are
   read from it, once, the first time a movie is opened),
 - and the movie file (bink/ in the data folder) is a Bink movie the decoder
   takes.

Otherwise BinkOpen reports that the movie cannot be opened, and the game
skips it, exactly as it does for a missing movie file.

The frames are paced by the clock from the first decoded frame; a movie that
decodes slower than it plays skips frames (decoding them without drawing),
and the audio, which is decoded with each frame, goes to a DirectSound
stream as it is.

The prototypes match the declarations in bink_playback.c; the RAD SDK's
RADEXPLINK is __stdcall.
*/

#include "platform.h"
#include "bink_decode.h"
#include "port_config.h"

#include <string.h>

enum
{
	MAXIMUM_EXECUTABLE_SIZE = 16 << 20,
	AUDIO_CHUNK_FRAMES = 16384,
	/* the most frames skipped at once to catch up with the clock */
	MAXIMUM_SKIPPED_FRAMES = 8,
	BINK_SURFACE_32 = 3
};

typedef void *(__stdcall *rad_memory_allocate_proc)(unsigned long size);
typedef void (__stdcall *rad_memory_free_proc)(void *memory);
typedef void *(__stdcall *bink_sound_system_open_proc)(unsigned long param);

/* the SDK's BINK, as far as bink_playback.c reads it, which must be first */
struct BINK
{
	unsigned long Width;
	unsigned long Height;
	unsigned long Frames;
	unsigned long FrameNum; /* the next frame to decode, from 1 */
	unsigned long LastFrameNum;
};
typedef struct BINK *HBINK;

struct bink_movie
{
	struct BINK bink; /* first */
	struct bink_decoder *decoder;
	HANDLE file;
	unsigned long start_time; /* GetTickCount of the first frame's time */
	int started;
	LPDIRECTSOUNDSTREAM stream;
	short *audio_buffer;
	unsigned long skipped_frames;
	unsigned long decoded_frames;
	unsigned long period_numerator; /* milliseconds per frame: numerator / denominator */
	unsigned long period_denominator;
	int failed;
};

static void *sound_system = NULL;
static int tables_tried = 0;
static int tables_loaded = 0;
static struct bink_tables tables;

void __stdcall RADSetMemory(rad_memory_allocate_proc allocate, rad_memory_free_proc release)
{
	(void)allocate;
	(void)release;
}

/* the sound system a movie's audio uses is the game's DirectSound, which
is what is passed through BinkSetSoundSystem */
void *__stdcall BinkOpenDirectSound(unsigned long param)
{
	return (void *)param;
}

long __stdcall BinkSetSoundSystem(bink_sound_system_open_proc open, unsigned long param)
{
	sound_system = open ? open(param) : NULL;
	return sound_system != NULL;
}

void __stdcall BinkSetIOSize(unsigned long io_size)
{
	(void)io_size;
}

static int movie_read(void *context, unsigned long offset, void *buffer, unsigned long size)
{
	struct bink_movie *movie = (struct bink_movie *)context;
	unsigned char *cursor = (unsigned char *)buffer;

	if (SetFilePointer(movie->file, (LONG)offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return 0;
	}
	while (size > 0)
	{
		DWORD count = 0;

		if (!ReadFile(movie->file, cursor, size, &count, NULL) || count == 0)
		{
			return 0;
		}
		cursor += count;
		size -= count;
	}
	return 1;
}

/* reads the tables out of the Xbox executable in the data folder, once */
static int load_tables(void)
{
	HANDLE file;
	DWORD size, count = 0;
	unsigned char *image;
	char error[128];

	if (tables_tried)
	{
		return tables_loaded;
	}
	tables_tried = 1;
	file = CreateFileA("d:\\default.xbe", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		platform_log("Bink movies need default.xbe (of the Xbox disc) in the data folder; there are none");
		return 0;
	}
	size = GetFileSize(file, NULL);
	image = size > 0 && size <= MAXIMUM_EXECUTABLE_SIZE ? (unsigned char *)malloc(size) : NULL;
	if (image && ReadFile(file, image, size, &count, NULL) && count == size)
	{
		tables_loaded = bink_tables_from_xbe(image, size, &tables, error, sizeof(error));
		if (!tables_loaded)
		{
			platform_log("Bink movies: default.xbe is not usable: %s", error);
		}
	}
	free(image);
	CloseHandle(file);
	return tables_loaded;
}

static void audio_start(struct bink_movie *movie)
{
	const struct bink_info *info = bink_get_info(movie->decoder);
	WAVEFORMATEX format;
	DSSTREAMDESC description;
	LPDIRECTSOUND sound = (LPDIRECTSOUND)sound_system;

	if (!sound || !info->sample_rate)
	{
		return;
	}
	movie->audio_buffer = (short *)malloc(AUDIO_CHUNK_FRAMES * (unsigned long)info->channels * sizeof(short));
	if (!movie->audio_buffer)
	{
		return;
	}
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = (WORD)info->channels;
	format.nSamplesPerSec = info->sample_rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = (WORD)(info->channels * 2);
	format.nAvgBytesPerSec = info->sample_rate * format.nBlockAlign;
	memset(&description, 0, sizeof(description));
	description.dwMaxAttachedPackets = 32;
	description.lpwfxFormat = &format;
	if (IDirectSound_CreateSoundStream(sound, &description, &movie->stream, NULL) < 0)
	{
		movie->stream = NULL;
		free(movie->audio_buffer);
		movie->audio_buffer = NULL;
	}
}

/* hands what has been decoded of the audio to the stream */
static void audio_send(struct bink_movie *movie)
{
	const struct bink_info *info = bink_get_info(movie->decoder);

	while (movie->stream && bink_audio_available(movie->decoder))
	{
		unsigned long frames = bink_take_audio(movie->decoder, movie->audio_buffer, AUDIO_CHUNK_FRAMES);
		XMEDIAPACKET packet;

		if (frames == 0)
		{
			break;
		}
		memset(&packet, 0, sizeof(packet));
		packet.pvBuffer = movie->audio_buffer;
		packet.dwMaxSize = frames * (unsigned long)info->channels * 2;
		/* (the stream copies the samples as it takes them) */
		IDirectSoundStream_Process(movie->stream, &packet, NULL);
	}
	if (!movie->stream)
	{
		/* no stream to play it on: drop what was decoded */
		while (bink_audio_available(movie->decoder))
		{
			static short discard[1024];

			bink_take_audio(movie->decoder, discard, 512);
		}
	}
}

HBINK __stdcall BinkOpen(const char *name, unsigned long flags)
{
	struct bink_movie *movie;
	const struct bink_info *info;
	char error[128];
	DWORD size;

	(void)flags;
	if (!name || !config_boolean("game.movies"))
	{
		return NULL;
	}
	if (!load_tables())
	{
		platform_log("Bink video is not available; skipping \"%s\"", name);
		return NULL;
	}
	movie = (struct bink_movie *)calloc(1, sizeof(*movie));
	if (!movie)
	{
		return NULL;
	}
	movie->file = CreateFileA(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (movie->file == INVALID_HANDLE_VALUE)
	{
		free(movie);
		return NULL;
	}
	size = GetFileSize(movie->file, NULL);
	movie->decoder = bink_open(&tables, movie_read, movie, size, error, sizeof(error));
	if (!movie->decoder)
	{
		platform_log("Bink movie \"%s\" cannot be played: %s", name, error);
		CloseHandle(movie->file);
		free(movie);
		return NULL;
	}
	info = bink_get_info(movie->decoder);
	movie->bink.Width = info->width;
	movie->bink.Height = info->height;
	movie->bink.Frames = info->frames;
	movie->bink.FrameNum = 1;
	movie->bink.LastFrameNum = 0;
	movie->period_numerator = info->frame_rate_divisor * 1000;
	movie->period_denominator = info->frame_rate;
	audio_start(movie);
	platform_log("Bink: playing \"%s\" (%lux%lu, %lu frames at %lu/%lu, %s)", name, info->width, info->height,
		info->frames, info->frame_rate, info->frame_rate_divisor, movie->stream ? "with sound" : "without sound");
	return &movie->bink;
}

void __stdcall BinkClose(HBINK bink)
{
	struct bink_movie *movie = (struct bink_movie *)bink;

	if (!movie)
	{
		return;
	}
	if (movie->stream)
	{
		IDirectSoundStream_Flush(movie->stream);
		IDirectSoundStream_Release(movie->stream);
	}
	platform_log("Bink: movie closed at frame %lu of %lu (%lu decoded, %lu skipped) after %lu ms", movie->bink.FrameNum,
		movie->bink.Frames, movie->decoded_frames, movie->skipped_frames, GetTickCount() - movie->start_time);
	free(movie->audio_buffer);
	bink_close(movie->decoder);
	CloseHandle(movie->file);
	free(movie);
}

/* the time frame number (from 1) is due, in milliseconds from the start */
static unsigned long frame_time(const struct bink_movie *movie, unsigned long frame)
{
	return (unsigned long)(((unsigned long long)(frame - 1) * movie->period_numerator) / movie->period_denominator);
}

/* decodes frames until number (from 1) is the last decoded, without
drawing any */
static void decode_to(struct bink_movie *movie, unsigned long frame)
{
	char error[128];

	/* the movie looped: its first frame is a key frame */
	if (frame == 1 && bink_next_frame(movie->decoder) != 0)
	{
		bink_rewind(movie->decoder);
	}
	while (!movie->failed && bink_next_frame(movie->decoder) < frame)
	{
		if (!bink_decode_frame(movie->decoder, error, sizeof(error)))
		{
			platform_log("Bink movie stopped: %s", error);
			movie->failed = 1;
			break;
		}
		movie->decoded_frames++;
		audio_send(movie);
	}
}

long __stdcall BinkDoFrame(HBINK bink)
{
	struct bink_movie *movie = (struct bink_movie *)bink;

	if (!movie->started)
	{
		movie->started = 1;
		movie->start_time = GetTickCount();
	}
	decode_to(movie, movie->bink.FrameNum);
	return 0;
}

void __stdcall BinkNextFrame(HBINK bink)
{
	struct bink_movie *movie = (struct bink_movie *)bink;
	unsigned long skipped = 0;

	movie->bink.LastFrameNum = movie->bink.FrameNum;
	if (movie->failed)
	{
		/* what cannot be decoded ends the movie */
		movie->bink.FrameNum = movie->bink.Frames - 1;
		return;
	}
	if (movie->bink.FrameNum < movie->bink.Frames)
	{
		movie->bink.FrameNum++;
	}
	else
	{
		movie->bink.FrameNum = 1;
		movie->start_time = GetTickCount();
	}
	/* behind the clock by a frame or more: decode the next without
	drawing, so that the one drawn is the one on time */
	while (movie->started && skipped < MAXIMUM_SKIPPED_FRAMES && movie->bink.FrameNum > 1 &&
		movie->bink.FrameNum < movie->bink.Frames - 1 &&
		GetTickCount() - movie->start_time >= frame_time(movie, movie->bink.FrameNum + 1))
	{
		decode_to(movie, movie->bink.FrameNum);
		movie->bink.FrameNum++;
		movie->skipped_frames++;
		skipped++;
	}
}

long __stdcall BinkWait(HBINK bink)
{
	struct bink_movie *movie = (struct bink_movie *)bink;

	if (!movie->started)
	{
		return 0;
	}
	if (GetTickCount() - movie->start_time >= frame_time(movie, movie->bink.FrameNum))
	{
		return 0;
	}
	/* the game spins on this until the frame is due */
	Sleep(1);
	return 1;
}

long __stdcall BinkCopyToBuffer(HBINK bink, void *destination, long destination_pitch,
	unsigned long destination_height, unsigned long destination_x, unsigned long destination_y,
	unsigned long flags)
{
	struct bink_movie *movie = (struct bink_movie *)bink;

	(void)flags;
	if (movie->decoded_frames > 0)
	{
		bink_copy_to_buffer(movie->decoder, destination, destination_pitch, destination_height, destination_x,
			destination_y);
	}
	return 0;
}

/* the statistics the game's debug overlay shows (bink_playback.c's BINKSUMMARY
and BINKREALTIME, as arrays of 32-bit values): only the skipped frames
and the frame count are kept */
void __stdcall BinkGetSummary(HBINK bink, void *summary)
{
	struct bink_movie *movie = (struct bink_movie *)bink;
	unsigned long *fields = (unsigned long *)summary;

	memset(summary, 0, 0x7C);
	/* SkippedFrames, SkippedBlits */
	fields[10] = movie->skipped_frames;
	fields[11] = 0;
	fields[8] = movie->bink.Frames;
}

void __stdcall BinkGetRealtime(HBINK bink, void *realtime, unsigned long frame_count)
{
	struct bink_movie *movie = (struct bink_movie *)bink;
	unsigned long *fields = (unsigned long *)realtime;

	(void)frame_count;
	memset(realtime, 0, 0x38);
	fields[0] = movie->bink.FrameNum;
	/* Frames (the overlay divides by it) */
	fields[3] = movie->decoded_frames ? movie->decoded_frames : 1;
}
