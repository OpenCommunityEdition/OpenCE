# High-Res HUD artwork corrections

With High-Res HUD enabled, the stock rocket reticle, sniper reticle and supplied
zoom labels use the owner's [original pictures](../../port/assets/hud/corrections/README.md).
Xbox Classic button letters are transparent cutouts. Tutorial thumbsticks use
the existing high-resolution controller path, including animated stick presses.
These are display changes; input bindings, simulation and networking are unchanged.

## Reading route

1. [hud_corrections.py](../../tools/hud_corrections.py) records the crop and atlas
   rectangles. [hud_assets.py](../../tools/hud_assets.py), `build_asset`, converts
   black-background luminance to premultiplied white coverage and proportionally
   resizes the supplied pixels. It does not reconstruct their shapes or font.
   The sniper reticle fits the stock visible footprint, not the larger padded
   sprite cell: 9x9 texels on Xbox/CE and 36x36 on MCC's four-times-resolution
   sheet. These bounds were measured from stock alpha coverage; the supplied
   image matches their size and centre within one generated high-resolution
   pixel. Sprite geometry and zoom-label placement remain unchanged.
2. Xbox retains its existing texture replacement path and atlas layout.
   [hud_hires.c](../../port/linux/src/hud_hires.c), `hud_hires_override_find`, checks
   the setting and original pixel signature. CE's rocket uses its existing
   compatible stock-sheet registration.
3. CE's scope has a different layout and an 8x label. Its separate atlas contains
   only the supplied 2x and reticle. In
   [hud_hires_tags.c](../../port/linux/game/hud_hires_tags.c),
   `hud_hires_crosshair_bitmap` selects those sprites through the existing
   CRC-checked placeholder machinery. Sequence 1, frame 1 keeps the original 8x.
   The small [hud_weapon.c](../../source/interface/hud_weapon.c) hook is needed
   because a whole-texture override cannot preserve one frame of a sequence.
   It runs after the source bitmap is resident and changes no sprite bounds,
   placement, zoom values or tag contents. Other map families return their original
   bitmap. CE embedded/custom art still fails stock registration or pixel checks.
4. [mcc_texture_bridge.c](../../port/linux/src/mcc_texture_bridge.c),
   `mcc_hud_texture_asset`, recognizes the complete converted stock MCC sheets by
   dimensions, format, byte count and CRC, including the `_r` variants. It uses
   independently packed MCC atlases. No Xbox/CE loading or registration is used.
   `xgpu_mcc_texture_get` checks High-Res HUD at draw time; disabling it returns the
   original texture immediately. Failed replacement uploads also fall back.
5. [button_assets.py](../../tools/button_assets.py), `classic`, masks the existing
   letter out of the coloured oval. The bounds in
   [hud_button_layout.h](../../port/linux/game/hud_button_layout.h) match the
   committed alpha mask, keeping the existing visible size and text anchoring.

For example, CE's first sniper zoom uses the supplied 2x picture, while its next
zoom still draws the map's 8x. Neither label changes the actual magnification.
Custom textures that do not match the stock signatures keep their artwork.

## State and checks

CE placeholders belong to the existing map lifetime and are released by
`hud_hires_tags_unloaded`. MCC holds only an asset index in its own texture entry;
the embedded-HUD service owns replacement GPU textures. MCC teardown frees the
entry and its original GPU texture, without deleting the borrowed replacement.

[Correction tests](../../tools/test_hud_corrections.py) check supplied-pixel
packing, transparency and CE's untouched 8x selection.
[MCC bridge tests](../../tools/test_mcc_texture_bridge.py) check stock/custom
identity, settings changes, upload failures and ownership.
[Button tests](../../tools/test_hud_button_style.py) and
[MCC HUD tests](../../tools/test_mcc_hud.py) cover control animation, prompt
geometry and split-screen scaling. Runtime captures are needed in addition to
these isolated tests to verify actual map loading and draw paths.

## Custom Edition authored HD scaling

A CE map can ask for smaller HUD geometry through bitmap flags, independently
of the High-Res HUD artwork setting. The setting still controls the supplied
replacement pictures. Correctly interpreting a map's own flags is part of CE
compatibility and is always active when CE tags are loaded.

[custom_edition_hud_scale.h](../../port/linux/game/custom_edition_hud_scale.h),
`custom_edition_hud_scale`, composes the flags:

| Placement high resolution | Bitmap flags | Geometry multiplier |
| --- | --- | --- |
| No | Neither | 1 |
| Yes | Neither | 1/2 |
| Either | Force high resolution | 1/2 |
| No | Half scale | 1/2 |
| Yes | Half scale | 1/4 |
| Either | Half scale and force high resolution | 1/4 |

For example, a 256-pixel bitmap with both bitmap flags occupies 64 HUD units
before the ordinary viewport scaling. The two halvings are independent;
placement high resolution and bitmap force-highres describe the same halving.

Read [cache_file_formats.c](../../port/linux/game/cache_file_formats.c),
`hud_conversion_placement`, for the bounds-checked conversion of statics,
meters, crosshairs and overlays. Crosshair and overlay items ignore placement
high resolution, retaining the existing CE behavior. Number placements retain
their flag: `hud_draw_numbers` scales digit geometry separately from authored
spacing. The known-map `hud_number_scale` correction already halves metrics
and supplies bitmap half-scale, so its placement flag cannot halve again.
`disable_bitmap_hud_scale_flags` still clears both bitmap flags before conversion.

[custom_edition_hud_draw.c](../../port/linux/game/custom_edition_hud_draw.c)
owns the runtime CE interpretation for numbers and draws without converted
placements: pickup icons, waypoint arrows, friendly markers, damage arrows,
scope expansion and convolution masks (the texture that shapes the scope's
screen effect). These hooks are necessary because the engine constructs their
geometry at draw time. Pickup text advance scales only the bitmap width;
authored offsets and absolute widths retain their meaning. Stock controller
glyphs and MCC icon handling retain their existing early returns.

`interface_draw_screen` borrows the selected mask only across its synchronous
`rasterizer_screen_effect` call. `custom_edition_hud_mask_transform` accepts
only that bitmap, then `custom_edition_hud_mask_end` clears both pointer and
scale before another view or map can draw. The renderer hook adjusts texture
coordinates as well as mask size; a viewport fallback or cinematic texture is
not treated as a CE HUD mask. No shared screen-effect structure changes layout.
All runtime helpers return identity outside CE, and MCC's own canvas, loaders,
textures and cleanup remain in MCC modules. Existing CRC-based artwork
replacement, warning placement, reticle selection and digit spacing are retained.

The selective backport follows [ChupathingyCE #352](https://github.com/ChupathingyCE/chupathingyce/pull/352)
by Boom Pen / ugoboom and [#545](https://github.com/ChupathingyCE/chupathingyce/pull/545)
by Milenko, under GPL-3.0-or-later. Flag composition is adapted from
[Chimera's `check_half_scale_bitmap_flag`](https://github.com/SnowyMouse/chimera/blob/master/src/chimera/fix/hud_bitmap_scale.cpp),
under GPL-3.0-only, as marked in the header. These licenses are compatible with
this repository's AGPL-3.0-or-later distribution; the derived header retains its
GPL-3.0-only notice. The backport does not import upstream's older placement
rewrites or its unrelated replacement art.

[Converter tests](../../tools/test_cache_file_formats.py) exercise real synthetic
tag conversion across all flag combinations. [Draw tests](../../tools/test_ce_hud_scaling.py)
compile the implementation's functions to check number exceptions, pickup
geometry, split-screen scaling, controller/MCC ownership and mask scope through
map-family transitions. They do not establish visual compatibility for every
custom map; runtime checks must include authored HD assets and stock Xbox/CE/MCC
maps, with the artwork setting both disabled and enabled.
