# HS compiler / ActorFiringPosition owner reconciliation — 2026-09-07

## Result and honest accounting

Baseline: published canonical `458ba910cc907048a667215f7dc583e52cdab5e5`.
This is a coherent shared-owner and caller-ABI batch assembled from two completed
Claude lanes. Donor totals are evidence only; the table below is a fresh
current-canonical comparison.

| Measurement | Before | After |
| --- | ---: | ---: |
| Meaningful exact code bytes | 933,425 | 938,227 |
| Credited functions | 6,304 | 6,321 |
| Stable strict owners | 6,350 / 8,245 | 6,367 / 8,245 |
| Matching objects | 391 / 833 | 390 / 833 |
| Matched data bytes | 2,062,532 | 2,062,532 |

**Gross gain: 18 functions / 5,239 meaningful exact bytes. Explicit debit:
one function / 437 meaningful bytes. Net: +17 functions / +4,802 exact
bytes.** The object-count decrease is deliberate: ActionGuard is no longer
labelled Matching after one previously exact function became an honestly
measured fuzzy residual. No semantic override, alias waiver or matching-only
construct hides that loss.

HS compiler contributes eleven exact functions / 3,137 meaningful bytes:

| Function | Meaningful | Padded |
| --- | ---: | ---: |
| `hs_get_parameter_indices` | 162 | 176 |
| `hs_macro_function_parse` | 277 | 288 |
| `hs_parse_if` | 436 | 448 |
| `hs_parse_logical` | 358 | 368 |
| `hs_parse_arithmetic` | 394 | 400 |
| `hs_parse_equality` | 260 | 272 |
| `hs_parse_inequality` | 454 | 464 |
| `hs_parse_sleep_until` | 217 | 224 |
| `hs_parse_wake` | 201 | 208 |
| `hs_parse_object_cast_up` | 119 | 128 |
| `hs_parse_debug_string` | 259 | 272 |

ActorFiringPosition contributes seven exact functions / 2,102 meaningful
bytes: `firing_position_store_evaluation_debug` (78),
`firing_position_reject` (15), `pre_evaluator_guard` (205),
`post_evaluator_pursuit` (383), `pre_evaluator_pursuit` (464),
`pre_evaluator_panic` (649), and `actor_change_firing_position` (308).
The complete 104-byte pre/post evaluator table is also strict exact with all
eleven ordered relocations, but matched-data accounting is unchanged and no
separate data credit is claimed.

After this packet ActorFiringPosition has 11 exact, seven emitted residual and
nine currently nonemitted/unwritten target functions. HS compile has 27 exact,
27 residual and ten unwritten target functions. Neither object is complete.

## Donors and owner boundary

- HS donor: `C:/halo-worktrees/opus-30k-ui-script-profiles-20260906`, tip
  `08d6bea92b1b7da925bf3687b44310b6203953dd`. The admitted draft and pinned
  saved-object evidence are under `scratch/hs-builtin-owner-packet-20260907`;
  its patch SHA-256 is
  `d0387bcf32e5710e6fd99ecc83f3e9cb421589c7d72dfe5e62b79b8cdbca6e97`.
- Actor donor: `C:/halo-worktrees/fable-50k-resume-20260906`, tip
  `740bbc33aeaa7c2c6ee86304b7a52279df44a930`. The admitted draft, complete
  49-consumer manifest and target evidence are under
  `scratch/actor-firing-position-owner-packet-20260907`; its integration patch
  SHA-256 is
  `0ad2a904c8e2b46c2d029c067b5508699939d90f943b977b303c796b551e649c`.

Only the current canonical before/after transition above is counted. The lane
screenshots start at older, overlapping bases. They are not added to 933,425.
The reproducible complete-tip census is summarized in
`docs/reconciliation_queue_20260907.md`; detailed research remains in
`scratch/two-lane-screenshot-accounting-20260907.md`,
`scratch/opus2-complete-tip-backlog-intake-20260907.md`, and
`scratch/fable_complete_tip_remaining_intake_20260907.md`.

The HS owner migration places the real January `hs_function_definition`,
syntax-node union aliases, script/function enums, typed callbacks and compiler
APIs in their existing subsystem headers. It removes C-local duplicate schemas
and declarations. `_code_000b4970` is mapped to the private
`hs_get_parameter_indices`; its authentic callers retain it naturally. The
excluded 704-byte recursive conditional parser is not renamed or credited.

The public expression compiler prototype now gives its two output parameters
their correct ownership/order. January and HCEA nevertheless both call it with
the output addresses reversed in the affected `hs.c` path. That authenticated
original caller behavior is documented at the call and preserved; the project
does not silently rewrite history merely because the bug looks surprising.

ActorFiringPosition now owns the January 0x670 evaluation context, the 0x3C
candidate, genuine private evaluator names, the scenario firing-position
definition and the complete pre/post tables. Duplicated opaque structures and
foreign C-local prototypes were removed from consumers. The HCEA source is an
algorithm/name map only: its context is 0x664 and is not transplanted as the
January layout.

The third argument of `actor_move_to_firing_position` is correctly
`struct path_state *`, not a widened Boolean. January forwards the full pointer
through `actor_path_refresh`, whose destination/path builders dereference it.
The genuine `path.h` owner is used; Guard/Fight/Search/Uncover and ActorMoving
callers use real `path_state`, Boolean and firing-position-group types. The
nearby-position query likewise uses the semantic group-selection enum rather
than TRUE/FALSE.

## Retained fuzzy work and disclosed debit

Seven complete ActorFiringPosition bodies are emitted and parked with fresh
hashes. They receive zero exact credit:

| Function | Target / candidate padded | Relocations target / candidate | Fuzzy % |
| --- | ---: | ---: | ---: |
| `pre_evaluator_combatmove` | 576 / 560 | 21 / 19 | 95.75 |
| `post_evaluator_global` | 224 / 208 | 8 / 7 | 68.10 |
| `post_evaluator_hide` | 224 / 224 | 11 / 11 | 82.50 |
| `post_evaluator_uncover` | 192 / 192 | 4 / 4 | 86.56 |
| `post_evaluator_attack` | 224 / 208 | 3 / 3 | 90.36 |
| `pre_evaluator_global` | 1,616 / 1,616 | 53 / 53 | 90.75 |
| `pre_evaluator_attack` | 1,120 / 1,120 | 39 / 38 | 93.62 |

`pre_evaluator_global` and `pre_evaluator_attack` are the first complete natural
January-layout reconstructions for two table entries absent from the donor.
They use real types, named modes/categories and January diagnostics; their
non-exactness is not concealed. Four additional credible private source bodies
(`firing_position_pre_evaluate`, `firing_position_post_evaluate`,
`firing_positions_get_post_evaluation_bound`, and `firing_position_compare`)
remain written but naturally nonemitted because their real selector callers are
still unwritten. They have no fake retention caller and receive no code credit.

`action_guard_perform` is explicitly debited: 437 meaningful bytes, 448/448
padded bytes, 13/13 ordered relocations, 99.22481%. The genuine evaluation
context, candidate and path-state ABI change alters ordinary local allocation
and independent stack-load order. The retained source is the honest typed
implementation, not a facade constructed to preserve the old bytes. Its park
is unclassified; ActionGuard's config status is NonMatching.

No dead lookup, unused call-result local, compiler-pressure declaration,
representation pun, raw-offset facade, forced inline/noinline, volatile/register
steering, assembly, fake dependency or nonsensical branch is retained. The
January inline schedule remains intact and neither unit emits
`point_from_line3d`.

## Verification

- Ordinary `ninja all_source progress semantic_progress`: pass.
- Full tools suite: **1,120 passed, two skipped, 26 subtests passed**.
- Complete stable sweep: 8,245 rows; 18 gains and the one disclosed loss.
- Existing multi-function COFF comparator: `all_equal=true` independently for
  all eleven HS gains, all seven Actor gains and the 104-byte evaluator table.
- Per-unit gates: Actor 11 exact / seven residual / nine unwritten; HS compile
  27 exact / 27 residual / ten unwritten. Both forbidden-emitted-symbol guards
  pass for `point_from_line3d`.
- Source fake-match scan: 18 affected C/H files, zero findings.
- Park audit: 317 active, zero stale and zero invalid. Eight fresh records cover
  the seven Actor residuals and the ActionGuard debit.
- Object-admission audit: zero candidates, contradictions and revocations; five
  inherited rejected objects remain rejected.

Frozen baseline: `scratch/hs-actor-batch-before-20260907.json`.
Final object/source capture: `scratch/hs-actor-batch-final-20260907.json`,
SHA-256 `931fc8e42b6e2116c5c1c47babb775c6dd24d91924bba3036500c8e1b8b61d8a`.
Full owner review: `scratch/hs-actor-final-owner-review-20260907.json`,
SHA-256 `19dc1e12e2686de903976a428139a3c08e84748444afe9880776237593bea1f8`.
Final stable snapshots are
`scratch/hs-actor-before-stable-20260907.json` and
`scratch/hs-actor-after-stable-20260907.json`. Final park and admission reports
are `scratch/hs-actor-final-parks-20260907.json` and
`scratch/hs-actor-final-admission-20260907.json`.

The next forced treemap refresh remains at 940,828 meaningful bytes. This batch
ends at 938,227, so publishing it does not manufacture an early cadence update.
