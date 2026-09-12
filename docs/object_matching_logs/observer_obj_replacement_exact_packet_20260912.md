# Observer replacement exact packet (2026-09-12)

## Disposition

The behavior-reviewed real Observer caller closure is now production-applicable.
It advances `observer.obj` from 11 to 17 strict exact functions without losing
an inherited exact owner. Six newly exact functions contribute **1,032 meaningful
code bytes** and **1,072 padded code bytes**:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `observer_apply_rotational_displacement` | 113 | 128 | 4 | `0cf86fd06942eff7a71400d65c94625c1614baac0fc5e058feb50c721dda8541` |
| `observer_rotational_displacement` | 285 | 288 | 21 | `d04926a73a36a26456a81edbcb1b585fa981ccc927f2b0c5b132d68ad48541fe` |
| `observer_find_displacement` | 112 | 112 | 1 | `acc0e2d94f86fd952adedc64c106ed7272466130ed4a9302901c1e3ed4391e44` |
| `observer_update_displacements` | 78 | 80 | 6 | `30a59c7d53b23bf06cf10c188832ee46c311f6d63d0a188fd9abe511b2791173` |
| `observer_pass_time` | 152 | 160 | 13 | `6ecf7cfcf7419bff33ac6dc8fb30813bef841b019d2abbded312def6bc7e4a05` |
| `observer_update` | 292 | 304 | 24 | `e75d7b642735c9969f2139f26906ee0b346cef54ce05aeff10893ea8359f97af` |

The final focused gate reports **17 exact / 8 residual / 1 unwritten**. The
object is not complete and must remain `NonMatching`. The eight natural
residuals are retained as useful fuzzy source, without exact-byte credit.

## Source and behavioral provenance

This packet starts from the frozen fourteen-function production manifest and
its independent behavioral review:

- `docs/object_matching_logs/observer_obj_real_update_caller_cluster_20260905.md`;
- `docs/object_matching_logs/observer_obj_integrator_penetration_independent_review_20260905.md`;
- `scratch/observer_real_caller_cluster_behavior_verified_production_20260905.edits.json`
  in the `rasterizer-model-wrapper-alias-20260829` evidence worktree;
- HCEA's recovered `observer_apply_rotational_displacement.c`,
  `observer_find_displacement.c`, `observer_update_displacements.c`, and
  `observer_pass_time.c` under the local `research/halocea/src` corpus.

The frozen packet already passed the saved public dispatch, timer/NaN,
polynomial endpoint, integrator-order, and penetration refinement executable
checks documented by those ledgers. No behavior was changed here. Two source
shapes were restored from that evidence and then confirmed instruction for
instruction against January:

1. `observer_find_displacement` walks the first eight scalar parameters with
   three advancing typed pointers, then walks the orientation group in six-real
   increments and calls the genuine rotational helper. This reproduces the
   January 112-byte body exactly and naturally retains its private callee.
2. `observer_pass_time` retains a pointer to the five command timers and
   advances it after each ordered-positive clamp. This reproduces January's
   NaN-to-zero behavior and its complete 160-byte section exactly.
3. `observer_update` uses January's stringized assertion spelling without
   spaces around `==`. The target assertion literal directly authenticates
   that source spelling; it makes both assertion-string relocations exact
   without changing control flow or instruction selection.

The rotational application uses HCEA-corroborated `(real)sin(angle)` and
`(real)cos(angle)` calls. Under the January compiler they become the target's
x87 instructions inside the helper; no `sine` or `cosine` function owner is
emitted. This is recovered donor source, not an inlining directive or a
synthetic ownership workaround.

## Ownership and inline-schedule boundary

The final candidate adds the fourteen intended Observer functions and two
ordinary math-header COMDATs relative to the frozen pre-packet object:
`valid_real_point3d` and `vector_from_points3d`. It adds no `sine`, `cosine`,
or `point_from_line3d` definition. The two remaining helpers use the existing
natural header APIs and SELECT_ANY linkage.

Their ordinary-link boundary was already resolved and admitted by commit
`3ddb4158e`:

- Action Alert's unique NODUPLICATES `vector_from_points3d` owner was
  normalized to the natural SELECT_ANY header owner;
- Particles' unique NODUPLICATES `valid_real_point3d` owner was normalized the
  same way;
- Observer was linked against both normalized objects in both input orders
  without `LNK2005` in the saved ownership audit.

No new header switch, alias, false export, forced call, forced inline, or
linker accommodation is introduced by this packet. The January
`point_from_line3d` schedule remains unchanged.

## Rejected differential retention

`observer_collision_test_differential` remains the sole unwritten target
function. HCEA supplies a credible six-argument implementation, but January's
complete Observer relocation census contains **zero** incoming calls to it.
`observer_check_penetration` instead contains three direct relocations to
`observer_collision_test_with_t`. Adding a fake call, changing linkage, or
otherwise forcing the unused private body to emit would violate the campaign
rules, so the 64-byte function receives no credit.

## Focused verification

The final PID-isolated command was:

```text
python tools/campaign/gate.py source/camera/observer --out scratch/observer_packet_final.obj --all --forbid-emitted-symbol _point_from_line3d --forbid-emitted-symbol _sine --forbid-emitted-symbol _cosine
```

Result: emitted-symbol guard pass; 17 exact, 8 residual, 1 unwritten. The final
candidate object SHA-256 is
`3a1b9e8728e32700f5096aaf54b5c7c2c15055f38dd54fcc65e98edaab52e15b`.
The root integration subsequently passed the full build, stable sweep,
admission audit, park validation, all-owner census, aggregate link, and test
suite. Aggregate publication evidence is in the sixth 10K batch ledger.

## Remaining honest fuzzy work

The following behavior-reviewed bodies remain fuzzy and receive no matching
credit: `observer_check_penetration`, `observer_collision_test_with_t`,
`observer_postcheck`, `observer_update_accelerations`, `observer_update_command`,
`observer_update_polynomial`, `observer_update_positions`, and
`observer_update_velocities`. Their current
implementations are retained as the closest authenticated source baseline for
future research rather than being register-tuned or rewritten with fake
matching constructs.
