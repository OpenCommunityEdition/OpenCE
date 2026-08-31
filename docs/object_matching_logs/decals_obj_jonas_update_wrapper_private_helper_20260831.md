# `decals.obj`: exact update wrapper and private helper boundary (2026-08-31)

## Result

The public `decals_update` iterator wrapper is admitted strict exact. Its
private per-datum owner is retained as coherent typed C under the authenticated
name `decal_update` and is explicitly fuzzy-parked.

The alias-aware direct gate moves `source/effects/decals` from 6 exact / 0
residual / 27 unwritten functions to 7 exact / 1 residual / 25 unwritten
functions. All six inherited exact functions remain exact.

| Function | January padded size / relocations / normalized SHA-256 | Rebuilt padded size / relocations / normalized SHA-256 | Result |
| --- | --- | --- | --- |
| `_decals_update` | 80 / 5 / `5cab5e28f44da9d43d9a5ad4b4a8e297370aef4ba716892d7b2a9daeea77d5df` | 80 / 5 / same | strict exact after the configured private-symbol rename |
| `_decal_update` | 400 / 25 / `dcc6a629481fabd93292d708b860675421698d6ef2dc9e0679b57331d784d35f` | 384 / 26 / `1ab3a89f52527a51460b6a1afa552e825fe26353faf14970500d56510166cf27` | fuzzy parked, 94.21311% objdiff |

January's meaningful function lengths are 77 and 387 bytes respectively; the
table reports the padded COMDAT envelopes used by the strict comparator.

## Name, linkage, and source provenance

The HCEA/PDB correspondence atlas records
`static void decal_update(long)` in the `effects/decals` compiland. The later
HCEA `decal_update.c` reconstruction independently corroborates the datum
layout and the permanent, lifetime, fade, locked-count, and deletion flow.
January's own assertions authenticate the critical source expressions
`decal->definition_index!=NONE` and `f>=0.0f && f<=1.0f`.

January's split object originally exposed the address placeholder
`_code_00087d20` because no private symbol ownership was configured. The
symbol atlas now names that address `_decal_update` and marks it static. The
one-shot invalid-lock-count guard is named `_decal_locked_count_reported` at
the exact BSS address referenced twice by January, and the existing
`_decal_globals` owner is likewise marked static.

The public wrapper cannot be admitted alone. VC7 reports C2129 for an
undefined static helper prototype. With the natural helper definition visible
in the same translation unit, the compiler uses EDI as the private live-in for
the datum index and emits the exact 80-byte wrapper without an artificial
calling-convention declaration. Published Marathon source provides the same
topology in `marathon2/media.c`: a public update loop calls a defined private
per-element update helper.

## Honest fuzzy boundary

The natural helper uses repository types and macros: `real`, `DECAL_GET`,
`TEST_FLAG`, and `SET_FLAG`. It contains no address-derived name, manual bit
mask, compiler barrier, volatile carrier, dead branch, or forced inline/noinline
attribute.

The remaining mismatch is not closed with a code-generation trick. January
uses a 32-bit inline FISTP for the final alpha conversion and has nearby
register/x87 scheduling differences. Ordinary C emits `__ftol2`; `/QIfist`
emits a qword FISTP and therefore does not reproduce the target. The helper is
parked until authoritative January source or a natural same-compiler spelling
explains both the conversion and schedule.

## Header and ownership boundary

Only the public `decals_update(void)` prototype is added to
`effects/decals.h`. Private structures, flags, and `decal_update` stay in
`decals.c`. The header has two C includers; a direct gate of
`source/structures/structure_runtime_decals` remains 7 exact / 0 residual / 0
unwritten.

The partial rebuilt TU places `_decal_locked_count_reported` at local BSS +0
and `_decal_globals` at +4, whereas January's complete compiland has many
still-unwritten owners between `_decal_globals` and the guard. No decals data
owner is currently credited by the semantic ledger. Regeneration must still
validate final split ownership and the full regression sweep before commit.

## Verification performed

- Direct VC7 gate compile of the tracked `decals.c` source: pass.
- Alias-aware whole-unit comparison: 7 exact / 1 residual / 25 unwritten.
- `decals_update`: exact normalized bytes and five exact relocation sites;
  only the pre-regeneration private-symbol spelling differs.
- Inherited six-function exact set: unchanged.
- `structure_runtime_decals` header-includer gate: 7 / 0 / 0.
- `tools/fake_match_scan.py source/effects/decals.c`: zero findings.
- `git diff --check`: no whitespace errors.

No configure, csplit regeneration, full Ninja build, staging, or commit was
performed in this packet.
