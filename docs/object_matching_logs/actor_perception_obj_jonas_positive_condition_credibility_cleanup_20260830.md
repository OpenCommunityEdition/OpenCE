# Actor Perception positive-condition credibility cleanup (Jonas, 2026-08-30)

## Scope and disposition

The source-credibility scanner identified the same decompiler-shaped pattern
in three functions: an empty `if (prop_index != target)` arm followed by an
`else` containing the only work. The retained source now states the program
intent directly:

`if (prop_index == actor->target.target_prop_index)`

This is a source-quality cleanup and grants no new matching credit. The
condition is meaningful for all inputs: target status and combat status are
updated only when the changed prop is the actor's current target.

## Strict evidence

The January target is
`build/split/source/ai/actor_perception.obj`, 62,717 bytes, raw SHA-256
`19704943ab15e64d0bebdfa0688a5779f3efca899bf8631f1f529a995da379e4`.
The rebuilt candidate is 35,821 bytes with raw SHA-256
`9c4e2be16eba918f2d65284a25852cb68ba9097e7e5e8e13619919f5cb61b9f7`.

All three changed functions remain strict:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_actor_perception_tried_to_uncover` | 96 | 6 | `58f17beb273a26458cad059091b7a4e183eb611a0afee6a6a80367927f0b7e28` |
| `_actor_perception_tried_to_search` | 96 | 6 | `01c91339247c4e21aa89c99fccc267c13a085cce8281d1aff44fb4b501a1c9ff` |
| `_actor_perception_abandoned_search` | 160 | 9 | `0296c3853644f20f8b64648974329b16035e2fb4cf53727f8af6a62a14a6e2c7` |

The whole-object exact census remains 33 before and 33 after, with
`lost_exact=[]` and `newly_exact=[]`. The positive condition therefore
preserves January's emitted code without relying on empty control flow.

## Verification

- XDK 3911 rebuilt the object through its generated Ninja edge.
- `tools/coff_compare.py` reports all three functions equal.
- `tools/audit/compare_object_exact_sets_20260827.py` reports no exact
  function loss or unreviewed gain.
- `python tools/fake_match_scan.py --format json
  source/ai/actor_perception.c` reports zero review leads.
- `git diff --check` passes.

Reopen only if authenticated original source supplies a different meaningful
condition topology. An empty branch is not retained merely because it can
compile to the same bytes.
