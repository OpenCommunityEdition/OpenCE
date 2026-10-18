/* CPU cost of Apple's OpenAL mixing looping voices, for comparison with the
   port's own mixer. Plays quietly through the default output.
   usage: mac_openal_bench voices seconds rate channels low|high [pitch
          [none|min|low|medium|high reverb [obstruction_db]]]
   Prints process CPU time (all threads, including OpenAL's) per second of
   playback. Build: gcc -std=gnu89 -O2 mac_openal_bench.c -framework OpenAL */
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define MAXIMUM_VOICES 64
#define RENDERING_QUALITY_HIGH 0x72716869 /* 'rqhi' */
#define RENDERING_QUALITY_LOW 0x72646c6f  /* 'rdlo' */

typedef ALvoid (*rendering_quality_proc)(const ALint value);
typedef ALenum (*asa_listener_proc)(const ALuint property, ALvoid *data, ALuint size);
typedef ALenum (*asa_source_proc)(const ALuint property, ALuint source, ALvoid *data, ALuint size);

#define ASA_REVERB_ON 0x72766f6e          /* 'rvon' */
#define ASA_REVERB_GLOBAL_LEVEL 0x7276676c /* 'rvgl' */
#define ASA_REVERB_ROOM_TYPE 0x72767274    /* 'rvrt' */
#define ASA_REVERB_QUALITY 0x72767174      /* 'rvqt' */
#define ASA_REVERB_SEND_LEVEL 0x7276736c   /* 'rvsl' */
#define ASA_OBSTRUCTION 0x6f627374         /* 'obst' */

static double seconds_of(struct timeval value)
{
    return (double)value.tv_sec + (double)value.tv_usec / 1000000.0;
}

static double cpu_seconds(void)
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return seconds_of(usage.ru_utime) + seconds_of(usage.ru_stime);
}

static double wall_seconds(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return seconds_of(now);
}

int main(int argc, char **argv)
{
    ALCdevice *device;
    ALCcontext *context;
    ALuint buffer, sources[MAXIMUM_VOICES];
    short *samples;
    unsigned long seed = 12345, count, i;
    int voices, rate, channels, low, v;
    double seconds, pitch, cpu_start, wall_start, cpu_used, wall_used;
    rendering_quality_proc set_quality;
    asa_listener_proc asa_listener = NULL;
    asa_source_proc asa_source = NULL;
    const char *reverb = argc > 7 ? argv[7] : "none";
    float obstruction = argc > 8 ? (float)atof(argv[8]) : 0.0f;
    ALint reverb_quality = -1;

    if (argc < 6) {
        fprintf(stderr, "usage: %s voices seconds rate channels low|high [pitch]\n", argv[0]);
        return 2;
    }
    voices = atoi(argv[1]);
    seconds = atof(argv[2]);
    rate = atoi(argv[3]);
    channels = atoi(argv[4]);
    low = strcmp(argv[5], "low") == 0;
    pitch = argc > 6 ? atof(argv[6]) : 1.0;
    if (voices < 0 || voices > MAXIMUM_VOICES || seconds <= 0.0 || rate <= 0 ||
        (channels != 1 && channels != 2)) {
        fprintf(stderr, "bad arguments\n");
        return 2;
    }
    device = alcOpenDevice(NULL);
    if (!device) { fprintf(stderr, "no OpenAL device\n"); return 1; }
    context = alcCreateContext(device, NULL);
    if (!context || !alcMakeContextCurrent(context)) { fprintf(stderr, "no context\n"); return 1; }
    /* (Apple's extension needs a current context) */
    set_quality = (rendering_quality_proc)alcGetProcAddress(device,
        (const ALCchar *)"alcMacOSXRenderingQuality");
    if (set_quality)
        set_quality(low ? RENDERING_QUALITY_LOW : RENDERING_QUALITY_HIGH);
    alListenerf(AL_GAIN, 0.05f);
    if (strcmp(reverb, "min") == 0) reverb_quality = 0;
    else if (strcmp(reverb, "low") == 0) reverb_quality = 0x20;
    else if (strcmp(reverb, "medium") == 0) reverb_quality = 0x40;
    else if (strcmp(reverb, "high") == 0) reverb_quality = 0x60;
    asa_listener = (asa_listener_proc)alcGetProcAddress(device, (const ALCchar *)"alcASASetListener");
    asa_source = (asa_source_proc)alcGetProcAddress(device, (const ALCchar *)"alcASASetSource");
    printf("openal bench: asa listener=%d source=%d\n", asa_listener != NULL, asa_source != NULL);
    if (reverb_quality >= 0 && asa_listener) {
        ALuint on_uint = 1;
        ALboolean on_bool = AL_TRUE;
        ALint room = 1, quality = reverb_quality;
        ALfloat level = 0.0f;
        printf("openal bench: reverb_on uint=%d", (int)asa_listener(ASA_REVERB_ON, &on_uint, sizeof(on_uint)));
        printf(" bool=%d", (int)asa_listener(ASA_REVERB_ON, &on_bool, sizeof(on_bool)));
        printf(" room=%d", (int)asa_listener(ASA_REVERB_ROOM_TYPE, &room, sizeof(room)));
        printf(" quality=%d", (int)asa_listener(ASA_REVERB_QUALITY, &quality, sizeof(quality)));
        printf(" level=%d\n", (int)asa_listener(ASA_REVERB_GLOBAL_LEVEL, &level, sizeof(level)));
    }

    /* one second of quiet noise, so the mixer cannot skip silence */
    count = (unsigned long)rate * (unsigned long)channels;
    samples = (short *)malloc(count * sizeof(short));
    if (!samples) return 1;
    for (i = 0; i < count; i++) {
        seed = seed * 1664525UL + 1013904223UL;
        samples[i] = (short)((long)((seed >> 16) & 0xffff) - 32768L) / 8;
    }
    alGenBuffers(1, &buffer);
    alBufferData(buffer, channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
        samples, (ALsizei)(count * sizeof(short)), rate);
    if (voices)
        alGenSources(voices, sources);
    for (v = 0; v < voices; v++) {
        double angle = 6.2831853 * (double)v / (double)(voices ? voices : 1);
        alSourcei(sources[v], AL_BUFFER, (ALint)buffer);
        alSourcei(sources[v], AL_LOOPING, AL_TRUE);
        alSourcei(sources[v], AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(sources[v], AL_POSITION, (ALfloat)sin(angle), 0.0f, (ALfloat)-cos(angle));
        alSourcef(sources[v], AL_PITCH, (ALfloat)pitch);
        alSourcef(sources[v], AL_GAIN, 1.0f);
        if (asa_source && reverb_quality >= 0) {
            ALfloat send = 0.3f;
            ALenum result = asa_source(ASA_REVERB_SEND_LEVEL, sources[v], &send, sizeof(send));
            if (v == 0) printf("openal bench: send=%d", (int)result);
            result = asa_source(ASA_OBSTRUCTION, sources[v], &obstruction, sizeof(obstruction));
            if (v == 0) printf(" obstruction=%d\n", (int)result);
        }
        alSourcePlay(sources[v]);
    }
    if (alGetError() != AL_NO_ERROR) { fprintf(stderr, "OpenAL setup error\n"); return 1; }
    usleep(500000);
    cpu_start = cpu_seconds();
    wall_start = wall_seconds();
    usleep((useconds_t)(seconds * 1000000.0));
    cpu_used = cpu_seconds() - cpu_start;
    wall_used = wall_seconds() - wall_start;
    printf("openal bench: voices=%d rate=%d channels=%d quality=%s pitch=%.3f quality_proc=%d "
        "reverb=%s obstruction=%.0f wall=%.2fs cpu=%.3fs cpu_percent=%.1f per_voice_percent=%.2f\n",
        voices, rate, channels, low ? "low" : "high", pitch, set_quality != NULL, reverb, obstruction,
        wall_used, cpu_used, 100.0 * cpu_used / wall_used,
        voices ? 100.0 * cpu_used / wall_used / voices : 0.0);
    if (voices) {
        alSourceStopv(voices, sources);
        alDeleteSources(voices, sources);
    }
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
    free(samples);
    return 0;
}
