/* January Xbox Bink DirectSound backend. C++ is proven by its PDB compile
 * record; member wrappers come from the supplied XDK, not invented thunks. */
#pragma code_seg("BINK")
#include <xtl.h>
#include <string.h>
#include <stdlib.h>
extern "C" {
#include "binkds.h"
#include "radmem.h"
#include "radtimer.h"
}

typedef HRESULT (__stdcall *ds_create_proc)(
	LPGUID guid,
	LPDIRECTSOUND *direct_sound,
	LPUNKNOWN outer_unknown);
#pragma data_seg("BINKDATA")
HWND MSShWnd= 0;
LPDIRECTSOUND lpds= (LPDIRECTSOUND)-1;
long ds_create_owned= 0;
long ds_device_users= 0;
long ds_primary_owned= 0;
ds_create_proc ds_create= 0;
long request_primary_format= 0;
LPDIRECTSOUNDBUFFER prim= 0;
long emulated_ds= 0;
short ds_volume_table[128]= {
#include "binkds_volume_table.inc"
};
long ds_open_count= 0;
#pragma data_seg()

static void apply_settings(
	struct bink_ds_state *state);
static void lock_range(
	struct bink_ds_state *state,
	unsigned long offset,
	unsigned long bytes);
static void unlock_range(
	struct bink_ds_state *state);
static void fill_silence(
	struct bink_ds_state *state,
	unsigned long offset,
	unsigned long bytes);
static void start_play(
	struct bink_ds_state *state);
static void stop_play(
	struct bink_ds_state *state);
static long acquire_device(
	void);
static void release_device(
	void);

#pragma code_seg("BINK")
static void apply_settings(
	struct bink_ds_state *state)
{
	if (state->buffer)
	{
		unsigned long frequency= state->frequency;
		long volume= state->volume;
		if (volume>32767) volume= 32767;
		state->buffer->SetFrequency(frequency);
		state->buffer->SetVolume(ds_volume_table[volume*127/32767]);
		state->buffer->SetHeadroom(0);
		state->buffer->SetMixBins(state->mixbins);
	}
	return;
}
static void lock_range(
	struct bink_ds_state *state,
	unsigned long offset,
	unsigned long bytes)
{
	state->buffer->Lock(offset,bytes,&state->lock_ptr1,&state->lock_len1,
		&state->lock_ptr2,&state->lock_len2,0);
	return;
}
static void unlock_range(
	struct bink_ds_state *state)
{
	state->buffer->Unlock(state->lock_ptr1,state->lock_len1,
		state->lock_ptr2,state->lock_len2);
	return;
}
static void fill_silence(
	struct bink_ds_state *state,
	unsigned long offset,
	unsigned long bytes)
{
	lock_range(state,offset,bytes);
	unsigned long silence= state->bits==16 ? 0:0x80808080UL;
	memset(state->lock_ptr1,silence,state->lock_len1);
	if (state->lock_ptr2)
		memset(state->lock_ptr2,silence,state->lock_len2);
	unlock_range(state);
	return;
}
static void resume_play(
	struct bink_ds_state *state)
{
	state->buffer->Play(0,0,DSBPLAY_LOOPING);
	state->paused= 0;
	return;
}
static void stop_play(
	struct bink_ds_state *state)
{
	state->buffer->Stop();
	state->paused= 1;
	return;
}
static void start_play(
	struct bink_ds_state *state)
{
	state->started= 1;
	resume_play(state);
	do
	{
		state->buffer->GetCurrentPosition(&state->previous_play_cursor,
			&state->previous_write_cursor);
	} while (state->previous_play_cursor>state->chunk_size);
	state->last_timer= RADTimerRead();
	return;
}
static long __stdcall sound_ready(
	struct BINKSND *sound)
{
	if (sound->ds.paused || !sound->OnOff) return 0;
	if (!sound->ds.started) return 1;
	unsigned long write= sound->ds.previous_write_cursor;
	unsigned long play= sound->ds.previous_play_cursor;
	sound->ds.buffer->GetCurrentPosition(&play,&write);
	unsigned long moved_write= write>=sound->ds.previous_write_cursor ?
		write-sound->ds.previous_write_cursor:
		sound->ds.buffer_size-sound->ds.previous_write_cursor+write;
	unsigned long moved_play= play>=sound->ds.previous_play_cursor ?
		play-sound->ds.previous_play_cursor:
		sound->ds.buffer_size-sound->ds.previous_play_cursor+play;
	/* Valid cursor advances are bounded by the allocated buffer. */
	if (abs((long)(moved_play-moved_write))>(long)(sound->ds.buffer_size/2)) return 0;
	sound->ds.previous_write_cursor= write;
	sound->ds.previous_play_cursor= play;
	unsigned long now= RADTimerRead();
	unsigned long elapsed= now-sound->ds.last_timer;
	sound->ds.last_timer= now;
	unsigned long available;
	if (elapsed>=sound->ds.timeout) goto dropout;
	for (;;)
	{
		available= play<sound->ds.write_offset ? sound->ds.buffer_size-sound->ds.write_offset+play:
			play-sound->ds.write_offset;
		if (available<1024) available= 0;
		else available= (available-1024)&~3UL;
		if (available<sound->ds.chunk_size && (!sound->ds.ready_bytes || !available)) return 0;
		if (available<sound->ds.dropout_threshold) break;
	dropout:
		sound->SoundDroppedOut= 1;
		fill_silence(&sound->ds,sound->ds.previous_write_cursor,sound->ds.buffer_size/2);
		sound->ds.write_offset= sound->ds.previous_write_cursor+sound->ds.buffer_size/2;
		if (sound->ds.write_offset>=sound->ds.buffer_size) sound->ds.write_offset-= sound->ds.buffer_size;
	}
	sound->ds.ready_bytes= available;
	return 1;
}
static long __stdcall sound_lock(
	struct BINKSND *sound,
	unsigned char **destination,
	unsigned long *bytes)
{
	lock_range(&sound->ds,sound->ds.write_offset,sound->ds.ready_bytes);
	*destination= (unsigned char *)sound->ds.lock_ptr1;
	*bytes= sound->ds.lock_len1;
	return 1;
}
static long __stdcall sound_unlock(
	struct BINKSND *sound,
	unsigned long bytes)
{
	sound->ds.write_offset+= bytes;
	if (sound->ds.write_offset>=sound->ds.buffer_size) sound->ds.write_offset-= sound->ds.buffer_size;
	sound->ds.ready_bytes-= bytes;
	unlock_range(&sound->ds);
	sound->ds.lock_ptr1= 0;
	if (!sound->ds.started && !sound->ds.ready_bytes && !sound->ds.paused) start_play(&sound->ds);
	return 1;
}
static void __stdcall sound_volume(
	struct BINKSND *sound,
	long volume)
{
	if (volume>32767) volume= 32767;
	sound->ds.volume= volume;
	apply_settings(&sound->ds);
	return;
}
static void __stdcall sound_pan(
	struct BINKSND *sound,
	long pan)
{
	if (pan>65536) pan= 65536;
	sound->ds.pan= pan;
	apply_settings(&sound->ds);
	return;
}
static void __stdcall sound_mix_bins(
	struct BINKSND *sound,
	unsigned long bins)
{
	sound->ds.mixbins= bins;
	apply_settings(&sound->ds);
	return;
}
static long __stdcall sound_on_off(
	struct BINKSND *sound,
	long on)
{
	if (on)
	{
		if (!sound->OnOff)
		{
			sound->ds.started= 0;
			sound->OnOff= 1;
			sound->ds.write_offset= 0;
			sound->ds.ready_bytes= sound->ds.prime_bytes;
		}
		return sound->OnOff;
	}
	else if (sound->OnOff)
	{
		unsigned long status;
		sound->ds.buffer->Stop();
		do { status= 0; sound->ds.buffer->GetStatus(&status); } while (status);
		sound->ds.buffer->SetCurrentPosition(0);
		fill_silence(&sound->ds,0,sound->ds.buffer_size);
		sound->OnOff= 0;
		return 0;
	}
	return sound->OnOff;
}
static long __stdcall sound_pause(
	struct BINKSND *sound,
	long pause)
{
	if (pause)
	{
		if (sound->ds.started) stop_play(&sound->ds);
		sound->ds.paused= 1;
		return 1;
	}
	if (sound->ds.started || !sound->ds.ready_bytes) start_play(&sound->ds);
	sound->ds.paused= 0;
	return sound->ds.paused;
}
static long acquire_device(
	void)
{
	if (++ds_open_count==1)
	{
		if (ds_create_owned)
		{
			if (!ds_create) { ds_open_count= 0; return 0; }
			if (ds_create(0,&lpds,0)<0) { --ds_open_count; return 0; }
		}
		emulated_ds= 0;
	}
	return 1;
}
static void release_device(
	void)
{
	if (--ds_open_count==0 && ds_create_owned && lpds && lpds!=(LPDIRECTSOUND)-1)
	{
		lpds->Release();
		lpds= (LPDIRECTSOUND)-1;
	}
	return;
}
static void __stdcall sound_close(
	struct BINKSND *sound)
{
	if (sound->ds.buffer)
	{
		stop_play(&sound->ds);
		sound->ds.buffer->Release();
		sound->ds.buffer= 0;
		radfree(sound->ds.memory);
		release_device();
	}
	return;
}
static long __stdcall sound_open(
	struct BINKSND *sound,
	unsigned long frequency,
	long bits,
	long channels,
	unsigned long flags,
	struct BINK *bink)
{
	WAVEFORMATEX format;
	DSBUFFERDESC desc;
	memset(sound,0,sizeof(*sound));
	if (!acquire_device()) return 0;
	sound->ds.bits= sound->Bits= bits;
	sound->ds.channels= sound->Channels= channels;
	format.wBitsPerSample= (unsigned short)sound->Bits;
	format.nChannels= (unsigned char)channels;
	format.nSamplesPerSec= frequency;
	format.nBlockAlign= (format.wBitsPerSample>>3)*format.nChannels;
	unsigned long bytes_per_second= (format.wBitsPerSample>>3)*format.nChannels*frequency;
	sound->ds.frequency= sound->Frequency= frequency;
	sound->SoundDroppedOut= 0;
	unsigned long movie_bytes= (unsigned long)((unsigned __int64)bink->Frames*bink->FrameRateDiv*bytes_per_second/bink->FrameRate);
	format.wFormatTag= WAVE_FORMAT_PCM;
	format.nAvgBytesPerSec= bytes_per_second;
	format.cbSize= 0;
	sound->ds.chunk_size= (bytes_per_second*50/1000+255)&~255UL;
	sound->ds.buffer_size= (bytes_per_second*600/1000+1023)&~1023UL;
	sound->ds.dropout_threshold= sound->ds.buffer_size*90/100;
	sound->ds.timeout= 540;
	sound->Latency= 0;
	sound->ds.memory= radmalloc(sound->ds.buffer_size);
	memset(&desc,0,sizeof(desc));
	desc.dwSize= sizeof(desc);
	desc.dwFlags= DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLVOLUME;
	desc.lpwfxFormat= &format;
	/* January returns immediately on buffer-creation failure. No silent cleanup
	 * fix is folded into this reconstruction; failure ownership remains explicit. */
	if (lpds->CreateSoundBuffer(&desc,&sound->ds.buffer,0)<0) return 0;
	sound->ds.buffer->SetBufferData(sound->ds.memory,sound->ds.buffer_size);
	sound->BestSizeIn16= sound->ds.chunk_size;
	if (sound->Bits==8) sound->BestSizeIn16*= 2;
	sound->ds.volume= sound->ds.pan= 32767;
	sound->ds.mixbins= 0;
	apply_settings(&sound->ds);
	fill_silence(&sound->ds,0,sound->ds.buffer_size);
	sound->ds.write_offset= 0;
	sound->ds.started= 0;
	if (movie_bytes>sound->ds.buffer_size) movie_bytes= sound->ds.buffer_size;
	sound->ds.paused= 0;
	sound->ds.prime_bytes= sound->ds.ready_bytes= movie_bytes;
	sound->OnOff= 1;
	sound->Ready= sound_ready;
	sound->Lock= sound_lock;
	sound->Unlock= sound_unlock;
	sound->Volume= sound_volume;
	sound->Pan= sound_pan;
	sound->Pause= sound_pause;
	sound->SetOnOff= sound_on_off;
	sound->Close= sound_close;
	sound->MixBins= sound_mix_bins;
	return 1;
}
extern "C" bink_sound_open_proc __stdcall BinkOpenDirectSound(
	unsigned long direct_sound)
{
	LPDIRECTSOUND device= (LPDIRECTSOUND)direct_sound;
	if (lpds==(LPDIRECTSOUND)-1) lpds= device;
	else if (lpds!=device)
	{
		if (ds_device_users) return 0;
		lpds= device;
	}
	if (lpds) { ds_create_owned= 0; ds_primary_owned= 0; }
	else { ds_create_owned= 1; ds_create= DirectSoundCreate; }
	if (!acquire_device()) return 0;
	release_device();
	return sound_open;
}
