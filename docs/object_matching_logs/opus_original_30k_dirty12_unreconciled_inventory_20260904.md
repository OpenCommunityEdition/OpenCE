# Original Opus 30k donor: dirty-12 unreconciled inventory (2026-09-04)

## Scope and conclusion

This is a read-only, fail-closed inventory of the original interrupted Opus
worktree at
`C:\halo-worktrees\opus-small-families-30k-20260902`, branch
`opus/small-families-30k-20260902`, committed tip
`eeedd72e5ba0fc1761519f68cad241fe606320e7`, plus its twelve modified C files.
The comparison began at canonical commit
`0ac295264659381fe8d3bb234588e01483f5ffde` and its generated January objects.
The independently verified Actor Combat result was subsequently published in
canonical commit `ebba3978e494f0f91af355d35746e3dad1436ea6`. The separate
new Fable 50k lane is expressly out of scope.

The old aggregate claims of 84,384 committed donor-only padded bytes and
15,632 dirty-object padded bytes were measurements against much older
canonical states. They are not a present-day backlog and must not be credited
again. Current target-fingerprint screening of the twelve dirty objects finds
three material fronts:

- one 64-byte Actor Combat helper, independently gated and now admitted for 56
  meaningful bytes in `ebba3978e`;
- twelve Leaf Map target fingerprints totaling 2,880 padded bytes, still
  requiring a selective caller-topology/source-quality admission; and
- fourteen Network Server Manager target fingerprints totaling 3,216 padded
  bytes, of which one 160-byte body is a documented fake match and thirteen
  totaling 3,056 bytes are plausible but reserved for the active new Fable
  lane.

After the Actor Combat admission, the raw screening output contains **25
candidate target fingerprints / 5,936 padded bytes** across Leaf Map and
Network Server Manager. These numbers describe preserved donor artifacts
only. They are neither matched-byte progress nor an estimate of an admissible
backlog: mapping, relocation, source-quality, caller-topology, and full-build
checks may reduce them. No other dirty translation unit produces a useful
current candidate lead in this census.

## Method and limits

The preserved `scratch/w6_*.obj` files were compared to January and current
canonical by target section padded size, relocation count, and relocation-
normalized SHA-256, excluding functions already strict in canonical. This is
useful for finding missed work, but it is deliberately weaker than admission:
it does not by itself prove semantic names, linkage, relocation destinations,
data ownership, source authenticity, a valid private-caller graph, or stable
whole-tree behavior. Every listed candidate still needs a current-context
`gate.py` replay, complete emitted-owner census, house-rule review, park audit,
and the orchestrator's full regression sweep.

The donor remains read-only. No donor source, index, object, branch, or scratch
artifact was modified during this inventory.

## Dirty translation-unit disposition

| Dirty donor translation unit | Current-only target fingerprints | Disposition |
| --- | ---: | --- |
| `source/ai/actor_combat.c` | 1 / 64 B | Genuine missed private helper; separately gated strict and admitted as `actor_get_grenade_definition` in `ebba3978e` for 56 meaningful bytes. See `actor_combat_obj_grenade_definition_opus_reaudit_20260904.md`. |
| `source/ai/encounters.c` | 0 | Dirty leaves were selectively reconciled by the two Encounters ledgers; no additional fingerprint remains. |
| `source/cache/cache_files_decompress_windows.c` | 0 | Full typed reconstruction was selectively integrated; 38 exact / 8 parked / 0 unwritten is documented in `cache_files_decompress_windows_obj_opus_reconciliation_20260904.md`. |
| `source/interface/ui_widget.c` | 0 | Pool ownership and the two safe leaves were integrated; other donor bodies were explicitly left fuzzy. |
| `source/networking/network_server_manager.c` | 14 / 3,216 B | Thirteen plausible candidates / 3,056 B plus one rejected fake 160-byte token body. Reserved for the active new Fable lane; do not duplicate or cherry-pick now. |
| `source/objects/object_lights.c` | 3 / 1,056 B apparent | Already adjudicated, not new admission: one exact is declaration-context dependent; two private bodies were falsely exported while their real caller remains unwritten. |
| `source/rasterizer/rasterizer_swizzle.c` | 0 | Semantic code/data ownership reconciliation and the one honest residual are already documented. |
| `source/rasterizer/xbox/rasterizer_xbox.c` | 2 / 32 B fingerprint collisions | Two generic 16-byte XDK thunk shapes collide with target fingerprints; the mapped function inventory has no new donor body. The dedicated dirty-state re-audit proves zero current gain. |
| `source/sound/sound_dsound_xbox.c` | 0 | The committed and dirty sound packet was already selectively reconciled with semantic linkage/data owners. |
| `source/structures/leaf_map.c` | 12 / 2,880 B | Genuine unadjudicated private/public caller family; high-value next non-Fable packet, but the whole donor TU is inadmissible as written. |
| `source/structures/structure_render.c` | 0 | Function code and the semantic 24-byte storage owner were already reconciled. |
| `source/units/bipeds.c` | 0 | Safe `biped_snap_facing` was integrated; the remaining donor paths are documented point-COMDAT, layout, x87, or private-caller boundaries. |

The apparent Object Lights functions are
`find_point_lights_for_object_in_cluster` (560 B), `render_debug_light`
(272 B), and `light_get_bounding_sphere` (224 B). The current Object Lights
ledger records why none is a forgotten admissible exact: the first depends on
a superseded declaration-position context, and the latter two are January-
private functions emitted only after the donor gives them false external
linkage. They require the genuine 2,400-byte `lights_preprocess_scene` caller,
not an export or anchor.

## Genuine Leaf Map frontier

The preserved object
`scratch/w6_leaf_map.obj` (SHA-256
`C997026A4DBA1ABBA0B1EA89A3DEA4CBC1EAC10D40494041C07A3F5B28B34A88`)
contains the following twelve candidate sections absent from the current
strict set:

| Candidate semantic owner | Padded bytes | Relocations |
| --- | ---: | ---: |
| `node_stack_push` | 80 | 8 |
| `node_stack_pop` | 64 | 8 |
| `node_stack_read` | 64 | 7 |
| `node_stack_find_plane` | 160 | 9 |
| `leaf_face_index_from_node` | 64 | 1 |
| `leaf_map_build_portal_from_leaves` | 832 | 43 |
| `normalize_three_dee` | 128 | 3 |
| `leaf_map_build_leaf_face_for_leaf_on_node` | 608 | 30 |
| `leaf_map_build_portals` | 240 | 18 |
| `leaf_map_build_leaf_faces_for_leaf` | 112 | 9 |
| `leaf_map_build_leaf_faces` | 224 | 17 |
| `leaf_map_initialize_from_bsp` | 304 | 31 |
| **Total** | **2,880** |  |

This corrects the incomplete conclusion in
`leaf_map_obj_opus_private_helper_emission_audit_20260904.md`: the narrow
current-canonical replay could not emit `leaf_face_index_from_node` or
`normalize_three_dee`, but the frozen dirty donor does contain their real
same-TU caller family. The helpers still must not be exported or anchored;
they should be reviewed and replayed with those callers.

The complete donor file cannot be transplanted. It emits a forbidden 48-byte
`_point_from_line3d` owner through an unrelated path, duplicates an owned
`leaf_portal` definition, carries stale local declarations, and its
`project_plane3d` path uses representation casts that require semantic cleanup.
The safe next packet is a selective leaf-face/portal/stack caller cluster,
retaining `project_plane3d` as an honest residual if necessary and proving the
private emission graph naturally. No declaration positioning, linkage change,
helper anchor, or scalar rewrite may be used merely to preserve the twelve
fingerprints.

The audited donor inputs are:

- source SHA-256:
  `3705AEFCD539702F95F55B5792D89D3F1366AFA4AB891799875592109F1E7114`;
- preserved object SHA-256:
  `C997026A4DBA1ABBA0B1EA89A3DEA4CBC1EAC10D40494041C07A3F5B28B34A88`.

## Fable-reserved Network Server Manager frontier

The preserved object
`scratch/w6_network_server_manager.obj` (SHA-256
`7920E226899900301F10D1CC40FB0091DF2A637DB6DB71550D5FCBF7A975F1BE`)
screens thirteen plausible candidate sections:

| Candidate owner | Padded bytes |
| --- | ---: |
| `network_game_server_add_new_client` | 368 |
| `network_game_server_handle_public_endpoint` | 160 |
| `network_game_server_send_rejection_message` | 160 |
| `network_game_server_reject_client_connection` | 48 |
| `network_game_server_handle_postgame` | 80 |
| `network_game_server_all_machines_have_precached` | 112 |
| `network_game_server_initialize_pregame_settings` | 208 |
| `network_game_dump` | 256 |
| `network_game_server_dump` | 304 |
| `network_game_server_remove_client_machine_from_game` | 320 |
| `network_game_server_remove_machine_from_game` | 320 |
| `network_game_server_idle` | 384 |
| `network_game_server_create` | 336 |
| **Plausible total** | **3,056** |

The separate 160-byte `network_game_generate_join_game_token` fingerprint is
not progress. It clears only pointer-width bytes through a pointer parameter
and then overwrites the output with an unsupported fixed stack literal. Its
redundant/nonsensical logic is already rejected under the anti-fake rule.

These thirteen candidates are explicitly reserved for the active new Fable
campaign and were not probed, replayed, or modified here. If they remain after
that lane returns, the smallest credible continuation is the natural
`network_game_server_remove_client_machine_from_game` / remove-machine caller
packet rather than the fake token or an isolated private-emission trick.

The audited donor inputs are:

- source SHA-256:
  `6B91A6622C1E2DE8E71364379B8FA798374CA4A0D6D62B35E67D4B9D573E251C`;
- preserved object SHA-256:
  `7920E226899900301F10D1CC40FB0091DF2A637DB6DB71550D5FCBF7A975F1BE`.

## Committed donor history

The committed series at the frozen tip is evidence, not a cherry-pick queue:

| Donor commit | Current disposition |
| --- | --- |
| `bb0d690a3` | Its broad 337-function wave has been selectively reconciled through the rasterizer, network/cache, UI, sound, object, and miscellaneous small-object ledgers. The old aggregate delta predates those admissions. |
| `8aed00b49` | Devices, game, path, render-camera/debug, profile, effects, first-person-weapons, networking, and other leaves have current object ledgers that admit, park, or reject the source individually. |
| `608c66380` | Cache decompression, network client/server-message, first-person weapons, effects, HS runtime, saved-game, structure-render, sound, and related reconstructions have current selective ledgers. |
| `02d350bbe` | `real_alpha_to_pixel32` was reconciled with the bitmap-owned inline in `rasterizer_xbox_water_obj_real_alpha_inline_reconciliation_20260903.md`. |
| `eeedd72e5` | The four Rasterizer Xbox header return-type corrections were reviewed as part of the safe/dirty Rasterizer Xbox reconciliation. |

No separate newer Opus ref, commit, or worktree was discovered by the
companion `claude_opus_lane_intake_inventory_20260904.md` census. Consequently
there is no evidence for another anonymous 30,000- or 40,000-byte packet to
credit. If such a provider run appears later, it must be frozen and inventoried
as a distinct donor rather than folded into this historical worktree.

## Actionable order

1. Treat the Actor Combat helper and refreshed
   `actor_combat_reaim_grenade` park as complete in `ebba3978e`; do not count
   the donor copy again.
2. Leave Network Server Manager to the active Fable lane and reconcile only
   what that handoff does not cover.
3. Outside Fable, take the Leaf Map stack/leaf-face/portal family as one real
   private-caller topology, starting from the small stack helpers but retaining
   them only through genuine callers. Gate out `_point_from_line3d`, clean
   representation casts and duplicate owner definitions, and park natural
   residual callers without forcing exactness.
4. Do not reopen the zero-delta dirty objects or the three Object Lights
   apparent matches without new authoritative owner/caller evidence.

No function or byte in this inventory is credited merely because it appears
in the donor object. Only the separate Actor Combat ledger has already passed
a focused current-context exact gate; all remaining totals are triage numbers.
