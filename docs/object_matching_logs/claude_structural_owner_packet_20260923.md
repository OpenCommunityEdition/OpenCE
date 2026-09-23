# Owner-decision packet: new primary evidence (2026-09-23)

Branch `claude/structural-40k-20260923` (base `ea84554a`). Every variant below
was re-measured on this tree today: `gate.py --all` row diff against
production (zero rows lost in every case) plus the new selected-provider
link check (`scratch/lane/provider_link.py`: VC7 `Link.Exe`, candidate object
against our build of January's selected provider, both input orders, any
`LNK2005` naming the symbol fails). Nothing here is landed; each needs a yes/no.

## 1. Provider COMDAT selection blocks all new helper emission (new finding)

Our provider objects define 47 shared helpers as ordinary functions
(COMDAT selection NODUPLICATES). For example, `action_charge.c` renames the
header inlines and hand-writes `point_from_line3d`, `scale_vector3d`,
`dot_product3d`, `magnitude3d` and `normalize3d`. A SELECT_ANY copy of any of
them in another object then fails the link with `LNK2005`. csplit writes
NODUP for every January section, so the split cannot show January's real
selection. For a header `__inline`, January's copies were almost certainly
SELECT_ANY. Census: `scratch/lane/comdat_selection.py` → 47 CONFLICT,
81 coalesce (e.g. `_vector_from_points3d`, `_cross_product3d`,
`_negate_vector3d`, `_magnitude_squared3d` are safe).

Effect today: the three held Lane B bodies are **still strict exact on this
tree** but fail the link, so under the brief they stay held:

| Object | Functions | Bytes | Conflicting new definitions |
|---|---|---|---|
| biped_limp_noodle | move_relax_and_constrain_positions, valid_joint_rotation (object 6/6) | 3,216 | `_point_from_line3d`, `_vector_intersect_plane3d` |
| actor_type_infection | `_infection_swarm_control` (object 5/5) | 3,616 | `_point_from_line3d` |
| vehicles | `_update_alien_fighter_physics_new` | 1,088 | `_point_from_line3d`, `_cross_product2d` |

`_effect_generate_part` (992 B, class A below) fails the same way on
`_real_local_random_range`. `_collision_debug_render` (4,192 B, wave 1 of
this lane) is exact against a scratch re-split only when it newly defines
`_point_from_line3d`, `_scale_vector3d` and `_add_vectors3d`. January inlines
none of them out of line there, so the reference-only header switch does not
apply. It also needs a three-way `.bss` static split of the invented
`_collision_debug_spray_globals`: the arrays are separate in the /Od build,
contribs.json module 140 owns the region, and a single aggregate gives 276 of
278 relocations. The split's names are descriptive (no HCEX compiland).
Rows blocked on this decision: 13,104 B in total.

Measured remodel (shadow tree, not landed): in `action_charge.c`, delete the
`#define point_from_line3d point_from_line3d_inline` / `#undef` pair and the
hand-written `point_from_line3d` body. That is a hand-expanded copy of the
`real_math.h` inline, which rule 13 disallows anyway. action_charge.obj then
emits `_point_from_line3d` from the header inline as a SELECT_ANY COMDAT.
Its row list is unchanged: 21 exact, `_action_charge_perform` with the same
`[size 3216!=3248, sha]` tag, and the `_point_from_line3d` row stays EXACT.
An object that emits the helper (items.obj) then links cleanly against it
(`provider_link` PASS where production gives LNK2005). The same treatment
would apply to `actor_combat.c` (`_cross_product2d`, `_add_vectors3d`),
`geometry.c` (`_vector_intersect_plane3d`) and the other NODUP providers.

**Decision:** (a) keep the NODUP provider model, so these rows stay held; or
(b) let me replace the providers' hand-written helper copies with the real
header inlines (one provider at a time, each with a whole-board sweep). That
makes the owner-approved emitters link-clean: the vehicles fighter, the
actor_perception danger-zone body and the actor_moving pfl package all sit in
ruling 1's 17 objects. Rows outside the 17 (limp noodle, infection, physics,
collision_debug) would still need ruling 1 widened.

## 2. PDB-typed `volatile` (cache_files_decompress_windows, 2,176 B)

HCEX type record, verified with DIA2Dump:
`simple_decompressor_definition` has `+0x908 volatile unsigned long flags`
and `+0xAA4 volatile float read_progress`. This is a source type qualifier
recorded in debug information, not a codegen inference. The variant (plus the
debug-build `short flag` parameter and local order) makes the object
**46/46 exact** with no surplus definitions (link: nothing to link). Without
`volatile` the other changes gain nothing. The 2026-09-14 rejection predates
the type-record evidence.

**Decision:** admit `volatile` where the PDB type record declares it?

## 3. `realcmp` macro ownership (geometry 1,792 B; connected_geometry +192 B)

January's own assert strings contain `realcmp(determinant, 0.0f)` and
`realcmp(plane3d_distance_to_point(&plane, &rotate_to_position), 0.f)`, so the
macro is Bungie's. It exists only as TU-local copies in `actors.c` and
`actor_looking.c`. `_convex_hull2d` is exact only with the macro expansion
(the plain `fabs` spelling fails the strip test). `_convex_polygon2d_clip_to_plane`
also needs the owner header's own point→vector cast in
`plane2d_distance_to_point` (a new site under rule 24).
`_connected_geometry_find_or_add_vertex` needs an epsilon-taking variant that
no January string attests.

Measured in a shadow worktree (full ninja + stable diff over 8,245 rows):
defining `realcmp` and `realcmp_epsilon` once in `real_math.h` (macros section)
and blanking the five TU-local copies (actors.c, actor_looking.c,
matrix_math.c, biped_limp_noodle.c for `realcmp`; sound_dsound_xbox.c for
`realcmp_epsilon`) gives **0 gained, 0 regressions, no C4005 warnings**.
Lines are blanked, not deleted, because those files bake `__LINE__` into
plain `assert`. On top of that, geometry gains `_convex_hull2d` (+784 B)
with no cast, and `_convex_polygon2d_clip_to_plane` (+1,008 B) only with the
cast. The connected_geometry vertex row does not close on the macro alone.

**Decision:** (a) consolidate `realcmp`/`realcmp_epsilon` in `real_math.h`
(ready: zero-regression measured; lands `_convex_hull2d` +784 B);
(b) admit the geometry cast (+1,008 B more).

## 4. Authentic January bugs (original-bug policy)

All strict exact today, zero rows lost, link PASS unless noted. Several now
carry first-party `/Od`+`/RTC` evidence: the debug build's `_RTC_UninitUse`
call names the uninitialised variable, which the earlier rejections lacked.

| Function | Bytes | Behaviour preserved | New evidence |
|---|---|---|---|
| `_dead_camera_update` | 1,248 | `next_unit_index` read uninitialised when no next player | January bytes (no store on that path) |
| `_connect_endpoint_process@4` | 304 | `thread` uninitialised on mutex failure | January homes it in a dead slot, no zeroing store |
| `_hs_parse_set` | 560 | shorts passed to `%s` | January format literal + arguments |
| `_hs_parse_boolean` | 304 | `value` uninitialised on error arm | /Od stores unconditionally |
| `_effect_allowed_by_environment` | 128 | `allowed` read if `system_exit` returns | /Od `_RTC_UninitUse("allowed")` |
| `_effect_generate_part` | 992 | csprintf format passed as buffer | January and /Od both call with 3 args; **link FAIL** (`_real_local_random_range`) |
| `_connected_geometry_find_or_add_edge` | 240 | `direction` uninitialised when edges.count < 0 | /Od `_RTC_UninitUse("direction")` |
| `_convex_hull3d_expand` (+`_convex_hull2d`, `_convex_polygon2d_clip_to_plane` via realcmp) | 1,808 (3,600 with the realcmp rows) | pointers formed before range checks | /Od shows the same form at all three sites |
| `_saved_game_files_enumerate_available_to_local_player_index` | 368 | indeterminate count on mutex failure | January bytes + /Od double zeroing |
| `_compare_profile_sections` | 288 | uninitialised `result` default arm (owner item 8) | /Od `_RTC_UninitUse("result")` |
| `_network_server_close_client_connection` | 320 | capacity (N+1) loop bound reads one past the list | same bound in the 2020 /Od build |

**Decision:** admit the RTC-attested rows (`_effect_allowed_by_environment`,
`_connected_geometry_find_or_add_edge`, `_compare_profile_sections`) as a
first class? Then decide the rest one by one. Each would land with the
methodology's `BUG (preserved for exact matching)` comment.

## 5. Aggregate 3D→2D view copies under the per-site rule-24 admission

The 2026-09-21 admission requires a byte-inert cast (the function stays
exact with the cast deleted). An aggregate copy such as
`fixed2d = *(real_vector2d const *)&actor->control.fixed_stationary_facing_vector;`
has no castless form except a field-by-field copy, so the test becomes
"does the field copy also match?" Where it does, the field copy lands
with no cast. Where only the aggregate copy matches, the cast is
load-bearing:

| Function | Bytes | Field copy |
|---|---|---|
| `_actor_look_update` | 4,720 | not exact (the `fixed2d` copy alone is load-bearing) |
| `_actor_perception_aiming_vector_test_blockage` | 400 | not measured (worker: /Od 0x4620dd shows the aggregate copy) |
| `_update_alien_scout_physics` | 2,464 | frame fix needs 6 casts; also needs inline `_point_from_line3d` |

**Decision:** does a /Od-attested aggregate view copy qualify when the
field-copy spelling does not match?

## 6. Recommend rejecting

`_network_connection_connect` (288 B): exact only with a redundant
`success = TRUE;` attested by January's block layout alone (the 2020 build
has no such path). It fails the strip test, so I recommend rejecting it.

Measurements: `scratch/owner/variants_measured.json`; variant sources are in
the Lane D worktree paths listed there.

## 7. Items this lane held (strict exact on a variant, not landed)

| Function | Bytes | Why held | Variant |
|---|---|---|---|
| `__rasterizer_environment_lightmap_draw` | 4,016 | Reopens the P7 hold. The duplicated stage-0 filter calls rest on a lab law (R-order fastcall setup occurs at only 4 of 448 January sites), not on first-party attestation of the duplication. | `scratch/lane/w/rasterizer__xbox__rasterizer_xbox_environment/candidate.c` |
| `_breakable_surface_effect` | 4,032 | Needs `_point_from_line3d` and `_projection_from_vector3d` emission (breakable_surfaces is outside the 17 objects; NODUP providers). The /Od and HCEX shape is otherwise exact 12/12. | `.../physics__breakable_surfaces/v1_genuine_helpers.c` |
| `_hud_update_weapon_local_player` | 1,472 | The default case leaves `result` unassigned after `match_assert(!"unreachable")`. The read happens only if `system_exit` returns (same class as `_effect_allowed_by_environment`). /Od is identical. | `.../interface__hud_weapon/candidate.c` |
| `_actor_action_handle_vehicle_entry` | 960 | Needs a TU-private `((d) * (d))` squaring macro. That extends the 2026-09-20 actor_perception ruling to actions.c. The plain `d*d` spelling fails the strip test. | `.../ai__actions/candidate.c` |
| `_poll_endpoint_set` | 560 | January tests `< 0` and `== SOCKET_ERROR` (the -1 compare is in its bytes). Exact only when that condition is first stored in a named boolean; plain `||` fails the strip test, and no /Od body attests the boolean. | `.../bungie_net__network__transport_endpoint_set_winsock/candidate.c` |
| `_transport_initialize` | 416 | Explicit zero field stores after the `= { sizeof }` initializer (rejected in Lane D as redundant). New measurement: VC7 keeps such stores, and January's bytes show them, so they are source statements. Necessary but not sufficient. | same unit |
| `_actor_look_update` | 4,720 | Load-bearing aggregate `(real_vector2d const *)` view copy (see section 5). | harvested Lane A probe |

Fidelity only (no code credit): decals' `.bss` order matches January if
`decal_points2d_temp` becomes a static local of `decal_clip_to_surface`
(HCEA lists it as one). That needs a paired symbols.json rename to a
function-static name whose scope ordinal is not attested for January, so
it is left for the owner.
