/* January Bink file backend. Descriptive private names are not PDB claims.
 * Drafts without exact proof retain zero matching credit.
 */
#include "binkfile.h"
#include "radtimer.h"
#include <string.h>

static unsigned long __stdcall bink_file_read_header(
	BINKIO *io,
	long offset,
	void *destination,
	unsigned long size);
static void bink_file_simulate_rate(
	BINKIO *io,
	unsigned long bytes,
	unsigned long started);
static unsigned long __stdcall bink_file_read_frame(
	BINKIO *io,
	unsigned long frame,
	long offset,
	void *destination,
	unsigned long size);
static unsigned long __stdcall bink_file_get_buffer_size(
	BINKIO *io,
	unsigned long size);
static void __stdcall bink_file_set_info(
	BINKIO *io,
	void *buffer,
	unsigned long size,
	unsigned long file_size,
	unsigned long simulate);
static void __stdcall bink_file_close(
	BINKIO *io);
static unsigned long __stdcall bink_file_idle(
	BINKIO *io);

/* Period RAD.H defines the non-threaded LockedAddFunc helper with this exact
 * register/ADD sequence. January likewise has no LOCK prefix. Keep the helper
 * out of line here because January owns a standalone function contribution.
 */
void LockedAddFunc(
	unsigned long *value,
	unsigned long amount)
{
	__asm
	{
		mov eax, [value]
		mov edx, [amount]
		add [eax], edx
	}
	return;
}

#pragma code_seg("BINK")
static unsigned long __stdcall bink_file_read_header(
	BINKIO *io,
	long offset,
	void *destination,
	unsigned long size)
{
	unsigned long read;
	unsigned long remaining;
	if (io->suspend_callback)
	{
		io->suspend_callback(io);
	}
	if (offset != -1 && io->file.read_position != (unsigned long)offset)
	{
		SetFilePointer(io->file.handle, io->file.start_offset+offset, 0, FILE_BEGIN);
		io->file.read_position= offset;
	}
	ReadFile(io->file.handle, destination, size, &read, 0);
	io->file.read_position += read;
	io->file.logical_position= io->file.read_position;
	remaining= io->file.file_size-io->file.logical_position;
	io->CurBufSize= remaining < io->BufSize ? remaining : io->BufSize;
	if (io->resume_callback)
	{
		io->resume_callback(io);
	}
	return read;
}

static void bink_file_simulate_rate(
	BINKIO *io,
	unsigned long bytes,
	unsigned long started)
{
	/* Target uses a wide multiply and hardware 64/32 divide. The input
	 * contract requires nonzero rate and a quotient fitting in 32 bits.
	 * Unsigned intermediate arithmetic preserves wrapping timer updates.
	 */
	unsigned long duration= (unsigned long)(((unsigned __int64)bytes*1000)/io->file.simulate_rate);
	unsigned long now= RADTimerRead();
	io->file.wait_debt= (long)((unsigned long)io->file.wait_debt+duration+started-now);
	while (io->file.wait_debt > 0)
	{
		unsigned long next;
		do
		{
			next= RADTimerRead();
		} while ((long)(next-now) < io->file.wait_debt);
		io->file.wait_debt= (long)((unsigned long)io->file.wait_debt+now-next);
		now= next;
	}
	return;
}

static unsigned long __stdcall bink_file_read_frame(
	BINKIO *io,
	unsigned long frame,
	long offset,
	void *destination,
	unsigned long size)
{
	unsigned long suspended= 0;
	unsigned long total= 0;
	unsigned long started;
	unsigned long read;
	unsigned long remaining;
	if (io->ReadError)
	{
		return 0;
	}
	started= RADTimerRead();
	if (offset != -1 && io->file.logical_position != (unsigned long)offset)
	{
		suspended= 1;
		if (io->suspend_callback)
		{
			io->suspend_callback(io);
		}
		if ((unsigned long)offset > io->file.logical_position &&
			(unsigned long)offset <= io->file.read_position)
		{
			unsigned long skip= offset-io->file.logical_position;
			io->file.logical_position= offset;
			io->file.empty_bytes += skip;
			io->CurBufUsed -= skip;
			/* Use the target's 32-bit address arithmetic without first
			 * forming a C pointer beyond the ring allocation. */
			{
				unsigned long position= (unsigned long)io->file.read_buffer+skip;
				if (position > (unsigned long)io->file.buffer_end)
				{
					position -= io->BufSize;
				}
				io->file.read_buffer= (unsigned char *)position;
			}
		}
		else
		{
			SetFilePointer(io->file.handle, io->file.start_offset+offset, 0, FILE_BEGIN);
			io->file.read_position= offset;
			io->file.logical_position= offset;
			io->file.empty_bytes= io->BufSize;
			io->CurBufUsed= 0;
			io->file.read_buffer= io->file.buffer;
			io->file.write_buffer= io->file.buffer;
		}
	}
	/* Recheck buffered bytes after suspension: a background read may have
	 * completed while the foreground caller was waiting for the callback.
	 */
	for (;;)
	{
		unsigned long buffered= io->CurBufUsed;
		if (buffered)
		{
			unsigned long tail;
			if (buffered > size)
			{
				buffered= size;
			}
			size -= buffered;
			io->file.logical_position += buffered;
			total += buffered;
			tail= io->file.buffer_end-io->file.read_buffer;
			if (tail <= buffered)
			{
				memcpy(destination, io->file.read_buffer, tail);
				destination= (unsigned char *)destination+tail;
				buffered -= tail;
				io->file.read_buffer= io->file.buffer;
				LockedAddFunc(&io->CurBufUsed, 0UL-tail);
				LockedAddFunc(&io->file.empty_bytes, tail);
			}
			if (buffered)
			{
				memcpy(destination, io->file.read_buffer, buffered);
				destination= (unsigned char *)destination+buffered;
				io->file.read_buffer += buffered;
				LockedAddFunc(&io->CurBufUsed, 0UL-buffered);
				LockedAddFunc(&io->file.empty_bytes, buffered);
			}
		}
		if (!size || suspended)
		{
			break;
		}
		suspended= 1;
		if (io->suspend_callback)
		{
			io->suspend_callback(io);
		}
	}
	if (size)
	{
		unsigned long read_started= RADTimerRead();
		unsigned long finished;
		ReadFile(io->file.handle, destination, size, &read, 0);
		if (read < size)
		{
			io->ReadError= 1;
		}
		io->file.read_position += read;
		io->file.logical_position += read;
		io->BytesRead += read;
		total += read;
		if (io->file.simulate_rate)
		{
			bink_file_simulate_rate(io, read, read_started);
		}
		finished= RADTimerRead();
		io->TotalTime += finished-read_started;
		io->ForegroundTime += finished-started;
	}
	else
	{
		io->ForegroundTime += RADTimerRead()-started;
	}
	remaining= io->file.file_size-io->file.logical_position;
	io->CurBufSize= remaining < io->BufSize ? remaining : io->BufSize;
	if (io->CurBufUsed+BINK_FILE_PAGE_SIZE > io->CurBufSize)
	{
		io->CurBufSize= io->CurBufUsed;
	}
	if (suspended && io->resume_callback)
	{
		io->resume_callback(io);
	}
	return total;
}

static unsigned long __stdcall bink_file_get_buffer_size(
	BINKIO *io,
	unsigned long size)
{
	return ((size+BINK_FILE_PAGE_SIZE-1)/BINK_FILE_PAGE_SIZE)*BINK_FILE_PAGE_SIZE;
}

static void __stdcall bink_file_set_info(
	BINKIO *io,
	void *buffer,
	unsigned long size,
	unsigned long file_size,
	unsigned long simulate)
{
	if (io->suspend_callback)
	{
		io->suspend_callback(io);
	}
	size= (size/BINK_FILE_PAGE_SIZE)*BINK_FILE_PAGE_SIZE;
	io->file.buffer= buffer;
	io->file.read_buffer= buffer;
	io->file.write_buffer= buffer;
	io->file.buffer_end= (unsigned char *)buffer+size;
	io->BufSize= size;
	io->file.empty_bytes= size;
	io->CurBufUsed= 0;
	io->file.file_size= file_size;
	io->file.simulate_rate= simulate;
	if (io->resume_callback)
	{
		io->resume_callback(io);
	}
	return;
}

static void __stdcall bink_file_close(
	BINKIO *io)
{
	if (io->suspend_callback)
	{
		io->suspend_callback(io);
	}
	if (!io->file.borrowed_handle)
	{
		CloseHandle(io->file.handle);
	}
	if (io->resume_callback)
	{
		io->resume_callback(io);
	}
	return;
}

static unsigned long __stdcall bink_file_idle(
	BINKIO *io)
{
	unsigned long working= io->Working;
	unsigned long read= 0;
	if (io->ReadError)
	{
		return 0;
	}
	if (io->try_suspend_callback && io->try_suspend_callback(io))
	{
		unsigned long remaining= io->file.file_size-io->file.read_position;
		if (io->file.empty_bytes >= BINK_FILE_PAGE_SIZE && remaining >= BINK_FILE_PAGE_SIZE)
		{
			unsigned long started= RADTimerRead();
			unsigned long elapsed;
			io->DoingARead= 1;
			ReadFile(io->file.handle, io->file.write_buffer, BINK_FILE_PAGE_SIZE, &read, 0);
			io->DoingARead= 0;
			if (read < BINK_FILE_PAGE_SIZE)
			{
				io->ReadError= 1;
			}
			io->BytesRead += read;
			io->file.read_position += read;
			io->file.write_buffer += read;
			if (io->file.write_buffer >= io->file.buffer_end)
			{
				io->file.write_buffer= io->file.buffer;
			}
			LockedAddFunc(&io->file.empty_bytes, 0UL-read);
			LockedAddFunc(&io->CurBufUsed, read);
			if (io->CurBufUsed > io->BufHighUsed)
			{
				io->BufHighUsed= io->CurBufUsed;
			}
			if (io->file.simulate_rate)
			{
				bink_file_simulate_rate(io, read, started);
			}
			elapsed= RADTimerRead()-started;
			io->TotalTime += elapsed;
			if (!working && !io->Working)
			{
				io->IdleTime += elapsed;
			}
			else
			{
				io->ThreadTime += elapsed;
			}
		}
		else
		{
			io->CurBufSize= io->CurBufUsed;
		}
		if (io->resume_callback)
		{
			io->resume_callback(io);
		}
	}
	else
	{
		if (io->idle_on_callback)
		{
			io->idle_on_callback(io);
		}
		read= (unsigned long)-1;
	}
	return read;
}

long __stdcall BinkFileOpen(
	BINKIO *io,
	const char *name,
	unsigned long flags)
{
	memset(io, 0, sizeof(*io));
	if (flags & BINK_FILE_HANDLE)
	{
		io->file.handle= (HANDLE)name;
		io->file.borrowed_handle= 1;
		io->file.start_offset= SetFilePointer(io->file.handle, 0, 0, FILE_CURRENT);
	}
	else
	{
		io->file.handle= CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, 0);
		if (io->file.handle == INVALID_HANDLE_VALUE)
		{
			io->file.handle= CreateFileA(name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, 0);
			if (io->file.handle == INVALID_HANDLE_VALUE)
			{
				return 0;
			}
		}
	}
	io->ReadHeader= bink_file_read_header;
	io->ReadFrame= bink_file_read_frame;
	io->GetBufferSize= bink_file_get_buffer_size;
	io->SetInfo= bink_file_set_info;
	io->Idle= bink_file_idle;
	io->Close= bink_file_close;
	return 1;
}
