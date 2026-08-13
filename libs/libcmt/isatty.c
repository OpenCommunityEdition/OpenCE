/*
ISATTY.C

symbols in this file:
	__isatty (.text, 0x2A)
*/

/* ---------- constants */

enum
{
	_ioinfo_array_shift = 5,
	_ioinfo_array_mask = 31,
	_file_device = 0x40
};

/* ---------- types */

struct runtime_critical_section
{
	void *debug_information;
	long lock_count;
	long recursion_count;
	void *owning_thread;
	void *lock_semaphore;
	unsigned long spin_count;
};

struct io_info
{
	long os_file_handle;
	char flags;
	unsigned char pipe_character;
	unsigned short alignment;
	int lock_initialized;
	struct runtime_critical_section lock;
	void *raw_event;
};

typedef char assert_runtime_critical_section_size[
	sizeof(struct runtime_critical_section) == 0x18 ? 1 : -1];
typedef char assert_io_info_size[
	sizeof(struct io_info) == 0x28 ? 1 : -1];

/* ---------- external data */

extern struct io_info *__pioinfo[];
extern int _nhandle;

/* ---------- public code */

int __cdecl _isatty(
	int file_handle)
{
	if ((unsigned int)file_handle >= (unsigned int)_nhandle)
		return 0;

	return __pioinfo[file_handle >> _ioinfo_array_shift]
		[file_handle & _ioinfo_array_mask].flags & _file_device;
}
