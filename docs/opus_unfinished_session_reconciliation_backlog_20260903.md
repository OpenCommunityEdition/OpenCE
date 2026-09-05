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

### Additional interrupted Opus intake reported 2026-09-04

The user reports that the newest Opus prompt was nearly finished when its
token budget expired. Keep that run on the reconciliation queue even though it
did not produce a final handoff. At this checkpoint there is no newer local
`opus/*` branch or commit than `opus/small-families-30k-20260902` at
`eeedd72e5`; its preserved dirty state is still the only discoverable
unfinished Opus worktree. If the newest run used that worktree, its changes are
covered by the object-by-object queue above. If it wrote elsewhere, identify
and freeze that path before changing it, then add its dirty files, scratch
reports, and last stable snapshot as a distinct donor instead of silently
folding its claimed progress into the existing totals.

#### Post-`c787f81ec` read-only intake check

No distinct newer Opus/Fable/Claude ref or linked-worktree HEAD is currently
discoverable. The only visible interrupted Opus state remains
`C:\\halo-worktrees\\opus-small-families-30k-20260902` at `eeedd72e5`; its dirty
files predate the current checkpoint. All twelve touched translation units and
both post-`608c66380` commits now have later canonical object-by-object
reconciliation/adjudication commits, so that historical dirty tree must not be
counted again as a new packet.

This does not close the user's newly reported interrupted run. Keep it as a
pending intake item until its exact worktree path, branch/ref, or commit hash
appears. Freeze that location before any build or edit, compare it against
canonical by function-section identity, and admit only verified net-new gains.

#### Latest Opus continuation still awaiting handoff

The user reconfirmed on 2026-09-04 that a newer Opus prompt made substantial
progress but exhausted its token budget just before completion. Preserve this
as an open intake item, separate from the already-audited historical donor,
until Opus supplies a branch, commit, worktree path, patch, or final session
summary. On discovery, freeze and inspect the donor read-only, inventory every
touched object and tool, recover any uncommitted scratch evidence, and
reconcile only net-new functions against the then-current canonical tree. Do
not credit or discard the run based on the interrupted status alone.

### Published checkpoint after scripted music (`fe1da9234`)

Canonical and both `bnunu/halo` publication branches contain 889,854 / 2,198,102
meaningful matched code bytes and 6,100 / 11,060 credited functions. These are
current admitted totals, not the donor union. Earlier byte counts and candidate
pools in this document are historical checkpoints; they must not be subtracted
from this total to estimate a current unreconciled backlog.

The sound lifecycle and impulse/foley packets, followed by the three scripted
music functions, are now admitted. `game_sound.obj` is 26 exact / 0 residual /
5 unwritten. The canonical full build and rename-stable sweep report zero
regressions; `units.obj` remains 189/189 exact; all 235 parks validate; all 284
tool tests pass. Both protected bitmap hashes are unchanged.

The next PAS helpers cannot be credited independently: VC7 discards their
unreferenced static definitions until the genuine game-sound update caller is
reconstructed. That caller cluster requires owning API corrections, including
the January frame argument and the sound-environment pointer output. It is
being reconciled in isolation, not forced into emission with fake references
or changed linkage. The collision-sphere packet likewise remains outside
canonical while its unintended helper definitions and residual-source quality
are corrected and independently reviewed. Neither packet is included in the
published total above.

The separately reported newer interrupted Opus run is still pending intake;
no new worktree/ref was found in the latest read-only census. Preserve the
existing frozen donor and do not count its already-adjudicated twelve dirty
translation units a second time.

### Collision admission and preserved PAS blocker (`119868d4b`)

The corrected collision-sphere reconstruction is now canonical: one strict
function / 44 meaningful bytes, plus three honest fuzzy bodies. Its complete
emitted-symbol audit removes all five packet-induced non-target helpers.
Canonical reaches 889,898 meaningful matched code bytes and 6,101 credited
functions with zero exact regressions; all 238 parks and 284 tests pass.

The PAS caller cluster is preserved only in the isolated sound lane as
`9d60117aa`. Its correct owner-header declaration of `game_sound_update(real
dt)` changes `units::_unit_preprocess_node_orientations` from exact to residual;
the draft's 112 padded candidate bytes are therefore not admitted. Keep its
complete source and evidence rather than suppressing the prototype, inventing
references, or overwriting canonical Units to collect a nominal gain.

The new complete-symbol guard also found an inherited Units ownership issue:
its 189 target functions remain strict exact, but it emits a 48-byte
`point_from_line3d` COMDAT absent from the split Units target. January does
genuinely call that shared helper twice, and the candidate copy matches the
selected `action_charge` implementation. Do not replace those authentic calls
with arithmetic or silently waive the user's no-COMDAT rule. Whole-object
approval is being audited separately from the valid target-function bytes.

### Publication-ready shared-helper accounting checkpoint

Canonical remains at 889,898 / 2,198,102 meaningful matched code bytes and
6,101 / 11,060 credited functions. The full build passes; the stable sweep
against the published scripted-music checkpoint shows one collision gain
(48 padded / 44 meaningful bytes) and zero exact regressions. All 238 parks
validate, all 284 tool tests pass, and the protected bitmap hashes are unchanged.

Units retains all 189 exact target functions but now has an explicit
candidate-only helper ownership veto. Collision Features likewise retains all
20 exact target functions but is changed from `Matching` to `NonMatching`
because its surplus projection helper has incorrect dominant-axis behavior.
This corrects the configured `Matching`-object count from 391 to 390 without deleting any
source or subtracting valid function bytes. The canonical treemap preserves
both 100% function-byte readings while withholding whole-object approval.
The detailed evidence is in
`docs/object_matching_logs/shared_inline_helper_admission_audit_20260904.md`.

The natural shared-header projection correction is also preserved outside
canonical as local-only commit `9633a0b4c` in the
`agent/collision-bsp-sphere-cluster-20260904` lane. Focused
gates preserved Units and closed the 176-padded-byte Leaf Map function, but
the full dependent-unit build regressed
`decals::_decals_delete_permanent_from_cluster` and
`game_engine::_populate_statistic_buffer` and invalidated the active
`bitmap_group::_bitmap_group_add_bitmap` park. Do not import that header or
refresh the affected park to conceal the failure. Its complete source and
before/after evidence remain available for a source-authentic reconciliation.

The PAS draft remains locally committed as `9d60117aa`, with authenticated
Units shared-helper evidence recorded in `e1eb62f05`. Neither draft contributes
to the published byte total, and neither is lost. The separately reported
newer interrupted Opus run still needs a worktree path, branch, commit, patch,
or handoff before its distinct unreconciled bytes can be measured.

### Current-context PAS admission and renewed leaf audit (2026-09-04)

The preserved PAS packet `9d60117aa` now passes with the same proper owning
prototype in the current genuinely reconstructed header context. A complete
production rebuild and stable sweep from publication `18238d906` preserve
all 6,186 inherited exact owners and add one strict helper: 102 meaningful /
112 padded bytes. The three genuine caller/PAS residuals are honestly parked,
not credited as exact. Units retains all 189 exact target functions and its
separate existing helper-ownership veto is not waived. The verified totals
are now **898,687 meaningful matched code bytes**, **6,141 credited
functions**, **391/833 Matching objects**, and **255 valid parks**. See
`object_matching_logs/game_sound_obj_pas_current_context_integration_20260904.md`.

This new audit also corrects the implication that every old-lane source-shaped
opportunity had been exhausted. Earlier object-level adjudication does not
prove that every individual natural helper or caller form was tested:

- The frozen old Fable Props iterator form yields a 544-byte strict scratch
  closure. It is withheld pending genuine public Actor Perception API/source
  reconciliation, not imported with its incompatible use-site prototype.
  See `object_matching_logs/props_obj_old_fable_iterator_reconciliation_20260904.md`.
- The original Opus audit identifies small Actor Combat and Leaf Map helper
  leads requiring selective replay and full ownership checks. These are
  candidates, not additional published bytes.
- Network Server Manager candidates remain reserved for the **new active
  Fable 50k lane**. Do not reconcile or implement that assigned scope until
  the user reports that lane finished or token-exhausted.

The new Fable lane itself remains untouched. A separate read-only census
found no distinct newer interrupted Opus worktree/ref/patch; that pending
intake item remains open, not counted as zero progress or silently conflated
with the already preserved September 2 donor. The historical 100,016-byte
candidate pool above must still not be used as a current backlog estimate.

### Props/Actor Perception and grenade-helper reconciliation (2026-09-04)

The two older-lane leads now survive proper canonical integration: Props
recovers 544 meaningful exact bytes, and Actor Combat recovers its genuine
private grenade-definition helper for 56 meaningful exact bytes. The same
packet removes Actor Perception's unsupported parameter-home carrier union
and incompatible caller declaration, deliberately withdrawing its prior
549-byte exact claim while retaining a credible typed 97.791664% fuzzy body.
Net progress from publication `0ac295264` is **+51 meaningful exact bytes**;
current verified totals are **898,738 bytes**, **6,142 credited functions**,
**391/833 Matching objects**, and **256 valid parks**. The full stable sweep
has exactly that one intentional source-policy loss and no collateral loss.

The Leaf Map helpers were also selectively re-audited: their private bodies
are credible and exact in the donor, but current canonical lacks their real
callers and correctly emits neither. They remain preserved for caller-cluster
reconstruction, not exported or anchored for nominal byte credit. See
`object_matching_logs/leaf_map_obj_opus_private_helper_emission_audit_20260904.md`.
The fresh provider census is recorded separately in
`object_matching_logs/claude_opus_lane_intake_inventory_20260904.md`; its
absence of a new provider ref is not a claim that all old source leads were
exhausted. The new Fable lane and its assigned Network Server Manager scope
remain untouched until the user's handoff.

### Older-lane boundary refresh and independent endpoint-set progress (2026-09-04)

The original Opus dirty-twelve census is now recorded in
`object_matching_logs/opus_original_30k_dirty12_unreconciled_inventory_20260904.md`.
After the grenade-helper admission, its raw target-fingerprint screen leaves
25 candidates / 5,936 padded bytes: Leaf Map 2,880 and Fable-reserved Network
Server Manager 3,056. These are screening leads, not an admissible-byte total;
the historical 100,016-byte delta must not be recounted. The Leaf Map donor
does contain the genuine missing caller family, but it cannot be imported
wholesale with its incorrect shared projection dependency and extra helper.

A fresh replay of the preserved projection correction in current canonical
resolves the old Game Engine collateral mismatch, preserves Units 189, and
again closes Leaf Map's 176-byte vertex helper. The Decals 400-byte exact
still regresses. The header is restored, the full remaining blast radius is
not certified, and the admitted delta is zero. See
`object_matching_logs/real_math_projection_current_context_recheck_20260904.md`.
Do not retry equivalent spellings or count the scratch Leaf Map gain.

Meanwhile, independent small-object work outside Fable's scope reconstructs
four endpoint-set lifecycle/iteration functions on their first natural
compile, adds **610 meaningful / 640 padded exact bytes**, and preserves all
6,188 inherited stable exact owners. Canonical's verified working total is
**899,348 meaningful code bytes**, **6,146 credited functions**, **391/833
Matching objects**, and **256 valid parks**. The proper owner-header signature
corrections, full-build proof, complete owner census, and 288-test result are
recorded in
`object_matching_logs/transport_endpoint_set_winsock_obj_lifecycle_packet_20260904.md`.
First Person Weapons and Saved Game Files genuine private caller clusters
continue independently in scratch; no incomplete cluster is yet credited.

### Metadata caller and nonce admission: 900,020 bytes (2026-09-04)

The Saved Game Files pair now passes proper production source/header placement
and the authenticated private rename to `set_nth_entry_in_mapfile`. Its helper
adds 474 meaningful exact bytes; the full synchronizer is honestly parked at
99.58147%. The independent transport nonce trio adds 198 exact bytes and one
99.60526% comparison park. From publication `06b5f2adf`, this batch gains
**672 meaningful / 704 padded exact bytes** with **zero strict regressions**.
Current verified totals are **900,020 meaningful bytes**, **6,149 credited
functions**, **391/833 Matching objects**, and **258 valid parks**. All 288
tests and Units' 189 target exacts pass. See
`object_matching_logs/saved_game_nonce_canonical_integration_20260904.md`.

The full First Person Weapons four-function reconstruction is now preserved
in scratch: three new strict owners (688 meaningful / 704 padded bytes) and
one complete natural residual. It is **not admitted** because its six new
shared helper code COMDATs conflict with existing selected-owner/linkage
rules; one local-random helper also differs from the selected implementation.
The source is not lost or counted as completed. Do not solve this by false
exports, suppressing authentic calls, declaration-order steering, or forcing
an inline schedule. The new Fable lane remains untouched until user handoff.

### Published boundary refresh: 900,486 bytes (2026-09-04)

Subsequent verified publications are `416c57f21` (Saved Game enumeration and
endpoint ownership), `a5c2c50ab` (endpoint-set operations and Telnet ownership),
`6132b8d0e` (Actor Stimulus fleeing and transport client lifecycle), and
`47c947b37` (the older Opus AI race mapper with its correct public declaration).
Both authorized GitHub refs are verified at the last commit. Canonical and
GitHub now contain **900,486 meaningful credited code bytes**, **6,153 credited
functions**, **2,028,615 credited data bytes**, and **391/833 Matching objects**.
All **6,199 inherited/current stable exact owners** survive the latest sweep;
266 parks validate with no stale or invalid measurements.

The mapper was not absent from all earlier investigations: the August 21
vehicle-event ledger called it already attempted. Frozen Opus source now
provides a successful, independently audited selective replay (+74 meaningful
bytes), not an unreviewed donor total. See
`object_matching_logs/ai_obj_opus_race_mapper_reconciliation_20260904.md`.

These publications do not mean every Claude lane is fully reconciled. The
new active Fable lane and all assigned implementations remain excluded until
the user hands them back. Leaf Map's shared projection boundary, the First
Person Weapons helper-linkage conflict, and the separately pending newer Opus
intake remain open. Other earlier object-level decisions may still contain
separable public leaves worth reviewing; Render Debug and Profile are the
next bounded checks. Historical raw candidate pools are not current
admissible-byte estimates and must not be added to canonical progress.

### Four further public Opus leaves: 901,368 bytes (2026-09-04)

The next bounded replay admits Render Debug's quadrilateral/2D-outline pair
and Profile's tick/window timer endings: **+882 meaningful / 912 padded
bytes**, four functions, no strict regressions, and no new code/helper owner.
The proper narrow geometry header passes its full blast; Profile uses its
existing owner declarations and audited timestamp primitive. Totals are now
**901,368 meaningful code bytes**, **6,157 credited functions**, **6,203 stable
strict owners**, **391/833 Matching objects**, and **266 valid parks**.
See `object_matching_logs/render_debug_profile_end_opus_reconciliation_20260904.md`.

The Render Debug private input/structure lead is withheld because its sole
real caller remains unwritten; the donor's external linkage is not admissible.
The independently reconstructed StackWalk loader/comparator is also withheld
because even its minimal correct common-header declaration regresses four
unrelated exact functions and fails a data-owner identity check. The complete
credible source and evidence are preserved in
`object_matching_logs/stack_walk_windows_obj_loader_cluster_withheld_20260904.md`.
All attempted header changes were restored, with a whole-tree zero-regression
rebuild before the public-leaf admission. Neither withheld packet contributes
to the totals above. Active Fable ownership remains excluded.

### Profile entry/exit with real HS ownership: 901,631 bytes (2026-09-04)

The next selective older-Opus packet admits `profile_enter_private` and
`profile_exit_private`: **+263 meaningful / 288 padded exact bytes**, two
functions, zero inherited exact regressions. Proper integration removes HS's
incompatible duplicate Profile type and local declarations, uses the owning
header/macros, and removes 23 unintended consumer COMMON allocations for
`profile_global_enable`. Profile alone owns that BSS byte. Four descriptive
member renames are explicitly identified as semantic inferences, not recovered
original spellings. See
`object_matching_logs/profile_obj_enter_exit_owner_reconciliation_20260904.md`.

The full header blast passes. Profile is **28E/0R/16U**, HS retains **445
exact functions**, Units retains **189 exact target functions**, and the whole
stable sweep reaches **6,205 exact owners**. Current verified totals are
**901,631 meaningful credited code bytes**, **6,159 credited functions**,
**2,028,615 credited data bytes**, and **391/833 Matching objects**. This
continues selective reconciliation; it does not close all older-lane work,
the separately pending Opus intake, or any withheld ownership dependency.
The active new Fable assignments remain excluded until user handoff.

The same publication also retains the general-campaign Biped placement leaf
as a complete typed **98.3871% fuzzy park**, not exact progress. Its 96-byte
target differs only in the placement-pointer register; one natural source
form was retained without tuning. The final combined whole-tree snapshot is
`scratch/profile-biped-final-20260904.json`, still 6,205 strict owners with
zero regressions, and **267 parks** now validate. See
`object_matching_logs/bipeds_obj_adjust_placement_fuzzy_20260904.md`.

### Profile starters and Xbox Profile data: 901,876 bytes (2026-09-04)

Two more genuine older-Opus public leaves, `profile_frame_start` and
`profile_tick_start`, add **245 meaningful / 272 padded exact code bytes**.
Their existing history updater now has an honest semantic private name,
`profile_sections_update`, supported by the complete January caller census.
Profile is **30E/0R/14U** without new helper/data owners. The full canonical
sweep has **6,207 strict owners and zero regressions**; Units keeps all 189.
See `object_matching_logs/profile_obj_frame_tick_start_reconciliation_20260904.md`.

An independent accounting audit also recovers **132 already-exact initialized
data bytes** in Xbox Profile. It grants no code or whole-object credit and
records the existing surplus SDK tables/source-layout boundary explicitly.
See `object_matching_logs/rasterizer_xbox_profile_obj_data_accounting_audit_20260904.md`.
Verified totals are **901,876 meaningful code bytes**, **6,161 credited
functions**, **2,028,747 data bytes**, **391/833 Matching objects**, and
**267 valid parks**. All 288 tests plus 26 subtests pass. Active Fable remains
untouched; older withheld dependencies and the pending separate Opus intake
remain open rather than being counted as integrated.

### Profile window boolean API: 902,009 bytes (2026-09-04)

`profile_render_window_start` adds **133 meaningful / 144 padded exact code
bytes**, accompanied by the correct owning boolean declaration and all three
Render caller repairs. The full 25-consumer header blast preserves every
inherited strict owner. Profile is **31E/0R/13U**; Render remains 13 exact,
Units 189 exact, and the full sweep reaches **6,208 strict owners**.
See `object_matching_logs/profile_obj_window_start_boolean_api_20260904.md`.

Verified working totals are **902,009 meaningful code bytes**, **6,162 credited
functions**, **2,028,747 data bytes**, **391/833 Matching objects**, and **267
valid parks**. The general campaign next examines genuine Network Connection
server-idle caller/helper topology and a Texture Cache source-owner repair.
Neither scratch proposal is credited before full integration. Fable's active
Network Server Manager and all other assigned files remain excluded.

### Authentic counter schema and Texture Cache fuzzy leaf (2026-09-04)

The shared Rasterizer/Main/Profile counter is now one real signed 64-bit
vertical-blank index, proven by January ADD/ADC and qword consumers. The
schema repair preserves all source-layout/runtime owners and every one of
the 6,208 strict functions. It is correctness progress, not added byte credit.
See `object_matching_logs/rasterizer_main_profile_vblank64_schema_20260904.md`.

Texture Cache's operational state no longer uses a fabricated 0x1600 prefix.
Its real 16-byte state is naturally owned, the delete callback has an
authenticated private name, and the complete public memory-steal path is
retained as a 96.95652% fuzzy park (209 meaningful / 224 padded bytes).
The preceding pointer-array owner stays deferred; inherited Bitmap/SDK and
unwritten callback debt still preclude whole-object approval. All 13 prior
exact functions survive the genuine symbol regeneration. See
`object_matching_logs/xbox_texture_cache_obj_steal_memory_owner_reconciliation_20260904.md`.

Combined full build/stable sweep: zero strict regressions; 288 tests plus 26
subtests pass; 268 valid parks. Totals remain 902,009 meaningful code bytes,
6,162 credited functions, 2,028,747 data bytes and 391/833 Matching objects.
Network Connection is still a separately audited scratch proposal. Fable's
active lane and every reserved implementation remain untouched.

### Network Connection genuine idle cluster: 902,137 bytes (2026-09-04)

The complete server/public idle cluster is now integrated through the real
Network Connection header: pointer-to-pointer client output, typed rejection
callback, all five existing client calls, and the HS scalar-global consumer.
The private server-side constructor materializes naturally as 128 meaningful
exact bytes, and its named timeout flag adds one strictly verified BSS byte.
The two full idle functions are honest fuzzy parks: 84.9819% and 99.26966%.
No unreachable duplicate diagnostic was fabricated to match the target.

Full dependency Ninja and stable sweep pass: 6,209 strict owners, one gain,
zero regressions. All 288 tests plus 26 subtests pass; 270 parks validate;
admission remains unchanged at 0 candidates / 0 contradictions / 5 rejections
/ 0 revocations. Verified totals are 902,137 meaningful code bytes, 6,163
credited functions, 2,028,748 data bytes and 391/833 Matching objects.
See `object_matching_logs/network_connection_obj_server_idle_natural_boundary_20260904.md`.

The reserved canonical Server Manager currently has no reconstructed setter
or idle caller to migrate; its future Fable code must consume this real API.
No active Fable tree or reserved implementation was edited. Network write
remains unwritten in this publication and is being separately researched;
its proposed API corrections and source are not yet credited. Older Opus
Profile frame-dump reconciliation also remains a separate unfinished packet.

### Profile frame-end cluster: 904,719 bytes (2026-09-04)

The older-Opus frame-end, formatter, dump and timer-subtraction cluster is
now reconciled as **2,582 meaningful / 2,624 padded exact code bytes**, four
functions. Profile reaches **35E/0R/9U**. Its helpers have honest private
names/linkage and its Main throttle flag uses the existing genuine internal
owner header. The confirmed original formatter-buffer defect is explicitly
documented, not silently fixed or mistaken for credible safe behavior.
See `object_matching_logs/profile_obj_frame_end_owner_reconciliation_20260904.md`.

The broad-header trial's UI/Units regressions and Shell data-identity failure
were not admitted. With the existing narrow Main interface and the unrelated
Profile seconds-elapsed API repair deferred, full Ninja and the stable sweep
pass: **6,213 strict owners**, four gains and zero regressions. Units remains
189 exact. The seconds-elapsed real-type evidence is preserved in the ledger
as an open dependency, not lost source or credited progress.

All 288 tests plus 26 subtests pass, 270 parks validate, and object admission
remains unchanged. Verified totals are **904,719 meaningful code bytes**,
**6,167 credited functions**, **2,028,748 data bytes**, and **391/833 Matching
objects**. Network write is still separate scratch research. Fable's active
lane and every reserved implementation remain excluded pending user handoff.

### Network write: final unwritten source gap filled (2026-09-04)

The general campaign retains the complete natural Network Connection write
path through its proper public API, the real Transport Endpoint declaration,
and the boolean client wrapper. The three-header full build preserves all
6,213 strict owners with zero regressions. Network Connection now has
**11 exact / 12 residual / zero unwritten functions**; write is an honest
**74.261536% unclassified fuzzy park**, not additional exact-byte credit.
See `object_matching_logs/network_connection_obj_write_natural_boundary_20260904.md`.

All 288 tests plus 26 subtests pass, 271 parks validate, and admission remains
unchanged. Published-byte accounting stays 904,719 meaningful code bytes,
6,167 credited functions, 2,028,748 data bytes and 391/833 Matching objects.
Older Profile dump/comparator work is separately being researched. The small
Transport server lifecycle pair is a new general-campaign candidate; neither
scratch proposal is credited or included here. Fable reservations remain in
force, including Network Server Manager.

### Transport server lifecycle pair: 905,027 bytes (2026-09-05)

The general campaign adds the two small, genuine server lifecycle functions
through the existing Winsock owner header: **308 meaningful / 320 padded exact
bytes**, two functions. The complete one-byte server-global allocation is a
natural tentative COMMON, supported by the all-object reference census and
linked owner; its source compiland attribution remains explicitly inferred.
No new data byte or whole-object completion is claimed. The unit reaches
**16 exact / 6 residual / 4 unwritten**.
See `object_matching_logs/transport_endpoint_set_winsock_obj_server_lifecycle_pair_20260905.md`.

Independent source/ownership review, the actual canonical gate, full Ninja,
and the stable 8,245-owner sweep all pass: **6,215 strict owners**, two gains,
zero regressions. All 288 tests plus 26 subtests pass; 271 parks validate;
admission remains unchanged. Verified totals are **905,027 meaningful code
bytes**, **6,169 credited functions**, **2,028,748 data bytes**, and **391/833
Matching objects**. Older Profile dump/comparator reconciliation is still a
separate scratch proposal pending genuine header/consumer verification.
The active Fable lane and every reserved implementation remain excluded.

### Older Opus Profile dump reconciled as natural fuzzy source (2026-09-05)

The complete dump/comparator cluster now uses its true public owner header,
short mode/count API, public enums, semantic section fields and private format
tables. The older donor's uninitialized comparator return was rejected;
defined natural C is retained instead. Profile reaches **35 exact / 2 residual
/ 7 unwritten**, with dump parked at **99.147285%** and the comparator at
**21.59292%**. No artificial private helper emissions or exact-code credit
are claimed. The two real tables add **16 matched data bytes**.
See `object_matching_logs/profile_obj_dump_comparator_natural_boundary_20260905.md`.

The genuine 25-consumer header build and full stable sweep preserve all
**6,215 strict owners**, with zero regressions. All 288 tests plus 26 subtests
pass; **273 parks** validate and admission is unchanged. A scratch census
indentation defect was caught and corrected before publication: all four new
literal pools have compatible selected target/canonical owners, so they are
not an unresolved ownership veto. Totals are **905,027 meaningful code bytes**,
**6,169 credited functions**, **2,028,764 data bytes**, and **391/833 Matching
objects**. The remaining genuine Profile getters and separate Transport write
caller/helper cluster are uncredited research, not completed reconciliation.
All Fable reservations remain in force.

### Genuine Transport datagram caller/socket helper: 905,328 bytes (2026-09-05)

The general campaign retains the full natural datagram writer with its real
private socket-construction dependency, gaining **301 meaningful /304 padded
exact code bytes**. The helper has a semantic static name; the caller is a
**75.76159%** fuzzy park. Nine copied WinSock declarations and four duplicate
SDK layouts are removed in favor of the existing actual owner header. The
unit reaches **17E/3R/10U**. See
`object_matching_logs/transport_endpoint_winsock_obj_write_socket_cluster_20260905.md`.

Independent source review, canonical no-alias gate, full Ninja and stable
sweep pass: **6,216 strict owners**, one gain, zero regressions. All288 tests
plus26 subtests pass; **274 parks** validate; admission is unchanged. Three
new stock SDK tables pass duplicate-symbol processing with real canonical
base copies, but diagnostics end on unrelated unresolveds, not a successful
whole link. They receive no data credit; no whole-object admission is claimed.
The split selection1 values are explicit reconstruction JSON, not evidence
authorizing target metadata changes or fabricated SDK include shims.

Verified totals are **905,328 meaningful code bytes**, **6,170 credited
functions**, **2,028,764 data bytes**, and **391/833 Matching objects**.
Older Profile getter reconstruction remains a separate uncredited scratch
packet with its real public API/seconds-elapsed field dependency. All Fable
reservations remain excluded.
