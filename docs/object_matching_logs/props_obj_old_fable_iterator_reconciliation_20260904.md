# Old Fable intake refresh and Props iterator reconciliation, 2026-09-04

## Scope and disposition

Canonical baseline: `18238d9065014b67fa2a1d745140f203be6e44dd`.
This is a read-only refresh of the **old** Fable lanes followed by one
canonical-based scratch implementation of `_prop_new_unacknowledged`.
The new Fable 50,000-byte lane was not inspected or modified.

The first natural iterator reconstruction is strict exact: **544 meaningful
and padded bytes, 21 relocations**. Props changes from 13 exact / 4 residual /
0 unwritten to **14 exact / 3 residual / 0 unwritten**, with all inherited
exact owners retained and no other code or non-code owner changed. This is a
real missed source-shape lead, not a claim that all historical donor-only
exact counts are outstanding admissible progress.

**Not yet admission-ready:** the existing public
`actor_perception_desire_prop` declaration in Props disagrees with its actual
definition in both its second and fourth parameter types. It is also still
declared only in the consuming C file. The proposal does not copy or add any
such declaration, but it necessarily inherits this unresolved cross-TU API.
The proper owner/signature must be reconciled and the affected TUs verified
before publication under the house rules. No production C, header, config,
build artifact, commit, or remote was changed by this investigation.

## Refreshed old-Fable inventory

### Old small-families dirty worktree

`C:\halo-worktrees\fable-small-families-20260901` remains on
`fable/small-families-20260901`, HEAD
`c59115611bad57ba03e31ae47660825608807002`.
Its 20 tracked dirty files hash identically to their preserved versions on
`root/fable-small-families-salvage-20260902`, tip `7f249a9b4`:

- 18 C files: `ai/{actors,ai_script,encounters}.c`,
  `bungie_net/common/key_agreement.c`,
  `game/{game_engine,game_engine_ctf,game_engine_king,game_engine_oddball,game_engine_race,game_engine_slayer,player_control}.c`,
  `interface/{ui_widget,virtual_keyboard}.c`, `models/models.c`,
  `objects/widgets/{flags,lightning}.c`,
  `rasterizer/rasterizer_frame_statistics.c`, and
  `tool/connected_geometry.c`, all below `source/`.
- `source/bungie_net/common/key_agreement.h` and `tools/campaign/gate.py`.

For each path, compare `git -C <donor> hash-object --path=<path> <path>` with
`git rev-parse root/fable-small-families-salvage-20260902:<path>` from
canonical. This establishes no later tracked dirty-file advance, not that
every historical line was admitted. Both the donor HEAD and the historical
closure-audit commit `9b0d6fc3e` are ancestors of the current canonical HEAD.

Relevant already-completed adjudication records:

- `docs/object_matching_logs/fable_backlog_canonical_admission_20260902.md`:
  the prior 18-TU sweep found no donor-written function still unwritten in
  canonical; its 46 strict closures / 15,568 padded bytes are historical,
  not additional credit in this report.
- `docs/object_matching_logs/post_fable_large_wave_canonical_integration_20260902.md`:
  later 22 strict closures / 3,936 padded bytes, also historical.
- `docs/object_matching_logs/ai_script_obj_fable_snapshot_reaudit_20260902.md`
  and `ui_widget_obj_fable_snapshot_reaudit_20260902.md`: the remaining raw
  old snapshots had declaration/global-name debt, not unharvested bodies.

### Old exact-finish worktree

`C:\halo-worktrees\fable-exact-finish-20260829` has clean tracked state at
`b2fa98b2a0a056bf259369f9beb25d6880656e78` on
`fable/exact-finish-20260829`. Its final commit remains the August 31 commit;
no later tracked source version was discovered there.

A read-only in-memory `tools.campaign.stable_verdicts.build_snapshot` over
each tree's cached `objdiff.json` and existing base/split objects found:

| Cached artifact set | Strict | Residual | Unwritten | Function owners |
|---|---:|---:|---:|---:|
| Old exact-finish | 4,648 | 67 | 3,531 | 8,246 |
| Canonical baseline | 6,186 | 393 | 1,666 | 8,245 |

There are **31 old-exact/current-nonexact owners totaling 10,864 padded
bytes** when keyed by stable target section, equal target size, and equal
target normalized hash. This is a refreshed **cached comparison**, not a
fresh donor build, a newly recovered byte total, or an admissible backlog
estimate. It supersedes the older 25 / 10,272 cached comparison only as an
inventory measurement. Six entries / 96 bytes are absent bare no-ops or SDK
copies; the other 25 / 10,768 bytes are already reconstructed residuals.

The 31 entries are distributed across wind (1), transparent geometry (1),
rasterizer lights (1), Xbox active camouflage (4 SDK functions), LRAR cache
(1), real math (2), progress bar (1), HUD sounds (1), players (12), game
statistics (1), game engine (1), recorded animations (1), dead camera (1),
Props (1), actor type infection (1), and action converse (1).

Most have explicit existing source-credibility dispositions: out-of-bounds
reset in `opus_misc_small_admission_20260903.md`; fake SDK wrappers in
`rasterizer_xbox_active_camouflage_obj_jonas_credibility_recovery_20260830.md`;
forced-inline/helper debt in `lrar_cache_obj_codex_small_closeout_20260903.md`;
representation casts in `real_math_obj_jonas_global_typed_boundary_20260830.md`;
unsupported volatile in `hud_sounds_obj_source_authenticity_review_20260904.md`;
player barriers/assembly/dead-emission debt in
`players_obj_january_safe_harvest_20260902.md`; and explicit natural parks in
the wind, transparent geometry, statistics, recorded-animation, dead-camera,
infection, and converse ledgers. These are not instructions to repeat those
rejected experiments. Only the Props iterator lead was reopened here.

The separately reported later Opus continuation remains the independent
intake question described in
`docs/opus_unfinished_session_reconciliation_backlog_20260903.md`; this audit
does not claim to locate or fully adjudicate that continuation.

## Props provenance and the single source change

Read `docs/object_matching_logs/props_obj.md` before probing. Its accepted
scalar traversal and failed declaration/branch-order experiments did not
test the existing typed iterator form. The new lead comes from
`fable/exact-finish-20260829:source/ai/props.c` at the frozen commit above.

The full-TU proposal copies current canonical Props and replaces only
`prop_new_unacknowledged`. It imports no donor globals, public declarations,
private declaration reorder, header, compiler setting, or other function.
The final donor call is corrected to the canonical and authenticated order:

```c
prop_add(actor_index, unit_index, prop_index);
```

The recovered traversal uses the existing `struct prop_iterator`,
`prop_iterator_new`, and `prop_iterator_next`. It preserves the two replacement
candidates, required same-side prop counts, enum-based orphan-state exclusion,
reuse assertions, `actor_switch_props`, removal, identifier preservation,
typed clear, and ordinary final private call. It has no raw offset view,
representation pun, volatile anchor, inline assembly, forced inline,
allocation probe, or declaration-order search. Public iterator functions
remain independently emitted and exact; any folding inside this caller is
the ordinary January compiler's decision, not source-forced inlining.

Pinned later-source corroboration is under
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\halocea-full-blobs-20260830\src\blam\ai\`:
`prop_new_unacknowledged.c`, `prop_add.c`, `prop_iterator_new.c`, and
`prop_iterator_next.c`. HCEA is semantic corroboration, not the byte or ABI
authority. The January split object is the sole byte authority.

### Genuine iterator layout

`source/ai/props.h` defines the two-long, 8-byte cursor: `index` at 0,
`next_index` at 4. Its public declarations already own both functions.
January `_prop_iterator_new` (48 bytes / 2 relocations) loads the actor's
first prop at `actor + 0x50` and stores cursor `+4`.
January `_prop_iterator_next` (48 / 2) copies `+4` to `+0`, tests `NONE`,
looks up the datum, and copies `prop + 8` back to cursor `+4`.
The proposal reads `iterator.index` only after a successful next call, so
there is no uninitialized cursor field read or invented handle-width change.

### Private `prop_add` argument identity

The authentic source order remains `(actor_index, unit_index, prop_index)`.
January's optimized private ABI promotes the unit handle to EAX and leaves
the actor and prop handles on the stack; it is not a public calling-convention
change to encode in the declaration.

- Callee `+8` loads `[ebp+0xc]` as the prop handle; `+0x11` saves EAX as
  the unit handle; `+0x53` loads `[ebp+8]` for the actor lookup.
- Callee `+0x78` writes the actor handle to `prop + 4`, and `+0x8a` writes
  the unit handle to `prop + 0x18`.
- Caller `+0x207` loads actor from `[ebp+8]`, `+0x20a` loads unit into EAX
  from `[ebp+0xc]`, then pushes selected prop and actor before the call at
  `+0x20f`, cleaning only 8 stack bytes afterwards.
- January `prop_new_blank` likewise passes EAX = `NONE` for the unit handle,
  with the allocated prop and actor on the stack.

The proposal reproduces this naturally while leaving the existing private
callee unchanged. Importing the old donor's actor/prop/unit declaration
reorder is neither necessary nor part of this packet.

## Frozen verification artifacts

All paths below are relative to canonical unless absolute.

| Artifact | SHA-256 |
|---|---|
| Production `source/ai/props.c`, unchanged | `47e295f444b8378d0859eeb7671ba1341833b2b2484b7b5bff4f933a3342d67e` |
| `scratch/props-fable-iterator-proposal-20260904.c` | `d8af83e4bf916af334de8bb1cf4f3c7b970444f6c89287fe026d5576fca5c651` |
| `scratch/props-fable-iterator-baseline-20260904.obj` | `228f8e5b6204d301e39886b3b19249df8dd907b425f6ed3f6d191e0303fe2f62` |
| `scratch/props-fable-iterator-first-shot-20260904.obj` | `6b4ed7c51a20bc038fde952a74268a9cc319068ee782364ee229cb0b798bccfe` |
| January `build/split/source/ai/props.obj` | `334d4328a8a8ca7274f2135df4ce4d5e56e54c8c28b471483c69634e4197e881` |

Reproduce the one-TU scratch gates, without Ninja or build-directory writes:

```powershell
python tools/campaign/gate.py source/ai/props --out scratch/props-fable-iterator-baseline-20260904.obj --all --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/ai/props --source scratch/props-fable-iterator-proposal-20260904.c --out scratch/props-fable-iterator-first-shot-20260904.obj --all --forbid-emitted-symbol _point_from_line3d
```

The complete candidate symbol guard passes for both. The new function's
baseline normalized SHA is
`a1a468c4a6846a4287d6ccadd45a8510786c71b3d5182c5ef545993c05c4b455`;
candidate and January both have
`4e1ed0d47f97343a146353af5bd690ae5ca07dbe5da2374d3015293440f957aa`.
All 21 relocation identities, addends, and addresses match, at offsets:
`8,51,77,84,205,362,368,384,391,418,423,428,434,456,461,466,472,486,493,509,528`.
The last return ends at offset `0x220`; this 544-byte gain contains no
alignment tail. Existing strict padded code is 1,328 bytes; proposed strict
padded code is 1,872 bytes. No whole-object Matching claim follows.

### Complete code-owner census

All owners other than `_prop_new_unacknowledged` have exactly unchanged
normalized section information from the baseline, including residuals and
the inherited extra helper.

| Candidate code owner | Padded bytes | Relocations | Proposed status |
|---|---:|---:|---|
| `_vector_from_points3d` | 48 | 0 | Inherited candidate-only owner, unchanged |
| `_props_initialize` | 64 | 7 | Exact, preserved |
| `_props_dispose` | 16 | 0 | Exact, preserved |
| `_props_initialize_for_new_map` | 16 | 2 | Exact, preserved |
| `_props_dispose_from_old_map` | 16 | 2 | Exact, preserved |
| `_prop_add` | 592 | 25 | Residual, unchanged |
| `_prop_new_blank` | 48 | 3 | Exact, preserved |
| `_prop_remove` | 320 | 22 | Exact, preserved |
| `_prop_iterator_new` | 48 | 2 | Exact, preserved |
| `_prop_iterator_next` | 48 | 2 | Exact, preserved |
| `_prop_new_unacknowledged` | 544 | 21 | New strict exact |
| `_prop_setup_orphan` | 224 | 6 | Exact, preserved |
| `_prop_orphan_transition` | 208 | 16 | Exact, preserved |
| `_prop_orphan_from_friend` | 240 | 18 | Exact, preserved |
| `_prop_orphan_update_information` | 32 | 1 | Exact, preserved |
| `_prop_delete` | 48 | 3 | Exact, preserved |
| `_prop_get_active_by_unit_index` | 144 | 5 | Residual, unchanged |
| `_prop_get_base_by_unit_index` | 384 | 15 | Residual, unchanged |

### Complete non-code census and extra owners

The same 15 named non-code owners are emitted before and after: the 4-byte
`.data` global `_last_prop_data_full_warn_time`, and 14 `.rdata` literals of
sizes `10,26,5,45,37,69,183,183,217,46,32,32,39,46` bytes. Each owner has
unchanged section bytes, normalized hash, linkage, and zero relocations.
There is no BSS or COMMON allocation and no new named owner of any kind.
`_prop_data` remains an undefined external, not a fabricated local allocation.

The existing two ordinary string COMDATs (`"prop"` and
`"prop->orphan_prop_index == NONE"`) are candidate-local but target-undefined;
their attribution is already documented in `props_obj.md` and is unchanged.
The inherited `_vector_from_points3d` helper also remains unchanged. Neither
is silently ignored or treated as a newly authenticated whole-link proof.

## Public API blocker and safe handoff

`source/ai/props.c:91` currently declares the public
`actor_perception_desire_prop` with second parameter `long ignored_prop_index`
and fourth parameter `long prop_actor_index`.
The actual definition in `source/ai/actor_perception.c:1298` instead takes
`short prop_type` and `union actor_perception_prop_actor_slot prop_actor`.
That union is TU-private at line 296: a `long actor_index` overlaps an
anonymous structure containing `byte padding[3]` and `boolean too_far`.
The function first consumes the actor handle, then reuses the last byte of
its parameter home as a boolean. This is not evidence that such a union was
an authentic public API; do not move this carrier union into a header merely
to make declarations agree.

The pinned HCEA prototype corroborates a short prop type and a plain integer
actor handle. January caller/callee evidence must determine the corrected
primitive public signature and a credible separate local boolean. There is
no `actor_perception.h`; the existing closest owner is the
`ACTOR_PERCEPTION.C` section in `source/ai/actors.h:1126`. Its current absence
there is genuine prototype ownership debt. `actor_switch_props` already has
its proper declaration in that header and needs no new local declaration.

Root may reconcile this public API in a separate bounded owner packet, with
Actor Perception source authenticity review and full dependent verification.
Then regate this frozen Props proposal against that proper owner header,
preserving all inherited exacts. No claim is made that the current scratch
544-byte closure will survive those header/source corrections; no false
regression exception or caller-local workaround is recommended.

Until then, retain this source-shaped missed gain as a **verified scratch
candidate pending API reconciliation**, not published progress or completed
old-Claude intake. The original residual and all prior rejection history
remain intact in canonical and in the original object ledger.
