#ifndef MCC_DAMAGE_H
#define MCC_DAMAGE_H

struct damage_definition;
union real_vector3d;

/* Updates MCC extension effects using the biased, normalized direction.
 * Returns unit admission; preserves the legacy result outside extensions. */
boolean mcc_damage_acceleration(struct damage_definition const *definition,
    real object_scale, union real_vector3d const *direction, union real_vector3d *acceleration);

#endif
