# `path_smoothing.obj` natural-source frontier and fake-match audit (2026-08-30)

## Disposition

`source/ai/path_smoothing` remains **NonMatching**. This wave accepts natural,
source-backed progress from one strict function to three, but does not call the
translation unit complete:

- strict padded code: `128 / 3056` -> `1536 / 3056` bytes;
- strict meaningful code: `116 / 3021` -> `1514 / 3021` bytes;
- strict functions: `1 / 6` -> `3 / 6`;
- January data remains `348 / 348` bytes matched;
- objdiff whole-code fuzzy score is `99.651825%`.

The net accepted movement is therefore **+1408 padded bytes, +1398 meaningful
bytes, and +2 strict functions**. The remaining three helpers have equal
padded size, equal instruction count, and exact ordered relocation semantics,
but they are still byte-different and receive no exact credit.

This audit applies the stronger acceptance rule that exact bytes are necessary
but not sufficient. No expression, local, branch, or helper form is retained
solely because it steers VC7.0. Every retained change must also be semantically
coherent, historically plausible for the January source, and supported by the
later source reconstruction and/or January machine evidence. Where a natural
exact source shape was not found, the result is left fuzzy.

## Frozen scope and toolchain

- base commit: `f366c3456b3976de630b895664da99d863cecac5`
- isolated worktree:
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/path-smoothing-natural-20260830`
- branch: `agent/path-smoothing-natural-20260830`
- compiler command: VC7 `CL.Exe /O2 /Oy- /DDEBUG /Dxbox`, using the configured
  XDK 3911 headers and libraries
- regenerated January target:
  `build/split/source/ai/path_smoothing.obj`, 6,937 bytes,
  SHA-256 `cf411b31a2dad27878bb31800d1d0b4016f7b8407e4ddb9850514b5daa948427`
- retained candidate:
  `build/base/source/ai/path_smoothing.obj`, 8,579 bytes,
  SHA-256 `ebf4dd548b166e4e86ec17004d5df90f88e31304efbe90eb0cdf44590c594b5e`
- retained source:
  `source/ai/path_smoothing.c`, 17,922 bytes,
  SHA-256 `0fe9762c2409e4e6854cdfe10e3c382077f66d608319f8c59979b121d0fbb5e2`

The target object physical hash changed from older ledgers because this wave
regenerated csplit after replacing five address placeholders with recovered
private names and correct static storage. Function bytes and normalized hashes
are the comparison authority.

## Documentation and provenance reviewed before editing

Repository policy and tooling:

- `README.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `docs/object_matching_logs/README.md`
- `docs/object_matching_logs/path_smoothing_obj_jonas_private_surface_predicate_20260826.md`
- `tools/audit/README.md`
- `tools/campaign/README.md`

Claude/Stian reconstruction policy:

- `research-cache/stian-halo-cseries-20260820/CLAUDE.md`
- `research-cache/stian-halo-cseries-20260820/AGENTS.md`
- `research-cache/stian-halo-cseries-20260820/docs/vc71-byte-accuracy-playbook.md`

Later open-source behavior and parameter evidence:

- `research-cache/halocea-cseries-20260820/src/blam/ai/path_smooth.c`
- `research-cache/halocea-cseries-20260820/src/blam/ai/choose_turning_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/game/find_tangent_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/game/find_avoidance_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/game/find_turning_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/ai/path_state_build_path.c`
- `research-cache/stian-halo-cseries-20260820/src/halo/ai/path_smoothing.c`

The HaloCEA material is a later Xbox build and is used as a semantic and
naming donor, not as January byte proof. January disassembly, COFF layout, and
relocations decide version-specific source shape.

Read-only prior experiment records were also reviewed in
`C:/halo-worktrees/fable-exact-finish-20260829/scratch/agents/`:

- `props_pure.md`
- `p3_push.md`
- `o8_w3.md`
- `w5_w2.md`
- `h6_rules_part2.md`

Those notes contain both successful source-shape controls and failed compiler
steering attempts. This wave independently rebuilt the final natural frontier
and reran the high-value reversions described below; it does not treat an old
reported percentage as acceptance evidence.

## Recovered symbol and declaration surface

The later function names align uniquely by parameter roles and behavior with
the six January code ranges. Five private csplit symbols are now correctly
named and marked static:

| January address | Retained symbol | Role |
|---:|---|---|
| `0x00051190` | `_surface_is_walkable` | pathfinding/breakable surface predicate |
| `0x00051210` | `_find_tangent_point` | selected tangent on an obstacle-radius circle |
| `0x00051360` | `_find_avoidance_point` | avoidance point from the two tangents |
| `0x00051480` | `_choose_turning_point` | clockwise/counterclockwise route choice |
| `0x000516A0` | `_find_turning_point` | collision-edge graph walk |
| `0x00051A20` | `_path_smooth` | public raw-path smoothing driver |

No `code_<address>` private name remains in this translation unit. The public
`path_smooth` declaration now lives in the closest associated header,
`source/ai/path.h`. The previously local declaration for
`structure_test_pill2d` now lives in its owning
`source/ai/path_structure_bsp.h`, with `real_point2d` parameters. Private
geometry helpers also use `real_point2d`, `real_vector2d`, and `real` rather
than primitive stand-ins. `TEST_FLAG` replaces the manual walkability bit
extraction without changing the exact predicate bytes.

## Strict function measurements

All sizes below are padded COMDAT sizes. Relocation equality means count,
type, address, symbolic destination, and addend agree in order under the
hardened COFF comparator.

| Function | Target / candidate bytes | Target / candidate relocs | Differing normalized bytes | Target SHA-256 | Candidate SHA-256 | Result |
|---|---:|---:|---:|---|---|---|
| `_surface_is_walkable` | 128 / 128 | 5 / 5 | 0 | `a58fa4f6cf9598874c2a0d1fb60f226f65dea7f5cbc7182a682b2faefc62580f` | same | strict exact |
| `_find_tangent_point` | 336 / 336 | 5 / 5 | 15 | `1b6ac8d66faebb7cd5f983b94bce9221b0ae02aac796075046c8ad6b761eaa26` | `9275ddd16d26875d7d9b2916f48f227c80a6f81c96e8243b0f0ba42abbbc1a41` | fuzzy |
| `_find_avoidance_point` | 288 / 288 | 6 / 6 | 2 | `00f44b3323d2a668374b4c4f3ecb541c790dfb458d40b3e194c8b3e30b462c6d` | `79e1808ab70fc302dd591269dba93edff9f2de692ee6b446a3bab0698c4e6c84` | fuzzy |
| `_choose_turning_point` | 544 / 544 | 16 / 16 | 0 | `1c9b040bff7029f845c2572fa8b5a4380cdb520ce7f47715186953e7a4f959de` | same | strict exact |
| `_find_turning_point` | 896 / 896 | 30 / 30 | 2 | `a7ae7c9184ec854251bf1ac815e774537ec9a58631da2407bc0e38a088fbbc7d` | `c727a31a7c4e017de5b999ebfb54a1e2d0b3be59b8d9f1e9451ef5de44323885` | fuzzy |
| `_path_smooth` | 864 / 864 | 30 / 30 | 0 | `7d8f91b3598f4f62f00b1af734a678202badbcc2dca2d1721157ca7a820d36de` | same | strict exact |

Objdiff reports the three residual meaningful bodies as:

- `_find_tangent_point`: 326 meaningful bytes, `96.806725%`;
- `_find_avoidance_point`: 285 meaningful bytes, `99.981310%`;
- `_find_turning_point`: 896 meaningful bytes, `99.993866%`.

## Natural source recovery

### `_choose_turning_point`

The retained code builds each direction through the existing
`vector_from_points2d` and `normalize2d` helpers, computes the two signed-turn
totals, and returns the corresponding center. This is the straightforward
geometry described by HaloCEA. The nested helper expression reproduces the
January inlining/dataflow and is strict exact without adding a new inline
definition, assembly, volatile state, or a no-op temporary.

### `_path_smooth`

The retained outer `if (raw_step_count > 1) ... else` shape follows the
January branch layout. The loop scans runs of blocked raw steps, walks both
sides of the collision edge, chooses the smaller turn, finds two tangents and
an avoidance point, reprojects the result, and advances the path. The result
is strict exact and agrees with the later build's algorithm.

Two edits deserved special fake-match scrutiny:

| Edit | Later donor shape | January evidence | Semantic/historical decision |
|---|---|---|---|
| materialize `smoothed_step = &smoothed_steps[smoothed_count]`, then increment before projection | indexes the array directly and increments after the writes | January increments the count before the projection call and reuses the materialized slot address for the point and surface writes | accepted: ordinary pointer-to-current-output-slot code, no dead operation, same valid-input behavior, and directly explains call/register dataflow |
| index `raw_steps[collision_step_index]` directly instead of caching `collision_step` | caches a pointer | January repeatedly forms the indexed raw-step address and does not preserve the donor pointer lifetime | accepted: the simpler direct-index source is natural and removes an evolved later-build local; no artificial reload or alias violation |

Neither edit is accepted merely because `_path_smooth` becomes exact. Both
are independently consistent with the target instruction order and with a
normal implementation. Reverting the exact nested source in
`_choose_turning_point` left `_path_smooth` exact, proving that its admission
does not depend on keeping unrelated byte steering in the callee.

### Remaining helpers

The retained tangent, avoidance, and edge-walk implementations follow the
HaloCEA geometry and graph traversal, with January-specific statement order,
assert spelling, surface-side selection, and radius handling where directly
shown by the target. They are intentionally not perturbed further to chase the
last x87 operand choices.

## Bounded control matrix

| Control | Observation | Decision |
|---|---|---|
| B0: untouched `f366c3456` source | 128 / 3056 padded bytes, 1 / 6 strict functions; only the surface predicate exact | frozen baseline |
| E1: full natural source recovery | 1536 / 3056 padded bytes, 3 / 6 strict; tangent/avoidance/turning equal-sized residuals | retained basis |
| E2: revert the nested `choose_turning_point` helper source while retaining the driver | driver stayed exact; chooser became residual | retained natural nested helper form on its own evidence |
| E3: revert tangent helper topology to the old hand-expanded macro | tangent grew to 368 bytes and the driver lost exactness; an added direct re-derivation grew it to 384 bytes | rejected; old hand expansion is the wrong source/ABI topology |
| E4: use direct 2-D cross expressions rather than forcing the inline cross helper | strict 3 / 6 result unchanged and the unnecessary `_cross_product2d` COMDAT disappeared | retained direct expressions; avoids gratuitous inlining/ownership |
| E5: replace primitive pointer stand-ins with recovered `real_point2d` types and move declarations to owning headers | every function size, hash, and relocation result unchanged | retained under house rules |
| E6: replace manual walkable-bit shift with `TEST_FLAG` | surface predicate remained strict exact | retained under house rules |

The prior Fable experiment records also contain broad permutations of
commutative operands, local order, flags, wrappers, and temporary forms that
did not improve the residuals naturally. They are research history, not a
license to keep contrived variants, and were not replayed as a brute-force
search in this wave.

## Residual disassembly classification

All three residuals have exact ordered relocations and the same instruction
count as January:

| Function | Target / candidate instruction count | First relevant divergence |
|---|---:|---|
| `_find_tangent_point` | 129 / 129 | at `+0x72`, January reloads `[ebp-0x14]` and multiplies `st(3)`; VC7 instead emits `fxch st(2)` then multiplies `[ebp-0x14]`, with related stack cleanup/scheduling changes through `+0x95` |
| `_find_avoidance_point` | 110 / 110 | at `+0x2A`, January loads `[ebp-0x14]` then multiplies `[ebp-0x10]`; candidate reverses the two commutative memory operands |
| `_find_turning_point` | 326 / 326 | at `+0x190`, January loads `[ebp-0x0C]` then multiplies `[ebp-0x34]`; candidate reverses the two commutative memory operands |

The repository's fail-closed codegen classifier returns `UNKNOWN` for each
one, while confirming equal size and exact relocation semantics. Accordingly,
this ledger describes the measured x87 scheduling/operand residuals but does
not promote them to a machine-classified park entry or grant credit. Reopen
only for new source provenance or a demonstrated natural legal-C form.

## Inline COMDAT ownership audit

The retained source preserves the configured January math-helper schedule and
does **not** emit `point_from_line3d`. VC7 emits six candidate-local helper
COMDAT copies:

| Helper | Bytes / relocs | Normalized SHA-256 | January owner used for comparison |
|---|---:|---|---|
| `_square_root` | 16 / 0 | `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7` | `source/ai/action_charge.obj` |
| `_vector_from_points2d` | 32 / 0 | `5c9ee61634eac376cfc65890719accfeb6e0f5db37c55ab93b4da88bcbfe4f80` | `source/ai/action_obey.obj` |
| `_scale_vector2d` | 32 / 0 | `496d82c0615b4ec0cb4211dd21dfbf507d8959357a60895b9fd4d930b554cf95` | `source/ai/action_charge.obj` |
| `_magnitude_squared2d` | 32 / 0 | `9d8460618a63e41e020b2ce8071486fe278a849abccad44e5ea444da9700d014` | `source/ai/action_charge.obj` |
| `_magnitude2d` | 32 / 0 | `17d586569ec882a34a898e8a589fe5140e8d8b79b85e2fd4e7816dfe6210d3fa` | `source/ai/action_charge.obj` |
| `_normalize2d` | 80 / 3 | `8a5659d5dd494fe2d1d880057d1836185ebfb8e81d9d63362a43e6a2cb287890` | `source/ai/action_charge.obj` |

Every copy is strict-equal to the January COMDAT of the same name owned by the
listed split object. `_find_tangent_point` has the January REL32 call to
`_normalize2d`; the other five copies are unreferenced emissions from the same
header-defined helper chain. This is link-foldable COMDAT attribution, not an
invented body or an object-completion claim. The six principal path-smoothing
owners are still compared independently, and the unit remains NonMatching.

## Rejected fake/implausible matching techniques

No retained production code uses:

- empty branches, self-comparisons, redundant writes, dead arithmetic, or
  delete/recreate-pointer no-ops;
- `volatile`, register-forcing locals, compiler barriers, custom pragmas, or
  per-function flag changes;
- undefined aliasing, pointer/integer conversion, raw address arithmetic, or
  intentionally uninitialized data;
- inline assembly or copied target instruction streams;
- helper definitions invented only to change COMDAT ownership;
- private names of the form `code_<address>` or globals of the form
  `bss_<address>` in this translation unit;
- manual inlining of `point_from_line3d` or a `point_from_line3d` COMDAT.

The direct cross products remain because they are normal geometry expressions,
match the donor semantics, and avoid emitting a gratuitous helper copy. The
two exact driver locals discussed above are live and meaningful; neither is a
compiler-only no-op.

## Validation

- `ninja build/base/source/ai/path_smoothing.obj`: pass
- `python tools/campaign/gate.py source/ai/path_smoothing --all`:
  3 exact, 3 residual, 0 unwritten
- `python tools/campaign/board.py path_smoothing`:
  1536 / 3056 padded bytes, 3 / 6 strict functions
- hardened per-function COFF comparison: exact sizes/hashes/ordered
  relocations recorded above
- candidate-only math COMDAT comparison against their January owners: all six
  strict exact
- inherited exact-set check: the baseline surface predicate remains exact;
  chooser and driver are newly exact; no inherited exact function was lost
- `ninja halobetacache_build`: pass, no work remaining on final rerun
- `ninja progress`: pass; 470 units scanned, 4,986 functions evaluated,
  4,890 accepted exact, zero semantic unit errors
- `python -m tools.audit_semantic_matches ...`: same 470 / 4,986 / 4,890,
  zero unit errors
- `python -m tools.audit_object_admission ...`: no completion candidate for
  `path_smoothing`; no new contradiction caused by this unit
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 212 / 212 pass
- `git diff --check`: pass

The full compile prints existing warnings in unrelated units. The final
`path_smoothing.obj` compile is clean. There is no runtime path-smoothing test
harness in this reconstruction; semantic validation is therefore source
cross-check plus target control flow/dataflow, strict COFF evidence, the full
configured build, and tooling regressions.

## Reopen criteria

Do not resume broad expression permutations. Reopen a residual only with one
of the following:

1. an authentic January/later source fragment that pins the exact expression
   or declaration topology;
2. PDB/local-variable evidence that pins one of the x87 stack lifetimes;
3. a small, semantically ordinary VC7 reproducer that changes only the
   measured residual while preserving size and all relocation identities.

Until then, the honest result is this 3/6 natural frontier, not a fabricated
6/6 object.
