#ifndef HALO_MAC_ENGINE_COLLISION_H
#define HALO_MAC_ENGINE_COLLISION_H
#include "retail_collision.h"
/* Owns engine-typed arrays; no casts from portable arrays or disk bytes.
   Build with the native ILP32 engine ABI. Free before reuse. */
struct mac_engine_collision { void *state; };
int mac_engine_collision_create(const struct mac_retail_collision *,
    struct mac_engine_collision *, const char **error);
void mac_engine_collision_free(struct mac_engine_collision *);
/* Calls the linked source/physics/bsp3d.c implementation. -2 means an invalid
   query; -1 is solid; nonnegative is the native engine leaf index. */
int32_t mac_engine_collision_leaf(const struct mac_engine_collision *, const float p[3]);
#endif
