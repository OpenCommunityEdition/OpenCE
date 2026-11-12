/* MCC weapon-only recordings have no direction to pass to native unit control.
 * Inherit the target's current directions; preserve all events and timing. */
#include "cseries.h"
#include "mcc_cache.h"
#include "mcc_recordings.h"
#include "cutscene/recorded_animation_definitions.h"
#include "units/unit_control_data.h"
#include "units/units.h"

void mcc_recording_control_begin(long unit_index,
    struct recorded_animation_definition const *animation, struct unit_control_data *control)
{
    struct unit_datum *unit;
    if (!mcc_cache_tags_loaded() || !animation || !control ||
        !mcc_cache_contains(animation->event_stream.address, animation->event_stream.size) ||
        !mcc_recording_weapon_only(animation->event_stream.address, animation->event_stream.size,
            animation->version, animation->unit_control_data_version)) return;
    unit = unit_try_and_get(unit_index);
    if (!unit || !valid_real_normal3d(&unit->object.forward) ||
        !valid_real_normal3d(&unit->unit.aiming_vector) ||
        !valid_real_normal3d(&unit->unit.looking_vector)) return;
    control->facing_vector = unit->object.forward;
    control->aiming_vector = unit->unit.aiming_vector;
    control->looking_vector = unit->unit.looking_vector;
}
