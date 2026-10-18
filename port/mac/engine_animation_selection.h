#ifndef HALO_MAC_ENGINE_ANIMATION_SELECTION_H
#define HALO_MAC_ENGINE_ANIMATION_SELECTION_H
#include "retail_animation_graph.h"
#include "retail_skeleton.h"
/* Selection-only native view: actual engine graph/animation structs contain
   the fields consumed by name lookup, fitting and diagnostic seat selection.
   Unused aiming/IK/runtime fields remain absent. This is not
   a complete runnable animation graph. Decoded graph must outlive this view; free the view before reuse.
   APIs run on the engine thread; native tag lookup is scoped to each call. */
struct mac_engine_animation_selection { void *state; };
int mac_engine_animation_selection_create(const struct mac_retail_animation_graph *,
    const struct mac_retail_skeleton *, struct mac_engine_animation_selection *,const char **error);
void mac_engine_animation_selection_free(struct mac_engine_animation_selection *);
/* Returns clip index or -1. Calls the whole-source engine name lookup, then
   checks model compatibility. No unit animation state machine is simulated. */
int mac_engine_animation_selection_find(const struct mac_engine_animation_selection *,const char *name);
/* Read the decoded seat/class slot, then resolve its name through the engine.
   This is explicit metadata selection, NOT unit_set_or_test_seat_and_weapon_label. */
int mac_engine_animation_selection_slot(const struct mac_engine_animation_selection *,
    const char *seat,const char *weapon_class,int slot);
/* Calls the actual engine's node-capacity filter, including its diagnostic
   for a clip with too many nodes. */
int mac_engine_animation_selection_fits(const struct mac_engine_animation_selection *,int index,int model_nodes);
int mac_engine_animation_selection_idle_slot(void);
int mac_engine_animation_selection_move_front_slot(void);
/* Diagnostic record fixture: allocates one object header through real data.c,
   calls real object lookup + unit_set_seat, then releases the record. Does not
   call object_new, create a playable biped, or run animation updates. Refuses
   to run while an engine object table already exists. Returns -1 on error. */
struct mac_engine_seat_result { int seat,base_seat,weapon_class,weapon_type;unsigned flags; };
int mac_engine_animation_diagnostic_seat(const struct mac_engine_animation_selection *,
    uint32_t biped_handle,const char *seat,struct mac_engine_seat_result *);
#endif
