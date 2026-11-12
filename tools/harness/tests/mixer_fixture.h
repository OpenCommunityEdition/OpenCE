/* Shared stand-ins for source-based DirectSound mixer tests.
 * Adapted in part from OpenH1/OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later. */
#ifndef MIXER_FIXTURE_H
#define MIXER_FIXTURE_H

#include "harness.h"

#include <math.h>
#include <pthread.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
/* The platform startup's crash reporter check; this executable only tests audio. */
int crash_reporter_process(void) { return 1; }
void mcc_memory_log_startup(void) { }
#endif

typedef int BOOL;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef void *LPVOID;
typedef void (*LPFNXMEDIAOBJECTCALLBACK)(void *, void *, DWORD);
typedef struct { void *lpVtbl; } IDirectSoundStream;
typedef long HRESULT;
typedef IDirectSoundStream *LPDIRECTSOUNDSTREAM;
typedef struct
{
	void *pvBuffer;
	DWORD dwMaxSize;
	DWORD *pdwCompletedSize, *pdwStatus;
	void *pContext, *hCompletionEvent;
} XMEDIAPACKET;
typedef const XMEDIAPACKET *LPCXMEDIAPACKET;
#define STDMETHODCALLTYPE
#define WINAPI
#define S_OK 0
#define DS_OK 0
#define E_INVALIDARG (-1)
#define E_OUTOFMEMORY ((HRESULT)0x8007000eL)
#define XMEDIAPACKET_STATUS_PENDING 1
#define XMEDIAPACKET_STATUS_FLUSHED 2
#define DSSTREAMPAUSE_PAUSE 1
#define DSSTREAMPAUSE_RESUME 0
#define SetEvent(event) ((void)(event))
static HRESULT stream_flush(IDirectSoundStream *object);
typedef struct
{
	LONG lRoom, lRoomHF;
	float flRoomRolloffFactor, flDecayTime, flDecayHFRatio;
	LONG lReflections;
	float flReflectionsDelay;
	LONG lReverb;
	float flReverbDelay, flDiffusion, flDensity, flHFReference;
} DSI3DL2LISTENER;
#define DSBVOLUME_MIN (-10000)
#define DS3DMODE_NORMAL 0x00000000
#define DS3DMODE_HEADRELATIVE 0x00000001
#define DS3DMODE_DISABLE 0x00000002

/* the settings (port_config.c): audio.resampler, which a case sets */
static const char *resampler_setting;

static const char *config_string(const char *name)
{
	return !strcmp(name, "audio.resampler") ? resampler_setting : NULL;
}

/* the players' voices (voice_audio.c): none */
static void voice_audio_mix(float *output, unsigned long frames)
{
	(void)output;
	(void)frames;
}

#ifdef UNDER_TEST_I3DL2_SOURCE
typedef struct { LONG lHFLevel; float flLFRatio; } DSI3DL2OBSTRUCTION, DSI3DL2OCCLUSION;
typedef struct
{
	LONG lDirect, lDirectHF, lRoom, lRoomHF;
	float flRoomRolloffFactor;
	DSI3DL2OBSTRUCTION Obstruction;
	DSI3DL2OCCLUSION Occlusion;
} DSI3DL2BUFFER;
typedef const DSI3DL2BUFFER *LPCDSI3DL2BUFFER;
#endif

#include "under_test.inc"

#endif
