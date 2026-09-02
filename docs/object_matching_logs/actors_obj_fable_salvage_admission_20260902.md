# `actors.obj` Fable salvage and admission audit

## Decision

The preserved Fable wave is accepted as a strict, source-authentic improvement
to `source/ai/actors.obj`. Against canonical commit `778961b89`, the object
moves from 29 exact, zero residual and 47 unwritten functions to 65 exact,
three residual and eight unwritten functions. This is a net gain of 36 strict
functions and 7,984 padded code bytes with zero semantic exact regressions.

The object is not complete and no whole-object admission is claimed. No new
park entry is claimed either: the three remaining bodies are retained as
honest residuals, but this integration did not establish enough shared
relocation/compiler evidence to classify all of them as fixed points.

## Provenance and isolation

- Canonical comparison base: `778961b89`.
- Preserved Fable source: local branch
  `root/fable-small-families-salvage-20260902`, commit `eefc8f877`.
- Isolated source snapshot commit: `a1cf440a4`.
- Original worker evidence:
  `C:/halo-worktrees/fable-small-families-20260901/scratch/workers/actors.log`.
- Original Fable scratch corpus:
  `C:/halo-worktrees/fable-small-families-20260901/scratch`.
- All integration, cleanup and verification occurred in the isolated branch
  `agent/fable-actors-salvage-20260902`. Canonical and GitHub were not changed.

The worker log records each gated wave from the 29-function baseline through
the 65-function candidate. The integration recompiled the final source rather
than trusting worker self-reports.

## Strict additions

The hardened COFF comparator agrees on padded bytes, relocation identity and
normalized bytes for every entry below.

| Function | Padded bytes | Relocations | Normalized SHA-256 prefix |
| --- | ---: | ---: | --- |
| `_actor_activation_debug_color` | 160 | 11 | `8e4a6ac0268216f4` |
| `_actor_attach_unit` | 464 | 33 | `55d978dbf785a22f` |
| `_actor_clear_orders` | 64 | 3 | `586fc94317a6e11e` |
| `_actor_clear_output` | 160 | 3 | `8ac4638b22ebe3c6` |
| `_actor_delete` | 288 | 25 | `2404525161283842` |
| `_actor_destination_tolerance` | 144 | 8 | `88fe860231ae600b` |
| `_actor_detach_from_unit` | 224 | 15 | `a8f4063759ce209d` |
| `_actor_find_pathfinding_location` | 192 | 6 | `0ccffee2d51eea23` |
| `_actor_flush_structure_indices` | 192 | 7 | `2c51e5f7268b9515` |
| `_actor_freeze` | 160 | 6 | `7d580a00086245b3` |
| `_actor_freeze_unit` | 128 | 7 | `5f4beeaa4fb0dd83` |
| `_actor_general_update` | 560 | 24 | `24a837b5cd448590` |
| `_actor_get_running_blind_vector` | 208 | 8 | `540f25ae00455afc` |
| `_actor_get_timeslice` | 160 | 4 | `efc142f3040d52e1` |
| `_actor_handle_communication` | 160 | 7 | `f71b04f6359b3d5b` |
| `_actor_handle_damage` | 224 | 10 | `ce50912b39d6197e` |
| `_actor_handle_unit_effect` | 448 | 20 | `fb6491f49021ec10` |
| `_actor_input_sample_position` | 112 | 6 | `eaabaf06260c93ff` |
| `_actor_kill` | 224 | 8 | `f941b10ee088bce3` |
| `_actor_randomly_control_unit` | 192 | 8 | `74a5a666d3a7a1fd` |
| `_actor_set_active` | 208 | 14 | `734242d0b094078b` |
| `_actor_set_dormant` | 272 | 12 | `6e0610a16861afef` |
| `_actor_swarm_cache_delete` | 128 | 8 | `dffb5a9f1f2a3f9a` |
| `_actor_swarm_cache_new` | 272 | 18 | `1bbcebddfafdb97e` |
| `_actor_swarm_component_refresh` | 80 | 4 | `b71a0a1e23319e5c` |
| `_actor_swarm_component_setup` | 112 | 9 | `53e52c1d11301681` |
| `_actor_swarm_detach_from_unit` | 464 | 27 | `f7a005fd9cc992a4` |
| `_actor_swarm_unit_died` | 160 | 13 | `19a2fe076d3d8542` |
| `_actor_update` | 320 | 33 | `13752ae7f754f1ad` |
| `_actor_update_begin` | 48 | 6 | `4b8de83f552e76b3` |
| `_actor_verify_activation` | 224 | 13 | `5303a050a9ad03ed` |
| `_actor_verify_unit_activation` | 128 | 8 | `a0c35d5566c9eda8` |
| `_actors_handle_spatial_effect` | 256 | 12 | `7af2928a09ce3539` |
| `_actors_handle_unit_effect` | 560 | 19 | `542d9355d33ad170` |
| `_actors_move_randomly` | 160 | 7 | `a91a8767f37e4083` |
| `_actors_update` | 128 | 8 | `e10c7ce552155793` |

The raw name-only sweep reports one apparent loss and 37 gains because the
baseline target calls the 16-byte exact leaf `_code_0002a150`, while the
candidate correctly calls it `_actor_update_end`. Comparing that same target
address under its semantic name proves that it remains exact. The resulting
alias-aware totals are zero losses and 36 gains. No other object changes its
exact set.

## Semantic names and linkage

The HCEA PDB at
`research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX.pdb`
directly corroborates the names, signatures and private linkage of
`actor_swarm_component_refresh`, `actor_freeze_unit`, `actor_freeze`,
`actor_get_timeslice`, `actor_swarm_component_setup`, `actor_general_update`,
`actor_input_update`, `actor_unit_control` and `actor_update`. DIA reports no
inline assembly for those functions. The PDB SHA-256 is
`F55CFE957DA8079A62A26A6753F1DDBB700B067A66DE0BD5DB5AAC182D985FF1`.

HCEA source filenames and behavior corroborate the remaining public actor
names and such private names as `actor_clear_orders` and
`actor_randomly_control_unit`. `actor_update_begin` and `actor_update_end` are
descriptive, convention-consistent names inferred from their symmetric writes
to `global_updating_actor_index`; `actor_clear_output` and
`actor_decision_loop` are likewise inferred from their exact bodies and call
graph. A January target xref census found every renamed address placeholder
private to `actors.obj`. `config/symbols.json` therefore marks those targets
`static` and retains no `code_0002...` placeholder for this object.

The reconstructed target-owned globals retain semantic names
`global_updating_actor_index` and `global_movement_animation_states`; no
address-derived BSS name was introduced. Narrow TU views used for the vehicle
AI radius, AI service globals and AI profile meters have semantic fields and
compile-time offset assertions.

## Prototype ownership and C2 header audit

Declarations were removed from the ad-hoc `actors.c` dependency block and
placed with their owning interfaces:

- action lifecycle declarations are in `source/ai/actions.h`;
- actor-looking declarations, including target-owned
  `valid_real_normal2d`, are in `source/ai/actor_looking.h`;
- communication declarations are in `source/ai/ai_communication.h`;
- encounter attach/detach declarations are in `source/ai/encounters.h`;
- public `actors.c` declarations are in `source/ai/actors.h`;
- the still-unwritten private `actor_input_update` keeps a documented
  TU-local forward declaration until its static body is reconstructed.

The owner audit caught and rejected two tempting header workarounds. Putting
`valid_real_normal2d` in `units.h` was not accepted because January owns the
body in `actor_looking.obj`. Likewise, forward-declaring
`ai_information_packet` in broad `actors.h` perturbed C2 context enough to
drop exact `_unit_preprocess_node_orientations`. Moving
`actor_handle_communication` to the already complete
`ai_communication.h` owner and keeping `valid_real_normal2d` in
`actor_looking.h` restores that 1,920-byte exact unit function without a
redundant compensating declaration. A fresh full-tree sweep confirms zero
exact regressions.

Canonical advanced to `293d9d0c4` after this candidate's frozen baseline. It
adds the narrow `source/ai/actor_placement.h` owner for the still-unwritten
public `actor_place`, expands `encounters.h`, and adds both includes to
`actors.c`. Integration must preserve `actor_placement.h` and its include.
Canonical's `encounters.h` is a superset of this candidate's encounter
declarations, so its expanded version should win any textual conflict; do not
duplicate those prototypes. The actor symbol renames are in a disjoint offset
range from canonical's encounter renames. `actors.h` and the prototype-removal
hunk in `actors.c` overlap textually and need semantic reconciliation rather
than an ours/theirs choice.

## House-rule audit

- All 68 implemented functions end in an explicit terminal `return`.
- No-argument functions use `void` on its own line and multi-argument
  declarations use one parameter per line.
- Actor, prop, swarm and unit access uses the typed subsystem macros. The two
  raw `object_get` uses read the common base object's location and do not cast
  to a derived datum. Tag-block access uses `TAG_BLOCK_GET_ELEMENT`.
- Source uses `real`/cseries types and named action, mode, combat-status,
  spatial-effect and unit-effect enum constants. Control flags use `FLAG`.
- No inline assembly, `volatile`, register forcing, optimization pragma,
  forced inline/noinline, byte emission, fake dependency or undefined
  representation trick is present.
- The January inline schedule remains untouched and the rebuilt object emits
  no `point_from_line3d` COMDAT.
- The one January-authenticated oddity in activation checking—the target's
  last-vehicle-index/time comparison—is documented rather than silently
  rewritten or disguised.
- `tools/fake_match_scan.py` finds zero review leads across the actor source
  and touched owner headers.

## Remaining honest frontier

| Function | Target size/relocs | Candidate size/relocs | Status |
| --- | ---: | ---: | --- |
| `_actor_attacking_target` | 176 / 7 | 160 / 7 | Same relocation identities; VC7 tail-merges the two vector-copy success paths. HCEA's early-guard form also yields 160. Explicit component copies overshoot to 192, and pointer/`memcpy` spellings either worsen register selection or add an unauthenticated relocation. |
| `_actor_decision_loop` | 560 / 34 | 560 / 34 | Complete typed behavior, but relocation identities and normalized schedule differ. |
| `_actor_unit_control` | 784 / 48 | 784 / 48 | Complete typed behavior, but relocation identities and normalized schedule differ. |

The eight still-unwritten functions are `_actor_create_for_unit` (624),
`_actor_customize_unit` (640), `_actor_died` (832), `_actor_input_update`
(2,384), `_actor_new` (944), `_actor_place` (800),
`_actor_swarm_attach_unit` (656), and `_actors_spawn_from_unit` (672).

## Verification

- `python tools/campaign/gate.py source/ai/actors --all`:
  65 exact, three residual, eight unwritten.
- Pinned `778961b89` baseline was configured and all 573 rebuilt units were
  compiled before comparison.
- Full candidate targets `all_split all_source progress semantic_progress`
  pass; semantic report: 5,186 accepted exact and zero unit errors.
- Final project report: 680,343 / 2,198,102 exact code bytes and
  5,139 / 11,060 exact functions.
- Alias-aware full-tree exact-set sweep: zero lost, 36 gained, all gains in
  `source/ai/actors.obj`.
- `python -m pytest -q -p no:cacheprovider`: 258 passed.
- Parked validation: 106 active, zero stale, zero invalid.
- Object admission audit: zero candidates and zero contradictions; the one
  inherited rejection is unchanged.
- `git diff --check`: clean.
- Reproducibility SHA-256 values:
  - `source/ai/actors.c`:
    `C740926601231B4291328F2BF3C4031E08D6563313A903E9DF0076161750796E`
  - `source/ai/actors.h`:
    `BFCB5C8FE772929F256E95E889733F7CBB753D3D9B3F234F873A94E1B3A13632`
  - January split `actors.obj`:
    `E07C0F6375FAF94DA001D8D594AEAE65FC57C1597CADF74323F3B1038C7C8E95`
  - rebuilt `actors.obj`:
    `7D1AC61215FFEA4863A043A7B796A76856C97DDEC6BF25BD37ACF93383BC14FE`
