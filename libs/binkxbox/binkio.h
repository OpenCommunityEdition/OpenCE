#ifndef BINK_IO_H
#define BINK_IO_H
#include "radplatform.h"
#include "radcb.h"
#include <stddef.h>

struct BINK;
struct BINKIO;
typedef unsigned long (__stdcall *bink_io_read_header_proc)(
	struct BINKIO *io,
	long offset,
	void *destination,
	unsigned long size);
typedef unsigned long (__stdcall *bink_io_read_frame_proc)(
	struct BINKIO *io,
	unsigned long frame,
	long offset,
	void *destination,
	unsigned long size);
typedef unsigned long (__stdcall *bink_io_buffer_size_proc)(
	struct BINKIO *io,
	unsigned long size);
typedef void (__stdcall *bink_io_set_info_proc)(
	struct BINKIO *io,
	void *buffer,
	unsigned long size,
	unsigned long file_size,
	unsigned long simulate);
typedef unsigned long (__stdcall *bink_io_idle_proc)(
	struct BINKIO *io);
typedef void (__stdcall *bink_io_callback_proc)(
	struct BINKIO *io);
typedef long (__stdcall *bink_io_try_suspend_proc)(
	struct BINKIO *io);

/* January file-backend state in the 128-byte iodata area. Fields are named
 * from their actual uses, not claimed as recovered RAD private identifiers.
 * This is a typed view owned by Bink IO, not casts through a byte array.
 */
struct bink_file_state
{
	HANDLE handle;
	unsigned long read_position;
	unsigned long logical_position;
	unsigned char *read_buffer;
	unsigned long empty_bytes;
	unsigned long reserved_0014;
	unsigned char *buffer;
	unsigned char *buffer_end;
	unsigned char *write_buffer;
	unsigned long borrowed_handle;
	unsigned long start_offset;
	unsigned long file_size;
	unsigned long simulate_rate;
	long wait_debt;
	unsigned char reserved_0038[72];
};

/* Public field names and callback contracts are corroborated by the later
 * 1.5t Xbox SDK, but the January layout is smaller: callback_control[8],
 * 128 bytes of backend data, and total size 0xfc. The callback control
 * storage now has its genuine RADCB record type (24 bytes + 8 reserved). Every offset below is
 * pinned to BinkFileOpen / the six January callbacks, not the later SDK.
 * Volatile qualifiers from that later header are intentionally NOT imported
 * into this work-in-progress source. Concurrent access equivalence remains
 * unverified; do not admit the file backend as a runtime replacement yet.
 */
typedef struct BINKIO
{
	bink_io_read_header_proc ReadHeader;
	bink_io_read_frame_proc ReadFrame;
	bink_io_buffer_size_proc GetBufferSize;
	bink_io_set_info_proc SetInfo;
	bink_io_idle_proc Idle;
	bink_io_callback_proc Close;
	struct BINK *bink;
	unsigned long ReadError;
	unsigned long DoingARead;
	unsigned long BytesRead;
	unsigned long Working;
	unsigned long TotalTime;
	unsigned long ForegroundTime;
	unsigned long IdleTime;
	unsigned long ThreadTime;
	unsigned long BufSize;
	unsigned long BufHighUsed;
	unsigned long CurBufSize;
	unsigned long CurBufUsed;
	struct bink_file_state file;
	bink_io_callback_proc suspend_callback;
	bink_io_try_suspend_proc try_suspend_callback;
	bink_io_callback_proc resume_callback;
	bink_io_callback_proc idle_on_callback;
	struct radcb_callback callback;
	unsigned long reserved_callback_tail[2];
} BINKIO;

typedef char bink_file_state_size_assert[sizeof(struct bink_file_state)==128 ? 1:-1];
typedef char bink_io_size_assert[sizeof(BINKIO)==0xfc ? 1:-1];
typedef char bink_io_file_offset_assert[offsetof(BINKIO,file)==0x4c ? 1:-1];
typedef char bink_io_suspend_offset_assert[offsetof(BINKIO,suspend_callback)==0xcc ? 1:-1];
typedef char bink_io_resume_offset_assert[offsetof(BINKIO,resume_callback)==0xd4 ? 1:-1];
#endif
