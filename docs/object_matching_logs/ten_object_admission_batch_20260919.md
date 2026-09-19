# Ten-object whole-object admission batch (2026-09-19)

## Result

This batch promotes exactly ten January translation units from `NonMatching`
to `Matching` after function, data, COFF ownership, and source-quality review:

| Unit | Functions | Meaningful code | Data |
| --- | ---: | ---: | ---: |
| `source/interface/hud_sounds` | 1 | 349 | 38 |
| `source/game/game_statistics` | 4 | 802 | 1 |
| `source/bitmaps/bitmaps_quantitize` | 3 | 1,065 | 164 |
| `source/bitmaps/tiff_file` | 3 | 2,495 | 924 |
| `source/memory/circular_queue` | 8 | 761 | 264 |
| `source/memory/lra_cache` | 14 | 1,392 | 160 |
| `source/memory/lrar_cache` | 12 | 2,074 | 528 |
| `source/rasterizer/xbox/shader_transparent_chicago_preprocessor` | 2 | 421 | 328 |
| `source/rasterizer/rasterizer_cinematics` | 16 | 1,454 | 332 |
| `source/rasterizer/rasterizer_swizzle` | 13 | 4,043 | 476 |
| **Total** | **76** | **14,856** | **3,215** |

These bytes and functions were already present in the accepted exact ledger.
This is an accounting and ownership hardening batch, so it claims **zero new
exact code bytes**.  The object count moves from 395/833 to 405/833 without
inflating the 1,442,095-byte accepted-code total.

## Admission standard

For every admitted unit, all January function bodies compare exact under the
hardened comparator, including ordered relocation address, type, addend, and
semantic destination.  Every target-owned runtime data section is present in
the candidate with matching bytes or logical zero-BSS contents, section flags,
alignment, COMDAT selection, symbol offset/type/storage, and relocation
identity.  No unit owns COMMON data, an unexplained target-only runtime
section, or a candidate-only behavioral code owner.

Compiler support records (`.drectve`, CodeView sections, `@comp.id`, and
`__fltused`) are not runtime owners.  Candidate-materialized `SELECT_ANY`
pooled literals are accepted only where the January split leaves the same
symbol undefined, an exact selected provider exists in the January corpus,
and ordinary duplicate-owner diagnostics are clean.  No such literal receives
additional target-byte credit.

The one-byte `game_statistics` BSS and the private rasterizer BSS owners use
the same logical zero contents, sizes, flags, symbol offsets, and storage as
January.  The split tool's materialized raw zeros versus MSVC's true BSS
encoding is the established representation-only difference, already present
in 69 admitted control objects.

## Source-ownership corrections

- The three quantization channel tables now have declarations in the narrow
  `bitmaps_quantitize_internal.h` owner interface.  The consumer-local externs
  were removed from `bitmap_extract.c`.  A declaration in the broad
  `bitmaps.h` was rejected because it perturbed two unrelated exact functions;
  the narrow header preserves the January compilation schedule with zero
  inherited regressions.
- `tiff_file.h` now owns the only cross-translation-unit TIFF API,
  `tiff_export`.  The caller-local declaration in `main.c` was removed, and
  `error_message_buffer` now has January's private/static ownership.  The
  other two TIFF functions have no external consumers and therefore do not
  gain gratuitous public declarations.
- The LRAR private definitions now follow the source order proved by January's
  assertion line anchors: getter, block verifier, cache verifier.  Atlas names
  replace the inferred spellings: `get_lrar_cache_block`,
  `verify_lrar_cache_block`, and `verify_lrar_cache`.  Target metadata and C
  identifiers were renamed together; all 12 functions and all 10 runtime data
  sections remain exact.
- `rasterizer_screen_effects_time` and `compute_swizzle_masks` are recorded as
  private/static in symbol metadata, matching their source and authenticated
  ledgers.  `rasterizer_cinematics.c` now obtains
  `main_get_window_count` from `main/main_internal.h` instead of a local
  declaration.  The complete four-field `rasterizer_global_defaults` type and
  its extern declaration now live in the narrow
  `rasterizer_globals_internal.h` owner interface; the incompatible one-field
  cinematics prefix facade was removed.  The owner and all 131 functions in
  `rasterizer.obj` remain exact.
- The Chicago shader preprocessor now consumes the genuine complete
  `pixel_shader_definition` from `rasterizer_xbox_pixel_shader.h`; its duplicate
  translation-unit-local structure definition was removed without changing
  either exact function.

All edited source passes the fake-match scanner.  No address-derived private
name, BSS-address global, volatile/barrier steering, pragma steering, new
inline assembly, or `_point_from_line3d` owner was introduced.

## Fail-closed candidates

The following zero-gap or near-zero-gap objects were reviewed and deliberately
not promoted:

- `lruv_cache`: its exact assertion text currently depends on a local
  `#define index datum_index` facade.  January and supplied Bungie source
  support the field name `index`, but the genuine shared `data_iterator`
  owner has not yet been renamed and broadly re-gated.
- `key_agreement`: `_key_agreement_packets` has the wrong candidate section
  alignment, and stock SDK ownership still requires resolution.
- `action_uncover`, `actor_stimulus`, `action_flee`, and `action_guard`:
  candidate-only math COMDAT owners remain unresolved; the latter two also
  retain source/linkage ownership debt.
- `input_abstraction` and `devices`: ordinary links reject candidate-only math
  or SDK owners with `LNK2005`; both also retain public declaration debt.
- `debug_memory`, `player_queues_new`, and `rasterizer_xbox_widgets`: surplus
  SDK/helper owners or selection mismatches remain.

Exact function progress in those units remains valid independently.  It is not
converted into whole-object credit.

## Verification

- Full source build: pass.
- Stable whole-tree comparison against
  `scratch/ten-objects-before-20260919.json`: **0 gained, 0 regressed**.
- Whole-object admission audit: no contradicted `Matching` label.
- Fake-match scan of all changed/admitted source: 0 findings.
- `_point_from_line3d` emitted-symbol guard: pass for every admitted unit.
- Park audit: 190 active, 0 stale, 0 invalid.
- Tests: 1,152 passed, 5 skipped, plus 26 subtests passed.
- `git diff --check`: pass (checkout line-ending notices only).
- Final progress: **405/833 Matching objects**, 7,530 accepted exact
  functions, 1,442,095 accepted meaningful code bytes.
