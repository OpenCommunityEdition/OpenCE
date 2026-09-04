# Unfinished Opus session reconciliation backlog (2026-09-03)

## Frozen donor

- Worktree: `C:\halo-worktrees\opus-small-families-30k-20260902`
- Branch: `opus/small-families-30k-20260902`
- Committed tip: `eeedd72e5ba0fc1761519f68cad241fe606320e7`
- Last stable donor snapshot: `scratch/opus-30k-after-hdr2.json`
- Donor policy: read-only. Reconcile selectively onto current canonical; do not bulk cherry-pick.

### 2026-09-04 token-exhaustion checkpoint

The user reported that the follow-up Opus prompt nearly completed but exhausted
its token budget before producing a final handoff.  A fresh read-only check
confirmed that the donor still points at `eeedd72e5ba0fc1761519f68cad241fe606320e7`
and retains the twelve modified translation units and `scratch/` evidence listed
below.  Treat that live working-tree state, including its per-object reports and
scratch candidates, as an unfinished reconciliation source.  Do not infer
completion from the interrupted run, discard its dirty state, or replace it
with the committed tip; audit and incorporate it object by object under the
normal canonical gates when the current admitted queue is clear.

Against canonical `c700b911f`, the committed donor retains 414 donor-only strict exact functions totaling 84,384 padded bytes. Most of the earlier Opus wave is already canonical: 306 of 337 functions and 67,184 of 72,720 padded bytes from `bb0d690a3` have been incorporated. The genuine committed review backlog is concentrated in `8aed00b49`, `608c66380`, and `02d350bbe`.

## Interrupted dirty session

The donor also contains uncommitted edits in these twelve translation units:

- `source/ai/actor_combat.c`
- `source/ai/encounters.c`
- `source/cache/cache_files_decompress_windows.c`
- `source/interface/ui_widget.c`
- `source/networking/network_server_manager.c`
- `source/objects/object_lights.c`
- `source/rasterizer/rasterizer_swizzle.c`
- `source/rasterizer/xbox/rasterizer_xbox.c`
- `source/sound/sound_dsound_xbox.c`
- `source/structures/leaf_map.c`
- `source/structures/structure_render.c`
- `source/units/bipeds.c`

The isolated `scratch/w6_*.obj` artifacts establish 38 additional strict exact functions totaling 15,632 padded bytes, plus 17 newly written fuzzy bodies covering 12,160 target bytes. This is preserved candidate evidence, not an admission result: the session produced no final stable whole-tree snapshot, and its tree fails `git diff --check` at the end of `rasterizer_xbox.c`.

The combined committed and dirty candidate pool is 100,016 strict padded bytes before source-quality exclusions. Every packet must independently pass current canonical compilation, rename-stable regression comparison, fake-match review, parked-entry validation, ownership/admission checks, and the house rules.

## Reconciliation order

1. `rasterizer_swizzle.obj`: reconcile the semantic function and data ownership splits. Expected frontier is 12/13 exact with one honest 800-byte scheduler residual.
2. `structure_render.obj`: 19/19 function code exact; clean up the 24-byte BSS owner, linkage, types, and prototypes.
3. `object_lights.obj`: 16 new exact functions / 6,064 bytes plus two credible fuzzy bodies; replace address names, repair linkage and owner declarations, and use typed accessors.
4. `sound_dsound_xbox.obj`: 24 candidate exact functions across committed and dirty work / 5,232 bytes; repair private/public linkage and semantic code/BSS names.
5. `real_alpha_to_pixel32`: 2,080 exact water bytes from `02d350bbe`; manually reconcile around canonical bitmap-header work and rerun the full header blast.
6. Small leaves: Encounters (448 bytes), UI widget (368), cache read issuer (208), network server (480), draw-string tab stops (112), render sphere diameter (96), AI path trio (224), devices leaves (304), and game leaves (160).
7. Larger audit groups: `hs_runtime`, `network_client_manager`, `ai_script`, `render_debug`, `network_server_manager`, `first_person_weapons`, `ai`, `profile`, `network_server_message_handler`, `saved_game_files`, `effects`, and `cache_files_decompress_windows`.

Do not re-open the already adjudicated widget, transparent-geometry, screen-effect, environment-fog, render-sprite, rasterizer-lights, progress-bar, or naked-particle remnants without new evidence. The dirty `leaf_map.c` and `bipeds.c` both emit `point_from_line3d` and are inadmissible as written. `rasterizer_xbox.c` belongs to its dedicated active reconciliation lane.

## Fable disposition

The prior Fable backlog is fully audited. `fable/small-families-20260901` is already canonical ancestry. The 25 functions present only on `fable/exact-finish-20260829` (10,272 padded bytes) are all previously documented semantic rejects or honest residuals; there is no forgotten Fable bulk packet to import.

## Canonical reconciliation checkpoint (2026-09-04)

The interrupted Opus state remains frozen and read-only at the path and tip
above.  The following object packets have now been selectively replayed,
house-rule audited, rebuilt, swept against rename-stable whole-tree snapshots,
and committed to canonical with zero strict regressions:

- cache decompression worker: `933b9df1d`;
- render-debug geometry: `87fbeb758`;
- profile timers/render texture: `43bc9e074`;
- first-person weapons: `6d351b357`;
- network client: `7d74519b1`, with accounting corrections `c2a08e0e9`
  and `99a8e4735`;
- network server message handler: `10e330359`;
- saved-game files: `1c834b12f`.

The current admitted tree contains 871,741 / 2,198,102 meaningful matched code
bytes, 6,021 / 11,060 credited functions, 2,015,906 matched data bytes, and
391 / 833 linked objects.  The remaining interrupted-session work is still a
candidate pool rather than presumed progress.

Active isolated reconciliations are `effects.obj`, the dirty
`network_server_manager.obj` packet, and the dirty `encounters.obj` leaf.  Once
those finish, continue the object-by-object queue in the order above, skipping
anything already admitted and preserving every remaining dirty donor file and
scratch artifact until its owning object has an explicit admit/park/reject
ledger.  A completed-looking donor report is never sufficient by itself: each
packet still needs current-canonical compilation, stable whole-tree comparison,
semantic and ownership audits, park validation, fake-match review, tooling
tests, the `units.obj` sentinel, and the January COMDAT check.
