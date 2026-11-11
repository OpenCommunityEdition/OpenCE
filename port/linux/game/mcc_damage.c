/* MCC damage impulses. Admission, damage ownership and object movement remain
 * with the engine and its existing host-authoritative damage path. */
#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "game/game_globals.h"
#include "objects/damage_effect_definitions.h"
#include "mcc_cache.h"
#include "mcc_damage.h"
#include <float.h>
#include <stddef.h>
#include <string.h>

typedef char mcc_damage_vector_layout[
    offsetof(struct damage_definition, instantaneous_acceleration_zero_scale_factor) ==
        offsetof(struct damage_definition, instantaneous_acceleration) + sizeof(real) &&
    offsetof(struct damage_definition, instantaneous_acceleration_unused) ==
        offsetof(struct damage_definition, instantaneous_acceleration) + 2 * sizeof(real) ? 1 : -1];

boolean mcc_damage_acceleration(struct damage_definition const *definition,
    real object_scale, real_vector3d const *direction, real_vector3d *acceleration)
{
    real values[3], scale;
    unsigned long flags;
    int i, count;
    boolean allowed = FALSE;
    if (!mcc_cache_tags_loaded()) return definition->instantaneous_acceleration > _real_epsilon;
    flags = definition->flags;
    if (!(flags & 0x30000u)) return definition->instantaneous_acceleration > _real_epsilon;
    /* MCC flag 16 supplies per-axis multipliers for the biased, normalized
     * damage direction, not an independent world-space impulse. */
    count = flags & 0x10000u ? 3 : 1;
    memcpy(values, &definition->instantaneous_acceleration, count * sizeof(real));
    for (i = 0; i < count; ++i) {
        if (!(values[i] >= -FLT_MAX && values[i] <= FLT_MAX)) goto invalid;
    }
    /* Native unit admission tests the squared tag-vector length, or the
     * absolute scalar, against epsilon before applying object scaling. */
    allowed = count == 3 ?
        values[0]*values[0] + values[1]*values[1] + values[2]*values[2] >= _real_epsilon :
        values[0] >= _real_epsilon || values[0] <= -_real_epsilon;
    if (count == 3) {
        scale = 0.033333335f;
        for (i = 0; i < 3; ++i) {
            values[i] *= object_scale;
            values[i] *= scale;
            if (!(values[i] >= -FLT_MAX && values[i] <= FLT_MAX)) goto invalid;
        }
        acceleration->i = values[0] * direction->i;
        acceleration->j = values[1] * direction->j;
        acceleration->k = values[2] * direction->k;
    }
    if (!(acceleration->i >= -FLT_MAX && acceleration->i <= FLT_MAX) ||
        !(acceleration->j >= -FLT_MAX && acceleration->j <= FLT_MAX) ||
        !(acceleration->k >= -FLT_MAX && acceleration->k <= FLT_MAX)) goto invalid;
    return allowed;
invalid:
    acceleration->i = acceleration->j = acceleration->k = 0.f;
    return FALSE;
}
