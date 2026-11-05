# Widescreen loading and the Xbox chant

`display.widescreen_loading` (Video > Graphics > Widescreen Loading) is
on by default. Xbox, Custom Edition and MCC use the original loading effect:
the installation's 320x240 picture, the 128x16 sweep mask, the original
feedback blur and glow trails, progress wobble and fade. Widescreen changes
only the horizontal framing. It neither crops the artwork nor introduces
replacement art, cubic filtering, a new sweep shader or a different glow.
The complete 4:3 composition expands horizontally to fill the screen.

An existing saved `false` preference still takes precedence over the default;
enable the setting in Video > Graphics to adopt it. Off keeps the central
4:3 presentation. The Xbox chant plays with either framing.

## Reading order

1. [progress_bar.c](../../source/interface/progress_bar.c):
   `progress_bar_begin` starts a presentation session, then
   `progress_bar_load_loading_texture` reads `maps/loading.tga` through the
   established cache path. `progress_bar_render` runs the stock renderer in
   both modes. Only the projection's horizontal scale differs: widescreen
   uses the full viewport; off retains the original central 640 columns.
   The original mask, samplers, blending, `draw_fade_layer` and
   `draw_initial_layer` remain responsible for every visual effect.
2. [loading_widescreen.c](../../port/linux/src/loading_widescreen.c):
   `loading_widescreen_begin` reads the setting once per loading session.
   `loading_widescreen_active` supplies that same decision to drawing and
   capture. `loading_widescreen_end` clears it on completion, failure or
   shutdown. There are no owned textures, shaders or map pointers.
3. [d3d8_gl.c](../../port/linux/src/d3d8_gl.c): `front_buffer_take`
   copies the completed loading frame back into the original 640x480
   feedback texture. During widescreen loading it copies the full width;
   otherwise it keeps the existing central capture. The next loading frame
   blurs and fades this image exactly as before. Capture still happens at
   `BlockUntilVerticalBlank`, before any download overlay is drawn.
4. [game.c](../../source/game/game.c): the ordinary Xbox and Custom Edition
   loading path calls the shared progress interface around map loading.
   [mcc_loading.c](../../port/linux/game/mcc_loading.c) owns MCC's conversion
   progress and calls that same interface. Its guards, stages and session
   ownership are unchanged. It never draws a normal game frame against
   partially converted tags just to update the loading screen.

For example, at 1920x1080 the stock effect occupies the whole target when
widescreen is on. Its completed frame is reduced to the existing 640x480
feedback image, which the following frame draws over the same whole target.
With widescreen off, both drawing and capture use the central 1440x1080
region. At 4:3 the two modes have the same framing and effects.

## Integration and isolation

Drawing and feedback capture must agree on framing: widening only the
projection would feed a cropped middle section back into the next frame,
changing the trail. These two small integration points are necessary because
the projection lives in the game's progress renderer and front-buffer
capture is private to the device. Feature state remains in its own module.
No map loader, map-family boundary, texture import, network message or
simulation value changes. MCC uses the shared presentation service without
routing Xbox or Custom Edition data through MCC modules.

The chant's original four voices and volume envelopes remain in the progress
renderer. `progress_bar_end` stops them on success or failure without waiting
for a normal frame. See [maps and resources](maps_and_resources.md) for the
audio implementation and its validation limits.

## Checks

`tools/test_loading_widescreen.py` records calls made by the actual stock
renderer and device feedback-capture function. Across 4:3, 16:9 and ultrawide
widths it checks that only the projection changes: mask/blur shader use,
fade, progress, color and chant volumes agree. It also checks matching
capture bounds, session reset and the disabled path. The former tests for
the removed custom shader are replaced by these stock-effect checks.

`tools/test_mcc_loading.py` checks MCC controller guards, viewport and
success/failure lifetime. `tools/test_loading_chant.py` checks the mixer and
voice lifetime. These recording tests do not prove visual output or full
map compatibility: inspect native captures and transitions as well, and
report actual Android/web build and runtime limits separately.
