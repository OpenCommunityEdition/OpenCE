# AMD same-query readback comparison

1. Copy the extracted package contents into your GulchCE folder beside `maps`,
   replacing the previous test files. Keep `BUILD.txt` beside `halo.exe`.
2. Run **Run-AMD-Query-Compare-Test.cmd**. This single launcher enables the
   comparison and logs it; saves remain in `amd-visibility-saves`.
3. Enter these in the game console:

   ```lisp
   (set rasterizer_lens_flares_occlusion true)
   (set rasterizer_lens_flares_occlusion_debug false)
   (set rasterizer_zbias 8)
   ```

4. Use the same map, resolution and anti-aliasing as the failed test. Look at
   the Warthog headlight/brakelight glows from a clear view, move behind solid
   cover and back, and try the same affected lights as before. Play for at least
   one minute. Report missing lights, flickering, delayed changes or slowdown.
5. Quit normally and send the newest
   **amd-visibility-logs/query-compare-*.log**, a short video and your observations.
   Send the log even if the lights are still entirely missing: this package
   compares results and is not expected to restore lights by itself.

Expected startup: `same-query visibility comparison enabled; mapped rendering
retained, direct reads only when available`. The renderer checks both paths
for the same completed query and keeps using the mapped result for rendering.
There is no forced GPU synchronization or depth override, but the extra driver
calls can affect performance. Do not use an older CMD for this test.
