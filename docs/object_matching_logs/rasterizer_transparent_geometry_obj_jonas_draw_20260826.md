# `rasterizer_transparent_geometry.obj` draw recovery

## Scope and provenance

This checkpoint reconstructs
`_rasterizer_transparent_geometry_draw` without changing the unit's partial
status. The January split object remains the byte, padding, relocation, and
ownership authority.

The evidence wave incorporated all object-specific local records available at
the start of the attempt:

- `docs/object_matching_logs/rasterizer_transparent_geometry_codex_checkpoint.md`;
- `docs/object_matching_logs/rasterizer_transparent_geometry_hcea_checkpoint.md`;
- `docs/object_matching_logs/rasterizer_transparent_geometry_obj_jonas_sort_driver_lifetime_20260825.md`;
- Claude research files `research/transparent_geometry_recovery.md` and
  `research/transparent_geometry_sort_comparator.md` at
  `74a085e90ae66511c22820a8f5256b3273342542`.

The public semantic donor was
`surreptitiousresearch/halocea@570c83`, especially
`src/blam/rasterizer/rasterizer_transparent_geometry_draw.c` and its group
draw/header, initialization, and sort collaborators. HCEA established the
high-level water/non-water traversal topology. It was not treated as a PC
code-generation oracle. January disassembly supplied the exact profile IDs,
debug-flag transitions, first-person assertions, frustum changes, loop order,
and relocation identities.

No shared header, configuration entry, parked-function record, frozen object,
or protected Units source changed in this wave.

## Frozen evidence and candidate

The clean baseline was commit
`142ce8cce45251f5ad6d87f780305e56202afb96`; its manifest is
`build/audit/rasterizer_transparent_draw_baseline_20260826.json`. The baseline
source SHA-256 was
`874d31a32c82f5ac75330c668444e44cd4a12fd5fe12f84305a7e4badb6bfd5a`.

The recovered source follows the target's observable sequence:

1. select and begin the water or queued-transparent profile;
2. initialize water sorting and the group cursor when required;
3. begin the group batch and clear the debug draw flag;
4. walk sorted groups, applying the water shader/decal filter;
5. switch to the first-person frustum only after the target assertions;
6. draw and advance each group;
7. bracket group finalization with the window-dependent debug flag;
8. restore the ordinary frustum and end the profile.

The one frozen code-producing candidate compile emitted object SHA-256
`b7b77de443e8c90dc9828d7ccfd1f83f5378ca7370ad0869d1a315bec69006e7`.
The retained source SHA-256 is
`45e9cc4aa0431cf95e15ac47d1a1688c1e0be2b580d67be46b32b529067747dd`.

## Strict result

The new function matched on the first compile:

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_transparent_geometry_draw` | 466 | 480 | 40 | `91bb8c7f95d843b19f8636ac8035fced6413bf58b8ef649fab964e7432c613db` |

Direct hardened comparison of all sixteen accepted functions reports
`all_equal: true`. This independently proves that both pending-status helpers
and all fourteen other inherited matches retained their padded bytes and
ordered relocation semantics.

The unit advances from 15/19 to 16/19 exact functions:

- meaningful exact code: 1,229 -> 1,695 bytes;
- padded exact code: 1,328 -> 1,808 bytes;
- exact relocations: 78 -> 118;
- ordinary unit code match: 77.97% meaningful and 84.21% by function count.

Three real emitted residuals remain: `initialize`, `next_group`, and the sort
driver `_code_00174120`. The object therefore remains partial and receives no
whole-object completion label.

## Runtime data and ownership

Two new January-owned assertion-string COMDATs are independently exact:

| Literal | Logical bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `!water` | 7 | 0 | `fb5f43b5e19f47b41739d2de22dc56deb5f40e5595ed96b5d8f3f86cd5ca242a` |
| `!first_person_flag` | 19 | 0 | `e5a09728c020e8c17b8e5f63b857859e54c8ca28f6b178c66c22b7790a6b7060` |

Target-owned exact RDATA therefore rises from 6/11 and 481 logical bytes to
8/11 and 507 logical bytes. The long
`global_window_parameters.rasterizer_target==_rasterizer_target_render_primary`
literal is naturally emitted by VC7, but January represents that same-named
relocation as an undefined cross-TU symbol. It receives no local data credit.
The existing candidate-only `"group"` assertion literal has the same boundary.

The 74-byte `_bss_004b8ad8` layout and every code addend into it remain fixed.
January owns a defined BSS section while the candidate uses the previously
reviewed COMMON form, so this checkpoint makes no new whole-BSS ownership
claim. Aggregate matched data is intentionally unchanged.

## Regression-gate interpretation

The pre-change whole-TU manifest correctly reports the draw as `NEWLY_EXACT`
and no item in `changed_nonexact`. It also fails closed on two kinds of expected
structural evidence change:

- the pending-status helpers' relocations now resolve to the locally defined
  `_bss_004b8ad8` COMMON owner rather than the baseline's symbolic-only view;
- the three newly emitted assertion COMDATs and draw dependencies change the
  symbol/section inventory.

Those findings are not waived or adjudicated. Direct hardened comparison proves
both pending helpers strict exact (`64/64`, two relocations, hash `7f6405d3...`;
`96/96`, four relocations, hash `6969b1dc...`), and proves the full accepted
sixteen-function set together. The ownership audit above separately classifies
each new runtime literal.

## Policy and gates

- C89 declarations are block-initial and all added functions/prototypes use the
  campaign's one-parameter-per-line and explicit-return formatting;
- no asm, `register`, `volatile`, barrier, optimizer pragma, intrinsic,
  attribute, raw address, pointer/integer reconstruction, representation pun,
  byte forcing, object patch, or synthetic emission anchor was introduced;
- the file's inherited packing pragmas predate this diff and are unchanged;
- forced XDK object compile and the full Halo/libcmt graphs: pass;
- semantic report: 470 units, 4,333 evaluated functions, 4,191 semantic exact,
  4,252 accepted exact, zero unit errors;
- campaign progress: 510,604/2,198,102 meaningful code bytes and
  4,226/11,060 functions;
- Halo progress: 497,690/1,770,166 meaningful code bytes and
  4,059/7,574 functions;
- object admission: zero candidates, zero revocations; only the inherited
  `shell_xbox` contradiction remains;
- parked-function audit: 3 active, 0 stale, 0 invalid;
- tooling tests: 179 passed;
- `git diff --check`: pass.
