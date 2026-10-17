# AMD visibility diagnostic

This is a desktop OpenGL experiment on `AMD-render-fix`, not a gameplay fix.
The previous synchronized-readback test did not restore correct AMD glows.
This comparison checks whether depth rejection explains the remaining failure.

## Player instructions

1. Extract the new package into its own folder. Keep `SDL3.dll` beside
   `halo.exe` and supply your own game data in `maps`, `custom_maps`, or
   `mcc_maps` as appropriate. Do not mix executables from different packages.
2. Run `Run-Baseline.cmd`, load the affected scene, and use the console:

   ```lisp
   (set rasterizer_lens_flares_occlusion true)
   (set rasterizer_lens_flares_occlusion_debug false)
   (set rasterizer_zbias 8)
   ```

   Park the Warthog, stand where its lights should be visible, and record
   about 20 seconds. Record the map and position, then quit normally.
3. Repeat with `Run-AMD-Visibility-Test.cmd` (synchronized reads, normal
   depth testing), using the same commands, camera, resolution and AA setting.
4. Repeat with `Run-AMD-No-Depth-Test.cmd` (synchronized reads, depth disabled
   only for visibility-test draws). Test the unobstructed lights first, then
   move behind a wall to compare occlusion. Glows appearing through walls in
   this mode are an expected diagnostic side effect.
5. Send the whole `amd-visibility-logs` folder, the recordings labelled with
   their mode, GPU/driver, map, and whether brightness/flicker changed. Each
   launcher saves a separate randomly named log. All modes share the isolated
   `amd-visibility-saves` folder beside the executable. Preserve it for reruns.

The two synchronized modes can reduce performance. Launchers explicitly set
both diagnostic switches and enable OpenGL error reporting. Running the EXE
directly uses config/environment settings, with both diagnostics off by default.
Restart to change modes. No change to `rasterizer_zbias` beyond restoring the
stock value of 8 is needed for this test.

## Reading the results

`GPU: visibility diagnostic depth=...` identifies the mode. A summary every
second reports completed reads, raw zero/nonzero counts, nonzero counts rounded
to zero by scaling, the raw range, and cumulative GL errors. Idle seconds are
reported too. Up to two detail lines follow: the most recent raw zero and raw
nonzero query in that interval, including slot, GL query ID, submission frame,
draw count, scale, normalized pixels and draw state. These are representative
examples, not an exhaustive trace. Slots are reused and do not identify lights.
An example records the last draw's state if a query contains multiple draws.
The first 16 GL errors get operation/frame labels; the error total keeps growing.

Normal-depth failure with no-depth recovery supports depth rejection as the
next investigation target. Failure in both modes calls for inspecting geometry,
clipping, stencil, culling, shader execution and query handling. GL errors must
be considered before treating zero counts as valid occlusion. A raw nonzero
result that normalizes to zero points to a separate scaling/coverage issue.

Only visibility draws bypass depth; ordinary rendering restores its requested
depth test and write mask through the normal state cache. Stencil, clipping,
culling and the remaining visibility path are unchanged. The diagnostic is
renderer-local, applies equally to map families, and changes no network state.
Android/WebGL paths are excluded. Source settings and their environment names
are documented in `port/linux/README.md`.
