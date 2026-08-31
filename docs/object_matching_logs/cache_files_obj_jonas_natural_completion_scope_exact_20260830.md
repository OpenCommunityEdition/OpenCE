# Cache Files natural completion-scope exact cleanup (Jonas, 2026-08-30)

## Result

`scenario_structure_bsp_load` remains strict after removing three
inadmissible reconstruction devices:

- a volatile reinterpretation of the `reference` parameter;
- a duplicate pointer local whose purpose was to force a parameter-home
  reload; and
- manual use of byte 3 of the pointer parameter's stack home as the
  asynchronous completion flag.

The retained source uses `reference` directly and declares a real
`boolean read_complete` inside the block containing `cache_file_read` and
its wait loop. That lifetime is meaningful, safe ordinary C. VC7 naturally
reuses the dead parameter-home byte for the narrow local and emits January's
object without source-level aliasing tricks.

This cleanup grants no new function credit. The object remains 28/28 strict.

## Name cleanup

The private lookup helper formerly called `code_001a95d0` is now
`cache_get_tag_instance` in source and in the csplit symbol record at file
offset 1,742,288. The address placeholder was not an acceptable source name.
The replacement describes the helper's checked tag-table lookup, follows the
module's naming convention, and has independent community-source precedent in
commit `eb93ce635e`. It is convention-backed rather than claimed as a
January-PDB-authenticated spelling.

The renamed helper remains exact at 240 padded bytes, 24 relocations, and
normalized SHA-256
`84ab0f23cdc299ec7ddbc8d1c9dd119f7f4cedb1d749696f4d1d3efe06c783a1`.
All callers compare against the renamed target identity.

## Experiment matrix

| ID | Source shape | Size / relocs | Normalized SHA-256 | Decision |
| --- | --- | ---: | --- | --- |
| E00 | Volatile pointer-home reload plus `(byte *)&reference + 3` completion storage | 288 / 25 | `3494a8100bcaf113a37d9590a2b5e8ee63a08b1c862ef7ad82489819eb5db3a7` | exact but rejected as fake/unsafe source |
| E01 | Ordinary broad-scope local initialized to `FALSE` | 288 / 25 | `f70226f983847da363941689a97afe83c0928b7094ad03a4740029d2e0fc42de` | safe fuzzy; first four relocations shifted by five bytes |
| E02 | Ordinary broad-scope uninitialized local, matching the sibling `scenario_tags_load` contract | 288 / 25 | `0b46fcd23edefcf48dabc0801447256bab489c5c5ca07d0f9e2cf93f90334aea` | safe fuzzy; first four relocations shifted by one byte |
| E03 | Uninitialized local scoped only to the read/wait block; direct use of `reference` | 288 / 25 | `3494a8100bcaf113a37d9590a2b5e8ee63a08b1c862ef7ad82489819eb5db3a7` | retained natural exact |

The uninitialized local is semantically valid under the existing
`cache_file_read` completion-flag contract; the adjacent
`scenario_tags_load` function uses the same pattern. No manual stack
address, volatile access, alias violation, pragma, assembly, or compiler flag
is retained.

## Strict evidence

- `_scenario_structure_bsp_load`: 288 padded bytes, 25 relocations,
  normalized SHA-256
  `3494a8100bcaf113a37d9590a2b5e8ee63a08b1c862ef7ad82489819eb5db3a7`.
- Post-rename generated target:
  `build/split/source/cache/cache_files.obj`, 17,585 bytes, raw SHA-256
  `9273a26667630ed5de5ecc67763f22fc1ce162bd6a894ae8198ee75313fb1ce9`.
- Rebuilt candidate: 18,430 bytes, raw SHA-256
  `7987e0ccdebe3b5cbedce78008ebd2897cb6fe96f9c70c7477b77b4729ccaa4f`.
- Direct strict census: 28 exact function owners, zero residuals and zero
  absent target owners.

The target object's raw hash changes when csplit emits the longer private
symbol name; the immutable machine-code authority remains `cachebeta.exe`.
The helper and every relocation to it are compared after the same reviewed
identity rename.

## Verification and do-not-repeat

- XDK 3911 rebuilt the base object and csplit regenerated the target object.
- `tools/coff_compare.py` reports both
  `_cache_get_tag_instance` and `_scenario_structure_bsp_load` equal.
- The whole-object exact census reports 28 strict owners.
- The conservative fake-match scanner reports zero leads in
  `source/cache/cache_files.c`.
- The full Python suite passes: 221 tests.
- `git diff --check` passes.

Do not restore the volatile pointer cast, a manual byte inside a parameter
home, or the address-derived private name. If future work changes the local
lifetime, it must preserve both the asynchronous completion semantics and the
strict whole-object gate.
