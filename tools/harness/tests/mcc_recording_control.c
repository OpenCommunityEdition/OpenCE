/* Exercise the real MCC playback hook, including inactive/foreign data. */
#include "cseries.h"
#include "cutscene/recorded_animation_definitions.h"
#include "units/unit_control_data.h"
#include "units/units.h"
#include "mcc_recordings.h"
#include <stdio.h>
#include <string.h>
#undef memset
#undef memcmp

#define CHECK(x) do {if (!(x)) {fprintf(stderr, "line %d: %s\n", __LINE__, #x);return 1;}} while (0)
static boolean loaded = TRUE, owned = TRUE;
static unsigned char stream[78];
static struct unit_datum unit;
boolean mcc_cache_tags_loaded(void) {return loaded;}
boolean mcc_cache_contains(void const *p, long bytes) {return owned && p==stream && bytes==sizeof(stream);}
void *object_try_and_get_and_verify_type(long index, unsigned long mask) {
    return index==7 && (mask & (1u<<unit.object.type)) ? &unit : NULL;
}

int main(void)
{
    struct recorded_animation_definition animation;
    struct unit_control_data control, expected, original;
    memset(&animation, 0, sizeof(animation));
    animation.version=animation.unit_control_data_version=4;
    animation.event_stream.address=stream;animation.event_stream.size=sizeof(stream);
    stream[0]=3;stream[72]=5<<2;stream[73]=1;stream[76]=(1<<2)|2;stream[77]=30;
    unit.object.type=_object_type_biped;
    unit.object.forward.j=1;unit.unit.aiming_vector.k=1;unit.unit.looking_vector.i=-1;
    memset(&control, 0, sizeof(control));control.weapon_index=1;control.zoom_level=-1;
    original=control;expected=control;
    expected.facing_vector=unit.object.forward;
    expected.aiming_vector=unit.unit.aiming_vector;
    expected.looking_vector=unit.unit.looking_vector;
    mcc_recording_control_begin(7, &animation, &control);
    CHECK(!memcmp(&control, &expected, sizeof(control)));
    control=original;loaded=FALSE;mcc_recording_control_begin(7, &animation, &control);
    CHECK(!memcmp(&control, &original, sizeof(control)));loaded=TRUE;
    owned=FALSE;mcc_recording_control_begin(7, &animation, &control);
    CHECK(!memcmp(&control, &original, sizeof(control)));owned=TRUE;
    mcc_recording_control_begin(NONE, &animation, &control);
    CHECK(!memcmp(&control, &original, sizeof(control)));
    unit.object.type=_object_type_scenery;mcc_recording_control_begin(7, &animation, &control);
    CHECK(!memcmp(&control, &original, sizeof(control)));unit.object.type=_object_type_biped;
    unit.unit.aiming_vector.k=0;mcc_recording_control_begin(7, &animation, &control);
    CHECK(!memcmp(&control, &original, sizeof(control)));unit.unit.aiming_vector.k=1;
    stream[18]=0x80;stream[19]=0x3f; /* authored facing (1,0,0) */
    mcc_recording_control_begin(7, &animation, &control);
    CHECK(!memcmp(&control, &original, sizeof(control)));
    return 0;
}
