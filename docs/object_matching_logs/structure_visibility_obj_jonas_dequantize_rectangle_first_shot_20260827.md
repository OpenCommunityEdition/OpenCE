# `structure_visibility.obj` rectangle dequantization first shot (Jonas, 2026-08-27)

## Result

`_dequantize_byte_to_real_rectangle3d` is byte-exact and relocation-exact.
The object advances from 1/21 to 2/21 exact code owners while preserving the
inherited exact `_debug_pvs` owner.

The recovered function contributes 279 meaningful bytes in a 288-byte padded
owner and six `DIR32` relocations. Its normalized SHA-256 is
`0773c1267afd8b1d7c150553f335cd0287c12a338b4f99c069134bda116e33e7`.

## Frozen evidence

- Target object raw SHA-256:
  `DBE1A7580E7D22B6F01899073049DB6CC2F6F556523CA148BB456E86BE3F7697`.
- Pre-edit source blob: `ac6fcd7b76e072bd870c21ae65290bbefb28d2d5`.
- Pre-edit source raw SHA-256:
  `C60464EE22571710753DA4BF719B279169F83EAF057A57281C0C405501F800FD`.
- Frozen baseline candidate:
  `build/audit/structure_visibility_dequantize_rectangle_baseline_20260827.obj`.
- Retained source blob: `99613d0c77793621040b7f8a02200ea1cdbb548f`.
- First and only code-producing candidate:
  `build/audit/structure_visibility_dequantize_rectangle_first_shot_20260827.obj`.
- First-shot raw SHA-256:
  `8EF4AAACF34F911BC6B4D39C917A9BCC680B8FE0986C7244266A9CABAB4658F3`.

The target has relocations at owner offsets `+39`, `+84`, `+127`, `+170`,
`+213`, and `+265`. All six are `DIR32` references to the canonical
`__real@3b808081` (`1.0f / 255.0f`) literal. The first shot reproduces all
six relocation addresses, types, identities, addends, and ownership.

## Source recovery

The already-exact `real_math.obj::_dequantize_byte_to_real` owner supplied the
canonical scalar topology:

```c
if (value == UNSIGNED_CHAR_MAX)
{
	return max;
}

return (max - min) * ((real)value / 255.f) + min;
```

The January target consists of six inlined copies of this donor, in rectangle
field order `x0`, `x1`, `y0`, `y1`, `z0`, `z1`, followed by `return result`.
The source therefore uses a private `static __inline` scalar donor and six
direct assignments. A narrow include-time rename hides the external declaration
from `real_math.h` without changing the shared header or exporting an unwanted
scalar owner.

The HCEA reference at
`build/audit/refs/halocea/src/blam/math/dequantize_byte_to_real_rectangle3d.c`
confirms the six-axis semantics. Its loop and `void` PPC signature are not used
as topology authority; the January Xbox target proves the direct assignments
and pointer return.

## Strict comparison

The first shot matches all 288 padded bytes. It also matches all six
relocations exactly and introduces no extra owner. The strict containment
comparison retains `_debug_pvs`, adds only
`_dequantize_byte_to_real_rectangle3d`, and reports `lost_exact: []`.

## Repository gates

- `build\local-tools\ninja.exe`: pass.
  - 470 semantic units scanned.
  - 4,366 functions evaluated.
  - 4,221 semantic-exact and 4,281 accepted-exact functions.
  - zero unit errors.
  - Campaign: 377/833 complete objects, 4,255/11,060 exact functions,
    512,122/2,198,102 code bytes.
  - Halo: 275/468 complete objects, 4,088/7,574 exact functions,
    499,208/1,770,166 code bytes.
- `python -m tools.audit_object_admission`: pass, 0 candidates,
  0 contradicted, 0 revoked.
- `python -m tools.parked_functions`: pass, 3 active, 0 stale, 0 invalid.
- `python -m pytest -q`: 179 passed; the existing cache-directory permission
  warning is unchanged.
- `git diff --check`: pass.
