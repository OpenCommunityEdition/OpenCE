# xbox_sound_cache.obj complete fuzzy closeout (2026-09-01)

## Result

- Unit: `source/cache/xbox_sound_cache`
- Gate: 15 exact, 3 residual, 0 unwritten (18 functions)
- Unit fuzzy similarity: 95.79071%
- New strict closure: `_sound_cache_sound_hardware_unlock`, 48 padded bytes, 3 relocations
- Complete residuals retained for evidence-backed fuzzy parking:
  - `__sound_cache_sound_request`: 352 target / 368 candidate padded bytes, 25 / 27 relocations, 83.92623%
  - `_render_inverse_transform_screen_point`: 256 / 256 padded bytes, 22 / 22 relocations, 99.347824%
  - `_sound_cache_debug_render`: 368 / 368 padded bytes, 14 / 14 relocations, 86.17647%

## Source and naming reconciliation

The missing hardware-unlock leaf was reconstructed from the later HCEA implementation and compiles byte-identically to January. The remaining bodies were checked against January disassembly and relocations, the HCEA Xbox sound-cache family, the HCEA render helper, prior local campaign ledgers, repository history, and the available same-compiler candidates.

Address placeholders were replaced with evidence-backed or semantic names:

- `code_001adc60` -> `sound_cache_locked_block_proc` (HCEA)
- `code_001adca0` -> `sound_cache_delete_block_proc` (HCEA)
- `code_001add60` -> `sound_cache_debug_block_name` (semantic `lruv_debug_block_name_proc` callback)
- `code_001adda0` -> `sound_cache_start_loading_sound` (HCEA)
- `code_001ae1a0` -> `render_inverse_transform_screen_point` (HCEA)
- `bss_004d1088` -> `xbox_sound_cache_globals` (assertion strings and HCEA `pc_sound_cache_globals` analogue)

Public declarations now live in `cache/sound_cache.h`; misplaced consumer-local declarations were removed. The render helper uses the authentic point/vector types and parameter order. Sound-permutation fields use semantic local aliases rather than repeated anonymous field names.

## Residual disposition

`sound_cache_sound_request` is behavior-complete. January homes the result boolean and preserves separate false-return tails; the natural VC7 candidate folds those paths. A historical strict candidate depends on `volatile result`, which is rejected under the house rules.

`render_inverse_transform_screen_point` has the exact padded envelope and relocation sequence. Its normalized stream differs only in the equivalent ECX/EDX assignment of the output pointers after ordinary declaration, type, and parameter-order probes reached the same fixed point.

`sound_cache_debug_render` has the exact padded envelope and relocation sequence. Moving color initialization under the debug predicate naturally recovered January's prefix. The remaining packet is nested-loop register/lifetime scheduling and inherits the private helper's equivalent ECX/EDX allocation. No volatile/register forcing, compiler barrier, pragma, forced inline, assembly, fake dependency, undefined behavior, or nonsensical control flow is retained.

Reopen these residuals only for authoritative January source/local records, an authenticated compiler/QFE difference, or a natural same-compiler donor that explains the outstanding allocation schedules.

## Validation

- `ninja all_source`: all 571 source objects compile
- `tools/campaign/gate.py source/cache/xbox_sound_cache --all`: 15 exact / 3 residual / 0 unwritten
- Consumer gates unchanged after moving declarations to the owner header:
  - `source/cache/predicted_resources`: 2 exact
  - `source/cache/cache_files`: 28 exact
  - `source/sound/sound_manager`: 10 exact / 55 unwritten
  - `source/hs/hs`: 445 exact / 3 residual
- No January `point_from_line3d` schedule or COMDAT behavior was changed.
