/* Initial public track/control packet. Remaining BinkRead functions and
 * runtime data owners are unwritten; this is not a complete replacement.
 */
#include "bink.h"
#include "radmem.h"
#include "radtimer.h"
#include "radmath.h"
#include <string.h>

/* These are independent global options, not a struct mirroring the split
 * object's anonymous section anchor. Original-location proofs are separate.
 */
#pragma data_seg("BINKDATA")
static unsigned long forced_frame_rate= ~0UL;
static unsigned long forced_io_size= ~0UL;
static unsigned long forced_simulate_rate= ~0UL;
static unsigned long forced_sound_track= ~0UL;
static bink_io_open_proc forced_io_open= 0;
static unsigned long pending_allocation_bytes= 0;
static unsigned long pending_allocation_count= 0;
static bink_sound_system_open_proc sound_system_open= 0;
static bink_sound_open_proc sound_open= 0;
static unsigned long sound_open_count= 0;
static struct radcb_handler *io_handler= 0;
static struct radcb_handler *sound_handler= 0;
/* The embedded BIK movie is a private target asset and is intentionally not
 * published with the reconstructed source. Its external owner remains a data
 * admission boundary; BinkLogoAddress itself can still be verified. */
extern unsigned char LogoData[14848];
#pragma data_seg()
static unsigned long forced_frame_rate_div;
static char bink_error[256];
static unsigned long pending_allocation_sizes[128];
static void **pending_allocation_destinations[128];

static void __stdcall service_sound(
	BINK *bink);
static void update_slowest_frame(
	BINK *bink,
	unsigned long now);


#pragma code_seg("BINK")
void __stdcall BinkSetError(
	const char *message)
{
	/* Original unbounded copy; callers must keep messages within 255 bytes. */
	strcpy(bink_error, message);
	return;
}

char *__stdcall BinkGetError(
	void)
{
	return bink_error;
}

void __stdcall BinkSetFrameRate(
	unsigned long rate,
	unsigned long divisor)
{
	forced_frame_rate= rate;
	forced_frame_rate_div= divisor;
	return;
}

void __stdcall BinkSetIOSize(
	unsigned long size)
{
	forced_io_size= size;
	return;
}

void __stdcall BinkSetIO(
	bink_io_open_proc open)
{
	forced_io_open= open;
	return;
}

void __stdcall BinkSetSimulate(
	unsigned long rate)
{
	forced_simulate_rate= rate;
	return;
}

void __stdcall BinkSetSoundTrack(
	unsigned long track)
{
	forced_sound_track= track;
	return;
}

void *__stdcall BinkLogoAddress(
	void)
{
	return LogoData;
}

unsigned long __stdcall BinkGetTrackType(
	BINK *bink,
	unsigned long track)
{
	/* As in the target, nonnull handles require a valid track index. */
	return bink ? bink->tracktypes[track] : 0;
}

unsigned long __stdcall BinkGetTrackMaxSize(
	BINK *bink,
	unsigned long track)
{
	return bink ? bink->tracksizes[track] : 0;
}

unsigned long __stdcall BinkGetTrackID(
	BINK *bink,
	unsigned long track)
{
	return bink ? bink->trackIDs[track] : 0;
}

BINKTRACK *__stdcall BinkOpenTrack(
	BINK *bink,
	unsigned long track)
{
	struct BINKAUDIO *decoder;
	BINKTRACK *result;
	if (!bink || track >= (unsigned long)bink->NumTracks ||
		!(bink->tracktypes[track]&BINK_TRACK_HAS_AUDIO))
	{
		return 0;
	}
	decoder= BinkAudioDecompressOpen(bink->tracktypes[track]&BINK_TRACK_FREQUENCY_MASK,
		((bink->tracktypes[track]>>BINK_TRACK_STEREO_BIT)&1)+1, 1);
	if (!decoder)
	{
		return 0;
	}
	result= radmalloc(sizeof(*result));
	if (!result)
	{
		BinkAudioDecompressClose(decoder);
		return 0;
	}
	memset(result, 0, sizeof(*result));
	result->bink= bink;
	result->decoder= decoder;
	result->Frequency= bink->tracktypes[track]&BINK_TRACK_FREQUENCY_MASK;
	result->Bits= (((bink->tracktypes[track]>>(BINK_TRACK_16BIT_BIT-3))&8)+8);
	result->Channels= ((bink->tracktypes[track]>>BINK_TRACK_STEREO_BIT)&1)+1;
	result->MaxSize= (bink->tracksizes[track]+3)&~3UL;
	if (result->Bits == 8)
	{
		result->MaxSize >>= 1;
	}
	result->trackindex= track;
	return result;
}

void __stdcall BinkCloseTrack(
	BINKTRACK *track)
{
	if (track)
	{
		if (track->decoder)
		{
			BinkAudioDecompressClose(track->decoder);
			track->decoder= 0;
		}
		radfree(track);
	}
	return;
}

long __stdcall BinkSetVideoOnOff(
	BINK *bink,
	long on)
{
	if (bink)
	{
		bink->VideoOn= on;
	}
	return on;
}

void __stdcall BinkSetVolume(
	BINK *bink,
	long volume)
{
	if (bink && bink->trackindex != -1 && bink->sound.Volume)
	{
		bink->sound.Volume(&bink->sound, volume);
	}
	return;
}

void __stdcall BinkSetPan(
	BINK *bink,
	long pan)
{
	if (bink && bink->trackindex != -1 && bink->sound.Pan)
	{
		bink->sound.Pan(&bink->sound, pan);
	}
	return;
}

void __stdcall BinkSetMixBins(
	BINK *bink,
	unsigned long bins)
{
	if (bink && bink->trackindex != -1 && bink->sound.MixBins)
	{
		bink->sound.MixBins(&bink->sound, bins);
	}
	return;
}

/* Frame is one-based. As in the original API, a nonnull handle requires a
 * valid frame index and a frame-offset table covering all movie frames. */
unsigned long __stdcall BinkGetKeyFrame(
	BINK *bink,
	unsigned long frame,
	unsigned long flags)
{
	long previous;
	if (bink)
	{
		if (!(flags&BINK_GET_KEY_NOT_EQUAL) && (bink->frameoffsets[frame-1]&1))
		{
			return frame;
		}
		switch (flags&0x7f)
		{
		case BINK_GET_KEY_PREVIOUS:
			frame= frame-2;
			while ((long)frame >= 1)
			{
				if (bink->frameoffsets[frame]&1)
				{
					break;
				}
				--frame;
			}
			return frame+1;
		case BINK_GET_KEY_NEXT:
			while (frame < bink->Frames)
			{
				if (bink->frameoffsets[frame++]&1)
				{
					return frame;
				}
			}
			break;
		case BINK_GET_KEY_NEAREST:
			previous= (long)(frame-2);
			for (;;)
			{
				if (previous >= 0)
				{
					if (bink->frameoffsets[previous]&1)
					{
						return previous+1;
					}
				}
				if (frame < bink->Frames)
				{
					if (bink->frameoffsets[frame++]&1)
					{
						return frame;
					}
				}
				else if (previous < 0)
				{
					return 0;
				}
				--previous;
			}
			break;
		}
	}
	return 0;
}

/* The compressed frame contains one length-prefixed packet per sound track.
 * Packet payloads are valid Bink streams; the original has no length/NULL
 * checks beyond the handle and zero-packet checks reproduced here. */
unsigned long __stdcall BinkGetTrackData(
	BINKTRACK *track,
	void *destination)
{
	const unsigned char *input;
	unsigned long packet_size;
	long index;
	if (track)
	{
		input= track->bink->compframe;
		for (index= 0; index < track->bink->NumTracks; ++index)
		{
			memcpy(&packet_size, input, sizeof(packet_size));
			input+= sizeof(packet_size);
			if ((unsigned long)index == track->trackindex && packet_size)
			{
				unsigned long remaining;
				unsigned long total= 0;
				unsigned char *output;
				memcpy(&remaining, input, sizeof(remaining));
				input+= sizeof(remaining);
				if (remaining)
				{
					output= destination;
					do
					{
						void *samples;
						unsigned long bytes;
						unsigned long sample;
						BinkAudioDecompress(track->decoder, &samples, &bytes,
						input, &input);
						if (bytes > remaining)
						{
							bytes= remaining;
						}
						remaining-= bytes;
						if (track->Bits == 16)
						{
							memcpy(output, samples, bytes);
						}
						else
						{
							const unsigned short *source= samples;
							bytes >>= 1;
							{
								unsigned char *dest= output;
								for (sample= 0; sample < bytes; ++sample)
								{
									*dest++= (*source++>>8)+128;
								}
							}
						}
						total+= bytes;
						output+= bytes;
					} while (remaining);
				}
				return total;
			}
			input+= packet_size;
		}
	}
	return 0;
}


/* Stage suballocations for a single owned allocation. Caller preconditions:
 * at most 128 pending entries and sizes that do not overflow. Neither the
 * target nor this reconstruction checks those preconditions. Aligned starts
 * are distributed over 32-byte cache-line slots. */
void __stdcall pushmalloc(
	void **destination,
	unsigned long size)
{
	unsigned long cache_lines= pending_allocation_bytes/32;
	size= (size+15)&~15UL;
	cache_lines= (cache_lines-size/32+1)%32;
	size+= cache_lines*32;
	pending_allocation_bytes+= size;
	pending_allocation_sizes[pending_allocation_count]= size;
	pending_allocation_destinations[pending_allocation_count]= destination;
	++pending_allocation_count;
	return;
}

void *__stdcall popmalloc(
	unsigned long size)
{
	unsigned char *allocation;
	size= (size+15)&~15UL;
	allocation= radmalloc(pending_allocation_bytes+size);
	pending_allocation_bytes= 0;
	if (allocation)
	{
		unsigned char *next= allocation+size;
		unsigned long index;
		for (index= 0; index < pending_allocation_count; ++index)
		{
			*pending_allocation_destinations[index]= next;
			next+= pending_allocation_sizes[index];
		}
	}
	pending_allocation_count= 0;
	return allocation;
}

long __stdcall BinkSetSoundSystem(
	bink_sound_system_open_proc open,
	unsigned long parameter)
{
	bink_sound_open_proc result;
	if (!open)
	{
		return 0;
	}
	if (!sound_system_open)
	{
		sound_system_open= open;
	}
	else if (sound_system_open != open)
	{
		if (sound_open_count)
		{
			return 0;
		}
		sound_system_open= open;
	}
	result= sound_system_open(parameter);
	if (result)
	{
		sound_open= result;
	}
	return sound_open != 0;
}

/* S16 little-endian PCM -> U8 PCM. Return the advanced destination so the
 * two spans of a wrapped ring can share one output buffer. Source is valid
 * interleaved PCM; samples and byte count are even on decoder/service paths. */
static unsigned char *convert_sound_to_u8(
	unsigned char *destination,
	const void *buffer,
	unsigned long bytes)
{
	const unsigned char *source= buffer;
	bytes >>= 1;
	while (bytes--)
	{
		unsigned short sample;
		memcpy(&sample, source, sizeof(sample));
		*destination++= (sample>>8)+128;
		source+= sizeof(sample);
	}
	return destination;
}

static void __stdcall service_sound(
	BINK *bink)
{
	unsigned char *destination;
	unsigned long bytes;
	struct BINKSND *sound= &bink->sound;
	if (sound->Ready && RADCB_try_to_suspend_callback(sound_handler,
		&bink->sound_callback))
	{
		while (sound->Ready(sound))
		{
			unsigned long tail;
			if (bink->sndamt <= bink->sound.BestSizeIn16 &&
				((long)bink->FrameNum <= bink->sndendframe || !bink->sndamt))
			{
				break;
			}
			if (!bink->sound.Lock(sound, &destination, &bytes))
			{
				break;
			}
			if (bink->sndconvert8)
			{
				bytes*= 2;
			}
			if (bytes > bink->sndamt)
			{
				bytes= bink->sndamt;
			}
			bink->sndamt-= bytes;
			tail= (unsigned long)(bink->sndend-bink->sndreadpos);
			if (tail < bytes)
			{
				if (tail)
				{
					if (bink->sndconvert8)
					{
						destination= convert_sound_to_u8(destination,
							bink->sndreadpos, tail);
					}
					else
					{
						memcpy(destination, bink->sndreadpos, tail);
						destination+= tail;
					}
				}
				if (bink->sndconvert8)
				{
					convert_sound_to_u8(destination, bink->sndbuf, bytes-tail);
				}
				else
				{
					memcpy(destination, bink->sndbuf, bytes-tail);
				}
				bink->sndreadpos= bink->sndbuf+(bytes-tail);
			}
			else
			{
				if (bink->sndconvert8)
				{
					convert_sound_to_u8(destination, bink->sndreadpos, bytes);
				}
				else
				{
					memcpy(destination, bink->sndreadpos, bytes);
				}
				bink->sndreadpos+= bytes;
			}
			if (bink->sndconvert8)
			{
				bytes >>= 1;
			}
			bink->sound.Unlock(sound, bytes);
		}
		RADCB_resume_callback(sound_handler, &bink->sound_callback);
	}
	return;
}

void __stdcall BinkService(
	BINK *bink)
{
	/* The target requires a nonnull handle here, unlike most public APIs. */
	if (bink->trackindex != -1)
	{
		service_sound(bink);
	}
	return;
}

static void update_slowest_frame(
	BINK *bink,
	unsigned long now)
{
	if (bink->longestframetime)
	{
		now-= bink->longestframetime;
		bink->longestframetime= 0;
		if (now > bink->slowestframetime)
		{
			bink->slowest2frametime= bink->slowestframetime;
			bink->slowest2frame= bink->slowestframe;
			bink->slowestframetime= now;
			bink->slowestframe= bink->FrameNum;
		}
		else if (now > bink->slowest2frametime)
		{
			bink->slowest2frame= bink->FrameNum;
			bink->slowest2frametime= now;
		}
	}
	return;
}

long __stdcall BinkPause(
	BINK *bink,
	long pause)
{
	unsigned long now;
	if (!bink)
	{
		return 0;
	}
	now= RADTimerRead();
	if (bink->startblittime)
	{
		bink->timeblit+= now-bink->startblittime;
		bink->startblittime= 0;
	}
	update_slowest_frame(bink, now);
	if (!pause && bink->Paused)
	{
		bink->starttime= 0;
		bink->big_sound_skip_adj= 0;
	}
	bink->Paused= pause;
	if (bink->trackindex != -1)
	{
		bink->sound.Pause(&bink->sound, pause);
	}
	bink->skippedlastblit= 0;
	if (bink->playedframes && bink->trackindex != -1)
	{
		service_sound(bink);
	}
	return bink->Paused;
}

long __stdcall BinkSetSoundOnOff(
	BINK *bink,
	long on)
{
	long result= 0;
	if (bink && bink->trackindex != -1 && bink->sound.SetOnOff)
	{
		result= bink->sound.SetOnOff(&bink->sound, on);
		if (!result)
		{
			if (bink->SoundOn)
			{
				RADCB_suspend_callback(sound_handler, &bink->sound_callback);
				bink->sndwritepos= bink->sndbuf;
				bink->sndreadpos= bink->sndbuf;
				bink->sndamt= 0;
				bink->SoundOn= 0;
				RADCB_resume_callback(sound_handler, &bink->sound_callback);
			}
		}
		else if (!bink->SoundOn)
		{
			RADCB_suspend_callback(sound_handler, &bink->sound_callback);
			if (bink->FrameNum != 1 && bink->FrameNum != bink->InternalFrames)
			{
				bink->sndamt= bink->sndprime;
				memset(bink->sndbuf, 0, bink->sndamt);
				bink->sndwritepos= bink->sndbuf+bink->sndamt;
				bink->sndreadpos= bink->sndbuf;
			}
			bink->SoundOn= 1;
			bink->starttime= 0;
			bink->big_sound_skip_adj= 0;
			RADCB_resume_callback(sound_handler, &bink->sound_callback);
		}
	}
	return result;
}


void __stdcall BinkClose(
	BINK *bink)
{
	if (bink != NULL)
	{
		struct radcb_handler *io;
		struct radcb_handler *sound;
		BinkPause(bink, 1);
		io= (!(bink->OpenFlags&BINK_NO_THREADED_IO) && bink->preloadptr == NULL)
			? io_handler : NULL;
		sound= (bink->trackindex != -1) ? sound_handler : NULL;
		switch (RADCB_unregister_2_callbacks(io, &bink->io.callback,
			sound, &bink->sound_callback, RADCB_FREE_EMPTY_HANDLERS))
		{
		case 1:
			io_handler= NULL;
			break;
		case 2:
			sound_handler= NULL;
			break;
		case 3:
			sound_handler= NULL;
			io_handler= NULL;
			break;
		}
		if (bink->trackindex != -1)
		{
			bink->sound.Close(&bink->sound);
			BinkAudioDecompressClose(bink->sndcomp);
		}
		if (bink->preloadptr != NULL)
		{
			if (!(bink->OpenFlags&BINK_FROM_MEMORY))
			{
				radfree(bink->preloadptr);
			}
		}
		else
		{
			bink->io.Close(&bink->io);
			radfree(bink->iobuffer);
		}
		if (bink->sndbuf != NULL)
		{
			radfree(bink->sndbuf);
		}
		if (bink->MaskPlane != NULL)
		{
			radfree(bink->MaskPlane);
		}
		memset(bink, 0, sizeof(*bink));
		radfree(bink);
	}
	return;
}


/* Prepend silence to the PCM ring until its initial playback threshold is
 * met. The target forms a before-array pointer before checking wraparound;
 * this reconstruction instead performs the same wrap using bounded offsets.
 * Valid rings have sndprime <= sndbufsize and positive format rates. */
static unsigned long pad_sound(
	BINK *bink)
{
	unsigned long missing;
	unsigned long duration;
	unsigned long position;
	unsigned long tail;
	if (bink->sndamt >= bink->sndprime)
	{
		return 0;
	}
	missing= bink->sndprime-bink->sndamt;
	duration= (missing*1000)/(((unsigned long)bink->sound.Channels*
		(unsigned long)bink->sound.Bits*bink->sound.Frequency)>>3);
	position= (unsigned long)(bink->sndreadpos-bink->sndbuf);
	if (missing > position)
	{
		tail= missing-position;
		bink->sndreadpos= bink->sndbuf+(bink->sndbufsize-tail);
		memset(bink->sndbuf, 0, position);
		memset(bink->sndreadpos, 0, tail);
	}
	else
	{
		bink->sndreadpos-= missing;
		memset(bink->sndreadpos, 0, missing);
	}
	bink->sndamt+= missing;
	return duration;
}

/* Zero and one select the first frame. Ordinary internal calls use one-based
 * indices. Frame offsets include a keyframe flag in bit zero and one sentinel.
 */
static void set_frame(
	BINK *bink,
	unsigned long frame)
{
	if (frame)
	{
		--frame;
	}
	if (bink->trackindex != -1)
	{
		service_sound(bink);
	}
	bink->LastFrameNum= bink->FrameNum;
	if (!frame)
	{
		if (bink->SoundOn)
		{
			BinkSetSoundOnOff(bink, 0);
			BinkSetSoundOnOff(bink, 1);
		}
		bink->starttime= 0;
		bink->big_sound_skip_adj= 0;
	}
	if (bink->preloadptr)
	{
		bink->compframe= (unsigned char *)bink->preloadptr+
			((bink->frameoffsets[frame]&~1UL)-(bink->frameoffsets[0]&~1UL));
	}
	else
	{
		bink->io.ReadFrame(&bink->io, frame, bink->frameoffsets[frame]&~1UL,
			bink->compframe, (bink->frameoffsets[frame+1]&~1UL)-
			(bink->frameoffsets[frame]&~1UL));
	}
	if (bink->trackindex != -1)
	{
		service_sound(bink);
	}
	bink->FrameNum= frame+1;
	return;
}

void __stdcall BinkNextFrame(
	BINK *bink)
{
	if (bink)
	{
		unsigned long now;
		/* January also stores 1 here, but the missing concurrency qualifier or
		 * intervening owner is unresolved. Do not retain a dead store or add a
		 * volatile qualifier solely to force its instruction sequence. */
		bink->io.Working= 0;
		if (bink->sound.SoundDroppedOut)
		{
			bink->sound.SoundDroppedOut= 0;
			if (bink->FrameNum > 1 && (long)bink->FrameNum <= bink->sndendframe)
			{
				RADCB_suspend_2_callbacks(sound_handler, &bink->sound_callback,
					io_handler, &bink->io.callback);
				BinkSetSoundOnOff(bink, 0);
				while (((bink->io.CurBufUsed+1)*100)/(bink->io.CurBufSize+1) < 90)
				{
					if (!bink->io.Idle(&bink->io))
					{
						break;
					}
				}
				++bink->soundskips;
				pad_sound(bink);
				bink->starttime= 0;
				bink->big_sound_skip_adj= 0;
				BinkSetSoundOnOff(bink, 1);
				RADCB_resume_callback(sound_handler, &bink->sound_callback);
				RADCB_resume_callback(io_handler, &bink->io.callback);
			}
		}
		bink->io.Working= 1;
		if (bink->trackindex != -1)
		{
			service_sound(bink);
		}
		now= RADTimerRead();
		if (bink->startblittime)
		{
			bink->timeblit+= now-bink->startblittime;
			bink->startblittime= 0;
		}
		if (bink->FrameNum >= bink->Frames)
		{
			set_frame(bink, 1);
		}
		else
		{
			set_frame(bink, bink->FrameNum+1);
		}
		bink->io.Working= 0;
	}
	return;
}

void __stdcall BinkGoto(
	BINK *bink,
	unsigned long frame,
	unsigned long flags)
{
	if (bink)
	{
		if (!frame)
		{
			frame= 1;
		}
		bink->io.Working= 1;
		if (frame > bink->Frames)
		{
			frame= bink->Frames;
		}
		if (!(flags&BINK_GOTO_QUICK))
		{
			if (bink->FrameNum != frame)
			{
				unsigned long key= BinkGetKeyFrame(bink, frame, BINK_GET_KEY_PREVIOUS);
				if (frame < bink->FrameNum || key > bink->FrameNum)
				{
					set_frame(bink, key);
					if (key == frame)
					{
						bink->io.Working= 0;
						return;
					}
				}
				BinkPause(bink, 1);
				if (bink->FrameNum != bink->lastdecompframe)
				{
					BinkDoFrame(bink);
				}
				BinkNextFrame(bink);
				while (bink->FrameNum != frame)
				{
					BinkDoFrame(bink);
					BinkNextFrame(bink);
				}
				BinkPause(bink, 0);
			}
		}
		else
		{
			set_frame(bink, frame);
		}
		bink->io.Working= 0;
	}
	return;
}

/* Trim zero-mask block rows/columns. The caller supplies a nonnegative,
 * block-aligned source rectangle within its mask allocation. Empty regions
 * return zero without writing the destination, as in January. */
static long trim_rect(
	BINKRECT *destination,
	const unsigned char *mask,
	long pitch,
	const BINKRECT *rectangle)
{
	const unsigned char *start= mask+(rectangle->Top/16)*pitch+rectangle->Left/16;
	long height= rectangle->Height/16;
	long width= rectangle->Width/16;
	const unsigned char *row= start;
	long i;
	long j;
	for (i= 0; i < height; ++i)
	{
		for (j= 0; j < width; ++j)
		{
			if (row[j])
			{
				goto found_top;
			}
		}
		row+= pitch;
	}
	return 0;
found_top:
	start+= i*pitch;
	height-= i;
	destination->Top= rectangle->Top+i*16;
	destination->Height= rectangle->Height-i*16;
	if (destination->Height > 1)
	{
		row= start+(height-1)*pitch;
		for (i= 0; i < height; ++i)
		{
			for (j= 0; j < width; ++j)
			{
				if (row[j])
				{
					goto found_bottom;
				}
			}
			row-= pitch;
		}
found_bottom:
		height-= i;
		destination->Height+= (-i)*16;
	}
	for (i= 0; i < width; ++i)
	{
		row= start+i;
		for (j= 0; j < height; ++j)
		{
			if (*row)
			{
				goto found_left;
			}
			row+= pitch;
		}
	}
found_left:
	width-= i;
	start+= i;
	destination->Left= rectangle->Left+i*16;
	destination->Width= rectangle->Width-i*16;
	if (destination->Width > 1)
	{
		for (i= width-1; i >= 0; --i)
		{
			row= start+i;
			for (j= 0; j < height; ++j)
			{
				if (*row)
				{
					goto found_right;
				}
				row+= pitch;
			}
		}
found_right:
		destination->Width+= (i-width)*16+16;
	}
	return 1;
}

/* A tightly trimmed input has occupied blocks on both sides of any interior
 * split. Thus both subrectangles are nonempty; trim_rect does not leave their
 * output storage uninitialized. Prefer width splitting on equal savings. */
static long split_rect(
	const BINKRECT *rectangle,
	BINKRECT *first,
	BINKRECT *second,
	const unsigned char *mask,
	long pitch)
{
	BINKRECT half;
	BINKRECT vertical_first;
	BINKRECT vertical_second;
	long split;
	long horizontal_savings;
	long vertical_savings;
	if (rectangle->Width >= 32)
	{
		half= *rectangle;
		split= (half.Width/2+15)&~15L;
		half.Width= split;
		trim_rect(first, mask, pitch, &half);
		half= *rectangle;
		half.Left+= split;
		half.Width-= split;
		trim_rect(second, mask, pitch, &half);
		horizontal_savings= rectangle->Width*rectangle->Height-
			first->Width*first->Height-second->Width*second->Height;
	}
	else
	{
		horizontal_savings= 0;
	}
	if (rectangle->Height >= 32)
	{
		half= *rectangle;
		split= (half.Height/2+15)&~15L;
		half.Height= split;
		trim_rect(&vertical_first, mask, pitch, &half);
		half= *rectangle;
		half.Top+= split;
		half.Height-= split;
		trim_rect(&vertical_second, mask, pitch, &half);
		vertical_savings= rectangle->Width*rectangle->Height-
			vertical_first.Width*vertical_first.Height-
			vertical_second.Width*vertical_second.Height;
		if (vertical_savings > horizontal_savings)
		{
			*first= vertical_first;
			*second= vertical_second;
			return vertical_savings;
		}
	}
	return horizontal_savings;
}

long __stdcall BinkGetRects(
	BINK *bink,
	unsigned long flags)
{
	BINKRECT rectangles[BINKMAXDIRTYRECTS];
	BINKRECT first[BINKMAXDIRTYRECTS];
	BINKRECT second[BINKMAXDIRTYRECTS];
	long savings[BINKMAXDIRTYRECTS];
	long i;
	long largest;
	unsigned long key;
	unsigned long smallest;
	if (flags&BINKSURFACEDIRECT)
	{
		return 0;
	}
	if (!(flags&BINKNOSKIP) && !(bink->OpenFlags&BINKNOSKIP) && bink->skippedlastblit)
	{
		return 0;
	}
	if (bink->NumRects == -1)
	{
		rectangles[0].Width= bink->dirty_width;
		rectangles[0].Height= bink->dirty_height;
		rectangles[0].Left= rectangles[0].Top= 0;
		bink->NumRects= trim_rect(&bink->FrameRects[0], bink->dirty_mask,
			bink->dirty_pitch, &rectangles[0]);
		if (!(flags&BINKSURFACESLOW) && bink->NumRects)
		{
			long best_split;
			long scan;
			rectangles[0]= bink->FrameRects[0];
			savings[0]= split_rect(&rectangles[0], &first[0], &second[0],
				bink->dirty_mask, bink->dirty_pitch);
			for (;;)
			{
				best_split= -1;
				largest= 0;
				for (scan= 0; scan < bink->NumRects; ++scan)
				{
					if (savings[scan] > largest)
					{
						best_split= scan;
						largest= savings[scan];
					}
				}
				if (best_split == -1)
				{
					break;
				}
				rectangles[best_split]= first[best_split];
				rectangles[bink->NumRects]= second[best_split];
				++bink->NumRects;
				if (bink->NumRects == BINKMAXDIRTYRECTS)
				{
					break;
				}
				savings[best_split]= split_rect(&rectangles[best_split], &first[best_split],
					&second[best_split], bink->dirty_mask, bink->dirty_pitch);
				best_split= bink->NumRects-1;
				savings[best_split]= split_rect(&rectangles[best_split], &first[best_split],
					&second[best_split], bink->dirty_mask, bink->dirty_pitch);
			}
		}
		for (i= 0; i < bink->NumRects; ++i)
		{
			long j;
			long best;
			smallest= 0x7fff0000UL;
			for (j= 0; j < bink->NumRects; ++j)
			{
				key= ((unsigned long)rectangles[j].Top<<16)+rectangles[j].Left;
				if (key < smallest)
				{
					smallest= key;
					best= j;
				}
			}
			switch (bink->lastblitflags&BINKCOPYNOSCALING)
			{
			case BINKCOPY2XH:
			case BINKCOPY2XHI:
				rectangles[best].Top*= 2;
				rectangles[best].Height*= 2;
				break;
			case BINKCOPY2XW:
				rectangles[best].Left*= 2;
				rectangles[best].Width*= 2;
				break;
			case BINKCOPY2XWH:
			case BINKCOPY2XWHI:
				rectangles[best].Left*= 2;
				rectangles[best].Width*= 2;
				rectangles[best].Top*= 2;
				rectangles[best].Height*= 2;
				break;
			}
			key= rectangles[best].Left+rectangles[best].Width;
			if (key > bink->Width)
			{
				rectangles[best].Width+= bink->Width-key;
			}
			key= rectangles[best].Top+rectangles[best].Height;
			if (key > bink->Height)
			{
				rectangles[best].Height+= bink->Height-key;
			}
			bink->FrameRects[i]= rectangles[best];
			rectangles[best].Left= 0;
			rectangles[best].Top= 0x7fffffffL;
		}
	}
	return bink->NumRects;
}

void __stdcall BinkGetSummary(
	BINK *bink,
	BINKSUMMARY *summary)
{
	unsigned long now;
	unsigned long total_frame_bytes;
	if (bink && summary)
	{
		now= RADTimerRead();
		if (bink->startblittime)
		{
			bink->timeblit+= now-bink->startblittime;
			bink->startblittime= 0;
		}
		update_slowest_frame(bink, now);
		memset(summary, 0, sizeof(*summary));
		summary->FrameRate= bink->FrameRate;
		summary->FrameRateDiv= bink->FrameRateDiv;
		summary->SkippedBlits= bink->skippedblits;
		summary->SoundSkips= bink->soundskips;
		summary->FileFrameRate= bink->fileframerate;
		summary->FileFrameRateDiv= bink->fileframeratediv;
		summary->TotalFrames= bink->Frames;
		summary->TotalPlayedFrames= bink->playedframes;
		summary->TotalTime= RADTimerRead()-bink->firstframetime;
		summary->TotalOpenTime= bink->timeopen;
		summary->TotalAudioDecompTime= bink->timeadecomp;
		summary->TotalVideoDecompTime= bink->timevdecomp;
		summary->TotalBlitTime= bink->timeblit;
		summary->HighestMemAmount+= bink->totalmem;
		summary->TotalIOMemory= bink->iosize;
		summary->TotalReadSpeed= mult64anddiv(bink->io.BytesRead, 1000, bink->io.TotalTime+1);
		summary->TotalReadTime= bink->io.ForegroundTime;
		summary->TotalIdleReadTime= bink->io.IdleTime;
		summary->TotalBackReadTime= bink->io.ThreadTime;
		total_frame_bytes= bink->Size-(bink->frameoffsets[0]&~1UL);
		summary->AverageDataRate= mult64anddiv(total_frame_bytes,
			bink->fileframerate, bink->Frames*bink->fileframeratediv);
		summary->AverageFrameSize= total_frame_bytes/bink->Frames;
		summary->Highest1SecRate= bink->Highest1SecRate;
		summary->Highest1SecFrame= bink->Highest1SecFrame+1;
		summary->Width= bink->Width;
		summary->Height= bink->Height;
		summary->SlowestFrameTime= bink->slowestframetime;
		summary->Slowest2FrameTime= bink->slowest2frametime;
		summary->SlowestFrameNum= bink->slowestframe;
		summary->Slowest2FrameNum= bink->slowest2frame;
		/* Original overwrites the allocation count with the backend's BufSize. */
		summary->TotalIOMemory= bink->io.BufSize;
		summary->HighestIOUsed= bink->io.BufHighUsed;
	}
	return;
}

void __stdcall BinkGetRealtime(
	BINK *bink,
	BINKREALTIME *realtime,
	unsigned long frames)
{
	unsigned long now= RADTimerRead();
	if (bink->startblittime)
	{
		bink->timeblit+= now-bink->startblittime;
		bink->startblittime= 0;
	}
	update_slowest_frame(bink, now);
	/* Original requires a valid handle, output, and allocated history arrays. */
	if (!frames || frames >= bink->runtimeframes)
	{
		frames= bink->runtimeframes-1;
	}
	if (frames > bink->FrameNum)
	{
		frames= bink->FrameNum-1;
		if (!frames)
		{
			frames= 1;
		}
	}
	realtime->FrameNum= bink->LastFrameNum;
	realtime->FrameRate= bink->FrameRate;
	/* BUG (original): offset 0x14 is read twice, not FrameRateDiv at 0x18.
	 * A corrected nonmatching build should use bink->FrameRateDiv here. */
	realtime->FrameRateDiv= bink->FrameRate;
	realtime->ReadBufferSize= bink->io.CurBufSize;
	realtime->ReadBufferUsed= bink->io.CurBufUsed;
	realtime->FramesDataRate= mult64anddiv(
		bink->frameoffsets[bink->FrameNum]-bink->frameoffsets[bink->FrameNum-frames],
		bink->fileframerate, bink->fileframeratediv*frames);
	realtime->Frames= frames;
	realtime->FramesTime= bink->rtframetimes[0]-bink->rtframetimes[frames];
	if (!realtime->FramesTime)
	{
		realtime->FramesTime= 1;
	}
	realtime->FramesVideoDecompTime= bink->rtvdecomptimes[0]-bink->rtvdecomptimes[frames];
	realtime->FramesAudioDecompTime= bink->rtadecomptimes[0]-bink->rtadecomptimes[frames];
	realtime->FramesBlitTime= bink->rtblittimes[0]-bink->rtblittimes[frames];
	realtime->FramesReadTime= bink->rtreadtimes[0]-bink->rtreadtimes[frames];
	realtime->FramesIdleReadTime= bink->rtidlereadtimes[0]-bink->rtidlereadtimes[frames];
	realtime->FramesThreadReadTime= bink->rtthreadreadtimes[0]-bink->rtthreadreadtimes[frames];
	return;
}
