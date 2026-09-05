# HUD Unit renderer conversion-precision follow-up, 2026-09-04

## Disposition

The shield and auxiliary meter conversions in the frozen
`hud_render_unit_interface` reconstruction are semantically compatible with
January. The extended-product versus binary32-spill defect found in a separate
HUD Draw scratch candidate does **not** occur here. No precision-based
revocation of the HUD Unit initializer credit or renderer fuzzy reconstruction
is warranted.

This was a read-only disassembly and type-domain review. It changed no C source,
header, config, target, candidate object, build output, or admission record.

## Frozen inputs

- January target: `build/split/source/interface/hud_unit.obj`.
- Final source SHA-256:
  `cc98970ee2318245d01706a1fd6941b4207aab62a3ea1e2293879a8738d260eb`.
- Final diagnostic candidate:
  `scratch/hud-unit-renderer-frozen-final-20260904.obj`, SHA-256
  `35c94a866e13c7154f30cf23cf8701355fff7b6a6343f10f98bc54f9e570bfb2`.
- Renderer normalized candidate SHA-256:
  `4071a4f0c7b1f808423bd5673e822e7f3c6a036e9a3a2db0fe7bd14b413b1817`.

The candidate was gated with the exact forbidden spelling
`--forbid-emitted-symbol _point_from_line3d`; the guard passed.

## Complete `fistp` census

January has exactly twelve `fistp` instructions in the renderer:

- shield current/reference PIN evaluations: `+0x567`, `+0x584`, `+0x5A2`;
- shield maximum/current PIN evaluations: `+0x5B7`, `+0x5CE`, `+0x5E6`;
- first auxiliary PIN argument: `+0xB7F`, `+0xBA3`, `+0xBC2`;
- second auxiliary PIN argument: `+0xBDA`, `+0xBF8`, `+0xC17`.

The final candidate likewise has exactly twelve:

- shield groups at `+0x581`, `+0x59E`, `+0x5B9` and `+0x5D4`, `+0x5EB`,
  `+0x606`;
- auxiliary groups at `+0xB83`, `+0xBA0`, `+0xBBD` and `+0xBCC`, `+0xBE3`,
  `+0xBFB`.

Every January conversion reloads a four-byte stack value that was populated by
`fst` or `fstp` after the multiplication. The shield path stores the product to
two `real` locals before the first conversion and reloads a dword for all three
PIN evaluations. The auxiliary path recomputes the multiplication for each PIN
evaluation, but each result is still stored with `fstp dword` and reloaded with
`fld dword` before `fistp`. The final candidate has the same binary32 staging at
all twelve sites. Differences are stack-slot allocation and scheduling, not
the precision presented to `fistp`.

## Input-domain check

`meter_hud_element_definition::value_scale` is a signed `short`, so conversion
to project `real` is exact. Shield vitality and the auxiliary channel are
already project `real` values. January and the candidate therefore both:

1. multiply two binary32 inputs using x87;
2. round the product to a four-byte `real` stack slot; and
3. reload that binary32 value before the current-rounding-mode `fistp`.

For the precision-sensitive example used to expose the HUD Draw scratch bug,
`0.0019607844296842813f * 255` is slightly greater than 0.5 in the extended
product but rounds to binary32 `0.5f`. HUD Unit gives `fistp` the rounded
`0.5f` on both sides, so both produce the ties-to-even result. The rejected HUD
Draw scratch candidate differed because its January target converted the
extended product directly while the candidate first spilled it.

An independent second reviewer separately enumerated the same twelve target and
candidate instructions and reached the same conclusion. This follow-up changes
only the precision adjudication; the separate candidate-only `_fast_ftol`
COMDAT ownership veto against whole-object Matching remains in force.
