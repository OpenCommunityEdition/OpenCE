/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "cache/cache_files.h"
#include "cache/cache_files_decompress_windows.h"
/* port: the port's zlib (1.3), not the game's 1.1.3, inflates the maps,
which are anyone's files; its inflate needs about 40 KB of ZLIB_BUFFER_SIZE,
and frees what it takes in the reverse order, as cache_copy_compressed_free
wants */
#include "../../port/third_party/zlib/zlib_prefixed.h"

#include <xtl.h>

/* ---------- constants */

enum
{
	FILE_BLOCK_SIZE = 0x20000,

	NUMBER_OF_READ_BUFFERS = 8,
	NUMBER_OF_WRITE_BUFFERS = 1,

	_read_buffer_base = 0,
	_raw_read_offset = NUMBER_OF_READ_BUFFERS,
	_write_buffer_base,
	_raw_write_offset = _write_buffer_base + NUMBER_OF_WRITE_BUFFERS,
	NUMBER_OF_OVERLAPPED_STRUCTURES,

	READ_BUFFER_SIZE = FILE_BLOCK_SIZE,
	WRITE_BUFFER_SIZE = 0x400000,
	ZLIB_BUFFER_SIZE = 0x12000,

	TOTAL_BUFFER_SIZE =
		NUMBER_OF_READ_BUFFERS * READ_BUFFER_SIZE +
		NUMBER_OF_WRITE_BUFFERS * WRITE_BUFFER_SIZE +
		ZLIB_BUFFER_SIZE,

	DECOMPRESSOR_MESSAGE_BUFFER_SIZE = 256,

	CACHE_COPY_THREAD_STACK_SIZE = 0x4000,
};

enum
{
	_copy_write_failed_bit,
	_copy_read_failed_bit,
	_copy_bad_file_bit,
	NUMBER_OF_COPY_FLAGS,

	ALL_COPY_FAILURE_FLAGS =
		FLAG(_copy_write_failed_bit) | FLAG(_copy_read_failed_bit) | FLAG(_copy_bad_file_bit)
};

/* timer 0 is never printed; it brackets the copy thread's read-data + zlib setup */
enum decompressor_timer
{
	_decompressor_timer_setup,
	_decompressor_timer_read_file,
	_decompressor_timer_write_file,
	_decompressor_timer_zlib,
	_decompressor_timer_zlib_during_write_file,
	_decompressor_timer_thread_blocked,
	_decompressor_timer_thread_blocked_on_read,
	_decompressor_timer_thread_blocked_on_write,
	_decompressor_timer_copying,
	NUMBER_OF_DECOMPRESSOR_TIMERS
};

/* ---------- macros */

/* ---------- structures */

struct cache_file_header
{
	unsigned long header_signature;
	long version;
	long size;
	byte reservedC[4];
	long tag_data_offset;
	long tag_data_size;
	byte reserved18[8];
	char name[0x20];
	char build[0x20];
	byte reserved60[4];
	unsigned long checksum;
	byte reserved68[0x794];
	unsigned long footer_signature;
};

struct cache_copy_read_request
{
	short read_sequence_index;
};

struct cache_copy_write_request
{
	short write_sequence_index;
};

struct simple_decompressor_definition
{
	char src_name[260];
	struct cache_file_header header;
	volatile unsigned long flags;
	z_stream zlib_stream;
	byte *zlib_buffer;
	long zlib_buffer_size;
	byte *next_allocation;
	HANDLE copy_start_event;
	HANDLE copy_stop_event;
	HANDLE copy_complete_event;
	HANDLE progress_update_event;
	HANDLE copy_thread;
	void *allocated_buffer;
	void *read_buffers[NUMBER_OF_READ_BUFFERS];
	void *write_buffers[NUMBER_OF_WRITE_BUFFERS];
	boolean blocking;
	byte pad0[3];
	HANDLE destination_file;
	HANDLE source_file;
	long overlapped_in_use_flags[BIT_VECTOR_SIZE_IN_LONGS(NUMBER_OF_OVERLAPPED_STRUCTURES)];
	long overlapped_completed_flags[BIT_VECTOR_SIZE_IN_LONGS(NUMBER_OF_OVERLAPPED_STRUCTURES)];
	OVERLAPPED overlapped[NUMBER_OF_OVERLAPPED_STRUCTURES];
	struct cache_copy_read_request read_requests[NUMBER_OF_READ_BUFFERS];
	struct cache_copy_write_request write_requests[NUMBER_OF_WRITE_BUFFERS];
	byte pad1[2];
	long read_file_size;
	long async_read_bytes_left;
	long read_bytes_left;
	long async_write_bytes_left;
	long write_bytes_left;
	volatile real read_progress;
	long current_write_offset;
	long current_read_offset;
	struct cache_copy_read_request *current_request;
	struct cache_copy_write_request *current_write_request;
	long write_requests_pending;
	short next_read_sequence_index;
	short current_read_sequence_index;
	short current_write_buffer_index;
	short current_write_sequence_index;
	short next_write_sequence_index;
	short current_read_sequence_count;
	byte pad2[4];
	LARGE_INTEGER overlapped_timer_starts[NUMBER_OF_OVERLAPPED_STRUCTURES];
	byte pad3[0x30];
};

struct decompressor_runtime_globals
{
	char message[DECOMPRESSOR_MESSAGE_BUFFER_SIZE];
	long times[NUMBER_OF_DECOMPRESSOR_TIMERS];
	LARGE_INTEGER timer_starts[NUMBER_OF_DECOMPRESSOR_TIMERS];
	struct simple_decompressor_definition self;
};

typedef char verify_cache_file_header_size[
	sizeof(struct cache_file_header) == 0x800 ? 1 : -1];
typedef char verify_simple_decompressor_zlib_stream_offset[
	offsetof(struct simple_decompressor_definition, zlib_stream) == 0x908 ? 1 : -1];
typedef char verify_simple_decompressor_copy_stop_event_offset[
	offsetof(struct simple_decompressor_definition, copy_stop_event) == 0x950 ? 1 : -1];
typedef char verify_simple_decompressor_copy_complete_event_offset[
	offsetof(struct simple_decompressor_definition, copy_complete_event) == 0x954 ? 1 : -1];
typedef char verify_simple_decompressor_copy_thread_offset[
	offsetof(struct simple_decompressor_definition, copy_thread) == 0x95C ? 1 : -1];
typedef char verify_simple_decompressor_blocking_offset[
	offsetof(struct simple_decompressor_definition, blocking) == 0x988 ? 1 : -1];
typedef char verify_simple_decompressor_overlapped_offset[
	offsetof(struct simple_decompressor_definition, overlapped) == 0x99C ? 1 : -1];
typedef char verify_simple_decompressor_read_requests_offset[
	offsetof(struct simple_decompressor_definition, read_requests) == 0xA78 ? 1 : -1];
typedef char verify_simple_decompressor_read_progress_offset[
	offsetof(struct simple_decompressor_definition, read_progress) == 0xAA0 ? 1 : -1];
typedef char verify_simple_decompressor_current_request_offset[
	offsetof(struct simple_decompressor_definition, current_request) == 0xAAC ? 1 : -1];
typedef char verify_simple_decompressor_overlapped_timers_offset[
	offsetof(struct simple_decompressor_definition, overlapped_timer_starts) == 0xAC8 ? 1 : -1];
typedef char verify_simple_decompressor_size[
	sizeof(struct simple_decompressor_definition) == 0xB50 ? 1 : -1];
typedef char verify_decompressor_runtime_globals_times_offset[
	offsetof(struct decompressor_runtime_globals, times) == 0x100 ? 1 : -1];
typedef char verify_decompressor_runtime_globals_timer_starts_offset[
	offsetof(struct decompressor_runtime_globals, timer_starts) == 0x128 ? 1 : -1];
typedef char verify_decompressor_runtime_globals_self_offset[
	offsetof(struct decompressor_runtime_globals, self) == 0x170 ? 1 : -1];
typedef char verify_decompressor_runtime_globals_size[
	sizeof(struct decompressor_runtime_globals) == 0xCC0 ? 1 : -1];

/* ---------- prototypes */

static void cache_copy_print_timing(
	void);
static unsigned long cache_copy_get_flags(
	void);
static voidpf cache_copy_compressed_alloc(
	voidpf opaque,
	uInt items,
	uInt size);
static void cache_copy_compressed_free(
	voidpf opaque,
	voidpf address);
static boolean any_bit_vector_flag_set(
	long *bit_vector,
	long size_in_longs);
static void decompressor_timer_start(
	long timer_index);
static void decompressor_timer_stop(
	long timer_index);
static void decompressor_reset_timing(
	void);
static boolean cache_copy_stop_requested(
	void);
static void cache_copy_yield(
	void);
static void cache_copy_set_flag(
	short flag);
static long cache_copy_read_buffer_size(
	void);
static long cache_copy_write_buffer_size(
	short write_buffer_index);
static void *cache_copy_get_read_buffer(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request);
static void *cache_copy_get_write_buffer(
	struct simple_decompressor_definition *self,
	short write_buffer_index);
static void cache_copy_initialize_zlib(
	struct simple_decompressor_definition *self);
static void cache_copy_dispose_zlib(
	struct simple_decompressor_definition *self);
static void cache_copy_initialize_read_buffers(
	struct simple_decompressor_definition *self);
static void cache_copy_initialize_file_data(
	struct simple_decompressor_definition *self);
static void cache_copy_block_on_raw_read(
	struct simple_decompressor_definition *self);
static void cache_copy_block_on_raw_write(
	struct simple_decompressor_definition *self);
static void cache_copy_wait_for_async_io(
	struct simple_decompressor_definition *self);
static void cache_copy_issue_read(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset,
	short overlapped_index);
static void cache_copy_issue_write(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset,
	short write_buffer_index);
static void cache_copy_issue_read_raw(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset);
static void cache_copy_issue_write_raw(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset);
static void cache_copy_issue_read_internal(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request,
	short read_buffer_index);
static void cache_copy_issue_read_request(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request);
static void cache_copy_issue_read_by_index(
	struct simple_decompressor_definition *self,
	short read_buffer_index);
static void cache_copy_release_read_request(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request);
static void cache_copy_issue_write_internal(
	struct simple_decompressor_definition *self,
	short write_buffer_index);
static void cache_copy_update_write_buffers(
	struct simple_decompressor_definition *self);
static void cache_copy_initialize_read_data(
	struct simple_decompressor_definition *self);
static void cache_copy_issue_initial_reads(
	struct simple_decompressor_definition *self);
static void cache_copy_run_decompression(
	struct simple_decompressor_definition *self);
static unsigned long __stdcall simple_cache_copy_thread(
	void *parameter);
void CALLBACK cache_copy_FileIOCompletionRoutine(
	unsigned long error_code,
	unsigned long bytes_transferred,
	OVERLAPPED *overlapped);

/* ---------- globals */

static struct decompressor_runtime_globals decompressor_globals;
static struct simple_decompressor_definition *global_self = &decompressor_globals.self;
static long performance_frequency = 1;
/* January PDB: public, module 33 .bss+0xcc0; explicit zero avoids COMMON. */
boolean decompressor_print_timing = FALSE;

/* ---------- code */

static boolean cache_copy_stop_requested(
	void)
{
	return WaitForSingleObject(global_self->copy_stop_event, 0) == 0;
}

long cache_copy_buffer_size(
	boolean blocking)
{
	cache_copy_set_priority(blocking);

	return TOTAL_BUFFER_SIZE;
}

void cache_copy_set_priority(
	boolean blocking)
{
	struct simple_decompressor_definition *self = global_self;

	self->blocking = blocking;
	if (blocking)
		SetThreadPriority(self->copy_thread, THREAD_PRIORITY_ABOVE_NORMAL);
	else
		SetThreadPriority(self->copy_thread, THREAD_PRIORITY_NORMAL);

	return;
}

boolean cache_copy_compressed_file_complete(
	void)
{
	return WaitForSingleObject(global_self->copy_complete_event, 0) == 0;
}

void cache_copy_begin(
	void *buffer,
	long size,
	HANDLE destination_file,
	long destination_file_size,
	const char *source_file_name)
{
	if (WaitForSingleObject(global_self->copy_complete_event, 0) == 0)
	{
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			524,
			source_file_name);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			525,
			destination_file!=INVALID_HANDLE_VALUE);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			526,
			buffer);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			527,
			size>= TOTAL_BUFFER_SIZE);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			528,
			destination_file_size==GetFileSize(destination_file, NULL));

		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			530,
			global_self->copy_complete_event);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			531,
			global_self->copy_stop_event);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			532,
			global_self->copy_start_event);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			533,
			global_self->progress_update_event);

		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			535,
			global_self->copy_thread);

		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			537,
			global_self->zlib_stream.zalloc);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			538,
			global_self->zlib_stream.zfree);

		global_self->flags = 0;
		global_self->allocated_buffer = buffer;
		csstrcpy(global_self->src_name, source_file_name);
		global_self->destination_file = destination_file;
		global_self->read_progress = 0.0f;

		ResetEvent(global_self->copy_complete_event);
		ResetEvent(global_self->copy_stop_event);

		csmemset(&global_self->header, 0, sizeof(global_self->header));

		SetEvent(global_self->copy_start_event);
	}
	else
	{
		match_vassert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			564,
			FALSE,
			"previous copy session did not complete");
	}

	return;
}

void cache_copy_queue_end(
	void)
{
	if (WaitForSingleObject(global_self->copy_complete_event, 0))
		SetEvent(global_self->copy_stop_event);

	return;
}

static void cache_copy_initialize_zlib(
	struct simple_decompressor_definition *self)
{
	self->zlib_stream.next_in = NULL;
	self->zlib_stream.avail_in = 0;
	self->zlib_stream.next_out = NULL;
	self->zlib_stream.avail_out = 0;

	inflateInit(&self->zlib_stream);

	return;
}

static void cache_copy_dispose_zlib(
	struct simple_decompressor_definition *self)
{
	inflateEnd(&self->zlib_stream);

	self->zlib_stream.next_in = NULL;
	self->zlib_stream.avail_in = 0;
	self->zlib_stream.next_out = NULL;
	self->zlib_stream.avail_out = 0;

	return;
}

static voidpf cache_copy_compressed_alloc(
	voidpf opaque,
	uInt items,
	uInt size)
{
	byte *address = global_self->next_allocation;

	global_self->next_allocation = address + items * size;
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		701,
		global_self->next_allocation-global_self->zlib_buffer<global_self->zlib_buffer_size);

	return address;
}

static void cache_copy_compressed_free(
	voidpf opaque,
	voidpf address)
{
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		710,
		(byte*)address<=global_self->next_allocation);
	global_self->next_allocation = address;

	return;
}

static void cache_copy_initialize_read_buffers(
	struct simple_decompressor_definition *self)
{
	byte *address = self->allocated_buffer;
	short read_buffer_index;

	for (read_buffer_index = 0; read_buffer_index < NUMBER_OF_READ_BUFFERS; read_buffer_index++)
	{
		global_self->read_buffers[read_buffer_index] = address;
		address += cache_copy_read_buffer_size();
	}
	global_self->write_buffers[0] = address;

	XPhysicalProtect(self->allocated_buffer, TOTAL_BUFFER_SIZE, PAGE_READWRITE);
	csmemset(self->allocated_buffer, 0xfd, TOTAL_BUFFER_SIZE - ZLIB_BUFFER_SIZE);
	XPhysicalProtect(self->allocated_buffer, TOTAL_BUFFER_SIZE - ZLIB_BUFFER_SIZE, PAGE_READONLY);

	self->zlib_buffer_size = ZLIB_BUFFER_SIZE;
	self->zlib_buffer = (byte *)self->allocated_buffer + TOTAL_BUFFER_SIZE - ZLIB_BUFFER_SIZE;
	self->next_allocation = self->zlib_buffer;

	csmemset(
		&self->source_file,
		0xfa,
		sizeof(*self) - offsetof(struct simple_decompressor_definition, source_file));

	return;
}

static void cache_copy_initialize_file_data(
	struct simple_decompressor_definition *self)
{
	short read_buffer_index;
	short write_buffer_index;

	self->source_file = CreateFile(
		self->src_name,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
		NULL);
	self->read_bytes_left = GetFileSize(self->source_file, NULL);
	self->read_file_size = self->read_bytes_left;
	self->async_read_bytes_left = self->read_bytes_left;

	csmemset(self->overlapped, 0, sizeof(self->overlapped));

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		964,
		self->read_bytes_left>=sizeof(self->header));
	/* port: a map that did not open, or is too short to hold a header, is a
	bad file, not copied (the assertion goes on in a release build, and an
	unopened file's size is -1, which it compared unsigned) */
	if (self->source_file == INVALID_HANDLE_VALUE ||
		self->read_bytes_left < (long)sizeof(self->header))
	{
		cache_copy_set_flag(_copy_bad_file_bit);
	}

	csmemset(self->overlapped_in_use_flags, 0, sizeof(self->overlapped_in_use_flags));
	csmemset(self->overlapped_completed_flags, 0, sizeof(self->overlapped_completed_flags));

	for (read_buffer_index = 0; read_buffer_index < NUMBER_OF_READ_BUFFERS; read_buffer_index++)
		self->read_requests[read_buffer_index].read_sequence_index = NONE;
	for (write_buffer_index = 0; write_buffer_index < NUMBER_OF_WRITE_BUFFERS; write_buffer_index++)
		self->write_requests[write_buffer_index].write_sequence_index = NONE;

	self->next_read_sequence_index = 0;
	self->current_read_sequence_index = 0;
	self->current_write_sequence_index = 0;
	self->next_write_sequence_index = 0;
	self->write_requests_pending = 0;
	self->current_write_buffer_index = NONE;

	return;
}

static void cache_copy_block_on_raw_read(
	struct simple_decompressor_definition *self)
{
	unsigned long wait_result = WaitForSingleObjectEx(self->copy_stop_event, 5000, TRUE);

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1482,
		!BIT_VECTOR_TEST_FLAG(self->overlapped_in_use_flags, _raw_read_offset));
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1483,
		wait_result==WAIT_IO_COMPLETION);

	BIT_VECTOR_SET_FLAG(self->overlapped_in_use_flags, _raw_read_offset, FALSE);

	return;
}

static void cache_copy_block_on_raw_write(
	struct simple_decompressor_definition *self)
{
	unsigned long wait_result;

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1494,
		BIT_VECTOR_TEST_FLAG(self->overlapped_in_use_flags, _raw_write_offset));

	wait_result = WaitForSingleObjectEx(self->copy_stop_event, 5000, TRUE);

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1498,
		!BIT_VECTOR_TEST_FLAG(self->overlapped_in_use_flags, _raw_write_offset));
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1499,
		wait_result==WAIT_IO_COMPLETION);

	BIT_VECTOR_SET_FLAG(self->overlapped_in_use_flags, _raw_write_offset, FALSE);

	return;
}

struct cache_copy_read_request *acquire_read_request(
	struct simple_decompressor_definition *self,
	short read_sequence_index)
{
	struct cache_copy_read_request *request = NULL;
	short read_buffer_index;

	for (read_buffer_index = 0; read_buffer_index < NUMBER_OF_READ_BUFFERS; read_buffer_index++)
	{
		if (self->read_requests[read_buffer_index].read_sequence_index == read_sequence_index &&
			BIT_VECTOR_TEST_FLAG(self->overlapped_completed_flags, _read_buffer_base + read_buffer_index))
		{
			request = &self->read_requests[read_buffer_index];
			XPhysicalProtect(
				self->read_buffers[read_buffer_index],
				FILE_BLOCK_SIZE,
				PAGE_READONLY);
			break;
		}
	}

	return request;
}

static long cache_copy_read_buffer_size(
	void)
{
	return READ_BUFFER_SIZE;
}

static void *cache_copy_get_read_buffer(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request)
{
	short read_buffer_index = (short)(request - self->read_requests);

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1606,
		read_buffer_index>=0 && read_buffer_index<NUMBER_OF_READ_BUFFERS);

	return self->read_buffers[read_buffer_index];
}

static void *cache_copy_get_write_buffer(
	struct simple_decompressor_definition *self,
	short write_buffer_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1633,
		write_buffer_index>=0 && write_buffer_index<NUMBER_OF_WRITE_BUFFERS);

	return self->write_buffers[write_buffer_index];
}

static long cache_copy_write_buffer_size(
	short write_buffer_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1642,
		write_buffer_index>=0 && write_buffer_index<NUMBER_OF_WRITE_BUFFERS);

	return WRITE_BUFFER_SIZE;
}

static boolean any_bit_vector_flag_set(
	long *bit_vector,
	long size_in_longs)
{
	boolean flag_set = FALSE;
	short index;

	for (index = 0; index < size_in_longs; index++)
		flag_set = flag_set || bit_vector[index];

	return flag_set;
}

static void cache_copy_wait_for_async_io(
	struct simple_decompressor_definition *self)
{
	short retry_count = NUMBER_OF_OVERLAPPED_STRUCTURES;

	while (any_bit_vector_flag_set(
			self->overlapped_in_use_flags,
			BIT_VECTOR_SIZE_IN_LONGS(NUMBER_OF_OVERLAPPED_STRUCTURES)) &&
		retry_count--)
	{
		unsigned long wait_result = SleepEx(5000, TRUE);

		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1695,
			wait_result==WAIT_IO_COMPLETION);
	}

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1699,
		!any_bit_vector_flag_set(self->overlapped_in_use_flags, BIT_VECTOR_SIZE_IN_LONGS(NUMBER_OF_OVERLAPPED_STRUCTURES)));

	csmemset(self->overlapped_completed_flags, 0, sizeof(self->overlapped_completed_flags));

	return;
}

static void cache_copy_set_flag(
	short flag)
{
	SET_FLAG(global_self->flags, flag, TRUE);

	return;
}

static unsigned long cache_copy_get_flags(
	void)
{
	return global_self->flags;
}

static void decompressor_reset_timing(
	void)
{
	csmemset(decompressor_globals.times, 0, sizeof(decompressor_globals.times));

	return;
}

static void decompressor_timer_start(
	long timer_index)
{
	QueryPerformanceCounter(&decompressor_globals.timer_starts[timer_index]);

	return;
}

static void decompressor_timer_stop(
	long timer_index)
{
	LARGE_INTEGER now;

	QueryPerformanceCounter(&now);
	decompressor_globals.times[timer_index] +=
		now.u.LowPart - decompressor_globals.timer_starts[timer_index].u.LowPart;

	return;
}

static void cache_copy_print_timing(
	void)
{
	error(_error_silent, "Timing for copying last cache file:");
	error(_error_silent, "    Total read file time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_read_file] / performance_frequency);
	error(_error_silent, "    Total write file time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_write_file] / performance_frequency);
	error(_error_silent, "    Total zlib time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_zlib] / performance_frequency);
	error(_error_silent, "    Total zlib during write file time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_zlib_during_write_file] / performance_frequency);
	error(_error_silent, "    Total thread blocked time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_thread_blocked] / performance_frequency);
	error(_error_silent, "    Total thread blocked on read time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_thread_blocked_on_read] / performance_frequency);
	error(_error_silent, "    Total thread blocked on write time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_thread_blocked_on_write] / performance_frequency);
	error(_error_silent, "    Total copying time: %.3f",
		(double)decompressor_globals.times[_decompressor_timer_copying] / performance_frequency);

	return;
}

static void cache_copy_yield(
	void)
{
	if (!global_self->blocking)
		SwitchToThread();

	return;
}

short cache_copy_get_status(
	real *progress)
{
	unsigned long flags = cache_copy_get_flags();
	short status = 0;

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		574,
		progress);

	if (global_self->blocking)
		Sleep(16);

	if (!flags && global_self->copy_thread)
	{
		if (global_self->header.size > 0)
		{
			status = (short)((WaitForSingleObject(global_self->copy_complete_event, 0) == 0) +
				_cache_copy_in_progress);
			if (WaitForSingleObject(global_self->progress_update_event, 0) == 0)
			{
				real read_progress;

				if (global_self->read_progress < 0.0f)
				{
					read_progress = 0.0f;
				}
				else if (global_self->read_progress > 1.0f)
				{
					read_progress = 1.0f;
				}
				else
				{
					read_progress = global_self->read_progress;
				}

				*progress = read_progress;
			}
		}
		else
		{
			*progress = 0.0f;
			status = _cache_copy_in_progress;
		}
	}
	else
	{
		if (TEST_FLAG(flags, _copy_read_failed_bit))
			status = _cache_copy_read_failure;
		else if (TEST_FLAG(flags, _copy_bad_file_bit))
			status = _cache_copy_bad_file_failure;
		else if (TEST_FLAG(flags, _copy_write_failed_bit))
			status = _cache_copy_write_failure;
		else
			match_assert(
				"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
				626,
				!"unreachable");
		*progress = 0.0f;
	}

	return status;
}

void cache_copy_end(
	void)
{
	if (WaitForSingleObject(global_self->copy_complete_event, 0))
	{
		SetEvent(global_self->copy_stop_event);
		WaitForSingleObject(global_self->copy_complete_event, INFINITE);
	}

	if (decompressor_print_timing)
		cache_copy_print_timing();

	return;
}

void CALLBACK cache_copy_FileIOCompletionRoutine(
	unsigned long error_code,
	unsigned long bytes_transferred,
	OVERLAPPED *overlapped)
{
	long *in_use_flags = global_self->overlapped_in_use_flags;
	long *completed_flags = global_self->overlapped_completed_flags;
	long overlapped_index = overlapped - global_self->overlapped;

	if (!error_code)
	{
		if (overlapped_index >= 0 && overlapped_index < NUMBER_OF_OVERLAPPED_STRUCTURES)
		{
			LARGE_INTEGER now;

			QueryPerformanceCounter(&now);
			BIT_VECTOR_SET_FLAG(in_use_flags, overlapped_index, FALSE);
			BIT_VECTOR_SET_FLAG(completed_flags, overlapped_index, TRUE);
		}

		if (overlapped_index >= _read_buffer_base &&
			overlapped_index <= _read_buffer_base + NUMBER_OF_READ_BUFFERS - 1)
		{
			match_assert(
				"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
				1291,
				global_self->async_read_bytes_left>0);

			global_self->async_read_bytes_left -= bytes_transferred;

			ResetEvent(global_self->progress_update_event);
			global_self->read_progress =
				(real)(global_self->header.size - global_self->async_read_bytes_left) /
				global_self->header.size;
			SetEvent(global_self->progress_update_event);
		}
		else if (overlapped_index >= _write_buffer_base &&
			overlapped_index <= _write_buffer_base + NUMBER_OF_WRITE_BUFFERS - 1)
		{
			match_assert(
				"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
				1300,
				global_self->async_write_bytes_left>0);

			global_self->async_write_bytes_left -= bytes_transferred;
		}
	}
	else
	{
		match_vassert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1306,
			FALSE,
			csprintf(
				decompressor_globals.message,
				"async i/o finished with error code %d",
				error_code));

		if (overlapped_index >= _write_buffer_base &&
			overlapped_index <= _write_buffer_base + NUMBER_OF_WRITE_BUFFERS - 1)
			cache_copy_set_flag(_copy_write_failed_bit);
		else
			cache_copy_set_flag(_copy_read_failed_bit);
	}

	return;
}

static void cache_copy_issue_read(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset,
	short overlapped_index)
{
	HANDLE file;
	OVERLAPPED *overlapped;
	long error;
	BOOL issued;

	decompressor_timer_start(_decompressor_timer_read_file);

	file = self->source_file;
	overlapped = &self->overlapped[overlapped_index];

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1334,
		!BIT_VECTOR_TEST_FLAG(self->overlapped_in_use_flags, overlapped_index));
	BIT_VECTOR_SET_FLAG(self->overlapped_in_use_flags, overlapped_index, TRUE);

	overlapped->hEvent = (HANDLE)overlapped_index;
	overlapped->Offset = offset;
	overlapped->OffsetHigh = 0;

	QueryPerformanceCounter(&self->overlapped_timer_starts[overlapped_index]);

	do
	{
		SleepEx(0, TRUE);
		SetLastError(ERROR_SUCCESS);
		issued = ReadFileEx(file, buffer, size, overlapped, cache_copy_FileIOCompletionRoutine);
		error = GetLastError();
	}
	while (!issued &&
		(error == ERROR_INVALID_USER_BUFFER || error == ERROR_NOT_ENOUGH_MEMORY ||
			error == ERROR_NO_SYSTEM_RESOURCES));

	if (!issued)
	{
		match_vassert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1369,
			FALSE,
			"couldn't issue an asynchronous read");
		cache_copy_set_flag(_copy_read_failed_bit);
	}

	decompressor_timer_stop(_decompressor_timer_read_file);

	return;
}

static void cache_copy_issue_write(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset,
	short write_buffer_index)
{
	long overlapped_index;
	HANDLE file;
	OVERLAPPED *overlapped;
	long error;
	BOOL issued;

	decompressor_timer_start(_decompressor_timer_write_file);

	file = self->destination_file;
	overlapped_index = _write_buffer_base + write_buffer_index;
	overlapped = &self->overlapped[overlapped_index];

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1411,
		!BIT_VECTOR_TEST_FLAG(self->overlapped_in_use_flags, overlapped_index));
	BIT_VECTOR_SET_FLAG(self->overlapped_in_use_flags, overlapped_index, TRUE);

	overlapped->hEvent = (HANDLE)overlapped_index;
	overlapped->Offset = offset;
	overlapped->OffsetHigh = 0;

	QueryPerformanceCounter(&self->overlapped_timer_starts[overlapped_index]);

	do
	{
		SleepEx(0, TRUE);
		SetLastError(ERROR_SUCCESS);
		issued = WriteFileEx(file, buffer, size, overlapped, cache_copy_FileIOCompletionRoutine);
		error = GetLastError();
	}
	while (!issued &&
		(error == ERROR_INVALID_USER_BUFFER || error == ERROR_NOT_ENOUGH_MEMORY ||
			error == ERROR_NO_SYSTEM_RESOURCES));

	if (!issued)
	{
		match_vassert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1441,
			FALSE,
			"couldn't issue an asynchronous write");
		cache_copy_set_flag(_copy_write_failed_bit);
	}

	decompressor_timer_stop(_decompressor_timer_write_file);

	return;
}

static void cache_copy_issue_read_raw(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset)
{
	cache_copy_issue_read(self, buffer, size, offset, _raw_read_offset);

	return;
}

static void cache_copy_issue_write_raw(
	struct simple_decompressor_definition *self,
	void *buffer,
	long size,
	long offset)
{
	cache_copy_issue_write(self, buffer, size, offset, NUMBER_OF_WRITE_BUFFERS);

	return;
}

static void cache_copy_issue_read_internal(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request,
	short read_buffer_index)
{
	void *buffer = cache_copy_get_read_buffer(self, request);
	long size = self->read_bytes_left < FILE_BLOCK_SIZE ? self->read_bytes_left : FILE_BLOCK_SIZE;

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1516,
		read_buffer_index>=0 && read_buffer_index<NUMBER_OF_READ_BUFFERS);
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1517,
		request->read_sequence_index==NONE);

	request->read_sequence_index = self->next_read_sequence_index;

	XPhysicalProtect(buffer, size, PAGE_READWRITE);
	cache_copy_issue_read(self, buffer, size, self->current_read_offset, read_buffer_index);

	self->next_read_sequence_index++;
	self->read_bytes_left -= size;
	self->current_read_offset += size;

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1537,
		self->current_read_offset<=self->read_file_size);

	return;
}

static void cache_copy_issue_read_request(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request)
{
	cache_copy_issue_read_internal(self, request, (short)(request - self->read_requests));

	return;
}

static void cache_copy_issue_read_by_index(
	struct simple_decompressor_definition *self,
	short read_buffer_index)
{
	struct cache_copy_read_request *request = &self->read_requests[read_buffer_index];

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1561,
		read_buffer_index>=0 && read_buffer_index<NUMBER_OF_READ_BUFFERS);

	cache_copy_issue_read_internal(self, request, read_buffer_index);

	return;
}

static void cache_copy_release_read_request(
	struct simple_decompressor_definition *self,
	struct cache_copy_read_request *request)
{
	short read_buffer_index = (short)(request - self->read_requests);

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1618,
		read_buffer_index>=0 && read_buffer_index<NUMBER_OF_READ_BUFFERS);
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1619,
		BIT_VECTOR_TEST_FLAG(self->overlapped_completed_flags, _read_buffer_base+read_buffer_index));

	BIT_VECTOR_SET_FLAG(
		self->overlapped_completed_flags,
		_read_buffer_base + read_buffer_index,
		FALSE);
	request->read_sequence_index = NONE;

	cache_copy_issue_read_request(self, request);

	return;
}

static void cache_copy_issue_write_internal(
	struct simple_decompressor_definition *self,
	short write_buffer_index)
{
	void *buffer = cache_copy_get_write_buffer(self, write_buffer_index);
	long size = self->write_bytes_left < cache_copy_write_buffer_size(write_buffer_index) ?
		self->write_bytes_left :
		cache_copy_write_buffer_size(write_buffer_index);

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1654,
		write_buffer_index>=0 && write_buffer_index<NUMBER_OF_WRITE_BUFFERS);

	XPhysicalProtect(buffer, WRITE_BUFFER_SIZE, PAGE_READONLY);

	cache_copy_issue_write(
		self,
		cache_copy_get_write_buffer(self, write_buffer_index),
		size,
		self->current_write_offset,
		write_buffer_index);

	self->current_write_offset += size;
	self->write_bytes_left -= size;

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1666,
		self->current_write_offset<=self->header.size);

	return;
}

static void cache_copy_initialize_read_data(
	struct simple_decompressor_definition *self)
{
	self->async_write_bytes_left = 0;
	csmemset(&self->header, 0, sizeof(self->header));

	cache_copy_issue_write_raw(self, &self->header, sizeof(self->header), 0);
	cache_copy_block_on_raw_write(self);

	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		1003,
		global_self->async_write_bytes_left==0);

	cache_copy_issue_read_raw(self, &self->header, sizeof(self->header), 0);
	cache_copy_block_on_raw_read(self);

	cache_file_header_verify(&self->header, "blah", TRUE);

	self->read_bytes_left -= sizeof(self->header);
	self->current_read_offset = sizeof(self->header);
	self->current_write_offset = sizeof(self->header);
	self->read_progress = 0.0f;
	self->current_request = NULL;
	self->current_read_sequence_count = 0;
	self->current_write_request = NULL;

	return;
}

static void cache_copy_issue_initial_reads(
	struct simple_decompressor_definition *self)
{
	short overlapped_index;

	for (overlapped_index = 0; overlapped_index < NUMBER_OF_READ_BUFFERS; overlapped_index++)
	{
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1046,
			!BIT_VECTOR_TEST_FLAG(self->overlapped_in_use_flags, overlapped_index));

		cache_copy_issue_read_by_index(self, overlapped_index);

		BIT_VECTOR_SET_FLAG(self->overlapped_in_use_flags, overlapped_index, TRUE);
	}

	return;
}

static void cache_copy_update_write_buffers(
	struct simple_decompressor_definition *self)
{
	short write_buffer_index;

	if (self->write_requests_pending > 0 && self->current_write_request)
	{
		for (write_buffer_index = 0; write_buffer_index < NUMBER_OF_WRITE_BUFFERS; write_buffer_index++)
		{
			if (BIT_VECTOR_TEST_FLAG(self->overlapped_completed_flags, _write_buffer_base + write_buffer_index))
			{
				self->write_requests[write_buffer_index].write_sequence_index = NONE;
				BIT_VECTOR_SET_FLAG(self->overlapped_completed_flags, _write_buffer_base + write_buffer_index, FALSE);
				self->write_requests_pending--;
				self->current_write_request = NULL;

				match_assert(
					"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
					1075,
					self->write_requests_pending>=0);
			}
		}
	}

	if (self->write_bytes_left &&
		!self->current_write_request &&
		self->write_requests_pending > 0)
	{
		if (self->current_write_buffer_index == NONE ||
			self->write_requests[self->current_write_buffer_index].write_sequence_index >
				self->current_write_sequence_index)
		{
			for (write_buffer_index = 0;
				write_buffer_index < NUMBER_OF_WRITE_BUFFERS;
				write_buffer_index++)
			{
				if (self->current_write_buffer_index != write_buffer_index &&
					self->write_requests[write_buffer_index].write_sequence_index ==
						self->current_write_sequence_index)
				{
					self->current_write_request = &self->write_requests[write_buffer_index];
					cache_copy_issue_write_internal(self, write_buffer_index);
					self->current_write_sequence_index++;
					break;
				}
			}
		}
	}

	if (self->current_write_buffer_index == NONE && self->write_requests_pending < 1)
	{
		for (write_buffer_index = 0;
			write_buffer_index < NUMBER_OF_WRITE_BUFFERS;
			write_buffer_index++)
		{
			if (self->write_requests[write_buffer_index].write_sequence_index == NONE)
			{
				self->write_requests[write_buffer_index].write_sequence_index =
					self->next_write_sequence_index;
				self->next_write_sequence_index++;
				self->write_requests_pending++;
				self->current_write_buffer_index = write_buffer_index;
				XPhysicalProtect(
					self->write_buffers[write_buffer_index],
					WRITE_BUFFER_SIZE,
					PAGE_READWRITE);
				break;
			}
		}

		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1124,
			self->current_write_buffer_index!=NONE);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1125,
			self->write_requests_pending<=NUMBER_OF_WRITE_BUFFERS);
		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1126,
			self->write_requests[self->current_write_buffer_index].write_sequence_index+1==self->next_write_sequence_index);
	}

	return;
}

static void cache_copy_run_decompression(
	struct simple_decompressor_definition *self)
{
	z_stream *zlib_stream = &self->zlib_stream;

	for (;;)
	{
		long zlib_result;

		SleepEx(0, TRUE);
		cache_copy_update_write_buffers(self);

		if (!zlib_stream->avail_in)
		{
			match_assert(
				"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
				1149,
				!self->current_request);
			match_assert(
				"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
				1150,
				!self->current_read_sequence_count);

			self->current_request = acquire_read_request(self, self->current_read_sequence_index);
			if (self->current_request)
			{
				zlib_stream->avail_in = FILE_BLOCK_SIZE;
				zlib_stream->next_in = cache_copy_get_read_buffer(self, self->current_request);
				self->current_read_sequence_count = 1;

				match_assert(
					"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
					1163,
					zlib_stream->avail_in==FILE_BLOCK_SIZE);
			}
			else
			{
				break;
			}
		}

		match_assert(
			"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
			1172,
			self->current_request);

		if (self->current_write_buffer_index == NONE)
			break;

		if (!zlib_stream->avail_out)
		{
			zlib_stream->next_out = cache_copy_get_write_buffer(self, self->current_write_buffer_index);
			zlib_stream->avail_out = cache_copy_write_buffer_size(self->current_write_buffer_index);
		}

		if (!zlib_stream->avail_in || !zlib_stream->avail_out)
			continue;

		cache_copy_yield();

		decompressor_timer_start(_decompressor_timer_zlib);
		if (self->write_requests_pending > 1)
			decompressor_timer_start(_decompressor_timer_zlib_during_write_file);

		zlib_result = inflate(zlib_stream, 0);

		decompressor_timer_stop(_decompressor_timer_zlib);
		if (self->write_requests_pending > 1)
			decompressor_timer_stop(_decompressor_timer_zlib_during_write_file);

		/* port: a stream that ends before the size the header gives the map
		is a bad file: the rest of the cache file would be taken for it */
		if (zlib_result == Z_STREAM_END &&
			zlib_stream->total_out != (uLong)(self->header.size - sizeof(self->header)))
		{
			match_vassert(
				"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
				1248,
				FALSE,
				csprintf(
					decompressor_globals.message,
					"decompression ended after %lu of %ld bytes",
					(unsigned long)zlib_stream->total_out,
					self->header.size - (long)sizeof(self->header)));
			cache_copy_set_flag(_copy_bad_file_bit);

			break;
		}

		if (zlib_result == Z_OK || zlib_result == Z_STREAM_END)
		{
			if (!zlib_stream->avail_in)
			{
				cache_copy_release_read_request(self, self->current_request);
				self->current_read_sequence_index++;
				self->current_read_sequence_count--;
				self->current_request = NULL;
			}

			if (!zlib_stream->avail_out || zlib_result == Z_STREAM_END)
				self->current_write_buffer_index = NONE;
		}
		else
		{
			if (cache_copy_stop_requested())
				break;

			match_vassert(
				"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
				1248,
				FALSE,
				csprintf(
					decompressor_globals.message,
					"decompression fucked up with error code (%d), msg '%s'",
					zlib_result,
					zlib_stream->msg ? zlib_stream->msg : ""));
			cache_copy_set_flag(_copy_bad_file_bit);

			break;
		}
	}

	return;
}

static unsigned long __stdcall simple_cache_copy_thread(
	void *parameter)
{
	struct simple_decompressor_definition *self = global_self;

	for (;;)
	{
		WaitForSingleObject(self->copy_start_event, INFINITE);

		decompressor_reset_timing();
		decompressor_timer_start(_decompressor_timer_copying);

		cache_copy_initialize_read_buffers(self);
		cache_copy_initialize_file_data(self);

		/* port: nor is a bad file (cache_copy_initialize_file_data) read */
		if (!cache_copy_stop_requested() && !(self->flags & ALL_COPY_FAILURE_FLAGS))
		{
			decompressor_timer_start(_decompressor_timer_setup);
			cache_copy_initialize_read_data(self);
			cache_copy_initialize_zlib(self);
			decompressor_timer_stop(_decompressor_timer_setup);

			/* port: a header that is not a map's is a bad file, so the copy
			fails (it ended as if it had worked, and the map was precached
			again, or, of no size, seemed never to end) */
			if (!cache_file_header_verify(&self->header, "cache decompressed", TRUE))
			{
				cache_copy_set_flag(_copy_bad_file_bit);
			}
			else
			{
				boolean keep_going = TRUE;

				self->write_bytes_left = self->header.size - sizeof(self->header);
				self->async_write_bytes_left = self->write_bytes_left;

				cache_copy_issue_initial_reads(self);

				while (!cache_copy_stop_requested() && self->write_bytes_left > 0 && keep_going)
				{
					unsigned long wait_result;

					if (self->overlapped_in_use_flags[0])
					{
						boolean blocked_on_read =
							acquire_read_request(self, self->current_read_sequence_index) == NULL;
						boolean blocked_on_write =
							self->write_requests_pending == 1 &&
							self->current_write_buffer_index == NONE;

						if (blocked_on_read || blocked_on_write)
						{
							if (blocked_on_read)
								decompressor_timer_start(_decompressor_timer_thread_blocked_on_read);
							if (blocked_on_write)
								decompressor_timer_start(_decompressor_timer_thread_blocked_on_write);
							decompressor_timer_start(_decompressor_timer_thread_blocked);

							SetEvent(self->progress_update_event);
							wait_result = WaitForSingleObjectEx(self->copy_stop_event, 5000, TRUE);

							decompressor_timer_stop(_decompressor_timer_thread_blocked);
							if (blocked_on_write)
								decompressor_timer_stop(_decompressor_timer_thread_blocked_on_write);
							if (blocked_on_read)
								decompressor_timer_stop(_decompressor_timer_thread_blocked_on_read);
						}
						else
						{
							wait_result = WAIT_IO_COMPLETION;
						}
					}
					else
					{
						wait_result = WAIT_IO_COMPLETION;
					}

					keep_going = FALSE;
					switch (wait_result)
					{
						case WAIT_OBJECT_0:
							break;

						case WAIT_IO_COMPLETION:
							match_assert(
								"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
								849,
								any_bit_vector_flag_set(self->overlapped_completed_flags, BIT_VECTOR_SIZE_IN_LONGS(NUMBER_OF_OVERLAPPED_STRUCTURES)));

							cache_copy_update_write_buffers(self);
							cache_copy_run_decompression(self);

							keep_going = !(global_self->flags & ALL_COPY_FAILURE_FLAGS);
							break;

						case WAIT_TIMEOUT:
							match_vassert(
								"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
								866,
								FALSE,
								"timeout for asynchronous i/o");
							cache_copy_set_flag(_copy_read_failed_bit);
							break;

						default:
							match_assert(
								"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
								875,
								!"unreachable");
							break;
					}
				}

				if (!self->write_bytes_left)
				{
					cache_copy_wait_for_async_io(self);
					cache_copy_issue_write_raw(self, &self->header, sizeof(self->header), 0);
				}
			}

			cache_copy_dispose_zlib(self);
		}

		cache_copy_wait_for_async_io(self);

		CloseHandle(self->source_file);
		self->source_file = NULL;

		decompressor_timer_stop(_decompressor_timer_copying);

		self->destination_file = NULL;
		SetEvent(self->copy_complete_event);
	}

	return 0;
}

void cache_copy_initialize(
	void)
{
	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);
	match_assert(
		"c:\\halo\\SOURCE\\cache\\cache_files_decompress_windows.c",
		487,
		freq.u.HighPart==0);
	performance_frequency = freq.u.LowPart;

	global_self->copy_complete_event = CreateEvent(NULL, TRUE, TRUE, NULL);
	global_self->copy_start_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	global_self->copy_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	global_self->progress_update_event = CreateEvent(NULL, TRUE, FALSE, NULL);

	global_self->zlib_stream.zalloc = cache_copy_compressed_alloc;
	global_self->zlib_stream.zfree = cache_copy_compressed_free;

	global_self->copy_thread = CreateThread(
		NULL,
		CACHE_COPY_THREAD_STACK_SIZE,
		simple_cache_copy_thread,
		NULL,
		0,
		NULL);

	return;
}
