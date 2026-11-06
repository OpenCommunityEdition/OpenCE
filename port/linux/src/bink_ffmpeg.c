/*
BINK_FFMPEG.C

The Bink video SDK entry points bink_playback.c uses, decoded with FFmpeg
instead of the proprietary RAD SDK (bink_null.c keeps the stubs for builds
without FFmpeg; HALO_BINK_FFMPEG picks this one).

Why here and not in the Android app: the game already drives the movies
(bink_playback.c opens them, waits on their clock, copies every frame into a
locked texture and the renderer draws it), and its sound is the port's own
DirectSound. Implementing the SDK's own API means every port gets the movies
through the renderer and at the game's volume, with no per-platform player.

The contract (from bink_playback.c): the handle's first five members are read
directly by the game, and BinkCopyToBuffer writes into a linear X8R8G8B8
texture it has locked. The sound goes back the other way: the game hands Bink
its own DirectSound (BinkSetSoundSystem, BinkOpenDirectSound) and the movie's
decoded sound is queued into a stream of it, as the game's own sounds are.
*/

#ifdef HALO_BINK_FFMPEG

#include "platform.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef void *(__stdcall *rad_memory_allocate_proc)(unsigned long size);
typedef void (__stdcall *rad_memory_free_proc)(void *memory);
typedef void *(__stdcall *bink_sound_system_open_proc)(unsigned long param);

/* the five the game reads, in its order; the rest is ours */
struct BINK
{
	unsigned long Width;
	unsigned long Height;
	unsigned long Frames;
	unsigned long FrameNum;
	unsigned long LastFrameNum;

	AVFormatContext *format;
	AVCodecContext *video;
	AVCodecContext *audio;
	int video_stream;
	int audio_stream;
	AVPacket *packet;
	AVFrame *decoded;
	AVFrame *rgba;
	AVFrame *audio_frame;
	struct SwsContext *to_rgba;
	unsigned char *pixels;      /* one frame, X8R8G8B8, Width x Height */
	long pixels_pitch;
	int decoded_frame;          /* the frame in pixels now */
	int submitted;              /* the frame BinkDoFrame was asked for */
	uint64_t start_ticks;       /* when the movie started, for BinkWait */
	double frame_seconds;
	SwrContext *to_sound;
	LPDIRECTSOUNDSTREAM stream;  /* one of the game's DirectSound streams */
	uint8_t *audio_out;
	int audio_out_size;
	int sound_system_ready;
};

typedef struct BINK *HBINK;

void __stdcall RADSetMemory(rad_memory_allocate_proc allocate, rad_memory_free_proc release)
{
	(void)allocate;
	(void)release;
}

void __stdcall BinkSetIOSize(unsigned long io_size)
{
	(void)io_size;
}

/* BINK OPEN DIRECT SOUND

The game hands Bink its DirectSound (the port's own software mixer,
dsound_sdl.c, the one its own sounds play through) before opening a movie:
the movie's decoded sound is queued into a stream of it, so it is mixed
with the game's sounds and at the game's volume (port_config's
audio.volume). A game with no DirectSound plays the movie without sound,
as it did before there was a decoder. */
static LPDIRECTSOUND bink_dsound;

void *__stdcall BinkOpenDirectSound(unsigned long param)
{
	bink_dsound = (LPDIRECTSOUND)param;
	return param ? (void *)param : NULL;
}

long __stdcall BinkSetSoundSystem(bink_sound_system_open_proc open, unsigned long param)
{
	/* (the game's check: true when the sound system was taken) */
	return (open && open(param)) ? 1 : 0;
}

/* the movie's sound, as a stream of the game's DirectSound: 16-bit PCM,
stereo, at the movie's own rate, the format the port's mixer takes for a
voice (it resamples it as it does the game's sounds) */
static int bink_audio_open(struct BINK *bink)
{
	DSSTREAMDESC description;
	tWAVEFORMATEX format;

	if (!bink_dsound || !bink->audio || bink->audio->sample_rate <= 0)
	{
		platform_log("Bink video: no DirectSound from the game; playing the movie without sound");
		return 0;
	}
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = (unsigned long)bink->audio->sample_rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = (unsigned short)(format.nChannels * format.wBitsPerSample / 8);
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	memset(&description, 0, sizeof(description));
	/* (a few packets ahead is enough: the game retires one per frame) */
	description.dwMaxAttachedPackets = 8;
	description.lpwfxFormat = &format;
	if (IDirectSound_CreateSoundStream(bink_dsound, &description, &bink->stream, NULL) < 0 ||
		!bink->stream)
	{
		platform_log("Bink video: the game's DirectSound refused the movie's stream; playing it without sound");
		bink->stream = NULL;
		return 0;
	}
	return 1;
}

/* The frame the audio decoder just produced, queued into the game's stream
as the game queues its own sounds: from here the mixer owns the packet (it
resamples it, mixes it with the game's sounds and retires it once played,
from DirectSoundDoWork, which the game calls every frame). */
static void bink_audio_queue(struct BINK *bink)
{
	unsigned long status;
	XMEDIAPACKET packet;
	uint8_t *out[1];
	int samples = bink->audio_frame->nb_samples * 2;  /* (the stream is stereo) */
	int got;

	if (samples <= 0 || !bink->to_sound || !bink->stream)
		return;
	if (samples * (int)sizeof(short) > bink->audio_out_size)
	{
		uint8_t *grown = realloc(bink->audio_out, (size_t)samples * sizeof(short));

		if (!grown)
			return;
		bink->audio_out = grown;
		bink->audio_out_size = samples * (int)sizeof(short);
	}
	out[0] = bink->audio_out;
	got = swr_convert(bink->to_sound, out, samples,
		(const uint8_t **)bink->audio_frame->extended_data, bink->audio_frame->nb_samples);
	if (got <= 0)
		return;
	if (IDirectSoundStream_GetStatus(bink->stream, &status) < 0 ||
		!(status & XMO_STATUSF_ACCEPT_INPUT_DATA))
	{
		/* (the game has not played out a queued packet yet: its frame clock
		is ahead of the movie's sound, which an underrun here would sound
		like; the frame is dropped rather than piled up late) */
		return;
	}
	memset(&packet, 0, sizeof(packet));
	packet.pvBuffer = bink->audio_out;
	packet.dwMaxSize = (unsigned long)(got * 2 * (int)sizeof(short));
	if (IDirectSoundStream_Process(bink->stream, &packet, NULL) < 0)
		platform_log("Bink video: the game's DirectSound refused an audio packet");
}


HBINK __stdcall BinkOpen(const char *name, unsigned long flags)
{
	struct BINK *bink;
	const AVCodec *codec;
	double fps;
	char path[1024];

	(void)flags;
	/* the movie's name is the game's own path ("d:\bink\intro_es.bik"): the
	port's file layer is what knows where the game's data is (the disc's
	folder on Linux, the app's assets on Android) */
	if (name && name[0] && name[1] == ':')
		platform_translate_path(name, path, sizeof(path));
	else if (name)
		snprintf(path, sizeof(path), "%s", name);
	else
		return NULL;
	if (!name)
		return NULL;
	bink = calloc(1, sizeof(*bink));
	if (!bink)
		return NULL;
	bink->video_stream = -1;
	bink->audio_stream = -1;
	bink->decoded_frame = -1;
	bink->submitted = -1;
	if (avformat_open_input(&bink->format, path, NULL, NULL) < 0)
	{
		platform_log("Bink video: \"%s\" cannot be opened", path);
		free(bink);
		return NULL;
	}
	platform_log("Bink video: %s opened (%d streams)", path, bink->format->nb_streams);
	if (avformat_find_stream_info(bink->format, NULL) < 0)
	{
		platform_log("Bink video: %s: find_stream_info failed", path);
		goto fail;
	}
	bink->video_stream = av_find_best_stream(bink->format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (bink->video_stream < 0)
	{
		platform_log("Bink video: %s: no video stream", path);
		goto fail;
	}
	codec = avcodec_find_decoder(bink->format->streams[bink->video_stream]->codecpar->codec_id);
	if (!codec)
	{
		platform_log("Bink video: %s: no decoder for codec %d", path,
			(int)bink->format->streams[bink->video_stream]->codecpar->codec_id);
		goto fail;
	}
	bink->video = avcodec_alloc_context3(codec);
	if (!bink->video)
		goto fail;
	if (avcodec_parameters_to_context(bink->video, bink->format->streams[bink->video_stream]->codecpar) < 0)
		goto fail;
	if (avcodec_open2(bink->video, codec, NULL) < 0)
	{
		platform_log("Bink video: %s: open2 (%dx%d) failed", path, bink->video->width, bink->video->height);
		goto fail;
	}

	bink->Width = (unsigned long)bink->video->width;
	bink->Height = (unsigned long)bink->video->height;
	fps = av_guess_frame_rate(bink->format, bink->format->streams[bink->video_stream], NULL).num /
		(double)av_guess_frame_rate(bink->format, bink->format->streams[bink->video_stream], NULL).den;
	bink->frame_seconds = fps > 0.0 ? 1.0 / fps : 1.0 / 15.0;
	bink->Frames = bink->format->streams[bink->video_stream]->nb_frames > 0 ?
		(unsigned long)bink->format->streams[bink->video_stream]->nb_frames :
		(unsigned long)(bink->format->duration / (AV_TIME_BASE * bink->frame_seconds) + 0.5);

	bink->pixels_pitch = (long)bink->Width * 4;
	bink->pixels = malloc((size_t)bink->pixels_pitch * bink->Height);
	if (!bink->pixels)
		goto fail;
	memset(bink->pixels, 0, (size_t)bink->pixels_pitch * bink->Height);
	bink->packet = av_packet_alloc();
	bink->decoded = av_frame_alloc();
	bink->rgba = av_frame_alloc();
	bink->audio_frame = av_frame_alloc();
	if (!bink->packet || !bink->decoded || !bink->rgba || !bink->audio_frame)
		goto fail;
	bink->rgba->format = AV_PIX_FMT_BGRA;
	bink->rgba->width = (int)bink->Width;
	bink->rgba->height = (int)bink->Height;
	if (av_frame_get_buffer(bink->rgba, 0) < 0)
		goto fail;
	bink->to_rgba = sws_getContext((int)bink->Width, (int)bink->Height, bink->video->pix_fmt,
		(int)bink->Width, (int)bink->Height, AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
	if (!bink->to_rgba)
		goto fail;

	/* the movie's own sound, if the disc's file carries one */
	bink->audio_stream = av_find_best_stream(bink->format, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (bink->audio_stream >= 0)
	{
		const AVCodec *acodec = avcodec_find_decoder(
			bink->format->streams[bink->audio_stream]->codecpar->codec_id);

		if (acodec)
		{
			bink->audio = avcodec_alloc_context3(acodec);
			if (bink->audio &&
				avcodec_parameters_to_context(bink->audio,
					bink->format->streams[bink->audio_stream]->codecpar) >= 0 &&
				avcodec_open2(bink->audio, acodec, NULL) >= 0)
			{
				AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
				AVChannelLayout in_layout = bink->audio->ch_layout;

				if (in_layout.nb_channels <= 0)
					av_channel_layout_default(&in_layout, 2);
				if (swr_alloc_set_opts2(&bink->to_sound, &stereo, AV_SAMPLE_FMT_S16,
						bink->audio->sample_rate, &in_layout, bink->audio->sample_fmt,
						bink->audio->sample_rate, 0, NULL) >= 0 && swr_init(bink->to_sound) >= 0)
				{
					bink->sound_system_ready = bink_audio_open(bink) != 0;
				}
			}
		}
	}
	bink->start_ticks = SDL_GetTicks();
	return bink;

fail:
	platform_log("Bink video: \"%s\" cannot be decoded", name);
	if (bink)
	{
		if (bink->format)
			avformat_close_input(&bink->format);
		avcodec_free_context(&bink->video);
		avcodec_free_context(&bink->audio);
		av_packet_free(&bink->packet);
		av_frame_free(&bink->decoded);
		av_frame_free(&bink->rgba);
		free(bink->pixels);
		free(bink);
	}
	return NULL;
}

/* The audio packet into the game's stream. The decoder holds a frame back,
so it is drained after every packet (that frame is the movie's sound):
sending first and asking later loses it, because the decoder answers EAGAIN
while it still has one to give, and the packet would be dropped unheard. */
static void bink_audio_decode(struct BINK *bink)
{
	int sent = avcodec_send_packet(bink->audio, bink->packet);

	if (sent < 0 && sent != AVERROR(EAGAIN))
		return;
	while (avcodec_receive_frame(bink->audio, bink->audio_frame) >= 0)
		bink_audio_queue(bink);
	if (sent == AVERROR(EAGAIN) && avcodec_send_packet(bink->audio, bink->packet) >= 0)
		/* (already drained: the packet fits now) */
		while (avcodec_receive_frame(bink->audio, bink->audio_frame) >= 0)
			bink_audio_queue(bink);
}

/* the next video frame, decoded into pixels: the decoder may hold frames the
game has not asked for yet, so it is drained first and fed only when it asks
for more (a frame per packet, as FFmpeg's decoders delay by one) */
static int bink_next_video_frame(struct BINK *bink)
{
	for (;;)
	{
		int received = avcodec_receive_frame(bink->video, bink->decoded);

		if (received == 0)
		{
			sws_scale(bink->to_rgba, (const uint8_t *const *)bink->decoded->data,
				bink->decoded->linesize, 0, (int)bink->Height, bink->rgba->data, bink->rgba->linesize);
			memcpy(bink->pixels, bink->rgba->data[0], (size_t)bink->pixels_pitch * bink->Height);
			bink->decoded_frame++;
			return 1;
		}
		if (received == AVERROR_EOF)
			return 0;

		/* (EAGAIN: it wants another packet) */
		for (;;)
		{
			if (av_read_frame(bink->format, bink->packet) < 0)
				return 0;
			if (bink->packet->stream_index == bink->video_stream)
			{
				avcodec_send_packet(bink->video, bink->packet);
				av_packet_unref(bink->packet);
				break;
			}
			if (bink->packet->stream_index == bink->audio_stream && bink->sound_system_ready)
				bink_audio_decode(bink);
			av_packet_unref(bink->packet);
		}
	}
}


/* decodes into pixels until the frame the game asked for is the one there */
static void bink_decode_to(struct BINK *bink, int frame)
{
	while (bink->decoded_frame < frame)
		if (!bink_next_video_frame(bink))
			return;
}

long __stdcall BinkDoFrame(HBINK bink)
{
	if (!bink)
		return 1;
	bink_decode_to(bink, (int)bink->FrameNum);
	bink->submitted = (int)bink->FrameNum;
	return 0;
}

long __stdcall BinkWait(HBINK bink)
{
	uint64_t due;
	uint64_t now;

	if (!bink)
		return 0;
	due = bink->start_ticks + (uint64_t)(bink->FrameNum * bink->frame_seconds * 1000.0);
	now = SDL_GetTicks();
	if (now >= due)
		return 0;
	/* (the game polls this; a short nap keeps its frame loop from spinning) */
	SDL_Delay((uint32_t)(due - now > 8 ? 8 : due - now));
	return (long)(due - now);
}

void __stdcall BinkNextFrame(HBINK bink)
{
	if (!bink)
		return;
	bink->LastFrameNum = bink->FrameNum;
	bink->FrameNum++;
}

long __stdcall BinkCopyToBuffer(HBINK bink, void *destination, long destination_pitch,
	unsigned long destination_height, unsigned long destination_x, unsigned long destination_y,
	unsigned long flags)
{
	unsigned long row;
	const unsigned char *source;
	unsigned char *target;
	unsigned long copy_width;

	(void)flags;
	if (!bink || !destination || !bink->pixels)
		return 1;
	if (bink->decoded_frame < (int)bink->submitted)
		bink_decode_to(bink, bink->submitted);
	copy_width = bink->Width;
	if (destination_x + copy_width > (unsigned long)(destination_pitch / 4))
		copy_width = (unsigned long)(destination_pitch / 4) - destination_x;
	if (destination_y >= destination_height)
		return 0;
	if (destination_y + bink->Height > destination_height)
		row = destination_height - destination_y;
	else
		row = bink->Height;
	source = bink->pixels;
	target = (unsigned char *)destination + destination_y * destination_pitch + destination_x * 4;
	for (unsigned long y = 0; y < row; y++)
	{
		memcpy(target, source, copy_width * 4);
		target += destination_pitch;
		source += bink->pixels_pitch;
	}
	return 0;
}

void __stdcall BinkGetSummary(HBINK bink, void *summary)
{
	unsigned long *fields = summary;

	if (!fields)
		return;
	/* (Width, Height, TotalTime and a few more come first; the game's debug
	log reads the frame numbers) */
	fields[0] = bink ? bink->Width : 0;
	fields[1] = bink ? bink->Height : 0;
	fields[8] = bink ? bink->FrameNum : 0;
	fields[9] = bink ? bink->Frames : 0;
}

void __stdcall BinkGetRealtime(HBINK bink, void *realtime, unsigned long frame_count)
{
	unsigned long *fields = realtime;

	(void)frame_count;
	if (!fields)
		return;
	fields[0] = bink ? bink->FrameNum : 0;
	fields[1] = 1;
	fields[3] = bink ? bink->Frames : 0;
}

void __stdcall BinkClose(HBINK bink)
{
	if (!bink)
		return;
	if (bink->stream)
		/* (Release flushes what it has queued and takes it out of the mixer) */
		IDirectSoundStream_Release(bink->stream);
	swr_free(&bink->to_sound);
	sws_freeContext(bink->to_rgba);
	av_frame_free(&bink->rgba);
	av_frame_free(&bink->decoded);
	av_packet_free(&bink->packet);
	avcodec_free_context(&bink->video);
	avcodec_free_context(&bink->audio);
	if (bink->format)
		avformat_close_input(&bink->format);
	free(bink->audio_out);
	free(bink->pixels);
	free(bink);
}


#endif /* HALO_BINK_FFMPEG */
