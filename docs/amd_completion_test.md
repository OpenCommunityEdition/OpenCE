# AMD completion-aware readback test

1. Extract the package and copy its contents into your GulchCE game folder,
   beside `maps`, replacing the previous test executable. Keep `BUILD.txt`
   beside it so the log identifies this build. Maps are not included.
2. Run **Run-AMD-Completion-Test.cmd**. Use this launcher for this test; it
   enables logging and uses the separate `amd-visibility-saves` folder.
3. In the game console enter:

   ```lisp
   (set rasterizer_lens_flares_occlusion true)
   (set rasterizer_lens_flares_occlusion_debug false)
   (set rasterizer_zbias 8)
   ```

4. Keep the same resolution and anti-aliasing settings as your previous test.
   Check Warthog headlight/brakelight glows while stationary and moving, then
   move behind solid cover and back: glows should disappear and return
   appropriately. Watch for missing lights, flickering or delayed changes.
   If convenient, also check A10's flashing tutorial lights or dropship glows.
   Play for at least a minute, then quit normally.
5. Send the newest **amd-visibility-logs/completion-query-*.log**, a short video,
   and whether brightness, occlusion and performance look correct. Send that
   log even if it works; it records completed GPU measurements this time.

Expected startup: `visibility query pool created=16385 zeros=0 duplicates=0
invalid=0 GL_ok=1` and `completion-aware asynchronous visibility readback
enabled; 3 fenced batches, no CPU waits`. The code uses zero-time fence polls,
with no forced synchronization, depth override or disabled occlusion. This is
an AMD test candidate; it has not been merged into `testing`.
