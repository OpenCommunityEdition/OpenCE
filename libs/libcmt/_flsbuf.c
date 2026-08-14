/*
_FLSBUF.C

symbols in this file:
	__flsbuf (.text, 0x119)
*/

#include <io.h>
#include <stdio.h>

/* ---------- private CRT constants */

enum
{
	_io_your_buffer = 0x0100,
	_ioinfo_array_shift = 5,
	_ioinfo_array_mask = 31,
	_file_append = 0x20
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

long __cdecl lseek(
	int file_handle,
	long offset,
	int origin);

/* ---------- public code */

int __cdecl _flsbuf(
	int character,
	FILE *stream)
{
	struct io_info *info;
	int character_count;
	int written;
	int file_handle;

	file_handle = stream->_file;

	if (!(stream->_flag & (_IOWRT | _IORW)) ||
		(stream->_flag & _IOSTRG))
	{
		stream->_flag |= _IOERR;
		return EOF;
	}

	if (stream->_flag & _IOREAD)
	{
		stream->_cnt = 0;
		if (stream->_flag & _IOEOF)
		{
			stream->_ptr = stream->_base;
			stream->_flag &= ~_IOREAD;
		}
		else
		{
			stream->_flag |= _IOERR;
			return EOF;
		}
	}

	stream->_flag |= _IOWRT;
	stream->_flag &= ~_IOEOF;
	written = character_count = stream->_cnt = 0;

	if (!(stream->_flag & (_IOMYBUF | _IONBF | _io_your_buffer)))
	{
		if (!((stream == stdout || stream == stderr) &&
			_isatty(file_handle)))
		{
			_getbuf(stream);
		}
	}

	if (stream->_flag & (_IOMYBUF | _io_your_buffer))
	{
		character_count = (int)(stream->_ptr - stream->_base);
		stream->_ptr = stream->_base + 1;
		stream->_cnt = stream->_bufsiz - 1;

		if (character_count > 0)
		{
			written = _write(
				file_handle,
				stream->_base,
				character_count);
		}
		else
		{
			info = file_handle != -1
				? &__pioinfo[file_handle >> _ioinfo_array_shift]
					[file_handle & _ioinfo_array_mask]
				: &__badioinfo;

			if (info->flags & _file_append)
			{
				/* January names the public CRT alias `lseek`; the
				 * authenticated archive calls the same address `_lseek`.
				 */
				lseek(file_handle, 0L, SEEK_END);
			}
		}

		*stream->_base = (char)character;
	}
	else
	{
		character_count = 1;
		written = _write(file_handle, &character, character_count);
	}

	if (written != character_count)
	{
		stream->_flag |= _IOERR;
		return EOF;
	}

	return character & 0xff;
}
