# Lane reconciliation 10K batch 2 (2026-09-09)

## Published-base result

The frozen published base is
`6a9bc341e3fd175bba582195dc7860fd1a710101`, with 6,686 accepted exact
functions and 1,058,829 meaningful exact code bytes. The rebuilt semantic
ledger for this batch contains 6,721 accepted exact functions and 1,069,562
meaningful exact code bytes:

- **35 net accepted exact functions**;
- **10,733 net meaningful exact code bytes**;
- **zero lost accepted exact functions**;
- **zero unit errors**.

These totals are a direct accepted-ledger comparison, not an addition of donor
headline claims.

| reconciled packet | accepted functions | meaningful exact bytes |
| --- | ---: | ---: |
| `ai.obj` lifecycle and event graph | 22 | 4,973 |
| `render_debug.obj` string-at-point path | 1 | 296 |
| `render_cameras.obj` projection pair | 2 | 552 |
| `actor_type_infection.obj` ballistic jump | 1 | 686 |
| `unit_dialogue.obj` speech arbitration | 3 | 1,396 |
| `model_animations.obj` coherent animation graph | 6 | 2,830 |
| **published semantic-ledger gain** | **35** | **10,733** |

## House-rule reconciliation

The final source uses semantic private/global names, project scalar and
geometry types, named enums and flag macros, typed tag/object accessors,
one-parameter-per-line declarations, and explicit terminal returns. Public
prototypes live in their subsystem owners or established narrow owner headers.
No new inline assembly, forced inline, volatile/register steering, raw byte
emission, address-derived source name, synthetic dependency, or nonsensical
exactness carrier is admitted.

The full canonical sweep caught and removed two superficially exact but
inadmissible paths:

- `animation_graph_node_matrices_from_orientations` was mechanically exact for
  337 meaningful bytes only with a duplicate TU-private animation-node layout
  already rejected by the canonical owner audit. The body remains unwritten
  and receives zero credit.
- Promoting `hs_model_animation_compression_enabled` into a broad shared header
  reordered the already-exact 1,919-meaningful-byte
  `unit_preprocess_node_orientations`. Restoring the existing local declaration
  returns Units to 189/189 and the global ledger to zero losses.

Three natural compressed-keyframe helpers are retained as the best reviewed
fuzzy frontier. Their authenticated semantic names, signed frame-index
contracts, target/candidate measurements, and zero-credit status are recorded
in `config/parked.json` and the Model Animations object ledger. No donor total
includes them. The existing AI communication speech-timer park was refreshed
to its previously proved 97.67839-percent compiler schedule after the final
header context selected that form; its source and exact credit remain
unchanged.

## Inline and whole-object boundaries

Every changed translation unit except Units passes the explicit forbidden
`_point_from_line3d` emitted-symbol guard. Units remains 189/189 target
functions, but its inherited candidate-only `_point_from_line3d` COMDAT is
unchanged; this batch neither credits that helper nor claims fresh whole-object
admission for Units. The object-admission audit reports no new candidate or
contradiction, with the six established explicit rejections still visible.

## Publication gate

- `ninja all_source`: passed.
- Hardened semantic audit: 473 units, 7,209 functions evaluated, 6,721
  accepted exact, zero unit errors.
- Frozen published-ledger comparison: 35 gains, zero losses, +10,733
  meaningful exact bytes.
- Focused hardened gates for all nine changed implementation units: passed;
  the eight eligible forbidden-helper guards passed, and Units is 189/189.
- Park validation: 350 active, zero stale, zero invalid.
- Object-admission audit: zero candidates, zero contradictions, six inherited
  explicit rejections.
- Tool suite: 1,144 passed, 5 environment-dependent skipped, and 26 subtests
  passed.
- Changed-source fake-match scan: 22 files, zero review leads.
- `git diff --check`: passed.

The canonical treemap is refreshed at this 10K boundary. Its next required
refresh threshold is **1,079,562 meaningful accepted exact bytes**.
