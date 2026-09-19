/* Xbox Bink section loading and aligned allocator reconstruction. */
#include "radplatform.h"
#include <stdlib.h>
#include <string.h>
#include "binkxbox.h"
#include "radmem.h"

#pragma data_seg("BINKDATA")
char *conv_strs[16]=
{
	"8P", "24", "24R", "32", "32R", "32A", "32RA", "4444",
	"5551", "16", "16", "16", "16", "YUY2", "UYVY", "YV12"
};
static rad_malloc_proc rad_allocate_callback= 0;
static rad_free_proc rad_release_callback= 0;
#pragma data_seg()

struct rad_allocation_prefix
{
	rad_free_proc release;
	unsigned short reserved;
	unsigned char allocator;
	unsigned char adjustment;
};

enum
{
	RAD_ALLOCATION_ALIGNMENT= 32,
	RAD_ALLOCATION_OVERHEAD= 64,
	RAD_ALLOCATOR_STANDARD= 0,
	RAD_ALLOCATOR_CALLBACK= 3
};

/* Kept outside BINK so it can load or unload that section itself. */
void __stdcall BinkLoadUnload(
	int load)
{
	if (load)
	{
		XLoadSection("BINK");
		XLoadSection("BINKDATA");
	}
	else
	{
		XFreeSection("BINK");
		XFreeSection("BINKDATA");
	}
	return;
}

#pragma code_seg("BINK")
void __stdcall BinkLoadUnloadConverter(
	unsigned long flags,
	int load)
{
	char section_name[32];
	char doubled_name[32];
	strcpy(section_name, "BINK");
	if (flags & BINK_CONVERTER_FORMAT_MASK)
	{
		strcpy(section_name+4, conv_strs[flags & BINK_CONVERTER_FORMAT_MASK]);
		if (load) XLoadSection(section_name);
		else XFreeSection(section_name);
		if (flags & BINK_CONVERTER_X2_SUFFIX)
		{
			strcpy(doubled_name, section_name);
			strcat(doubled_name, "X2");
			if (load) XLoadSection(doubled_name);
			else XFreeSection(doubled_name);
		}
		if (flags & BINK_CONVERTER_M_SUFFIX)
		{
			strcat(section_name, "M");
			if (load) XLoadSection(section_name);
			else XFreeSection(section_name);
		}
		if (flags & BINK_CONVERTER_X2_SUFFIX)
		{
			strcat(section_name, "X2");
			if (load) XLoadSection(section_name);
			else XFreeSection(section_name);
		}
	}
	if (flags & BINK_CONVERTER_ALL_FORMATS)
	{
		int format;
		for (format= 0; format <= 15; ++format)
		{
			BinkLoadUnloadConverter((flags & ~(BINK_CONVERTER_ALL_FORMATS|BINK_CONVERTER_FORMAT_MASK))|format, load);
		}
	}
	return;
}

void __stdcall RADSetMemory(
	rad_malloc_proc allocate,
	rad_free_proc release)
{
	rad_allocate_callback= allocate;
	rad_release_callback= release;
	return;
}

void *__stdcall radmalloc(
	unsigned long bytes)
{
	unsigned char *allocation;
	unsigned char allocator;
	unsigned char adjustment;
	struct rad_allocation_prefix *prefix;

	/* Original allocation contract: the caller must supply a size whose
	 * 64-byte overhead fits in 32 bits. The target rejects 0 and ULONG_MAX
	 * but does not reject every other size that would wrap on addition.
	 */
	if (!bytes || bytes == (unsigned long)-1)
	{
		return 0;
	}
	if (rad_allocate_callback && (allocation= rad_allocate_callback(bytes+RAD_ALLOCATION_OVERHEAD)))
	{
		if (allocation == (void *)-1) return 0;
		allocator= RAD_ALLOCATOR_CALLBACK;
	}
	else
	{
		allocation= malloc(bytes+RAD_ALLOCATION_OVERHEAD);
		if (!allocation) return 0;
		allocator= RAD_ALLOCATOR_STANDARD;
	}
	adjustment= RAD_ALLOCATION_OVERHEAD-((unsigned char)(unsigned long)allocation & (RAD_ALLOCATION_ALIGNMENT-1));
	allocation += adjustment;
	prefix= ((struct rad_allocation_prefix *)allocation)-1;
	prefix->adjustment= adjustment;
	prefix->allocator= allocator;
	if (allocator == RAD_ALLOCATOR_CALLBACK)
	{
		prefix->release= rad_release_callback;
	}
	return allocation;
}

void __stdcall radfree(
	void *allocation)
{
	if (allocation)
	{
		struct rad_allocation_prefix *prefix= ((struct rad_allocation_prefix *)allocation)-1;
		if (prefix->allocator == RAD_ALLOCATOR_CALLBACK)
		{
			prefix->release((unsigned char *)allocation-prefix->adjustment);
		}
		else
		{
			free((unsigned char *)allocation-prefix->adjustment);
		}
	}
	return;
}
