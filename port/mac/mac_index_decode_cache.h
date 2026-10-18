#ifndef MAC_INDEX_DECODE_CACHE_H
#define MAC_INDEX_DECODE_CACHE_H

#include <stddef.h>

/* Per-draw map from a WORD index to its first expanded output slot. Initialize
 * the structure to zero before first use. Begin clears the active span on every
 * draw; no decoded record survives a draw. */
struct mac_index_decode_cache {
    unsigned short *first_output;
    size_t capacity;
    unsigned count;
    unsigned minimum;
    unsigned span;
    unsigned unique_count;
    unsigned reuse_count;
    int active;
};

/* Return 1 when an exact-span cache is activated, or 0 when this draw should
 * use ordinary decoding (including count<=span or allocation failure). */
int mac_index_decode_cache_begin(struct mac_index_decode_cache *cache,
    unsigned count, unsigned minimum, unsigned maximum);

/* Lookup returns 1 for a hit, 0 for a miss or inactive cache, and -1 for an
 * invalid key/argument. Record returns 1 when stored, 0 when inactive, and -1
 * for an invalid key/output slot or an already-recorded key. */
int mac_index_decode_cache_lookup(struct mac_index_decode_cache *cache,
    unsigned key, unsigned *first_output);
int mac_index_decode_cache_record(struct mac_index_decode_cache *cache,
    unsigned key, unsigned output_slot);

/* Deactivate without releasing reusable table storage; dispose releases it. */
void mac_index_decode_cache_end(struct mac_index_decode_cache *cache);
void mac_index_decode_cache_dispose(struct mac_index_decode_cache *cache);

#endif
