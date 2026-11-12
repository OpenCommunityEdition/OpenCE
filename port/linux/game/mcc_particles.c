/* MCC weapon particle systems can author separate first- and third-person
 * streams. Ruby's flamethrower uses "spawn fire" only on its first-person
 * model; querying the world model cannot produce that stream. */
#include "cseries.h"
#include "camera/director.h"
#include "effects/particle_system_definitions.h"
#include "interface/first_person_weapons.h"
#include "items/weapons.h"
#include "objects/object_definitions.h"
#include "mcc_cache.h"
#include "mcc_particles.h"

static short mcc_particle_system_local_player(long object_index)
{
    long local;
    if (object_index == NONE || !weapon_try_and_get(object_index)) return NONE;
    local = first_person_weapon_get_local_index(object_index);
    return local != NONE && director_get_perspective((short)local) == _director_perspective_first_person
        ? (short)local : NONE;
}

short mcc_particle_system_markers(long object_index, short attachment_index,
    unsigned long flags, struct object_marker *markers, short maximum, short world_count)
{
    struct weapon_datum *weapon;
    struct tag_block *attachments;
    struct object_attachment_definition *attachment;
    if (!mcc_cache_tags_loaded() ||
        !TEST_FLAG(flags, _particle_system_type_nodraw_third_person_bit)) return world_count;
    /* Both view exclusions mean no emission. Never substitute the world
     * origin for an absent first-person marker, a remote player or a holstered
     * weapon. The existing system owns and retires its particles as usual. */
    if (TEST_FLAG(flags, _particle_system_type_nodraw_first_person_bit) || maximum <= 0 ||
        mcc_particle_system_local_player(object_index) == NONE) return 0;
    weapon = weapon_try_and_get(object_index);
    attachments = &object_definition_get(weapon->definition_index)->object.attachments;
    if (!VALID_INDEX(attachment_index, attachments->count)) return 0;
    attachment = TAG_BLOCK_GET_ELEMENT(attachments, attachment_index, struct object_attachment_definition);
    return first_person_weapon_get_marker_by_name(object_index, attachment->marker_name, markers, maximum);
}

boolean mcc_particle_system_visible(long object_index, unsigned long flags, short viewer)
{
    short local;
    boolean first_person;
    if (!mcc_cache_tags_loaded() ||
        !(flags & (FLAG(_particle_system_type_nodraw_first_person_bit) |
                   FLAG(_particle_system_type_nodraw_third_person_bit)))) return TRUE;
    local = mcc_particle_system_local_player(object_index);
    first_person = local != NONE && local == viewer;
    return !TEST_FLAG(flags, first_person ? _particle_system_type_nodraw_first_person_bit
                                         : _particle_system_type_nodraw_third_person_bit);
}
