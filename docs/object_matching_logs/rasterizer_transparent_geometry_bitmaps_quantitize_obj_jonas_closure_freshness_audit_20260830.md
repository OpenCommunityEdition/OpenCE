# `rasterizer_transparent_geometry.obj` / `bitmaps_quantitize.obj` closure freshness audit

## Scope and decision

This lane began from canonical commit
`7d9b07a4f2e126d48646eafd4e96a77ac582d2ad` and evaluated, in order:

1. `source/rasterizer/rasterizer_transparent_geometry.obj`, currently 17/19
   functions strict exact; and
2. `source/bitmaps/bitmaps_quantitize.obj`, currently 2/3 functions strict
   exact with all owned data exact.

Neither object has fresh evidence that can close the whole translation unit.
No production spelling was compiled or admitted. This is deliberate: both
remaining transparent-geometry families and the quantitize scheduler family
have durable measured fixed points, and the campaign rules permit at most one
fresh evidence-backed production spelling. Retrying a frozen spelling would
not satisfy that threshold.

Production source and configuration are unchanged. Both objects remain
`NonMatching`.

## Primary object: authoritative residuals

Translation unit:
`source/rasterizer/rasterizer_transparent_geometry.c`

Current source identity:

- Git blob: `36a40829b046943aa65ce3b9993d68a964bfe21c`
- raw SHA-256:
  `5d5086cc57463a440a32c248c2de64058e2d970c2d7528afd06a110add869489`

The 17 inherited exact function owners and all inherited section/data/
relocation ownership remain protected. The only residuals are:

| Function | Target/base padded bytes | Target/base relocs | Target normalized SHA-256 | Current state |
| --- | ---: | ---: | --- | --- |
| `_rasterizer_transparent_geometry_initialize` | 160/unwritten | 16/unwritten | `63251d0d8cc730b0de774a4c06b489f06bd0af5849fa8281705d46e40473e678` | source absent |
| `_code_00174120` | 176/176 | 14/14 | `4f78b8476535a901325781b67d7ed04665e59478fb419ddd0c607c95220655d5` | readable nonexact sort driver; base hash `30cd227ef42506ffe5a4050491eff57faef6a200cbffc05301077bca69f40a50` |

### Initializer fixed point

Claude's most complete legal-C initializer is semantic, 160 bytes, and has all
16 relocations. Its normalized hash is
`15f6dc8cbb204d551c2c25b60baf4e21ca760125fc43678f40cb24a48fcbdbe4`.
It retains one basic-block placement residual. The four documented
allocation-result lifetime/error-path source shapes all remain at normalized
distance 21, except one regression at 26. The Stian donor's local `success`
shape compiled to the same fixed point and was removed.

Pastudan history adds readable pointer locals `a`, `b`, and `c` in commit
`da103816...`, after an exact but prohibited naked-assembly transcription in
`2d44115c...`. The readable form confirms allocation and early-error topology,
but is neither original January source nor a demonstrated exact spelling. It
does not address the independently blocked sort driver, so it cannot justify a
whole-object closure trial.

### Sort-driver fixed point

The first evidence-backed lifetime spelling improved 69 normalized positions
to 58 and aligned the tail, but left January's first-pass ECX/EDX lifetime
where the compiler selected EAX/ECX and loaded the `qsort` base one byte early.
The combined direct-expression, block-local-pointer, and pre-test-index family
is frozen. A later Pastudan loop-local group-pointer clue plus its measured
lifetime correction reproduced the same residual class and was removed.

Reopen this function only for original January local/statement provenance, an
independent strict-exact legal-C donor built by CL 13.00.9254.1 with the same
flags, or a genuinely new lifetime control not represented by those ledgers.

## Fallback object: authoritative residual

Translation unit: `source/bitmaps/bitmaps_quantitize.c`

Current source identity:

- Git blob: `298590e6c61ddc5c9a6bf8a94d0d2c49e787395b`
- raw SHA-256:
  `1ddbe04004450954182c6161eddf87558122b180969a47acfbe60de2f646a1b1`

`_bitmap_quantitize` (592 bytes, 28 relocations) and `_code_0006d5d0`
(32 bytes, zero relocations) are strict exact. The sole residual is January
`_code_0006d5f0`, identified cross-build as `row_dither`:

- target/base: 464/464 padded bytes, zero/zero relocations;
- target normalized SHA-256:
  `a5feef0c98bc8565991a9cf9722e0258d2bd556e23fc0326891aa0ce0ebca5db`;
- restored baseline hash:
  `7afe02e297bf06dd682a9c44bfd00aacf39c2f7ea0c0f45acce838dc9b696da4`;
- sole residual:
  target `inc eax; add ebx,2; add ecx,2`, versus baseline
  `add ebx,2; inc eax; add ecx,2`.

Reversing the two obvious source statements emits
`inc eax; add ecx,2; add ebx,2` and hash
`a5d1033d681cec568e19033f09f52ef415f989e690717817a578423a4f9b52d5`.
The formerly unexplained 24-artifact hash
`97e1ee6130804456124067f2595a879a6f80f9007e8ec74f6249cf64406828f`
was traced to the old experiment that reverses statement order in both the
quantization loop and dither loop (for example,
`C:\halo-worktrees\opus-regression-gate`). It is not a new candidate.

Original-source search, January/HCEA locals, declaration order, both direct
statement orders, loop-tail shapes, barriers, pragmas, pointer topology,
artificial dependencies, and prohibited diagnostic assembly experiments are
already documented and exhausted. Reopen only for original Bungie C/local
debug records, a same-compiler strict-exact C donor with the same dependency
context, or a newly demonstrated legal-C scheduler control.

## Exhaustive local evidence census

The freshness audit enumerated all 1,571 registered Git worktrees and compared
target source identities, committed history, scratch/research artifacts, and
candidate objects.

- Twelve transparent-geometry source hashes reduce to documented historical
  stages, the canonical source, Claude's measured initializer, and the frozen
  sort-driver variants. There is no unintegrated source family.
- Ten quantitize source hashes reduce to the canonical source and documented
  scheduler experiments. There is no unintegrated source family.
- 4,643 target-named objects were found under registered `build`, `scratch`,
  `research`, and `artifacts` trees; the scan completed with zero parse errors.
- Transparent initializer: 1,175 sections total = 1,174 raw targets plus the
  one Claude candidate at `15f6dc8c...`; no candidate-origin exact section.
- Transparent sort driver: 1,414 sections total = 1,174 raw targets plus 240
  copies of baseline `30cd227e...`; no candidate-origin exact section.
- Quantitize row dither: 1,868 sections total = 1,173 raw targets, 669 baseline
  `7afe02e2...`, 24 classified two-loop-order experiments `97e1ee61...`, and
  two reversed-tail experiments `a5d1033d...`; no candidate-origin exact
  section.

Fable's acceleration, campaign, cross-build, transfer, and exact-finish trees
contain inventory/ranking/correspondence data and copies of these already
classified sources. They contribute no original-C or debug-local provenance.
Authenticated HCEA history provides the later allocation and sort topology but
not January lifetimes. Stian and Pastudan history provide the already-measured
initializer and sort clues above. Searches of the local punpckhdq, demon,
nimbus, Baboon, and halopc-restored donors found no additional implementation.

## Claude-ready reopen packet

Work from a fresh branch at or after canonical
`7d9b07a4f2e126d48646eafd4e96a77ac582d2ad`. Read these files before touching
production source:

- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- `docs/object_matching_logs/rasterizer_transparent_geometry_codex_checkpoint.md`
- `docs/object_matching_logs/rasterizer_transparent_geometry_hcea_checkpoint.md`
- `docs/object_matching_logs/rasterizer_transparent_geometry_obj_jonas_sort_driver_lifetime_20260825.md`
- `docs/object_matching_logs/rasterizer_transparent_geometry_obj_jonas_draw_20260826.md`
- `docs/object_matching_logs/rasterizer_transparent_geometry_obj_jonas_next_group_recovery_20260827.md`
- `docs/object_matching_logs/rasterizer_transparent_geometry_obj_jonas_result_lifetime_20260828.md`
- `docs/object_matching_logs/bitmaps_quantitize_obj.md`
- this freshness audit
- `C:\tmp\bitmaps-quantitize-agent\ROW_DITHER_PROVENANCE_REPORT.md`

For transparent geometry, first obtain genuinely fresh evidence that can solve
`_code_00174120`; an initializer-only spelling cannot close the object. For
quantitize, first obtain original local/statement provenance or a legal-C
scheduler control that orders the independent EAX, EBX, and ECX advances as
January does. Do not repeat any frozen family, naked/inline assembly, `_emit`,
volatile/register forcing, pragmas, intrinsics, raw addresses or offsets,
type-punning, undefined behavior, barriers, object-byte patches, or compiler-
flag changes. Use natural typed House C89 only. Preserve every inherited exact
function, section, data owner, relocation, and whole-TU ownership. Admit at
most one fresh evidence-backed spelling, and fail closed on any regression.

## Validation

Although no production compile was authorized, a fresh configured full-tree
build and the complete cumulative admission suite passed:

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass;
- semantic audit: 470 units, 4,957 functions evaluated, 4,840 semantic exact,
  4,850 accepted exact, and zero unit errors;
- campaign progress: 384/833 objects, 4,811/11,060 functions,
  610,387/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- strict target board: transparent geometry 1,968/2,304 bytes and 17/19
  functions; quantitize 624/1,088 bytes and 2/3 functions;
- object-admission audit: zero candidates, contradictions, and revocations;
- parked-function audit: 12 active, zero stale, and zero invalid;
- tooling suite: 212/212 tests pass;
- protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `git diff --check`: pass.

The only tracked change is this ledger. No production source, object-status
record, configuration, semantic exception, parked entry, or campaign tool
changed.

## Disposition

No closure claim is made. The primary remains 17/19 strict exact and the
fallback remains 2/3 strict exact. Both are durably parked until their reopen
criteria are met.
