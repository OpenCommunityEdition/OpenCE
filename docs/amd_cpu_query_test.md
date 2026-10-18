# AMD asynchronous CPU readback test

1. Copy all extracted package files into your GulchCE folder beside `maps`,
   replacing the previous test executable. Keep `BUILD.txt` beside `halo.exe`.
2. Run **Run-AMD-CPU-Query-Test.cmd**. This is the only launcher for this test.
   Saves remain in `amd-visibility-saves`; logging is enabled automatically.
3. In the game console enter:

   ```lisp
   (set rasterizer_lens_flares_occlusion true)
   (set rasterizer_lens_flares_occlusion_debug false)
   (set rasterizer_zbias 8)
   ```

4. Use the same map, resolution and anti-aliasing as the failed test. Spend at
   least a minute checking Warthog headlight/brakelight glows and other affected
   lights. Move behind solid cover and back: glows should disappear when hidden
   and return when visible. Report flickering, missing glows, delayed changes,
   brightness differences and any frame-rate loss. Quit normally.
5. Send the newest **amd-visibility-logs/cpu-query-*.log**, a short video and your
   observations. Send the complete file, including its build/startup lines.

This build uses available queries' direct CPU counts for rendering, with no
query-buffer copies and no forced synchronization. The change is enabled in
normal desktop rendering; the CMD enables logging, not a special rendering mode.
Startup must say `asynchronous direct CPU visibility readback enabled`.

While gameplay is showing lights, `visibility CPU` summaries should show
completed batches and nonzero results. Zero submissions in menus do not test
lights. `unavailable` polls can be normal; report sustained drops, failures,
missing completions or obvious slowdown. Previous comparison CMDs and logs are
not the test requested here. Keep occlusion enabled for the whole test.
