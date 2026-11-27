#ifndef MCC_PROJECTILES_H
#define MCC_PROJECTILES_H

#include "math/real_math.h"

/* Returns FALSE without changing outputs when the original firing path owns
 * the shot. Preview queries must not record aim-assist state. */
boolean mcc_projectile_ray(long weapon_index, long owner, short trigger_index, boolean firing,
    real_point3d *origin, real_vector3d *forward, real *velocity, long *target_index);

#endif
