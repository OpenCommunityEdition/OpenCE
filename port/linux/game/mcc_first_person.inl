/* The first-person cache can outlive a scripted equipment replacement.
 * Release its effects while the deleted weapon still exists, on both the
 * authoritative machine and clients applying an object deletion. */
#include "mcc_cache.h"

void mcc_first_person_weapon_deleted(long weapon_index)
{
    short local;
    if (!mcc_cache_tags_loaded() || weapon_index == NONE) return;
    for (local = 0; local < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; ++local)
        if (first_person_weapon_get(local)->weapon_index == weapon_index)
            first_person_weapon_forget_weapon(local);
}
