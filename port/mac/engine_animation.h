#ifndef HALO_MAC_ENGINE_ANIMATION_H
#define HALO_MAC_ENGINE_ANIMATION_H
#include "retail_animation.h"
#include "retail_skeleton.h"
struct mac_engine_orientation {
    float rotation[4], translation[3], scale;
};
/* Samples an already-validated, native, uncompressed base clip through the
   actual engine. Overlay/replacement clips need an existing pose and are
   explicitly refused. Skeleton count/checksum must satisfy engine compatibility.
   No tag registration, animation selection or gameplay. The sampled branch
   does quaternion scalar multiplication and field copies only; compressed
   math and the floating-point rounding environment are not validated here. */
int mac_engine_animation_sample(const struct mac_retail_animation *,
    const struct mac_retail_skeleton *,
    uint32_t frame, struct mac_engine_orientation *, uint32_t capacity,
    const char **error);
#endif
