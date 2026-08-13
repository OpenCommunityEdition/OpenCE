/*
_FILBUF.C

symbols in this file:
	__filbuf (.text, 0xE1)
*/

#include <io.h>
#include <stdio.h>

/* ---------- private CRT constants */

enum
{
	_io_your_buffer = 0x0100,
	_io_setvbuf = 0x0400,
	_io_control_z = 0x2000,
	_small_buffer_size = 512,
	_internal_buffer_size = 4096,
	_ioinfo_array_shift = 5,
	_ioinfo_array_mask = 31,
	_file_text = 0x80,
	_file_eof = 0x02
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

extern struct io_info __badioinfo;
extern struct io_info *__pioinfo[];

/* ---------- external code */

void __cdecl _getbuf(
	FILE *stream);

/* ---------- public code */

int __cdecl _filbuf(
	FILE *stream)
{
	struct io_info *info;

	if (!(stream->_flag & (_IOREAD | _IOWRT | _IORW)) ||
		(stream->_flag & _IOSTRG))
	{
		return EOF;
	}

	if (stream->_flag & _IOWRT)
	{
		stream->_flag |= _IOERR;
		return EOF;
	}

	stream->_flag |= _IOREAD;
	if (!(stream->_flag & (_IOMYBUF | _IONBF | _io_your_buffer)))
	{
		_getbuf(stream);
	}
	else
	{
		stream->_ptr = stream->_base;
	}

	/*
	 * January's split target names this public CRT alias `read`; the
	 * authenticated XDK archive spells the same executable address `_read`.
	 */
	stream->_cnt = read(stream->_file, stream->_base, stream->_bufsiz);
	if (stream->_cnt == 0 || stream->_cnt == -1)
	{
		stream->_flag |= stream->_cnt ? _IOERR : _IOEOF;
		stream->_cnt = 0;
		return EOF;
	}

	if (!(stream->_flag & (_IOWRT | _IORW)))
	{
		info = stream->_file != -1
			? &__pioinfo[stream->_file >> _ioinfo_array_shift]
				[stream->_file & _ioinfo_array_mask]
			: &__badioinfo;

		if ((info->flags & (_file_text | _file_eof)) ==
			(_file_text | _file_eof))
		{
			stream->_flag |= _io_control_z;
		}
	}

	if (stream->_bufsiz == _small_buffer_size &&
		(stream->_flag & _IOMYBUF) &&
		!(stream->_flag & _io_setvbuf))
	{
		stream->_bufsiz = _internal_buffer_size;
	}

	--stream->_cnt;
	return 0xff & *stream->_ptr++;
}
