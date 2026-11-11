# MCC gameplay icon placement

The gameplay messaging icon path uses one scale for the bitmap, its authored
offsets and its extra text width. The port previously applied bitmap scaling
only to the picture, leaving offsets and extra width in native font units.
The shared MCC path now scales all three. No map or tag names are checked.
Xbox and CE still return to their existing renderer before this calculation.
Replacement controller buttons and pause-menu layout retain their own paths.

Native behavior was verified by read-only inspection of the installed Steam
`halo1/halo1.dll`, SHA-256
`0a12dc561780f449d3f4d0df10bb8d3bc7be7840a5beb2b236f672eb6cd42e6c`.
These RVAs identify that binary, relative to preferred base `0x180000000`:

- `0xAD9A80`: gameplay message bitmap routine; it retrieves the icon sequence
  and frame from the HUD globals' messaging bitmap.
- `0xAD9B54`: bitmap flag `0x10` halves its scale. The split-screen factor
  is then applied at `0xAD9B69` through `0xAD9B90`.
- `0xAD9B82` through `0xAD9BC2`: signed X/Y offsets are multiplied by that
  same scale and truncated toward zero before adding/subtracting them from
  the cursor. Truncating the sum instead changes negative fractional offsets.
- `0xAD9BF0`: that same scale is passed to the bitmap drawing routine.
- `0xAD9BFB` through `0xAD9C78`: absolute text width uses `width_offset *
  scale`; otherwise clipped or full bitmap width is added to `width_offset`
  before multiplying by scale and adding the positioned icon's X coordinate.

The port's existing bitmap helper additionally converts MCC's 960p canvas to
its 480p HUD units. Blood Covenant Mirrored's messaging bitmap, tag 1216
`aerial\ui\hud\weapons\bitmaps\pickup_icons`, has flag `0x10`. Its weapon
icons' `(16,-64)` authored offset therefore becomes `(4,-16)` before applying
the cursor's Y subtraction.
The texture, UVs, registration point and authored tag data are not rewritten.

Asset-free tests extract the production bitmap-scale and icon functions.
They cover ordinary, half-size and high-resolution flags, split screen,
negative fractional offsets, clipping, absolute width, pause-menu behavior,
replacement buttons and MCC/legacy transitions. Negative controls restore
the old offset scale, rounding and width behavior and must fail.

Local validation: 65 selected HUD and damage tests passed (14 new icon tests).
Windows and web builds passed. The changed Android guest unit compiled and
assembled; the full Android target remains blocked by Windows-host tooling.
A hidden Blood Covenant Mirrored smoke run reached 330 simulation ticks and
exited normally. This does not constitute a visual pickup-prompt comparison
against native MCC; positioning is checked by the production-function harness.
