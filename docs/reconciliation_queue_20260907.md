# Reconciliation queue: completed/interrupted Claude lanes

Updated 2026-09-08 after the user confirmed that both Opus 2 and the new
Fable 5 lane had concluded or exhausted their tokens. Their implementation
reservations are released for reconciliation. Donor worktrees remain read-only:
preserve committed, dirty and scratch work; do not restart either lane or send
another prompt without a new request.

## Verified canonical checkpoint

Canonical is
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`,
branch `jonas/ai-debug-integration-20260829`.

The Lights batch is committed as
`adc1f36a2a95cff0ae99ebbf3b3bc1ee026e1932`, independently verified on both
`jonas/exact-pilots` and `jonas/ai-debug-integration-20260829` at remote
`jonas` (`https://github.com/bnunu/halo.git`). It adds 289 meaningful exact
code bytes, reaching **930,363 code bytes / 6,283 credited functions**;
**391/833 Matching objects** and **2,061,024 matched data bytes** are unchanged.
The complete strict sweep is 6,329/8,245, one gain and zero regressions.
Full tests: 1,019 passed, two skipped, 26 subtests passed. Independent focused
runtime review: ten passed. See
[the Lights ledger](object_matching_logs/rasterizer_lights_obj_frame_start_reconciliation_20260906.md).

Published `34db75424bb629c061b5d8b714294aa9d41d5d22` is the new Fable continuation's solid RenderDebug box:
**+378 meaningful exact bytes**, reaching **930,741 code bytes / 6,284 credited
functions**. Stable census: **6,330/8,245**, one gain and zero losses; object/data
totals are unchanged. Full tests: **1,038 passed, two skipped, 26 subtests**;
root actual-x86 replay: 138 executions and two rejected byte mutations.
See [the Box ledger](object_matching_logs/render_debug_obj_box_reconciliation_20260907.md)
for frozen inputs, complete owner audit and the remaining graph holds.
Both authorized remote branch tips were independently verified at that commit.

The next verified Opus 2 packet is the private ProgressBar base-layer wrapper:
**+87 meaningful exact bytes**, reaching **930,828 code bytes / 6,285 credited
functions**; stable census **6,331/8,245**, one gain and zero regressions.
Full tests: **1,059 passed, two skipped, 26 subtests**. Its original-target/first/
ordinary private-ABI replay passes 72 executions plus two rejected byte mutations.
See [the ProgressBar ledger](object_matching_logs/progress_bar_obj_base_layer_reconciliation_20260907.md).
This is one small wrapper, not a whole-object or whole-lane completion.
Published as `62b3cec7c55c27ebc5aaf28f664a61763ac16f00`; both authorized
remote branch tips were independently verified at that commit.

The next verified packet is Weather delete plus three private helpers:
**+444 meaningful exact bytes**, reaching **931,272 code bytes / 6,289 credited
functions**; stable census **6,335/8,245**, four gains and zero regressions.
Full tests: **1,120 passed, two skipped, 26 subtests**. See
[the Weather ledger](object_matching_logs/weather_particle_systems_obj_delete_reconciliation_20260907.md).
The inherited BSS linkage discrepancy remains explicit; no whole-object claim.
Published as `e36d74756548de01732db37d3ff9eca7549dde3d`; both authorized
remote tips were independently verified there.

The next verified coherent batch is PlayerProfile plus S3TC:
**+3,712 newly exact bytes minus 1,559 explicit exact-to-fuzzy debits =
+2,153 net**, reaching **933,425 meaningful exact bytes / 6,304 credited
functions**. Stable census is 6,350/8,245: 17 gains and two documented losses,
not zero regressions. Matching objects remain 391/833; matched data is 2,062,532.
Both debits are independent stack-load order changes from proper header
ownership; no matching override hides them. Ten new useful fuzzy bodies are
also retained, with unsafe donor paths repaired and zero exact credit.
Full tests: 1,120 passed, two skipped, 26 subtests. Parks: 309 active, none
stale/invalid. See [the batch ledger](object_matching_logs/player_profile_s3tc_batch_reconciliation_20260907.md).
Published as `458ba910cc907048a667215f7dc583e52cdab5e5`; both authorized
remote branch tips were independently verified there.

The next verified coherent batch is HS compiler plus ActorFiringPosition:
**+5,239 gross exact bytes minus one explicit 437-byte ActionGuard debit =
+4,802 net**, reaching **938,227 meaningful exact code bytes / 6,321 credited
functions**. Stable census is **6,367/8,245**: 18 gains and one disclosed loss.
Matching objects decrease to **390/833** because ActionGuard is honestly
reclassified NonMatching; matched data remains **2,062,532**. Seven complete
Actor residuals plus four credible written-but-naturally-unemitted private
bodies are retained without exact credit. Full tests: **1,120 passed, two
skipped, 26 subtests**. Parks: 317 active, none stale/invalid. Admission audit:
zero candidates, contradictions or revocations. See
[the HS/Actor ledger](object_matching_logs/hs_actor_owner_reconciliation_20260907.md).
Published as `25c45ad20d49587d98640ca78c0c08014819bbf0`; both authorized
remote branch tips were independently verified there.

The next verified coherent packet combines CollisionBSP, Damage and the small
GameState Xbox allocation owner: **+2,653 meaningful exact code bytes / +7
credited functions**, reaching **940,880 meaningful exact code bytes / 6,328
credited functions**. The rename-stable census is **6,374/8,245**: seven gains /
2,704 padded bytes / zero regressions. Matching objects remain **390/833** and
matched data remains **2,062,692 bytes**. Five complete CollisionBSP bodies and
two complete Damage bodies are retained as explicit fuzzy parks with zero exact
credit. The GameState packet also replaces the inherited `bss_004d27d0` global
name with `xbox_game_state_globals`. Full tests: **1,120 passed, two skipped,
26 subtests**. See [the CollisionBSP/Damage ledger](object_matching_logs/collision_bsp_damage_owner_reconciliation_20260907.md)
and [the GameState ledger](object_matching_logs/game_state_xbox_obj_allocate_buffer_reconciliation_20260908.md).

Per the latest [house rules and batch cadence](campaign_house_rules.md), the
940,828-byte canonical treemap threshold has now been crossed. After this
refresh, the next forced cadence threshold is **950,828 meaningful exact bytes**.
Batch larger coherent packets and isolate blockers; do not repeat bespoke
runtime test development for every ordinary byte-exact leaf.

No other part of the new-lane totals below is added to that checkpoint.
They are donor-relative observations, not an audited union or promised future
credit. Fresh canonical rebuilds, source admission, complete runtime ownership
and regression checks are still required for every packet.

## Complete-tip lane accounting against current canonical

The user-provided screenshots are internally reproducible, but each starts at
its lane's older, overlapping base. Opus reaches 963,046 meaningful credited
bytes in its own tree and Fable reaches 948,240; neither total is added to
canonical. A complete stable-owner set comparison finds 231 unique lane gains /
64,872 meaningful bytes, with no intersection between these two particular
new-gain sets. Forty of those owners / 9,552 bytes are already exact after the
HS/Actor trial.

After the seven newly admitted CollisionBSP, Damage and GameState owners, the
remaining donor-strict/current-nonstrict set is **184 owners / 52,667 bytes**.
That still includes 1,409 bytes already reconciled as safer fuzzy source and the
615-byte rejected Alpha3 encoder. Removing those known dispositions leaves a
**50,643-byte upper screening ceiling**, not a promise of recoverable exact
credit. Further ABI, owner, source-quality and collateral-regression review will
reduce or partition it.

The broader **54,663-byte** donor-exact/current-nonexact difference additionally
contains exactly 1,996 bytes of known collateral debits: Bitmap mipmap (866),
solo-level UI (693) and this batch's ActionGuard (437). These are not fresh lane
work and must not be used to claim that reconciliation lost or ignored bytes.

Reproducible detailed evidence is in
`scratch/two-lane-screenshot-accounting-20260907.md` and its adjacent JSON/script.
Complete source/owner intake is in
`scratch/opus2-complete-tip-backlog-intake-20260907.md` and
`scratch/fable_complete_tip_remaining_intake_20260907.md`. The largest queued
families are UIWidget, UI game-data input, HS compile and saved-game storage.
The next packet must again be selected from the current-canonical owner set
difference, not from either lane headline. All
figures are screening bounds until rebuilt and admitted in current canonical.

## Opus 2: UI, scripting and profiles

- Worktree: `C:/halo-worktrees/opus-30k-ui-script-profiles-20260906`.
- Branch: `opus/30k-ui-script-profiles-20260906`.
- Base: `19765bb76b7a1d287a146ce37b2832810366d47a`.
- Observed tip: `08d6bea92b1b7da925bf3687b44310b6203953dd` (35 commits).
- Tracked tree clean; untracked `scratch/` preserved. Git reported permission
  warnings for its cache/global ignore; this does not inventory inaccessible
  untracked contents.
- Changes: 13 C files, one header, three configs, seven tools, 75 documents.

The handoff `docs/object_matching_logs/opus_30k_ui_script_profiles_HANDOFF_20260906.md`
is stale at `50bf63c0b`. Latest wave-13 commit instead reports **+46,273
meaningful code bytes** (916,773 to 963,046), 6,355 credited functions, 287
parks, and 502 tests plus 26 subtests. No new behavioral tests were added in
this lane. Read-only saved-snapshot comparison confirms **140 strict gains,
zero losses**, 6,258 to 6,398 exact rows out of 8,245; that is not a fresh build
or current-canonical admission.

First packets to inspect:

1. `playlist_profile_get_display_name`: 53 meaningful / 64 padded target
   bytes. Natural copy/accessor leaf; needs its public prototype in the genuine
   `source/saved games/playlist_profile.h` and proper dependency headers.
   Do not import the surrounding data/BSS facades or other profile bodies.
   **First owner trial is now held:** the leaf is exact and 20 runtime tests
   pass, but its genuine header declaration regresses an existing 693-byte
   exact UI function. The packet was fully restored with zero lasting losses;
   see [the held trial](object_matching_logs/playlist_profile_obj_display_name_owner_hold_20260907.md).
2. `draw_gravy_base_layer`: **87 meaningful / 96 padded bytes admitted** with
   unchanged real caller, full owner audit and explicit semantic-name/static
   inference. The empty 16-byte noise helper remains excluded: its January
   caller placement is not authenticated.
3. Other small player/playlist profile leaves and their real read/write graphs.
4. HS parser helper/caller packets, then larger UI and save-file logic.

Admission boundaries:

- Playlist retains address-named globals and prefix views; data ownership is
  expressly unaudited. New leaf credit does not close that object.
- Game-state source retains a BSS prefix, locally copied public/SDK prototypes
  and a new `static __inline` helper. No blanket import.
- `player_profile.h` changes a return type from `short` to `word`; a conflicting
  local `long` declaration remains in a different UI consumer. Resolve the
  genuine contract and full transitive header effects.
- Review unused call-result locals, repeated source-shape searches, duplicated
  accessors, original-bug claims and uninitialized failure paths individually.
  Exactness does not authenticate those mechanisms.
- Three config diffs include six semantic credits and symbol/static changes.
  Reconcile entries individually; no wholesale config replacement.
- A function-only "closed" saved-game file is not a whole-object admission.

Tool review is queued separately:
`tools/campaign/add_semantic_matches.py`, `merge_candidates.py`,
`refresh_parks.py`, `relcount.py`, `relocdiff.py`, `stdcall_scan.py`,
`unparked.py`. Configuration-mutating helpers must not be run to manufacture
admission. Review their safety/correctness before any tooling publication.

Read-only review found concrete blockers: `relcount` hardcodes a UI target;
`relocdiff` truncates unequal lists with `zip`; `unparked` discards compile
failure status; the return-convention scanner overclaims stdcall from `ret N`.
`merge_candidates` misses competing insertions at the same gap and mixed EOLs.
`refresh_parks` can alter target size through an unscoped replacement and remove
by function name without unit; `add_semantic_matches` accepts absent baseline
entries as fresh credit and hardcodes lane cases. Do not run/copy these as
trusted general tooling. Harden read-only diagnostics with negative tests first;
keep manifest-editing utilities held pending admission-policy review.

Primary pointers under the donor root:

- `docs/object_matching_logs/opus_30k_ui_script_profiles_session_20260906.md`.
- `docs/object_matching_logs/opus_lane_wave9_orchestrator_20260907.md`.
- `docs/object_matching_logs/hs_compile_finish_obj_opus_wave13_20260907.md`.
- `docs/object_matching_logs/never_swept_obj_opus_wave13_20260907.md`.
- `docs/object_matching_logs/uiw_finish_obj_opus_wave13_20260907.md`.
- `scratch/baseline-19765bb76.json`, `scratch/after-wave13.json`,
  `scratch/ninja13.log`, `build/report.json`, `build/semantic_report.json`.

Snapshot SHA-256 pins:

- Before: `fb3225b553f82edf883c4e89ac4bd6be147ae710d4bb9aabcc09f7b5e59ae973`.
- Wave 13: `5d92eb9450b5fadb7a9a578e26de5ed8220820e768f63c70bae1146dbdb68dd5`.
- Raw report: `778f02c804f162fbe55c8f6b81dd882b8608e5ea1e78d8e7a1f8bd3ee045adf0`.
- Semantic report: `f798f86c29a3c13c8f7e6dced40b243ebbe4b5684c5074313b9f150b9ad94552`.

## New Fable 5 continuation: committed and dirty waves

- Worktree: `C:/halo-worktrees/fable-50k-resume-20260906`.
- Branch: `fable/50k-resume-20260906`.
- Base: `aae36798942305d022ee9502915c15dac7431c39`.
- Observed tip: `740bbc33aeaa7c2c6ee86304b7a52279df44a930` (four commits).
- Committed changes: 14 C files, four headers, two configs and the ledger.
- Dirty tracked work: the ledger, `source/ai/encounters.c`,
  `source/cache/xbox_texture_cache.c`, `source/physics/collisions.c`.
  Preserve untracked `scratch/` too.

The main ledger is `docs/object_matching_logs/fable_50k_resume_lane_20260906.md`.
It reports **+21,455 meaningful credited code bytes** (926,785 to 948,240),
with 391/833 Matching objects unchanged. Saved-snapshot comparison confirms
**91 strict gains / 22,144 padded bytes, zero exact-status losses**, 6,314 to
6,405 of 8,245 rows. This includes naming/owner recovery, not exclusively newly
written bodies. Bink contributes **ten net gains**, not fourteen: four callers
were restored baseline matches, and must not be counted twice.

Saved `scratch/pytest-r4.log` records 950 passed, five skipped and 26 subtests.
`scratch/ninja-r4.log` ends FAILED on two stale structure-visibility parks.
The later `scratch/resume-parks-r4b.json` has no stale/invalid records and the
ledger claims a successful rerun, but intake did not locate that rerun's saved
Ninja log. Reproduce the build; do not silently convert this discrepancy into
a verified pass. The ledger's attribution of its chosen base to the refreshed
prompt is also stale; retain the real commit identity above.

First packets to inspect:

1. **RenderDebug box-only admitted:** 378 meaningful / 384 padded bytes, using
   the genuine narrow `source/render/render_debug_geometry.h` owner. All three
   complete header consumers preserve inherited code/data/ownership. This is
   one admitted new-continuation packet. The remaining cylinder/pill/
   height-matrix graph is conditional: the saved donor adds nine math code
   owners absent from the January object, including a normalize3d fallback.
   Do not restore its rejected debug-global header packet or the held circle
   caller with an omitted variadic offset. See existing RenderDebug ledgers.
2. ActionFlee flush/replace/modify-color: saved exact **186 meaningful / 208
   padded bytes**, held pending review of the inherited source-provenance
   boundary and proper public declarations. The minimal actions.h closure is
   47 objects; Race is not among them. A previous combined actions.h/units.h
   failure therefore does not prove this narrower packet regresses Race.
   AI communication is included and remains a measured sensitivity risk.
   Begin/update stay outside this packet.
3. Geometry verify/vector: saved exact **657 meaningful / 672 padded bytes**,
   held on prior source-topology/expression reviews; vector also activates the
   existing point-to-vector cast in its plane helper. The combined donor adds
   five SELECT_ANY math owners (288 padded bytes), not new target credit.
   A minimal declaration packet has six consumers; its emitted-owner subset
   has not been freshly isolated. The q-count ABI correction and changed
   structure-visibility parks remain separate work.
4. **Weather delete-only admitted:** public delete plus three private helpers,
   **444 meaningful / 464 padded bytes**, with genuine tag/runtime ownership,
   proper declarations/names and unchanged Game consumer. Natural private ABIs
   match; all inherited runtime owners survive. Target BSS storage2 versus
   candidate storage3 is an inherited gap, not resolved by this packet.
   New-system and rendering/update additions remain excluded. Damage, player
   queues, Bink, collision/path and physics still need genuine owner/schema work.

Preserve existing holds: no fabricated combined player-queues global, no
external-linkage conclusion from a BSS split or memset extent, no foreign
prototype/type facade, no speculative inline substitute, no camera-global
aggregation. The new camera donor does not resolve the original linkage hold.

### Dirty wave: not included in the r4 credit

- Encounters: claimed 640-padded pursuit closure uses an implicit foreign
  call; proper prototypes/enums are needed. Respawn is fuzzy, not exact credit.
- Collisions: the current dirty file **fails the forbidden
  `_point_from_line3d` emission guard** after adding `units/bipeds.h` and emits
  other math owners. Do not copy it wholesale. Keep its separate scratch
  helper/name-recovery candidates for a genuine owner packet.
- Xbox texture cache: three claimed name-gap owners total 112 padded bytes;
  two 144-byte format mappers have local-table identity differences. Audit
  table/global ownership, enums and prototypes before admission.

Read `scratch/reports/<unit>.md` and `scratch/hdr/<unit>.md` for each selected
unit, including all earlier canonical object ledgers and rejection criteria.
Snapshot/dirty SHA-256 pins:

- `scratch/resume-before.json`: `6821c47049642366b7ae3f6f20d7cd44bfe70bc102af1c7a4145e83540c69991`.
- `scratch/resume-r4.json`: `4974984f8030bdff429c81a1fb38579847d874c05560b1c47c3e659a7e613d6e`.
- Main dirty ledger: `a7153cf0cea8bda080692e88d9dedd64662c212f03fb316adc2b3f0506c36362`.
- Dirty encounters: `9c91916689b490f21ab2b4d0fe7843f414ca8882cbc302d7ea71214d63372440`.
- Dirty texture cache: `121364b93e575af119147e4bee311b47597bf5a2d82d27caec2598fb5c1c9a34`.
- Dirty collisions: `73b137815679268564abe744ce9cbee917b9e4d1a607cf5d9de18aa704ad7230`.

## Existing backlog remains queued

The [completed first Opus lane intake](object_matching_logs/opus1_reconciliation_intake_20260906.md)
tracks 8,195 admitted meaningful bytes across its reconciliation packets,
including 6,906 from the original 30,074-byte headline and 1,289 previously
uncounted strict bytes. **23,168 of that original headline remains unadmitted
as exact**, including honest fuzzy source and held candidates. This is not a
guaranteed remaining recoverable total.

Keep its FrameStatistics genuine-owner/draw research and other rasterizer
holds, plus the [older Opus backlog](opus_unfinished_session_reconciliation_backlog_20260903.md)
and [old interrupted Fable intake](object_matching_logs/fable_50k_small_families_interrupted_lane_intake_20260905.md).
The old Fable worktree is not the new continuation tree. No parked or rejected
body is reopened merely because a new lane repeats an exactness claim.

Only the canonical orchestrator edits shared headers/configs, runs full
Ninja/capture/sweeps, commits or pushes. Workers use disjoint read-only intake
or isolated scratch outputs. Freeze the current canonical baseline before
each material packet, preserve all inherited exact and credible partial work,
and publish coherent verified batches to the two authorized branches only.

## 2026-09-08 UI, Weather, AimAssist, RenderDebug, and HS admission

The commit containing this checkpoint reconciles the strongest admissible
packet from Opus `08d6bea92` and Fable `740bbc33a`. A clean replay from
canonical `24092ea0f` verifies the same result as the shared integration tree:
**975,317 meaningful exact code bytes / 6,467 credited functions**, a net gain
of **10,187 bytes / 40 functions**. The strict section sweep records 45 gains
and three explicit debits. Complete useful residual bodies are retained at
zero credit, and the declaration-context Decals debit is parked rather than
source-tuned.

The full evidence and per-function boundary list is in
[the reconciliation ledger](object_matching_logs/claude_ui_weather_aim_render_hs_reconciliation_20260908.md).
The canonical multi-lane treemap was force-refreshed after crossing the
972,207-byte cadence checkpoint. The next required refresh threshold is
**985,317 meaningful exact code bytes**.

## 2026-09-08 Fable/Opus final reconciliation packet

The current canonical full build and semantic ledger report **1,031,486
meaningful exact code bytes / 6,587 accepted exact functions**. Relative to
published commit `8074a18fc` at 982,763 bytes / 6,492 functions, this is a net
canonical gain of **48,723 meaningful exact bytes / 95 accepted functions**.
Donor-relative headline totals are not added separately.

The final universal strict snapshot for the last source slice records 22 net
new strict functions / 8,848 padded bytes. Its only debit from the provisional
snapshot is the deliberate rejection of an undefined saved-game failure path;
there are zero unintended regressions. Credible residual bodies are retained
in source and `config/parked.json` with zero exact credit; in particular, the
448-byte `_encounter_control_actors` scheduling residual is parked rather than
tuned further. See
[the final checkpoint](object_matching_logs/claude_fable_opus_reconciliation_checkpoint_20260908.md)
and [remaining-donor ledger](object_matching_logs/claude_remaining_donor_batch_20260908.md).

The canonical multi-lane treemap was force-refreshed after this build. The next
required refresh threshold is **1,041,486 meaningful exact code bytes**.
