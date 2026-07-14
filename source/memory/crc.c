/*
CRC.C

symbols in this file:
001088D0 0010:
	_crc_new (0000)
001088E0 0040:
	_code_001088e0 (0000)
00108920 0080:
	_crc_checksum_buffer (0000)
0027D2E4 000f:
	??_C@_0P@JPGOHOCM@buffer_size?$DO?$DN0?$AA@ (0000)
0027D2F4 001c:
	??_C@_0BM@FPJPBIIF@c?3?2halo?2SOURCE?2memory?2crc?4c?$AA@ (0000)
00456220 0401:
	_bss_00456220 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "memory/crc.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

#pragma pack(push, 1)
struct crc_globals
{
	unsigned long table[256];
	boolean initialized;
};
#pragma pack(pop)

/* ---------- prototypes */

/* ---------- globals */

#pragma bss_seg(".bss")
struct crc_globals bss_00456220;
#pragma bss_seg()

/* ---------- public code */

void crc_new(unsigned long *crc_reference)
{
	*crc_reference = 0xFFFFFFFF;
}

__declspec(noinline) void crc_table_initialize(unsigned long *crc_table)
{
	unsigned long byte_index;
	long byte_count;

	byte_index = 0;
	byte_count = 256;
	do
	{
		unsigned long crc = byte_index;
		long bit_count;

		bit_count = 8;
		do
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		} while (--bit_count);

		*crc_table = crc;
		byte_index++;
		crc_table++;
	} while (--byte_count);
}

/* ---------- private code */
