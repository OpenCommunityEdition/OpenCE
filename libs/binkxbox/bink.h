#ifndef BINK_RECONSTRUCTED_H
#define BINK_RECONSTRUCTED_H
#include <stddef.h>
#include "binkaudio.h"
#include "binkio.h"
#include "radcb.h"
#include "binkds_state.h"
struct BINKIO;
typedef long (__stdcall *bink_io_open_proc)(
	struct BINKIO *io,
	const char *name,
	unsigned long flags);

struct BINK;
struct BINKSND;
typedef long (__stdcall *bink_sound_ready_proc)(
	struct BINKSND *sound);
typedef long (__stdcall *bink_sound_lock_proc)(
	struct BINKSND *sound,
	unsigned char **destination,
	unsigned long *bytes);
typedef long (__stdcall *bink_sound_unlock_proc)(
	struct BINKSND *sound,
	unsigned long bytes);
typedef long (__stdcall *bink_sound_pause_proc)(
	struct BINKSND *sound,
	long pause);
typedef long (__stdcall *bink_sound_on_off_proc)(
	struct BINKSND *sound,
	long on);
typedef void (__stdcall *bink_sound_close_proc)(
	struct BINKSND *sound);
typedef long (__stdcall *bink_sound_open_proc)(
	struct BINKSND *sound,
	unsigned long frequency,
	long bits,
	long channels,
	unsigned long flags,
	struct BINK *bink);
typedef bink_sound_open_proc (__stdcall *bink_sound_system_open_proc)(
	unsigned long parameter);
typedef void (__stdcall *bink_sound_volume_proc)(
	struct BINKSND *sound,
	long volume);
typedef void (__stdcall *bink_sound_pan_proc)(
	struct BINKSND *sound,
	long pan);
typedef void (__stdcall *bink_sound_mix_bins_proc)(
	struct BINKSND *sound,
	unsigned long bins);

/* January Xbox sound object. The 0x44-byte callback/format prefix is followed
 * by the target-proven 0x80-byte DirectSound state. Unlike later Bink, the PCM
 * ring belongs to BINK. Unknown backend fields remain reserved. */
struct BINKSND
{
	bink_sound_ready_proc Ready;
	bink_sound_lock_proc Lock;
	bink_sound_unlock_proc Unlock;
	bink_sound_volume_proc Volume;
	bink_sound_pan_proc Pan;
	bink_sound_pause_proc Pause;
	bink_sound_on_off_proc SetOnOff;
	bink_sound_close_proc Close;
	bink_sound_mix_bins_proc MixBins;
	unsigned long BestSizeIn16;
	unsigned long SoundDroppedOut;
	long OnOff;
	unsigned long Latency;
	unsigned long VideoScale;
	unsigned long Frequency;
	long Bits;
	long Channels;
	struct bink_ds_state ds;
};

/* Dirty rectangles are measured in pixels; the private dirty mask is one
 * byte per 16-by-16 block. All fields below are checked against January code. */
typedef struct BINKRECT
{
	long Left;
	long Top;
	long Width;
	long Height;
} BINKRECT;
#define BINKMAXDIRTYRECTS 8

/* Partially decoded January BINK layout. BinkClose and BinkOpen prove the structure
 * footprint is 0x408 bytes (pooled allocations can include extra storage). Reserved spans are real unknown target storage,
 * not artificial compiler padding. Do not import later SDK offsets. */
typedef struct BINK
{
	unsigned long Width;
	unsigned long Height;
	unsigned long Frames;
	unsigned long FrameNum;
	unsigned long LastFrameNum;
	unsigned long FrameRate;
	unsigned long FrameRateDiv;
	unsigned long ReadError;
	unsigned long OpenFlags;
	unsigned long BinkType;
	unsigned long Size;
	unsigned long FrameSize;
	unsigned long SndSize;
	BINKRECT FrameRects[BINKMAXDIRTYRECTS];
	long NumRects;
	unsigned char reserved_00b8[4];
	void *MaskPlane;
	unsigned char reserved_00c0[0xcc-0xc0];
	long dirty_width;
	long dirty_height;
	unsigned char reserved_00d4[8];
	const unsigned char *dirty_mask;
	long dirty_pitch;
	unsigned char reserved_00e4[8];
	unsigned long InternalFrames;
	long NumTracks;
	unsigned long Highest1SecRate;
	unsigned long Highest1SecFrame;
	long Paused;
	unsigned char reserved_0100[4];
	unsigned char *compframe;
	void *preloadptr;
	unsigned long *frameoffsets;
	BINKIO io;
	void *iobuffer;
	unsigned long iosize;
	unsigned char reserved_0214[8];
	long trackindex;
	unsigned long *tracksizes;
	unsigned long *tracktypes;
	unsigned long *trackIDs;
	unsigned char reserved_022c[4];
	unsigned long playedframes;
	unsigned long firstframetime;
	unsigned char reserved_0238[4];
	unsigned long startblittime;
	unsigned long starttime;
	unsigned long startframe;
	unsigned long resynctime;
	unsigned long longestframetime;
	unsigned long slowestframetime;
	unsigned long slowestframe;
	unsigned long slowest2frametime;
	unsigned long slowest2frame;
	long SoundOn;
	long VideoOn;
	unsigned long totalmem;
	unsigned long timevdecomp;
	unsigned long timeadecomp;
	unsigned long timeblit;
	unsigned long timeopen;
	unsigned long fileframerate;
	unsigned long fileframeratediv;
	unsigned long runtimeframes;
	unsigned long runtimemoveamt;
	unsigned long *rtframetimes;
	unsigned long *rtadecomptimes;
	unsigned long *rtvdecomptimes;
	unsigned long *rtblittimes;
	unsigned long *rtreadtimes;
	unsigned long *rtidlereadtimes;
	unsigned long *rtthreadreadtimes;
	unsigned long lastblitflags;
	unsigned long lastdecompframe;
	unsigned long sndbufsize;
	unsigned char *sndbuf;
	unsigned char *sndend;
	unsigned char *sndwritepos;
	unsigned char *sndreadpos;
	struct BINKAUDIO *sndcomp;
	unsigned long sndamt;
	long sndconvert8;
	struct BINKSND sound;
	unsigned long skippedlastblit;
	unsigned long skippedblits;
	unsigned long soundskips;
	long sndendframe;
	unsigned long sndprime;
	unsigned char reserved_03a8[0x3d4-0x3a8];
	unsigned long big_sound_skip_adj;
	unsigned long big_sound_skip_reduce;
	unsigned char reserved_03dc[12];
	struct radcb_callback sound_callback;
	unsigned char reserved_0400[8];
} BINK;

typedef struct BINKTRACK
{
	unsigned long Frequency;
	unsigned long Bits;
	unsigned long Channels;
	unsigned long MaxSize;
	BINK *bink;
	struct BINKAUDIO *decoder;
	unsigned long trackindex;
} BINKTRACK;

typedef char bink_ds_size_assert[sizeof(struct bink_ds_state)==0x80 ? 1:-1];
typedef char bink_sound_size_assert[sizeof(struct BINKSND)==0xc4 ? 1:-1];
typedef char bink_ds_offset_assert[offsetof(struct BINKSND,ds)==0x44 ? 1:-1];
typedef char bink_ds_mixbins_assert[offsetof(struct bink_ds_state,mixbins)==0x5c ? 1:-1];
typedef char bink_track_size_assert[sizeof(BINKTRACK)==0x1c ? 1:-1];
typedef char bink_tracks_offset_assert[offsetof(BINK,NumTracks)==0xf0 ? 1:-1];
typedef char bink_compframe_offset_assert[offsetof(BINK,compframe)==0x104 ? 1:-1];
typedef char bink_frameoffsets_offset_assert[offsetof(BINK,frameoffsets)==0x10c ? 1:-1];
typedef char bink_trackindex_offset_assert[offsetof(BINK,trackindex)==0x21c ? 1:-1];
typedef char bink_sizes_offset_assert[offsetof(BINK,tracksizes)==0x220 ? 1:-1];
typedef char bink_types_offset_assert[offsetof(BINK,tracktypes)==0x224 ? 1:-1];
typedef char bink_ids_offset_assert[offsetof(BINK,trackIDs)==0x228 ? 1:-1];
typedef char bink_video_offset_assert[offsetof(BINK,VideoOn)==0x264 ? 1:-1];
typedef char bink_sound_offset_assert[offsetof(BINK,sound)==0x2d0 ? 1:-1];
typedef char bink_volume_offset_assert[offsetof(struct BINKSND,Volume)==0x0c ? 1:-1];
typedef char bink_pan_offset_assert[offsetof(struct BINKSND,Pan)==0x10 ? 1:-1];
typedef char bink_bins_offset_assert[offsetof(struct BINKSND,MixBins)==0x20 ? 1:-1];

enum
{
	BINK_TRACK_STEREO_BIT= 29,
	BINK_TRACK_16BIT_BIT= 30
};
#define BINK_FROM_MEMORY 0x04000000UL
#define BINK_NO_THREADED_IO 0x08000000UL
#define BINK_TRACK_HAS_AUDIO 0x80000000UL
#define BINK_TRACK_FREQUENCY_MASK 0x0000ffffUL

unsigned long __stdcall BinkGetTrackType(
	BINK *bink,
	unsigned long track);
unsigned long __stdcall BinkGetTrackMaxSize(
	BINK *bink,
	unsigned long track);
unsigned long __stdcall BinkGetTrackID(
	BINK *bink,
	unsigned long track);
BINKTRACK *__stdcall BinkOpenTrack(
	BINK *bink,
	unsigned long track);
void __stdcall BinkCloseTrack(
	BINKTRACK *track);
long __stdcall BinkSetVideoOnOff(
	BINK *bink,
	long on);
void __stdcall BinkSetVolume(
	BINK *bink,
	long volume);
void __stdcall BinkSetPan(
	BINK *bink,
	long pan);
void __stdcall BinkSetMixBins(
	BINK *bink,
	unsigned long bins);

/* Keyframe selection flags recovered from BinkGetKeyFrame's switch. */
enum
{
	BINK_GET_KEY_PREVIOUS= 0,
	BINK_GET_KEY_NEXT= 1,
	BINK_GET_KEY_NEAREST= 2,
	BINK_GET_KEY_NOT_EQUAL= 0x80
};
unsigned long __stdcall BinkGetKeyFrame(
	BINK *bink,
	unsigned long frame,
	unsigned long flags);
unsigned long __stdcall BinkGetTrackData(
	BINKTRACK *track,
	void *destination);

void __stdcall BinkSetError(
	const char *message);
char *__stdcall BinkGetError(
	void);
void __stdcall BinkSetFrameRate(
	unsigned long rate,
	unsigned long divisor);
void __stdcall BinkSetIOSize(
	unsigned long size);
void __stdcall BinkSetIO(
	bink_io_open_proc open);
void __stdcall BinkSetSimulate(
	unsigned long rate);
void __stdcall BinkSetSoundTrack(
	unsigned long track);
void *__stdcall BinkLogoAddress(
	void);

long __stdcall BinkSetSoundSystem(
	bink_sound_system_open_proc open,
	unsigned long parameter);
void __stdcall BinkService(
	BINK *bink);
long __stdcall BinkPause(
	BINK *bink,
	long pause);
long __stdcall BinkSetSoundOnOff(
	BINK *bink,
	long on);
void __stdcall BinkClose(
	BINK *bink);
void __stdcall BinkNextFrame(
	BINK *bink);
void __stdcall BinkGoto(
	BINK *bink,
	unsigned long frame,
	unsigned long flags);
long __stdcall BinkDoFrame(
	BINK *bink);
#define BINK_GOTO_QUICK 1UL

#define BINKSURFACEDIRECT 0x04000000UL
#define BINKSURFACESLOW 0x08000000UL
#define BINKNOSKIP 0x00080000UL
#define BINKCOPY2XH 0x10000000UL
#define BINKCOPY2XHI 0x20000000UL
#define BINKCOPY2XW 0x30000000UL
#define BINKCOPY2XWH 0x40000000UL
#define BINKCOPY2XWHI 0x50000000UL
#define BINKCOPYNOSCALING 0x70000000UL
long __stdcall BinkGetRects(
	BINK *bink,
	unsigned long flags);

typedef char bink_rect_size_assert[sizeof(BINKRECT)==16 ? 1:-1];
typedef char bink_rects_offset_assert[offsetof(BINK,FrameRects)==0x34 ? 1:-1];
typedef char bink_rect_count_assert[offsetof(BINK,NumRects)==0xb4 ? 1:-1];
typedef char bink_dirty_width_assert[offsetof(BINK,dirty_width)==0xcc ? 1:-1];
typedef char bink_dirty_height_assert[offsetof(BINK,dirty_height)==0xd0 ? 1:-1];
typedef char bink_dirty_mask_assert[offsetof(BINK,dirty_mask)==0xdc ? 1:-1];
typedef char bink_dirty_pitch_assert[offsetof(BINK,dirty_pitch)==0xe0 ? 1:-1];
typedef char bink_lastblitflags_assert[offsetof(BINK,lastblitflags)==0x2a8 ? 1:-1];

/* Internal pooled allocation API, named in the January PDB/public symbols. */
void __stdcall pushmalloc(
	void **destination,
	unsigned long size);
void *__stdcall popmalloc(
	unsigned long size);

typedef char bink_full_footprint_assert[sizeof(BINK)==0x408 ? 1:-1];
typedef char bink_io_offset_assert[offsetof(BINK,io)==0x110 ? 1:-1];
typedef char bink_sound_start_assert[offsetof(BINK,sndbuf)==0x2b4 ? 1:-1];
typedef char bink_sound_ready_assert[offsetof(BINK,sound.Ready)==0x2d0 ? 1:-1];
typedef char bink_sound_prime_assert[offsetof(BINK,sndprime)==0x3a4 ? 1:-1];
typedef char bink_sound_callback_assert[offsetof(BINK,sound_callback)==0x3e8 ? 1:-1];
typedef char bink_lastdecompframe_offset_assert[offsetof(BINK,lastdecompframe)==0x2ac ? 1:-1];
typedef char bink_skippedblits_offset_assert[offsetof(BINK,skippedblits)==0x398 ? 1:-1];
typedef char bink_soundskips_offset_assert[offsetof(BINK,soundskips)==0x39c ? 1:-1];
/* Output layouts proven by the January statistics writers. */
typedef struct BINKSUMMARY
{
	unsigned long Width;
	unsigned long Height;
	unsigned long TotalTime;
	unsigned long FileFrameRate;
	unsigned long FileFrameRateDiv;
	unsigned long FrameRate;
	unsigned long FrameRateDiv;
	unsigned long TotalOpenTime;
	unsigned long TotalFrames;
	unsigned long TotalPlayedFrames;
	unsigned long SkippedFrames;
	unsigned long SkippedBlits;
	unsigned long SoundSkips;
	unsigned long TotalBlitTime;
	unsigned long TotalReadTime;
	unsigned long TotalVideoDecompTime;
	unsigned long TotalAudioDecompTime;
	unsigned long TotalIdleReadTime;
	unsigned long TotalBackReadTime;
	unsigned long TotalReadSpeed;
	unsigned long SlowestFrameTime;
	unsigned long Slowest2FrameTime;
	unsigned long SlowestFrameNum;
	unsigned long Slowest2FrameNum;
	unsigned long AverageDataRate;
	unsigned long AverageFrameSize;
	unsigned long HighestMemAmount;
	unsigned long TotalIOMemory;
	unsigned long HighestIOUsed;
	unsigned long Highest1SecRate;
	unsigned long Highest1SecFrame;
} BINKSUMMARY;
typedef struct BINKREALTIME
{
	unsigned long FrameNum;
	unsigned long FrameRate;
	unsigned long FrameRateDiv;
	unsigned long Frames;
	unsigned long FramesTime;
	unsigned long FramesVideoDecompTime;
	unsigned long FramesAudioDecompTime;
	unsigned long FramesReadTime;
	unsigned long FramesIdleReadTime;
	unsigned long FramesThreadReadTime;
	unsigned long FramesBlitTime;
	unsigned long ReadBufferSize;
	unsigned long ReadBufferUsed;
	unsigned long FramesDataRate;
} BINKREALTIME;
void __stdcall BinkGetSummary(
	BINK *bink,
	BINKSUMMARY *summary);
void __stdcall BinkGetRealtime(
	BINK *bink,
	BINKREALTIME *realtime,
	unsigned long frames);
typedef char bink_size_offset_assert[offsetof(BINK,Size)==0x28 ? 1:-1];
typedef char bink_high_rate_offset_assert[offsetof(BINK,Highest1SecRate)==0xf4 ? 1:-1];
typedef char bink_high_frame_offset_assert[offsetof(BINK,Highest1SecFrame)==0xf8 ? 1:-1];
typedef char bink_io_alloc_offset_assert[offsetof(BINK,iosize)==0x210 ? 1:-1];
typedef char bink_firsttime_offset_assert[offsetof(BINK,firstframetime)==0x234 ? 1:-1];
typedef char bink_timeopen_offset_assert[offsetof(BINK,timeopen)==0x278 ? 1:-1];
typedef char bink_filefps_offset_assert[offsetof(BINK,fileframerate)==0x27c ? 1:-1];
typedef char bink_filediv_offset_assert[offsetof(BINK,fileframeratediv)==0x280 ? 1:-1];
typedef char bink_rtcount_offset_assert[offsetof(BINK,runtimeframes)==0x284 ? 1:-1];
typedef char bink_rtmove_offset_assert[offsetof(BINK,runtimemoveamt)==0x288 ? 1:-1];
typedef char bink_rtframe_offset_assert[offsetof(BINK,rtframetimes)==0x28c ? 1:-1];
typedef char bink_rtaudio_offset_assert[offsetof(BINK,rtadecomptimes)==0x290 ? 1:-1];
typedef char bink_rtvideo_offset_assert[offsetof(BINK,rtvdecomptimes)==0x294 ? 1:-1];
typedef char bink_rtblit_offset_assert[offsetof(BINK,rtblittimes)==0x298 ? 1:-1];
typedef char bink_rtread_offset_assert[offsetof(BINK,rtreadtimes)==0x29c ? 1:-1];
typedef char bink_rtidle_offset_assert[offsetof(BINK,rtidlereadtimes)==0x2a0 ? 1:-1];
typedef char bink_rtthread_offset_assert[offsetof(BINK,rtthreadreadtimes)==0x2a4 ? 1:-1];
typedef char bink_summary_size_assert[sizeof(BINKSUMMARY)==0x7c ? 1:-1];
typedef char bink_realtime_size_assert[sizeof(BINKREALTIME)==0x38 ? 1:-1];

#endif
