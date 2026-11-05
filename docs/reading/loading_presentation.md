# Widescreen loading and the Xbox chant

`display.widescreen_loading` (Video > Graphics > Widescreen Loading) is
on by default. When enabled, Xbox, Custom Edition and MCC loading screens
fill the current render target. The original 320x240 picture is retained:
there is no replacement artwork and no new detail invented by upscaling.
The light mask is evaluated at each rendered pixel, rather than stretched
from the stock 128x16 mask. With original resolution scaling selected, the
render target itself still has the original resolution.

An existing saved `false` preference still takes precedence over the default;
enable the setting in Video > Graphics or use Defaults to adopt it.

## Reading order

1. [progress_bar.c](../../source/interface/progress_bar.c):
   `progress_bar_load_loading_texture` reads the existing `maps/loading.tga`
   through the stock cache path. `progress_bar_render` updates the original
   chant's four volume envelopes before offering the image to the optional
   renderer. If disabled or preparation fails, the inherited renderer runs.
2. [loading_widescreen.c](../../port/linux/src/loading_widescreen.c):
   `loading_widescreen_draw` copies the stock texture into its own GPU
   texture, preserving the stock texture's ownership. The shader scales the
   image uniformly to cover the target, crops the excess axis, and samples
   enlarged pixels with a Catmull-Rom cubic filter. Its continuous sweep
   and short trailing glow follow the original smoothed progress.
3. [d3d8_gl.c](../../port/linux/src/d3d8_gl.c):
   `xgpu_presentation_target` binds the current target through the existing
   device machinery, including multisampling and write tracking, and
   supplies its actual pixel dimensions. This narrow hook is necessary
   because the current target and its binding machinery are private to the
   device. It contains no loading artwork or map-family logic.
4. [mcc_loading.c](../../port/linux/game/mcc_loading.c): the existing
   MCC-owned controller sets the viewport and calls the shared progress
   interface. Its activation guards, stages and session ownership are
   unchanged. It never runs a normal game frame against partially converted
   tags to draw the loading mask.

At 1920x1080, the picture is scaled uniformly to 1920x1440 and 180 rows
are cropped from each end. The image fills the screen without stretching
the ring or text; its centre remains in view. At 4:3, the entire picture
is visible. On wider displays more of the top and bottom is cropped.

The approach follows OpenH1's
[aspect-filling image presentation](https://github.com/OpenH1/OpenH1/pull/178)
and screen-resolution mask. This implementation uses our original artwork,
not OpenH1's redraw or its separate menu-ring geometry capture. The glow
is analytic rather than a copy of the preceding frame, so it cannot retain
pixels from a previous map or another window size. It does not animate
independently while the loading thread is busy between progress updates.

## Ownership, isolation and checks

`loading_widescreen_end` releases its texture, program and vertex array on
loading completion, failure and shutdown. Disabling the setting releases
them too. No map pointers or converted resources are retained. The GL state
cache is invalidated after direct rendering, and the progress renderer
invalidates the game's rasterizer cache. Normal drawing reinstalls its
viewport and state. The stock front-buffer feedback texture remains 640x480.

The only game-source additions are presentation dispatch and cleanup hooks:
the progress implementation owns these lifecycle boundaries, so a feature
module cannot receive them without integration. The Xbox and Custom Edition
loaders and MCC conversion/resource paths are unchanged. No network messages,
simulation values or compatibility version change.

The chant remains available with either presentation. Its buffers are
stopped at `progress_bar_end` even on failure, independently of any later
normal frame. See [maps and resources](maps_and_resources.md) for the audio
implementation and its limits.

Validation should cover the disabled stock path, 4:3, 16:9 and ultrawide
targets, renderer state after the pass, failure/next-load cleanup, and all
three map families. Controller tests alone do not establish successful MCC
map conversion; native checks do not establish Android or browser runtime
behavior.

`tools/test_loading_widescreen.py` compiles the actual presenter with a
synthetic stock texture and draws through hidden desktop GL and GLES
contexts. It checks coverage, cover cropping, sweep brightness, completion,
resize, disabled/null-renderer behavior, failed setup recovery and resource
release. `tools/test_mcc_loading.py` checks the existing MCC controller's
guards, viewport and success/failure lifetime; `tools/test_loading_chant.py`
checks the audio mixer and progress voice lifetime.
