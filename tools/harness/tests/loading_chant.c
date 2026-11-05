/* The real mixer and buffer API with no device; locks checked on one thread. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0
#define WINAPI
typedef unsigned char byte;
typedef void *LPDIRECTSOUND, *LPDIRECTSOUNDBUFFER, *LPUNKNOWN;
typedef struct {
    unsigned short wFormatTag, nChannels;
    unsigned long nSamplesPerSec, nAvgBytesPerSec;
    unsigned short nBlockAlign, wBitsPerSample, cbSize;
} WAVEFORMATEX;
typedef struct {unsigned long dwSize, dwFlags, dwBufferBytes; WAVEFORMATEX *lpwfxFormat;} DSBUFFERDESC;
typedef const DSBUFFERDESC *LPCDSBUFFERDESC;
#define DS_OK 0
#define DSERR_INVALIDCALL ((HRESULT)0x88780032L)
#define DSERR_UNSUPPORTED ((HRESULT)0x80004001L)
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_XBOX_ADPCM 0x69
#define DSBPLAY_LOOPING 1
#define DSBFREQUENCY_MIN 188
#define DSBFREQUENCY_MAX 191983
/* libc may expose pthread_mutex_t through stdlib.h on Linux. */
#define pthread_mutex_t chant_mutex_t
typedef int chant_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER 0
static void pthread_mutex_lock(pthread_mutex_t *lock) {assert(!*lock); *lock = 1;}
static void pthread_mutex_unlock(pthread_mutex_t *lock) {assert(*lock); *lock = 0;}
static float effects_volume = 1.0f;
static double config_real(const char *name) {(void)name; return effects_volume;}
static void audio_start(void) {}

/* TYPES */
/* MIXER */
/* BUFFERS */
/* CHANT */

typedef int boolean;
#define NUMBER_OF_PROGRESS_BAR_SOUNDS 4
static struct {LPDIRECTSOUNDBUFFER sounds[4];} progress_bar_globals;
static struct {int active, capture_frame;} progress_bar_mode;
static void SetThreadPriority(void *thread, int priority) {(void)thread; (void)priority;}
static void loading_widescreen_begin(void) { }
static void loading_widescreen_end(void) { }
/* LIFECYCLE */

static double energy(void)
{
    float output[512 * MAXIMUM_OUTPUT_CHANNELS];
    double sum = 0;
    unsigned int i, chunk;
    for (chunk = 0; chunk < 20; chunk++) {
        mix(output, 512);
        for (i = 0; i < 512 * output_channels; i++) sum += output[i] * output[i];
    }
    return sum;
}

static LPDIRECTSOUNDBUFFER make_voice(int adpcm, int channels, unsigned long rate)
{
    WAVEFORMATEX format;
    DSBUFFERDESC description;
    LPDIRECTSOUNDBUFFER voice;
    memset(&format, 0, sizeof(format));
    format.wFormatTag = adpcm ? WAVE_FORMAT_XBOX_ADPCM : WAVE_FORMAT_PCM;
    format.nChannels = (unsigned short)channels;
    format.nSamplesPerSec = rate;
    format.wBitsPerSample = adpcm ? 4 : 16;
    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    description.lpwfxFormat = &format;
    assert(DirectSoundCreateBuffer(&description, &voice) == DS_OK);
    return voice;
}

static void chant_test(void)
{
    LPDIRECTSOUNDBUFFER voice = make_voice(1, 1, 22050);
    assert(IDirectSoundBuffer_SetBufferData(voice, compressed_monk_tone, sizeof(compressed_monk_tone)) == DS_OK);
    assert(IDirectSoundBuffer_SetLoopRegion(voice, 0, sizeof(compressed_monk_tone)) == DS_OK);
    assert(IDirectSoundBuffer_SetPitch(voice, -4681) == DS_OK);
    assert(IDirectSoundBuffer_SetVolume(voice, -1500) == DS_OK);
    assert(IDirectSoundBuffer_Play(voice, 0, 0, DSBPLAY_LOOPING) == DS_OK);
    assert(energy() > 0.001);
    assert(IDirectSoundBuffer_Stop(voice) == DS_OK);
    energy(); /* drain the output limiter */
    assert(energy() == 0);
    assert(IDirectSoundBuffer_Release(voice) == 0);
}

/* A second of the dry front pair, bypassing only the limiter's delay. */
static double measure(unsigned int *crossings)
{
    float output[480 * MAXIMUM_OUTPUT_CHANNELS], previous = 0;
    unsigned int chunk, i;
    double sum = 0;
    *crossings = 0;
    for (chunk = 0; chunk < 100; chunk++) {
        memset(output, 0, sizeof(output));
        pthread_mutex_lock(&mixer_lock);
        mix_buffers(output, 480);
        pthread_mutex_unlock(&mixer_lock);
        for (i = 0; i < 480; i++) {
            float sample = output[i * output_channels];
            if (previous <= 0 && sample > 0) (*crossings)++;
            previous = sample;
            sum += sample * sample;
        }
    }
    return sum;
}

static void controls_test(void)
{
    short samples[480];
    unsigned int i, crossings;
    double full, quieter;
    LPDIRECTSOUNDBUFFER voice = make_voice(0, 1, 22050);
    for (i = 0; i < 480; i++) samples[i] = (short)(10000 * sin(2.0 * 3.14159265358979 * i / 480));
    assert(IDirectSoundBuffer_SetBufferData(voice, samples, sizeof(samples)) == DS_OK);
    assert(IDirectSoundBuffer_Play(voice, 0, 0, DSBPLAY_LOOPING) == DS_OK);
    measure(&crossings);
    assert(crossings >= 45 && crossings <= 47); /* original sample rate */
    assert(IDirectSoundBuffer_SetPitch(voice, 0) == DS_OK); /* absolute 48 kHz */
    measure(&crossings);
    full = measure(&crossings);
    assert(crossings == 100);
    assert(IDirectSoundBuffer_SetPitch(voice, 4096) == DS_OK);
    measure(&crossings);
    assert(crossings >= 199 && crossings <= 201);
    assert(IDirectSoundBuffer_SetPitch(voice, -4096) == DS_OK);
    measure(&crossings);
    assert(crossings >= 49 && crossings <= 51);
    assert(IDirectSoundBuffer_SetPitch(voice, 0) == DS_OK);
    assert(IDirectSoundBuffer_SetVolume(voice, -2000) == DS_OK);
    measure(&crossings);
    quieter = measure(&crossings);
    assert(fabs(quieter / full - 0.01) < 0.0001);
    effects_volume = 0;
    assert(IDirectSoundBuffer_SetVolume(voice, 0) == DS_OK);
    measure(&crossings);
    assert(measure(&crossings) < 1e-20);
    effects_volume = 1;
    assert(IDirectSoundBuffer_SetVolume(voice, 0) == DS_OK);
    master_volume = 0;
    assert(measure(&crossings) < 1e-20);
    master_volume = 1;
    assert(IDirectSoundBuffer_SetVolume(voice, DSBVOLUME_MIN) == DS_OK);
    assert(measure(&crossings) < 1e-20);
    assert(IDirectSoundBuffer_Release(voice) == 0);
}

static void buffers_test(void)
{
    short samples[8] = {1000, -1000, 2000, -2000, 3000, -3000, 4000, -4000};
    float output[48 * MAXIMUM_OUTPUT_CHANNELS];
    unsigned int i, crossings;
    LPDIRECTSOUNDBUFFER voice = make_voice(0, 2, 48000);
    assert(IDirectSoundBuffer_SetBufferData(voice, samples, sizeof(samples)) == DS_OK);
    memset(samples, 0, sizeof(samples)); /* caller's memory is no longer needed */
    assert(IDirectSoundBuffer_SetLoopRegion(voice, 4, 8) == DS_OK);
    assert(IDirectSoundBuffer_SetLoopRegion(voice, 4, 16) == DSERR_INVALIDCALL);
    assert(IDirectSoundBuffer_SetCurrentPosition(voice, 16) == DSERR_INVALIDCALL);
    assert(IDirectSoundBuffer_SetCurrentPosition(voice, 5) == DSERR_INVALIDCALL);
    assert(IDirectSoundBuffer_SetBufferData(voice, NULL, 4) == DSERR_INVALIDCALL);
    assert(IDirectSoundBuffer_Play(voice, 0, 0, DSBPLAY_LOOPING) == DS_OK);
    output_channels = 6;
    memset(output, 0, sizeof(output));
    mix_buffers(output, 48);
    for (i = 0; i < 48; i++) {
        assert(output[i * 6] > 0 && output[i * 6] == -output[i * 6 + 1]);
        assert(!output[i * 6 + 2] && !output[i * 6 + 3] && !output[i * 6 + 4] && !output[i * 6 + 5]);
    }
    assert(IDirectSoundBuffer_Stop(voice) == DS_OK);
    assert(IDirectSoundBuffer_SetCurrentPosition(voice, 0) == DS_OK);
    assert(IDirectSoundBuffer_Play(voice, 0, 0, 0) == DS_OK);
    assert(measure(&crossings) > 0);
    assert(measure(&crossings) == 0); /* one-shot has ended */
    assert(IDirectSoundBuffer_Play(voice, 0, 0, DSBPLAY_LOOPING) == DS_OK);
    assert(IDirectSoundBuffer_SetBufferData(voice, NULL, 0) == DS_OK);
    assert(measure(&crossings) == 0);
    assert(IDirectSoundBuffer_Release(voice) == 0);
    voice = make_voice(1, 1, 22050);
    assert(IDirectSoundBuffer_SetBufferData(voice, compressed_monk_tone, 35) == DSERR_INVALIDCALL);
    assert(IDirectSoundBuffer_SetBufferData(voice, compressed_monk_tone, sizeof(compressed_monk_tone)) == DS_OK);
    assert(IDirectSoundBuffer_SetLoopRegion(voice, 0xfffffff0UL, 36) == DSERR_INVALIDCALL);
    assert(IDirectSoundBuffer_SetCurrentPosition(voice, 0xfffffff0UL) == DSERR_INVALIDCALL);
    assert(IDirectSoundBuffer_Play(voice, 0, 0, DSBPLAY_LOOPING) == DS_OK);
    assert(IDirectSoundBuffer_Release(voice) == 0); /* remove while playing */
    assert(!buffers && measure(&crossings) == 0);
}

static void lifecycle_test(void)
{
    static const long pitches[] = {-4681, -6386, -3999, -8773};
    unsigned int i;
    for (i = 0; i < 4; i++) {
        LPDIRECTSOUNDBUFFER voice = make_voice(1, 1, 22050);
        progress_bar_globals.sounds[i] = voice;
        assert(IDirectSoundBuffer_SetBufferData(voice, compressed_monk_tone, sizeof(compressed_monk_tone)) == DS_OK);
        assert(IDirectSoundBuffer_SetPitch(voice, pitches[i]) == DS_OK);
    }
    for (i = 0; i < 2; i++) {
        progress_bar_begin(FALSE);
        assert(progress_bar_mode.active && energy() > 1e-6);
        progress_bar_end(); /* both success and failure use this boundary */
        assert(!progress_bar_mode.active);
        energy();
        assert(energy() == 0);
    }
    progress_bar_begin(TRUE);
    progress_bar_dispose();
    energy();
    assert(!buffers && energy() == 0);
    progress_bar_dispose();
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    resampler_initialize();
    resampler_phases_initialize();
    reverb_initialize();
    if (!strcmp(argv[1], "chant")) chant_test();
    else if (!strcmp(argv[1], "controls")) controls_test();
    else if (!strcmp(argv[1], "buffers")) buffers_test();
    else if (!strcmp(argv[1], "lifecycle")) lifecycle_test();
    else return 1;
    return 0;
}
