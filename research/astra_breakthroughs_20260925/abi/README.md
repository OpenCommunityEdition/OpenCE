# Return-contract scheduling census

Research baseline: `20d0563d508f7836670db62514ffc3d1f1e2217e`.

This tests one transferable historical success, not a general ABI verifier:
the actor-perception narrow-return correction changed the order of `add esp,8`
and `mov edi,eax` before a 16-bit compare. Its old report incorrectly called the
callee correction impossible. A later joint return-type/single-exit-body change
closed the package. Read the correction at the top of
`docs/object_matching_logs/claude_lane_a_narrow_return_package_20260920.md`,
not the superseded conclusion below it.

## Fresh result

Across 467 source comparison units, **43,445 paired direct-call observations**
produce **zero new candidates**. Exact-control population is 7,440 functions;
119 residual owner functions are considered. 62 functions are conservatively
skipped because linear decoding is incomplete. The synthetic linker pool has
no comparison pair. Zero findings is not proof that all prototypes are correct:
indirect calls, unsupported uses, casts, switch reachability and skipped code
remain outside this narrowly defined detector.

Positive control: using the saved *pre-fix* actor-perception object detects
exactly the original `actor_get_perception_knowledge` scheduling discrepancy
in `_actor_perception_update`. January adjusts ESP then copies EAX; the old
object copies EAX then adjusts ESP. Current canonical no longer has that cue.
Thus this signature is presently a negative worklist, not an excuse to repeat
the old 16 caller-side source-shape probes.

## Reproduce

```powershell
python research/astra_breakthroughs_20260925/abi/return_contract_census.py --output research/astra_breakthroughs_20260925/abi/current.json
python research/astra_breakthroughs_20260925/abi/return_contract_census.py --unit source/ai/actor_perception --override source/ai/actor_perception=C:/halo-worktrees/claude-lane-a-ai-core-20260920/scratch/verify5/cleanup-anomaly/floor.obj --output research/astra_breakthroughs_20260925/abi/positive-control.json
python -m pytest -q research/astra_breakthroughs_20260925/abi/test_return_contract_census.py
```

The positive control uses an existing local object, not a committed proprietary
binary. Synthetic byte-pattern tests also cover the positive signature and do
not require that worktree. The archived object is historical evidence, not a
newly recompiled source candidate.

## Review corrections and boundaries

Independent review is retained in `../census/ABI_REVIEW.md`. The final script
kills return-value provenance on explicit/implicit/partial register writes;
excludes high-byte comparisons; stops at calls, branches, LOOP, traps and joins;
skips incomplete linear decoding; and does not pair repeated names across
unobserved indirect/addend calls. Regression tests exercise these cases.

`add esp,N` is only a local scheduling observation. It can clean older deferred
arguments. Sign extension can arise from a caller cast rather than the declared
return type. Neither observation authorizes a header edit. The tool never grants
credit, changes prototypes, or overrides a source-authenticity hold.
