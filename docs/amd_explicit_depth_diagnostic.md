# Explicit visibility depth test

Extract this package separately and supply your own maps. Double-click the
single `Run-AMD-Explicit-Depth-Test.cmd` launcher. In the game console enter:

```lisp
(set rasterizer_lens_flares_occlusion true)
(set rasterizer_lens_flares_occlusion_debug false)
(set rasterizer_zbias 8)
```

Compare the same Warthog lights or other affected glows with the previous
synchronized, normal-depth test. Keep camera, resolution and AA settings the
same. Record about 20 seconds and quit normally. Send the recording and the
new log in `amd-visibility-logs`, noting the map and whether brightness or
flickering changed. Saves remain separate in `amd-visibility-saves`.

The log must say `explicit visibility depth offset enabled`. Query examples
should report `depth=1` and `explicit_offset=-4.76837e-07` with bias 8. Depth
testing stays active, so correctly hidden glows should still be occluded.
This is an experiment, not a confirmed fix. Both modes use blocking readback.

If a fresh comparison using this exact executable is needed, run the same
CMD with `--current-bias` from Command Prompt. This keeps synchronized reads
but restores the current native polygon bias. No other CMD is needed.

## What changes

The default-off desktop setting `debug.gpu_visibility_explicit_depth` replaces
native polygon offset only inside visibility queries with a dedicated fragment
shader that writes `clamp(gl_FragCoord.z - zbias/16777215, 0, 1)`. Native bias is
disabled for those draws to avoid applying it twice; other draws restore the
requested bias and use their original shaders. The shader uniform is refreshed
on every diagnostic draw, independently of the regular uniform cache. If solid
offset is disabled, the explicit offset is zero. The diagnostic takes precedence
over the older no-depth switch and implies synchronized readback.

The denominator describes one step of the renderer's D24 depth storage. This
does not establish that the NV2A bias unit is identical. Lens-flare test quads
have constant depth, so no slope term is needed; the diagnostic does not emulate
slope bias for the unrelated transparent-pixel debug counter. Leave that counter
off. Fragment depth is written after clipping, preserving the original clip
geometry and clamping the shifted depth. Writing depth also changes the driver's
depth-test execution path, so improvement alone would not prove a unit mismatch.

The launcher pins both older switches, enables GL error reporting and labels
each log separately. Direct EXE launch uses config/environment values; the new
setting defaults off. Restart between modes. Android/WebGL code is excluded.
Map-family loading, shader compatibility guards and networking are unchanged.
