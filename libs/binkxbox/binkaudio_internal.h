#ifndef BINK_AUDIO_INTERNAL_H
#define BINK_AUDIO_INTERNAL_H
#include <stddef.h>
#include "binkaudio.h"

/* January decoder core allocation is 0x2688 bytes. Unknown internal spans
 * stay reserved; trailing variable-size buffers are not part of this type.
 * The lock/unlock APIs prove the buffer offsets; Open proves allocation size.
 */
struct BINKAUDIO
{
	unsigned long transform_samples;
	float transform_scale;
	unsigned long overlap_samples;
	unsigned char *compressed_buffer;
	short *samples;
	unsigned long first_block;
	unsigned long band_count;
	unsigned long bands[26];
	unsigned long overlap_valid;
	unsigned char reserved_0088[0x2684-0x88];
	unsigned long compressed_buffer_bytes;
};

/* Compressor prefix only: do not allocate by sizeof this partial type. */
struct BINKAUDIOCOMP
{
	unsigned long transform_samples;
	unsigned char reserved_0004[8];
	unsigned long overlap_samples;
	short *samples;
	unsigned char reserved_0014[4];
	unsigned long first_block;
};
typedef char bink_audio_core_size_assert[sizeof(struct BINKAUDIO)==0x2688 ? 1:-1];
typedef char bink_audio_buffer_offset_assert[offsetof(struct BINKAUDIO,compressed_buffer)==0xc ? 1:-1];
typedef char bink_audio_capacity_offset_assert[offsetof(struct BINKAUDIO,compressed_buffer_bytes)==0x2684 ? 1:-1];
typedef char bink_audio_comp_overlap_assert[offsetof(struct BINKAUDIOCOMP,overlap_samples)==0xc ? 1:-1];
typedef char bink_audio_comp_samples_assert[offsetof(struct BINKAUDIOCOMP,samples)==0x10 ? 1:-1];
typedef char bink_audio_comp_first_assert[offsetof(struct BINKAUDIOCOMP,first_block)==0x18 ? 1:-1];
#endif
