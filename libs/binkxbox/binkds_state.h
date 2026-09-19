#ifndef BINK_DS_STATE_H
#define BINK_DS_STATE_H
struct IDirectSoundBuffer;
/* January Xbox backend: BinkDS open clears exactly 0xc4 sound bytes.
 * The 0x44-byte callback/format prefix is followed by this 0x80-byte state.
 * Last 32 bytes have no recovered fields. This is not the later SDK layout. */
struct bink_ds_state
{
	struct IDirectSoundBuffer *buffer;
	void *memory;
	unsigned long buffer_size;
	unsigned long chunk_size;
	unsigned long frequency;
	long bits;
	long channels;
	long volume;
	long pan;
	long started;
	long paused;
	unsigned long write_offset;
	unsigned long ready_bytes;
	unsigned long prime_bytes;
	unsigned long previous_write_cursor;
	unsigned long previous_play_cursor;
	unsigned long dropout_threshold;
	unsigned long last_timer;
	unsigned long timeout;
	unsigned long lock_len1;
	void *lock_ptr1;
	unsigned long lock_len2;
	void *lock_ptr2;
	unsigned long mixbins;
	unsigned char reserved_0060[32];
};
#endif
