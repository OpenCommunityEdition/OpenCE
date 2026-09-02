# `lightning.obj` credible fuzzy completion (2026-09-02)

> Historical baseline ledger. The current retained source and measurements are
> superseded by `lightning_obj_fable_salvage_reconciliation_20260902.md`.

## Result

This lane reconstructed the last two unwritten owners in
`source/objects/widgets/lightning.obj` as one source-backed, same-TU pair:

- private `_lightning_offset_marker_position`;
- public `_lightning_submit`.

Both functions now compile from ordinary semantic C. Neither is strict-exact,
so this change claims **zero strict code bytes** and does not pretend that a
parked residual is a match. It does recover all 2,808 meaningful / 2,816
padded target bytes from unwritten status into measured residuals, and emits
2,720 padded candidate code bytes. The object moves from 7 exact / 0 residual /
2 unwritten to **7 exact / 2 parked residual / 0 unwritten**.

The translation unit's target-owned data also becomes strict-exact: 268 bytes
of read-only data and four bytes of BSS, for a **272-byte strict data gain**.
The whole-tree address-keyed comparison records no exact-function regression.

| owner | target meaningful / padded | target relocs | candidate padded | candidate relocs | objdiff | disposition |
|---|---:|---:|---:|---:|---:|---|
| `_lightning_offset_marker_position` | 237 / 240 | 22 | 240 | 22 | 97.402596% | parked: proven independent instruction scheduling |
| `_lightning_submit` | 2,571 / 2,576 | 80 | 2,480 | 77 | 70.727030% | parked: broad unclassified code-generation residual |

Normalized hashes recorded by `tools/campaign/gate.py` and the parked ledger:

- helper target:
  `2fe97798519acba821968a88f76c5a3ac64a62c86a9919327b6ed89c8599f17d`;
- helper candidate:
  `18303a5b66698933663dbbd26cebba03fb0487eb8154f80ec6a8bfadd6d7c7f3`;
- submit target:
  `fb46c25559e11027286d7f9192b0cbceef54b7e56aece731f83c6d0c298c5377`;
- submit candidate:
  `8c2a94c7c5b19af4e3fae15c90c1c3fcfdde8606db804a924a2ac554604b3ab4`.

## Frozen base and evidence

- isolated detached base:
  `9a56c914fb121f866a853ba4f5c2bbc56f19efc5`;
- baseline gate: 7 exact / 0 residual / 2 unwritten;
- January target authority:
  `build/split/source/objects/widgets/lightning.obj`;
- candidate object:
  `build/base/source/objects/widgets/lightning.obj`.

Before editing, this lane read the current campaign rules and tooling
documentation, including:

- `docs/house_rules.md` from the current campaign documentation tree;
- `docs/matching_methodology.md`;
- `docs/exact_match_acceleration_playbook.md`;
- `docs/shape_transfer_executor_strategy.md`;
- `docs/campaign_throughput_lessons.md`;
- `docs/object_matching_logs/README.md`;
- `tools/campaign/README.md`;
- the complete earlier lightning one-shot and fixed-point ledgers;
- every applicable `CLAUDE.md` and `AGENTS.md` in the authenticated donor
  trees (the current checkout has neither file).

Semantic reconstruction used the authenticated later-build implementations
and their referenced type declarations from:

- `research-cache/halocea-cseries-20260820`;
- `research-cache/stian-halo-full-history-20260828`;
- `research-cache/pastudan-halo-full-history-20260828`.

Those sources supplied names, types, and behavior only. January disassembly,
COFF symbols, relocations, strings, frame shape, and call sites remained the
authority for this build.

## Reconstruction

The private helper is now named for its authenticated semantic owner rather
than an address. It draws a bounded random offset in marker-local space,
transforms it by the marker matrix, and adds it to the result point. It remains
`static` and is reached naturally from its complete same-TU caller, preserving
the target's private register ABI without source-level register forcing.

`lightning_submit` now implements the complete source-backed renderer:

- typed lightning-definition, marker, intermediate-point, and dynamic-vertex
  layouts whose sizes and offsets are proven by the January object;
- marker walking and marker-position resolution;
- recursive midpoint subdivision across the 4,097-point scratch array;
- random jitter, camera-facing cross products, and vector normalization;
- animation value/color modulation and packed vertex color emission;
- dynamic-vertex allocation, bounds construction, shader selection, drawing,
  and cleanup;
- January assertion expressions and line identities, without emitting a
  `point_from_line3d` helper or COMDAT.

Typed tag lookup goes through a subsystem macro. Object marker access uses the
existing typed object access surface. Source uses `real`, named constants,
semantic enums, ordinary declarations, and explicit returns. It adds no asm,
`volatile`, register forcing, pragma, intrinsic, compiler barrier, raw address,
pointer/integer conversion, undefined aliasing, synthetic anchor, or other
code-generation carrier.

The four-byte target BSS owner is emitted by defining
`lightning_globals = {0}`. Leaving the definition tentatively initialized made
VC7 emit a COMMON symbol instead of the January-owned `.bss`; the explicit
zero initializer is normal source and reproduces the target's real ownership.

`real_argb_color_to_pixel32` is defined by `hud_draw.c`, so its declaration is
now in the corresponding narrow owner header, `interface/hud_draw.h`.
`lightning.c` and `render_contrails.c` include that owner header, and the
obsolete use-site declaration in `render_contrails.c` was removed. No
prototype remains in an unrelated consumer or subsystem header.

## Residual classification

The helper has identical padded size and exact relocation semantics. The
remaining mismatch is one adjacent swap after the matrix transform: January
issues the x87 `fadd` before `add esp, 0x18`, while VC7 schedules the independent
stack cleanup first. The two instructions touch disjoint state and have no
dependency. The classifier returns `STOP`, class
`independent-instruction-scheduling`, confidence `PROVEN`. Retaining this
honest compiler tie follows the project's parked-function policy.

The submit body remains 96 padded bytes short and three relocations short. Its
subsystem behavior and large-frame topology are present, but the remaining
differences span broad instruction selection and scheduling. The classifier
returns `UNKNOWN`; it is parked as `unclassified`, not promoted as exact. No
residual tuning was attempted after reaching this credible fuzzy source.

## Validation

The final isolated-tree validation established:

- full `ninja all_source progress semantic_progress` success;
- lightning gate: 7 exact / 2 residual / 0 unwritten;
- Units gate: 189 exact / 0 residual / 0 unwritten;
- address-keyed verdict against a separately built worktree at the exact base:
  0 gained strict functions and **0 regressions** across 8,245 owners;
- fake-match scan: zero findings in every changed C/header file;
- parked audit: 93 active, zero stale, zero invalid;
- admission audit: one unchanged ownership-review candidate, zero contradicted,
  and zero revoked objects;
- tooling tests: 255 passed;
- no emitted `point_from_line3d` definition or COMDAT;
- `git diff --check` clean.

The strict whole-tree code counter is intentionally unchanged. The strict data
counter rises from 1,870,127 to 1,870,399 bytes solely because all 272 target
data bytes in `lightning.obj` now match.

## Reopen guidance

Do not tune either function with asm, volatile/barriers, register declarations,
pragmas, UB, artificial helper calls, or fake control flow. Do not repeat the
known helper source-shape experiments. Reopen the helper only if authenticated
January-era source or a natural caller change explains the final independent
scheduler swap. Reopen submit only with new January-specific provenance that
explains its remaining broad code-generation and relocation gap.
