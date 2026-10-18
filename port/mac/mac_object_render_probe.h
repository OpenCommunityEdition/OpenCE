#ifndef HALO_MAC_OBJECT_RENDER_PROBE_H
#define HALO_MAC_OBJECT_RENDER_PROBE_H
#ifndef MAC_INPUT_NO_CARBON
#define MAC_INPUT_NO_CARBON
#endif
#include "mac_runtime_view.h"
#include "mac_object_renderer.h"

struct mac_object_render_probe {
    long object_index;
    long additional_object_indices[3];
    struct mac_object_renderer_stats additional_stats[3];
    unsigned int object_count;
    unsigned int draw_calls;
    struct mac_object_renderer_stats stats;
};

/* Binds an actual live engine object. Its current model, active permutations
   and object_get_node_matrices() are resolved inside the GL callback each
   frame, so no copied pose or fabricated geometry enters the draw. */
int mac_object_render_probe_attach(struct mac_runtime_view *,struct mac_object_render_probe *,long object_index);
int mac_object_render_probe_attach_many(struct mac_runtime_view *,struct mac_object_render_probe *,
    const long *object_indices,unsigned int count);
int mac_object_render_probe_detach(struct mac_runtime_view *,struct mac_object_render_probe *);
int mac_object_render_probe_valid(const struct mac_object_render_probe *);
int mac_object_render_probe_object_valid(const struct mac_object_render_probe *,unsigned int index);
int mac_object_render_probe_object_textured(const struct mac_object_render_probe *,unsigned int index);
int mac_object_render_probe_uv_check(void);

#endif
