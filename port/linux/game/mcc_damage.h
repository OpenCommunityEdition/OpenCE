#ifndef MCC_DAMAGE_H
#define MCC_DAMAGE_H

struct damage_definition;
union real_vector3d;

/* Updates only MCC extension effects; returns whether units may accelerate. */
boolean mcc_damage_acceleration(struct damage_definition const *definition,
    real object_scale, union real_vector3d *acceleration);

#endif
