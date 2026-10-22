/* Included by dsound_sdl.c after its stream implementation. Keeping these
 * optional recordings in the existing mixer preserves the user's audio toggle,
 * master volume, output device and guest/host audio-thread handoff. */
#include "performance_audio.h"
#include "hardcore_assets.h"
#include <stdio.h>

enum
{
	PERFORMANCE_AUDIO_REQUIRED_CLIPS = 43,
	PERFORMANCE_AUDIO_MAXIMUM_BYTES = 1024 * 1024,
};

static const char *const performance_audio_cues[] =
{
	"timerbeep",
	"1_minute", "2_minutes", "3_minutes", "4_minutes", "5_minutes",
	"6_minutes", "7_minutes", "8_minutes", "9_minutes", "10_minutes",
	"11_minutes", "12_minutes", "13_minutes", "14_minutes", "15_minutes",
	"16_minutes", "17_minutes", "18_minutes", "19_minutes", "20_minutes",
	"21_minutes", "22_minutes", "23_minutes", "24_minutes", "25_minutes",
	"26_minutes", "27_minutes", "28_minutes", "29_minutes", "30_minutes",
	"30_seconds_left", "20_seconds",
	"10", "9", "8", "7", "6", "5", "4", "3", "2", "1",
	/* Item calls are separate from the general minute/countdown schedule. */
	"rocket", "camo", "overshield",
};

struct performance_audio_clip
{
	unsigned char *samples;
	unsigned long bytes;
	unsigned long sample_rate;
	unsigned short channels;
};
static struct performance_audio_clip performance_audio_clips[
	sizeof(performance_audio_cues) / sizeof(performance_audio_cues[0])];
static BOOL performance_audio_loaded;
static BOOL performance_audio_complete;
static LPDIRECTSOUNDSTREAM performance_audio_stream;
static float performance_audio_requested_volume;

void halo_audio_apply_settings(void)
{
	/* Read config before taking the mixer lock: the audio callback never needs
	to acquire the config lock, and its next buffer uses these new gains. */
	float master = audio_setting_volume("audio.volume");
	float timer = audio_setting_volume("audio.timer_volume");

	pthread_mutex_lock(&mixer_lock);
	master_volume = master;
	if (performance_audio_stream)
		stream_from_interface(performance_audio_stream)->volume = performance_audio_requested_volume * timer;
	pthread_mutex_unlock(&mixer_lock);
}

static unsigned long performance_audio_u32(const unsigned char *value)
{
	return (unsigned long)value[0] | ((unsigned long)value[1] << 8) |
		((unsigned long)value[2] << 16) | ((unsigned long)value[3] << 24);
}

static unsigned short performance_audio_u16(const unsigned char *value)
{
	return (unsigned short)((unsigned short)value[0] | ((unsigned short)value[1] << 8));
}

/* The importer writes canonical 44-byte PCM WAV headers. Refuse other WAV
 * layouts rather than introducing runtime compressed-audio parsers. */
static int performance_audio_take_wav(const unsigned char *file_bytes, unsigned long file_size,
	struct performance_audio_clip *clip)
{
	unsigned long bytes, rate, channels, align;

	if (!file_bytes || file_size < 44 ||
		memcmp(file_bytes, "RIFF", 4) || memcmp(file_bytes + 8, "WAVEfmt ", 8) ||
		performance_audio_u32(file_bytes + 16) != 16 ||
		performance_audio_u16(file_bytes + 20) != WAVE_FORMAT_PCM ||
		performance_audio_u16(file_bytes + 34) != 16 || memcmp(file_bytes + 36, "data", 4))
		return 0;
	channels = performance_audio_u16(file_bytes + 22);
	rate = performance_audio_u32(file_bytes + 24);
	align = performance_audio_u16(file_bytes + 32);
	bytes = performance_audio_u32(file_bytes + 40);
	if ((channels != 1 && channels != 2) || (rate != 22050 && rate != 44100) ||
		align != channels * 2 || performance_audio_u32(file_bytes + 28) != rate * align ||
		!bytes || bytes > PERFORMANCE_AUDIO_MAXIMUM_BYTES || bytes % align ||
		bytes > rate * align * 4 || performance_audio_u32(file_bytes + 4) != bytes + 36 ||
		file_size != bytes + 44)
		return 0;
	clip->samples = malloc(bytes);
	if (!clip->samples)
		return 0;
	memcpy(clip->samples, file_bytes + 44, bytes);
	clip->bytes = bytes;
	clip->sample_rate = rate;
	clip->channels = (unsigned short)channels;
	return 1;
}

static int performance_audio_load_wav(unsigned long index)
{
	struct performance_audio_clip *clip = &performance_audio_clips[index];
	unsigned char *file_bytes = NULL;
	unsigned long file_size;
	char path[4096];
	FILE *file;
	int path_length;
	int taken = 0;
	long told;
	const char *root = platform_data_root();

	if (!root || !root[0])
		return 0;
	path_length = snprintf(path, sizeof(path), "%s/sounds/performance/%s.wav", root, performance_audio_cues[index]);
	if (path_length < 0 || (unsigned long)path_length >= sizeof(path))
		return 0;
	file = fopen(path, "rb");
	if (!file)
		return 0;
	if (fseek(file, 0, SEEK_END) != 0 || (told = ftell(file)) < 44 ||
		(unsigned long)told > PERFORMANCE_AUDIO_MAXIMUM_BYTES + 44)
		goto done;
	file_size = (unsigned long)told;
	file_bytes = malloc(file_size);
	if (!file_bytes || fseek(file, 0, SEEK_SET) != 0 ||
		fread(file_bytes, 1, file_size, file) != file_size)
		goto done;
	taken = performance_audio_take_wav(file_bytes, file_size, clip);

done:
	if (file_bytes)
		free(file_bytes);
	fclose(file);
	return taken;
}

static int performance_audio_load_embedded(unsigned long index)
{
	const unsigned char *bytes;
	unsigned long size;

	if (!hardcore_assets_find(HARDCORE_ASSET_SOUND, performance_audio_cues[index], &bytes, &size))
		return 0;
	return performance_audio_take_wav(bytes, size, &performance_audio_clips[index]);
}

static int performance_audio_load_clip(unsigned long index)
{
	/* A sounds/performance WAV replaces the recording shipped with the game. */
	if (performance_audio_load_wav(index))
		return 1;
	return performance_audio_load_embedded(index);
}

/* The spoken cues are shipped with the game. The user's hardcore map still
has them as Xbox ADPCM when that copy is missing. */
#define PERFORMANCE_AUDIO_TAG_BASE 0x40440000UL
#define PERFORMANCE_AUDIO_TAG_MAXIMUM 0x1700000UL

static int performance_audio_span(unsigned long offset, unsigned long size, unsigned long limit)
{
	return size > 0 && offset < limit && size <= limit - offset;
}

static int performance_audio_place(unsigned long virtual_address, unsigned long tag_size, unsigned long *offset)
{
	if (virtual_address < PERFORMANCE_AUDIO_TAG_BASE)
		return 0;
	*offset = virtual_address - PERFORMANCE_AUDIO_TAG_BASE;
	return *offset < tag_size;
}

static int performance_audio_name_is(const unsigned char *tag, unsigned long tag_size,
	unsigned long virtual_address, const char *name)
{
	unsigned long offset;
	unsigned long length = 0;
	unsigned long expect;

	if (!name || !performance_audio_place(virtual_address, tag_size, &offset))
		return 0;
	expect = (unsigned long)strlen(name);
	while (offset + length < tag_size && tag[offset + length] && length < 256)
		length++;
	if (offset + length >= tag_size || tag[offset + length] || length != expect)
		return 0;
	return memcmp(tag + offset, name, length) == 0;
}

static int performance_audio_cue_name(unsigned long index, char *name, unsigned long name_size)
{
	int wrote;

	if (!strcmp(performance_audio_cues[index], "timerbeep"))
		wrote = snprintf(name, name_size, "sound\\timer\\beeps\\timerbeep");
	else
		wrote = snprintf(name, name_size, "sound\\timer\\cortana\\%s", performance_audio_cues[index]);
	return wrote > 0 && (unsigned long)wrote < name_size;
}

static int performance_audio_decode_tag(FILE *file, const unsigned char *tag, unsigned long tag_size,
	unsigned long file_size, unsigned long address, struct performance_audio_clip *clip)
{
	unsigned long sound_offset;
	unsigned long pitch_offset;
	unsigned long permutation_offset;
	unsigned long pitch_address;
	unsigned long permutation_address;
	unsigned long channels;
	unsigned long rate;
	unsigned long sample_bytes;
	unsigned long sample_file;
	unsigned long frames = 0;
	unsigned long bytes;
	long pitch_count;
	long permutation_count;
	short rate_enum;
	short encoding;
	const unsigned char *samples;
	unsigned char *adpcm = NULL;
	short *pcm;

	if (clip->samples)
		return 1;
	if (!performance_audio_place(address, tag_size, &sound_offset) ||
		!performance_audio_span(sound_offset, 0xA4, tag_size))
		return 0;
	rate_enum = (short)performance_audio_u16(tag + sound_offset + 0x06);
	encoding = (short)performance_audio_u16(tag + sound_offset + 0x6C);
	pitch_count = (long)performance_audio_u32(tag + sound_offset + 0x98);
	pitch_address = performance_audio_u32(tag + sound_offset + 0x9C);
	if (pitch_count != 1 || (rate_enum != 0 && rate_enum != 1) ||
		(encoding != 0 && encoding != 1) ||
		(short)performance_audio_u16(tag + sound_offset + 0x6E) != 1 ||
		!performance_audio_place(pitch_address, tag_size, &pitch_offset) ||
		!performance_audio_span(pitch_offset, 0x48, tag_size))
		return 0;
	permutation_count = (long)performance_audio_u32(tag + pitch_offset + 0x3C);
	permutation_address = performance_audio_u32(tag + pitch_offset + 0x40);
	if (permutation_count != 1 ||
		!performance_audio_place(permutation_address, tag_size, &permutation_offset) ||
		!performance_audio_span(permutation_offset, 0x7C, tag_size) ||
		(short)performance_audio_u16(tag + permutation_offset + 0x28) != 1)
		return 0;
	samples = tag + permutation_offset + 0x40;
	sample_bytes = performance_audio_u32(samples);
	sample_file = performance_audio_u32(samples + 8);
	channels = (unsigned long)encoding + 1;
	rate = rate_enum ? 44100UL : 22050UL;
	if ((performance_audio_u32(samples + 4) & 1) || !sample_bytes ||
		sample_bytes > PERFORMANCE_AUDIO_MAXIMUM_BYTES || sample_bytes % (36UL * channels) ||
		!performance_audio_span(sample_file, sample_bytes, file_size))
		return 0;
	adpcm = malloc(sample_bytes);
	if (!adpcm || fseek(file, (long)sample_file, SEEK_SET) != 0 ||
		fread(adpcm, 1, sample_bytes, file) != sample_bytes)
	{
		free(adpcm);
		return 0;
	}
	pcm = decode_adpcm(adpcm, sample_bytes, channels, &frames);
	free(adpcm);
	if (!pcm || !frames)
	{
		free(pcm);
		return 0;
	}
	bytes = frames * channels * 2;
	if (bytes > PERFORMANCE_AUDIO_MAXIMUM_BYTES || bytes > rate * channels * 2 * 4)
	{
		free(pcm);
		return 0;
	}
	clip->samples = (unsigned char *)pcm;
	clip->bytes = bytes;
	clip->sample_rate = rate;
	clip->channels = (unsigned short)channels;
	return 1;
}

static void performance_audio_load_map(void)
{
	FILE *file;
	unsigned char header[0x18];
	unsigned char *tag = NULL;
	unsigned long file_size;
	unsigned long tag_offset;
	unsigned long tag_size;
	unsigned long table;
	unsigned long count;
	unsigned long index;
	unsigned long cue;
	long told;
	char path[4096];
	char name[128];
	int path_length;
	const char *root = platform_data_root();

	if (!root || !root[0])
		return;
	path_length = snprintf(path, sizeof(path), "%s/hardcore_maps/prisoner.map", root);
	if (path_length < 0 || (unsigned long)path_length >= sizeof(path))
		return;
	file = fopen(path, "rb");
	if (!file)
		return;
	if (fseek(file, 0, SEEK_END) != 0 || (told = ftell(file)) < 0x800)
		goto done;
	file_size = (unsigned long)told;
	if (fseek(file, 0, SEEK_SET) != 0 || fread(header, 1, sizeof(header), file) != sizeof(header) ||
		header[0] != 'd' || header[1] != 'a' || header[2] != 'e' || header[3] != 'h' ||
		(long)performance_audio_u32(header + 4) != 7)
		goto done;
	tag_offset = performance_audio_u32(header + 0x10);
	tag_size = performance_audio_u32(header + 0x14);
	if (tag_offset < 0x800 || tag_size < 0x28 || tag_size > PERFORMANCE_AUDIO_TAG_MAXIMUM ||
		!performance_audio_span(tag_offset, tag_size, file_size))
		goto done;
	tag = malloc(tag_size);
	if (!tag || fseek(file, (long)tag_offset, SEEK_SET) != 0 || fread(tag, 1, tag_size, file) != tag_size)
		goto done;
	count = performance_audio_u32(tag + 0x0C);
	if (count == 0 || count > 65535 || !performance_audio_place(performance_audio_u32(tag), tag_size, &table) ||
		!performance_audio_span(table, count * 0x20UL, tag_size))
		goto done;
	for (cue = 0; cue < sizeof(performance_audio_cues) / sizeof(performance_audio_cues[0]); cue++)
	{
		if (performance_audio_clips[cue].samples || !performance_audio_cue_name(cue, name, sizeof(name)))
			continue;
		for (index = 0; index < count; index++)
		{
			const unsigned char *instance = tag + table + index * 0x20UL;

			if (performance_audio_name_is(tag, tag_size, performance_audio_u32(instance + 0x10), name))
			{
				performance_audio_decode_tag(file, tag, tag_size, file_size,
					performance_audio_u32(instance + 0x14), &performance_audio_clips[cue]);
				break;
			}
		}
	}

done:
	free(tag);
	fclose(file);
}

int halo_performance_audio_available(void)
{
	unsigned long index;
	if (!performance_audio_loaded)
	{
		int required = 1;

		performance_audio_loaded = TRUE;
		for (index = 0; index < sizeof(performance_audio_cues) / sizeof(performance_audio_cues[0]); index++)
		{
			if (!performance_audio_load_clip(index) && index < PERFORMANCE_AUDIO_REQUIRED_CLIPS)
				required = 0;
		}
		if (!required)
			performance_audio_load_map();
		performance_audio_complete = TRUE;
		for (index = 0; index < PERFORMANCE_AUDIO_REQUIRED_CLIPS; index++)
		{
			if (!performance_audio_clips[index].samples)
				performance_audio_complete = FALSE;
		}
		if (!performance_audio_complete)
			platform_log("hardcore: timer callouts were not loaded from the game, sounds/performance, or hardcore_maps/prisoner.map");
	}
	return performance_audio_complete != FALSE;
}

long halo_performance_audio_duration_ticks(const char *cue)
{
	unsigned long index;
	if (!cue || !halo_performance_audio_available()) return 0;
	for (index = 0; index < sizeof(performance_audio_cues) / sizeof(performance_audio_cues[0]); index++)
	{
		struct performance_audio_clip const *clip = &performance_audio_clips[index];
		if (!strcmp(cue, performance_audio_cues[index]) && clip->samples)
		{
			unsigned long bytes_per_second = clip->sample_rate * clip->channels * 2;
			return (long)((clip->bytes * 30 + bytes_per_second - 1) / bytes_per_second);
		}
	}
	return 0;
}

int halo_performance_audio_busy(void)
{
	int busy = 0;
	pthread_mutex_lock(&mixer_lock);
	if (performance_audio_stream)
	{
		struct sdl_stream *stream = stream_from_interface(performance_audio_stream);
		busy = stream->packet_count && !stream->packets[stream->packet_head].finished;
	}
	pthread_mutex_unlock(&mixer_lock);
	return busy;
}

void halo_performance_audio_stop(void)
{
	if (performance_audio_stream)
	{
		stream_release(performance_audio_stream);
		performance_audio_stream = NULL;
	}
}

int halo_performance_audio_play(const char *cue, float volume)
{
	unsigned long index;
	struct performance_audio_clip *clip;
	struct sdl_stream *stream;
	WAVEFORMATEX format;
	DSSTREAMDESC description;
	XMEDIAPACKET packet;
	float timer_volume;
	if (!cue || !(volume >= 0.0f && volume <= 1.0f) || volume == 0.0f ||
		!config_boolean("audio.enabled"))
		return 0;
	for (index = 0; index < sizeof(performance_audio_cues) / sizeof(performance_audio_cues[0]); index++)
		if (!strcmp(cue, performance_audio_cues[index]))
			break;
	if (index == sizeof(performance_audio_cues) / sizeof(performance_audio_cues[0]) ||
		!halo_performance_audio_available())
		return 0;
	clip = &performance_audio_clips[index];
	if (!clip->samples)
		return 0;
	halo_performance_audio_stop();
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = clip->channels;
	format.nSamplesPerSec = clip->sample_rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = clip->channels * 2;
	format.nAvgBytesPerSec = clip->sample_rate * format.nBlockAlign;
	memset(&description, 0, sizeof(description));
	description.lpwfxFormat = &format;
	description.dwMaxAttachedPackets = 1;
	audio_start();
	if (IDirectSound_CreateSoundStream(NULL, &description, &performance_audio_stream, NULL) != DS_OK)
		return 0;
	stream = stream_from_interface(performance_audio_stream);
	timer_volume = audio_setting_volume("audio.timer_volume");
	pthread_mutex_lock(&mixer_lock);
	performance_audio_requested_volume = volume;
	stream->volume = volume * timer_volume;
	pthread_mutex_unlock(&mixer_lock);
	memset(&packet, 0, sizeof(packet));
	packet.pvBuffer = clip->samples;
	packet.dwMaxSize = clip->bytes;
	if (stream_process(performance_audio_stream, &packet, NULL) != S_OK)
	{
		halo_performance_audio_stop();
		return 0;
	}
	return 1;
}
