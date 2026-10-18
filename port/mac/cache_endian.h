#ifndef HALO_MAC_CACHE_ENDIAN_H
#define HALO_MAC_CACHE_ENDIAN_H
#include <stddef.h>
#include <stdint.h>

/* Fixed 32-bit cache layouts, independent of host pointer size. This is a
   codec, not a tag validator or pointer relocator. Not wired into loading
   until every reachable tag and GPU payload has conversion coverage. */
enum mac_cache_record {
    MAC_CACHE_HEADER, MAC_CACHE_TAG_HEADER, MAC_CACHE_TAG_INSTANCE,
    MAC_CACHE_BSP_HEADER, MAC_CACHE_BSP_REFERENCE, MAC_CACHE_GPU_BUFFER,
    MAC_CACHE_POINT_PHYSICS, MAC_CACHE_WIND, MAC_CACHE_SOUND_ENVIRONMENT
};
/* Exact-size, in-place conversion. encode=0: LE disk -> native; encode=1:
   native -> LE disk. Padding and byte strings are preserved. Invalid calls
   leave the buffer unchanged. Caller must track which representation it owns. */
int mac_cache_record_convert(void *bytes, size_t size,
    enum mac_cache_record record, int encode);
/* Validates the serialized allocation before changing any byte. Converts
   ALL allocated nodes, including inactive slots. No pointer dereferences.
   The containing tag range/alignment and script strings are caller checks. */
int mac_cache_syntax_convert(void *bytes, size_t size,
    unsigned int expected_capacity, int encode);
/* Overflow-safe mapping of an Xbox virtual address into an available span.
   Does not access memory, validate nested fields or accept one-past objects. */
int mac_cache_address_offset(uint32_t address, uint32_t base,
    size_t span, size_t object_size, size_t *offset);
#endif
