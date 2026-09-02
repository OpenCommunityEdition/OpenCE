# `actions.obj` large fuzzy closeout (2026-09-01)

## Result

The canonical `source/ai/actions` reconstruction remains the strongest known
source frontier: **58/63 strict-exact functions, five residual functions, and
zero unwritten functions**.  No production C changed in this pass.  All five
remaining bodies are now evidence-backed fuzzy parks under the current house
rule, so future exact work requires genuinely new provenance or a natural
same-compiler code-generation lever.

The five honest residuals account for 5,392 January padded bytes:

| Function | Target | Candidate | Relocations | Fuzzy |
| --- | ---: | ---: | ---: | ---: |
| `_actor_action_handle_vehicle_entry` | 960 | 960 | 22 / 22 | 96.54422% |
| `_actor_action_handle_lost_contact` | 1,872 | 1,872 | 55 / 55 | 95.328384% |
| `_actor_action_handle_danger_avoidance` | 1,520 | 1,504 | 34 / 34 | 90.41458% |
| `_actor_action_try_to_dive` | 720 | 704 | 29 / 29 | 85.18107% |
| `_actors_searching_same_position` | 320 | 320 | 14 / 14 | 87.87611% |

These parks do not claim strict byte credit.  They classify completed semantic
source whose remaining differences have resisted credible source-level
control.

## Evidence reviewed

Before parking, this pass read the preserved Claude, Fable, and integration
records:

- `docs/object_matching_logs/actions_obj.md`;
- `docs/object_matching_logs/actions_obj_codex_closure.md`;
- `docs/object_matching_logs/actions_obj_jonas_policy_reconciled_replay_20260827.md`;
- `docs/object_matching_logs/actions_obj_jonas_large_natural_pair_no_fake_20260830.md`;
- `docs/object_matching_logs/actions_obj_jonas_pursuit_name_recovery_20260831.md`;
- `docs/object_matching_logs/actions_obj_semantic_private_names_20260831.md`;
- the campaign throughput and methodology records.

`tools/campaign/branch_sweep.py source/ai/actions` compiled nine unique source
blobs reachable from the registered campaign branches.  None exceeded the
canonical 58-exact/5-residual/0-unwritten frontier.  Older blobs either failed
against the current headers or regressed to 54 or fewer exact functions.

## Anti-fake ruling

The only known strict form of `_actor_action_handle_danger_avoidance` uses an
empty positive `if` arm followed by all meaningful work in `else`.  That form
was independently identified as an exact-score-only topology and previously
reverted.  It remains rejected.  The retained production source uses the
coherent negative predicate, accepts the honest 16-byte envelope difference,
and contains no volatile/register steering, barriers, pragmas, artificial
dependencies, inert carriers, undefined behavior, or nonsensical branches.

The other four retained bodies are likewise ordinary typed C.  Their residuals
are compiler allocation, private convention, frame coloring, instruction
scheduling, or translation-unit-context differences documented in the parked
manifest.  No inline assembly is introduced, and the object emits no
`point_from_line3d` COMDAT.

## Reopen conditions

Reopen a parked function only for one of:

- authenticated January source or local-variable records;
- a compatible original compiler/QFE that naturally emits the target;
- a natural same-compiler donor with the same control-flow and lifetime shape;
- new cross-build provenance that contradicts the retained semantics.

Pure score steering is not new evidence.

## Verification

The admission commands for this closeout are:

```text
python -B tools/campaign/gate.py source/ai/actions
python -B -m tools.parked_functions
python -B tools/fake_match_scan.py source/ai/actions.c
git diff --check
```

The final integration wave must additionally run the full canonical Ninja,
semantic, regression, test, and COMDAT checks before publishing.
