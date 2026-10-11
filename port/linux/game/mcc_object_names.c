#include "cseries.h"
#include "mcc_object_names.h"
#include "mcc_cache.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "objects/objects.h"
#include <string.h>

/* Signed script and placement indices reserve -1 for no name. A count of
 * 32767 also lets the native short-index loops terminate without wrapping. */
typedef char mcc_name_handle_size[sizeof(long) == 4 ? 1 : -1];
static long mcc_names[MCC_OBJECT_NAMES_MAXIMUM];

long mcc_object_names_count(void)
{
    long count;
    if (!mcc_cache_tags_loaded()) return 0;
    count = global_scenario_get()->object_names.count;
    return count > MAXIMUM_OBJECT_NAMES_PER_SCENARIO && count <= MCC_OBJECT_NAMES_MAXIMUM ? count : 0;
}

int mcc_object_names_active(void) { return mcc_object_names_count() != 0; }

void mcc_object_names_clear(void)
{
    memset(mcc_names, 0xFF, sizeof(mcc_names));
}

long mcc_object_names_get(short name)
{
    return name >= 0 && name < mcc_object_names_count() ? mcc_names[name] : NONE;
}

void mcc_object_names_set(short name, long object)
{
    if (name >= 0 && name < mcc_object_names_count()) mcc_names[name] = object;
}

void mcc_object_names_claim(short name, long object)
{
    if (object == NONE || name < 0 || name >= mcc_object_names_count()) return;
    if (mcc_names[name] != NONE) return;
    mcc_names[name] = object;
    object_get(object)->object.name_index = name;
}

void mcc_object_names_delete(long object)
{
    long i, count = mcc_object_names_count();
    /* Script reassignment can give an object several names. Clear all of them. */
    for (i = 0; i < count; ++i) if (mcc_names[i] == object) mcc_names[i] = NONE;
    object_get(object)->object.name_index = NONE;
}

unsigned long mcc_object_names_snapshot(void *out)
{
    unsigned long bytes = (unsigned long)mcc_object_names_count() * 4;
    if (out && bytes) memcpy(out, mcc_names, bytes);
    return bytes;
}

int mcc_object_names_validate(void const *in, unsigned long bytes)
{
    return bytes == mcc_object_names_snapshot(NULL) && (!bytes || in != NULL);
}

int mcc_object_names_restore(void const *in, unsigned long bytes)
{
    if (!mcc_object_names_validate(in, bytes)) return 0;
    if (bytes) memcpy(mcc_names, in, bytes);
    return 1;
}
