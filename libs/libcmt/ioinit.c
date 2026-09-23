/*
IOINIT.C

Initialize and release the CRT's low-level I/O handle table.
*/

#include <stddef.h>
#include <stdlib.h>

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

enum
{
	_ioinfo_array_count= 64,
	_ioinfo_entries_per_array= 32,
	_file_open= 0x01,
	_file_device= 0x40,
	_file_text= 0x80,
	_standard_file_flags= _file_open | _file_device | _file_text,
	_newline= 10
};

extern struct io_info *__pioinfo[_ioinfo_array_count];
extern int _nhandle;

struct io_info __badioinfo= { -1, _file_text, _newline };

int __cdecl _ioinit(
	void)
{
	struct io_info *info;
	int index;

	info= (struct io_info *)malloc(_ioinfo_entries_per_array*sizeof(*info));
	if (!info)
		return -1;
	__pioinfo[0]= info;
	_nhandle= _ioinfo_entries_per_array;

	for (; info<__pioinfo[0]+_ioinfo_entries_per_array; info++)
	{
		info->flags= 0;
		info->os_file_handle= -1;
		info->lock_initialized= 0;
		info->pipe_character= _newline;
	}

	index= 0;
	while (index<3)
	{
		struct io_info *standard_info= __pioinfo[0]+index;
		index++;
		standard_info->flags= _standard_file_flags;
	}

	return 0;
}

void __cdecl _ioterm(
	void)
{
	int index;
	for (index=0; index<_ioinfo_array_count; index++)
	{
		if (__pioinfo[index])
		{
			free(__pioinfo[index]);
			__pioinfo[index]= NULL;
		}
	}
	return;
}

typedef int (__cdecl *io_initializer_proc)(
	void);
#pragma data_seg(".CRT$RII20")
io_initializer_proc io_initialize_handles= _ioinit;
#pragma data_seg()
