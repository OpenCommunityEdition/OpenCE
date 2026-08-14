/*
_OPEN.C

symbols in this file:
	__openfile
*/

#include <fcntl.h>
#include <stdio.h>

#define _CMASK 0644
#define _IOCOMMIT 0x4000

extern int _cflush;
extern int _commode;

FILE * __cdecl _openfile(
	char const *filename,
	char const *mode,
	int share_flag,
	FILE *stream)
{
	int mode_flag;
	int stream_flag = _commode;
	int commit_mode_set = 0;
	int scan_mode_set = 0;
	int valid;
	int file_descriptor;

	switch (*mode)
	{
	case 'r':
		mode_flag = _O_RDONLY;
		stream_flag |= _IOREAD;
		break;

	case 'w':
		mode_flag = _O_WRONLY | _O_CREAT | _O_TRUNC;
		stream_flag |= _IOWRT;
		break;

	case 'a':
		mode_flag = _O_WRONLY | _O_CREAT | _O_APPEND;
		stream_flag |= _IOWRT;
		break;

	default:
		return NULL;
	}

	valid = 1;

	while (*++mode && valid)
	{
		switch (*mode)
		{
		case '+':
			if (mode_flag & _O_RDWR)
			{
				valid = 0;
			}
			else
			{
				mode_flag |= _O_RDWR;
				mode_flag &= ~(_O_RDONLY | _O_WRONLY);
				stream_flag |= _IORW;
				stream_flag &= ~(_IOREAD | _IOWRT);
			}
			break;

		case 'b':
			if (mode_flag & (_O_TEXT | _O_BINARY))
				valid = 0;
			else
				mode_flag |= _O_BINARY;
			break;

		case 't':
			if (mode_flag & (_O_TEXT | _O_BINARY))
				valid = 0;
			else
				mode_flag |= _O_TEXT;
			break;

		case 'c':
			if (commit_mode_set)
			{
				valid = 0;
			}
			else
			{
				commit_mode_set = 1;
				stream_flag |= _IOCOMMIT;
			}
			break;

		case 'n':
			if (commit_mode_set)
			{
				valid = 0;
			}
			else
			{
				commit_mode_set = 1;
				stream_flag &= ~_IOCOMMIT;
			}
			break;

		case 'S':
			if (scan_mode_set)
			{
				valid = 0;
			}
			else
			{
				scan_mode_set = 1;
				mode_flag |= _O_SEQUENTIAL;
			}
			break;

		case 'R':
			if (scan_mode_set)
			{
				valid = 0;
			}
			else
			{
				scan_mode_set = 1;
				mode_flag |= _O_RANDOM;
			}
			break;

		case 'T':
			if (mode_flag & _O_SHORT_LIVED)
				valid = 0;
			else
				mode_flag |= _O_SHORT_LIVED;
			break;

		case 'D':
			if (mode_flag & _O_TEMPORARY)
				valid = 0;
			else
				mode_flag |= _O_TEMPORARY;
			break;

		default:
			valid = 0;
			break;
		}
	}

	file_descriptor = _sopen(
		filename,
		mode_flag,
		share_flag,
		_CMASK);
	if (file_descriptor < 0)
		return NULL;

	_cflush++;
	stream->_flag = stream_flag;
	stream->_cnt = 0;
	stream->_tmpfname = stream->_base = stream->_ptr = NULL;
	stream->_file = file_descriptor;

	return stream;
}
