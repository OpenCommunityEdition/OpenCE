# High-resolution reticle and zoom-label corrections

With High-Res HUD enabled, the stock rocket reticle, sniper reticle and supplied
zoom labels use the contributor's [original pictures](../../port/assets/hud/corrections/README.md).
The sniper reticle retains the low-resolution artwork's visible size and center.
Input, simulation, zoom magnification and sprite geometry are unchanged.

## Reading route

1. [hud_corrections.py](../../tools/hud_corrections.py) records the source crops and
   atlas rectangles. [hud_assets.py](../../tools/hud_assets.py), `build_asset`,
   converts black-background luminance to premultiplied white coverage and scales
   proportionally. It never redraws shapes or substitutes a font. Visible reticle
   bounds match the stock 9x9 texel footprint within one generated high-res pixel.
2. Xbox uses its existing texture override and atlas. The setting and original
   pixel signature gate replacement in [hud_hires.c](../../port/linux/src/hud_hires.c).
   CE's rocket uses the existing compatible-sheet registration.
3. CE's separately packed scope contains the supplied 2x and reticle. The map's
   existing 8x artwork is preserved. `hud_hires_crosshair_bitmap` in
   [hud_hires_tags.c](../../port/linux/game/hud_hires_tags.c) selects only those
   verified sprites through the existing placeholder mechanism. Sequence 1,
   frame 1 remains the original 8x.
4. The small marked hook in [hud_weapon.c](../../source/interface/hud_weapon.c)
   is necessary because a whole-sheet override would replace that 8x frame too.
   It runs after the source texture is resident. Custom art failing the stock
   signature checks keeps its original pixels, as does High-Res HUD OFF.

For example, CE's first sniper zoom shows the supplied 2x picture while the next
still shows its own 8x. Xbox uses the supplied 2x and 10x pictures. No zoom values
or weapon behavior change. Placeholders belong to the existing map lifetime and
are released by `hud_hires_tags_unloaded`.

[Correction tests](../../tools/test_hud_corrections.py) verify original source
hashes, exact supplied-pixel packing, the stock visible reticle footprint and
CE's frame selection, including disabled, custom-art and failed-match fallbacks.
Actual maps and renderers additionally require runtime checks.
