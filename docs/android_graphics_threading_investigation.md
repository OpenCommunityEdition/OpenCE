# Android Slowdown Investigation

## Status

**Solved, and not by threading.** The slowdown and the apparent freeze on
Adreno were a per-call cost in the driver's buffer upload, not a stall and not
a race. Dropping `GL_MAP_INVALIDATE_RANGE_BIT` from `host_gl_buffer_write` took
the game from 1.8 fps to 120 fps on a Galaxy Z Flip 4 (Snapdragon 8 Gen 1), its
display's refresh rate. The threading rework this document was written to plan
turned out not to be needed, and the plan below has been replaced by what was
actually found.

Keep the instrumentation. It is what settled the question, and it is how a
regression of this kind would be caught:

- `debug.fence_stats` - times and classifies the frame fence waits
- `debug.gpu_stats` - splits the frame between Present, uploads and the rest

## What was actually wrong

`port/android/host/host_gl.c` wrote each streamed range with

```c
glMapBufferRange(target, offset, size,
    GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
```

`GL_MAP_UNSYNCHRONIZED_BIT` promises the caller that no queued draw is reading
the range. `GL_MAP_INVALIDATE_RANGE_BIT` tells the driver the range's previous
contents are undefined and must be discarded. The two contradict each other,
and the driver pays for the discard.

The cost was per call and independent of the range written - about 0.85 ms to
write a kilobyte - and the renderer makes one call per draw, a few hundred per
frame. That put 96% of the frame inside this one function, left the GPU idle
for the whole frame, and produced 1.8 fps in ordinary gameplay.

Measured, before and after, in the same scene:

| | Before | After |
| --- | --- | --- |
| Frame rate | 1.8 fps | 120 fps (display refresh) |
| Frame time | ~550 ms | 8.3 ms |
| Uploading, per frame | ~530 ms | 0.11 ms |
| Driver time, per second | 1085 ms | 10 ms |
| Per upload call | 0.85 ms | under 0.001 ms |
| GPU fences waited on | 0 | 0 |

The fix is to drop the invalidation. The safety argument is unchanged: the
renderer only writes ranges that no queued draw reads, because
`host_gl_wait_frame` releases the slot first, and the `RACE` counter reports
if that ever stops being true.

## What was measured and ruled out

Both hypotheses this document was written to test were wrong, and the
instrumentation is what ruled them out. Recording them so they are not
re-investigated:

- **The GPU falling behind.** Refuted. Across every run, including the ones at
  1.2 fps, every fence was already signalled when it was waited on: zero
  waits, zero timeouts, 0.1 ms of a 1000 ms window blocked. `Present` cost
  0.4 ms of a 32 ms frame in the good case and 0.5 ms of a 550 ms one in the
  bad. The main thread was never waiting for the GPU.
- **The unsynchronized mapping corrupting vertex data.** Refuted. The `RACE`
  count was zero in every window of every run, including the frozen ones. The
  ring discipline in `host_gl_wait_frame` holds.

Three things tried before the flag was found, none of which changed anything:

| Tried | Result |
| --- | --- |
| `glMapBufferRange`/`glUnmapBuffer` instead of `glBufferSubData` | No change, ~0.85 ms per call either way. The API is not the cost. |
| Shrinking `STREAM_BUFFER_SIZE` 16 MB to 2 MB | **Worse**: 1.77 ms per call. The cost is not proportional to the buffer, and the smaller buffer just overflowed more. |
| Timing a write to an unused buffer as a control | Broke the renderer, then crashed the driver. The lesson is under "Method" below. |

A fourth, the fix, is in "What was actually wrong" above.

## Method

Two process notes, both learned the hard way on this.

**Confirm the picture before believing a frame time.** A control that wrote
each range to a scratch buffer first, and then restored the binding, produced
120 fps - and a game with no world geometry, followed by a SIGSEGV inside the
Adreno driver. Every number in the log was accurate; the renderer simply was
not drawing anything, so of course it was fast. Nothing about the log alone
distinguished that from a fix. `debug.gl_debug` also reports the invalid
operations that caused it, and is worth turning on whenever the driver is
being poked at directly.

**Do not manipulate GL state behind the renderer's back.** The renderer keeps
its own cache of bindings and skips redundant calls, so anything that changes
a binding behind it desynchronises that cache, and a binding left at 0 makes
the driver dereference null. A control that read the binding back with
`glGetIntegerv` still failed: the query is not valid for that enum, so it
returned 0 and the restore bound nothing.

## The remaining question

Threading was never the right fix for this, and the corrected scope below
remains the right way to build a command queue if one is ever wanted - it is
not expensive, and it would take the remaining 8.3 ms frame off the main
thread. But it is not what was wrong. The 8 ms frame that is left is 2 ms of
draw submission and 8 ms of the game's own work, so there is little for a
render thread to win, and the argument that made it look worthwhile - that the
main thread was blocked on the GPU - is false.

## Older text

Everything below this point was written before the measurements, and describes
a problem that did not exist in the form assumed. It is kept because the
rejected options and the corrected scope are still worth having, but the plan
in "Plan" is superseded by the sections above.

## Problem

On Android, the game slows down over time and eventually freezes on some devices, particularly those with Snapdragon/Adreno GPUs. The author has stated that the renderer currently does all GL work on a single thread and that a fix requires moving the GL pipeline to a separate thread.

Note the shape of the symptom. Performance *degrades over time* rather than
simply starting out bad. A GPU that cannot keep up produces a constant poor
frame rate, not one that gets steadily worse over minutes. Progressive
degradation points at something accumulating, and the buffer-mapping hazard
under "Competing hypothesis" fits that shape better than the stall does. This
is why Phase 0 measures before anything gets built.

## Suspected root cause

Real mechanism, but not an established explanation of the freeze. Whether it
accounts for the reported behaviour is what Phase 0 exists to determine.

The Linux and Windows ports use `mesa_glthread` to move GL command submission off the main thread. This is enabled in `port/linux/src/sdl_platform.c`:

```c
#if !defined(HALO_ANDROID) && !defined(_WIN32)
    /* Mesa's GL thread: the renderer makes thousands of GL calls a frame
    and never waits for their results, so handing them to a thread of
    their own takes a fifth of the main thread's time off it. It leaves an
    explicit mesa_glthread setting alone and other drivers ignore it. */
    setenv("mesa_glthread", "true", 0);
#endif
```

On Android this path is skipped because Adreno/Mali drivers do not implement `mesa_glthread`. As a result:

1. The main game thread records rendering commands.
2. The main thread issues every GL call directly.
3. At the end of the frame, `D3DDevice_Present` waits for the GPU with `glClientWaitSync`.

The relevant code in `port/linux/src/d3d8_gl.c`:

```c
void WINAPI D3DDevice_Present(CONST RECT *source_rectangle, CONST RECT *destination_rectangle,
    void *unused, void *unused2)
{
    ...
    platform_video_swap();
    xgpu_gl_state_invalidate();
    xgpu_texture_cache_begin_frame();
#ifdef HALO_ANDROID
    host_gl_fence_frame((unsigned int)device.buffer_ring);
    device.buffer_ring = (device.buffer_ring + 1) % STREAM_BUFFER_RING;
    host_gl_wait_frame((unsigned int)device.buffer_ring);
    device.stream_buffer = device.stream_buffers[device.buffer_ring];
    device.index_buffer = device.index_buffers[device.buffer_ring];
    device.stream_offset = 0;
    device.index_offset = 0;
#else
    device.stream_offset = STREAM_BUFFER_SIZE; /* orphan next frame */
    device.index_offset = INDEX_BUFFER_SIZE;
#endif
    ...
}
```

`host_gl_wait_frame` is implemented in `port/android/host/host_gl.c`:

```c
void host_gl_wait_frame(uint32_t slot)
{
    if (slot >= FRAME_FENCE_SLOTS || !frame_fences[slot])
        return;
    /* at most a second: a lost context must not hang the game */
    glClientWaitSync(frame_fences[slot], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
    glDeleteSync(frame_fences[slot]);
    frame_fences[slot] = NULL;
}
```

This call blocks the main thread until the GPU finishes the frame. On Adreno, the driver does not offload command submission the way Mesa does, so the CPU stalls. Over time this leads to the observed slowdown and freeze.

The buffer ring is small (`STREAM_BUFFER_RING = 3` in `port/linux/src/d3d8_gl.c`), meaning the main thread often has to wait for the GPU to catch up.

## Competing hypothesis

`host_gl_buffer_write` in `port/android/host/host_gl.c` maps buffer ranges
with `GL_MAP_UNSYNCHRONIZED_BIT` while queued draws may still reference them.
The comment above it already acknowledges this hazard and works around Mali
copying whole buffers on `glBufferSubData`. On Adreno, mapping in-use storage
unsynchronized is undefined behaviour.

If that produces garbage vertex data, the result is degenerate triangles, then
overdraw, then slower frames, compounding until the game appears to hang. That
matches the reported "slows down over time and eventually freezes" much better
than a sync bubble does, and it would be *reproduced* rather than fixed by
moving GL onto another thread.

## Methods Considered

### 1. Dedicated Render Thread

Move **all** GL calls to a single background thread that owns the EGL context and window surface. The main thread runs game logic and posts a "render this frame" message to the render thread.

- Pros: Matches the canonical Android model (`GLSurfaceView`); completely removes GL stalls from the main thread.
- Cons: Requires moving the entire `d3d8_gl.c` backend to another thread, or wrapping every D3D call.

### 2. Shared-Context Worker Thread

Create a second EGL context sharing textures and buffers with the main context, and use it for background uploads or shader compilation.

- Pros: Helps with upload stalls.
- Cons: Does **not** solve the core problem because the actual draw calls still happen on the main thread; a single GL context can only be current on one thread. It was dismissed too quickly, though. It is the cheapest of the three and it targets the upload path, which the competing hypothesis says may be the actual fault. Keep it as a fallback if Phase 0 implicates uploads rather than submission.

### 3. Command-Buffer Queue (Filament-Style)

Record every GL call into a lock-free command queue on the main thread, and have a dedicated driver thread replay the commands into real GL calls.

- Pros: Cleanest multi-threaded design; decouples command recording from GPU submission.
- Cons: Requires serializing every GL call and every piece of state. The first version of this document called that a rewrite of the GL backend; see "Corrected scope" for why it is not.

### 4. A Deeper Buffer Ring

Not one of the three threading designs, but the cheapest thing to try and the
basis of Phase 1:

```c
#define STREAM_BUFFER_RING 5   /* was 3 */
```

This reduces stalls by giving the GPU more headroom. It was previously written
off as speculative without being run. It may increase input latency, and it
treats the stall as the cause, so on its own it is a no-op if the competing
hypothesis is the real fault.

## Corrected scope

The first version of this document scoped the work at the D3D entry-point
layer and concluded it was a rewrite of the GL backend. That is wrong.
`d3d8_gl.c` does not call GL on Android. It calls generated guest wrappers
which are imports named `hostgl_<function>`, resolved in the host by
`host_resolve_import` (`build/android/host/host_import_table.c`) straight to
the libGLESv3 driver entry point via `host_gl_resolve`
(`port/android/host/host_gl.c`).

That means there is already exactly one function through which every GL call
passes. The command queue belongs there, in the host, and `d3d8_gl.c` is left
alone. The change is naturally Android-only; Linux and Windows already have
`mesa_glthread`.

Two properties make this contained:

- Of the 99 functions in the Android GL list, 95 return `void`. The non-void
  ones are `glGetString`, `glGetError`, `glCheckFramebufferStatus`,
  `glCreateShader`, `glCreateProgram` and `glGetUniformLocation`. All are rare
  and all are cheap to keep synchronous; only `glCreateShader` and
  `glCreateProgram` genuinely matter, because they mint names the game stores.
- `tools/android_gl_stubs.py` already parses every prototype. Extended to emit
  a host-side thunk per function alongside the guest wrapper, it produces 99
  correctly typed thunks for free, with no ID-based dispatch: each is a direct
  call, so recording costs about what a driver call costs.

### What the queue still has to handle

The real design work is argument lifetime, not bookkeeping.

- **Draw calls are safe to defer.** They pass VBO-relative offsets, not
  pointers (`glDrawElements` at `d3d8_gl.c:3300` and `3334`,
  `glDrawElementsBaseVertex` at `3361`).
- **Uploads are not.** A queued call carrying a pointer is a deferred read of
  memory the guest is free to overwrite on the next frame. Upload payloads
  must be copied into the queue entry at record time. That is a memcpy on the
  recording thread, far cheaper than the GL call it replaces, and it is the
  same trade Filament makes.
- **Readback points are few.** What actually reads GPU state back is the
  visibility results (`glGetQueryObjectuiv`, `d3d8_gl.c:1396`, `1449`, `1452`),
  the screenshot path (`glReadPixels`, `3566`), capability queries at init
  (`glGetIntegerv`, `873`-`891`), the persistent query-buffer mapping
  (`glMapBufferRange`, `946`), and the debug overlay's `glGetError` drain
  (`2424`). Five flush-and-wait points, not a general shadow-state system. D3D
  level state already lives in `d3d8_gl.c`'s own `device` struct and stays on
  the recording thread untouched, which is why the earlier claim that "the
  main thread must maintain shadowed state for anything the game reads back"
  overstates the problem.
- **Context ownership.** The EGL context moves to the render thread. The guest
  thread must unbind it first. The render thread replays GL only and never
  enters guest code, so `host_native_thread_create` gives it a thread with no
  guest stack bookkeeping.
- **Present.** `SDL_GL_SwapWindow` becomes a queue op; ring advance and fence
  bookkeeping move to the render thread, which publishes a completion flag the
  guest can poll without blocking. Expect about one frame of added input
  latency. That is the genuine cost of this design and should be decided on
  deliberately.

## Plan

### Phase 0 - measure

Do not start the rewrite on the current evidence. Instrument, and let the
numbers pick the branch:

- Log `glClientWaitSync`'s return value and block duration per frame. Block
  time climbing toward the 1-second timeout means the GPU is genuinely behind
  and threading pays off heavily. Small block time while frames still degrade
  means it is the buffer race instead, which is cheaper and more urgent.
- Record the split between GL submission and game logic. The comment at
  `port/linux/src/sdl_platform.c:13` puts Mesa's GL thread at about a fifth of
  the main thread's time, so that is roughly the ceiling on the CPU-side win
  from threading. A profile matching it means threading buys 20%. A profile
  dominated by one-second stalls means it buys an order of magnitude. Very
  different projects.

**Implemented.** `debug.fence_stats` in `[debug]` of `config.toml` turns on the
fence telemetry in `port/android/host/host_gl.c`, and `debug.gpu_stats` now
prints a per-frame time split from `d3d8_gl.c`. Both default off. See
[port/android/README.md](../port/android/README.md#measure-the-frame) for the
settings and what the lines mean.

The two together are enough to choose a branch, because they distinguish the
racing hypothesis as well as the stalling one. A `RACE` count above zero in
the upload line means the renderer filled a buffer the GPU was still reading,
which is the competing hypothesis and not a pacing problem at all.

### Phase 1 - remove the stall without threading

A day, no new architecture. `D3DDevice_Present` (`d3d8_gl.c:3599`) fences the
slot being left and waits on the slot being entered every frame regardless of
whether the GPU is behind. Deepen the ring and wait only when the slot about to
be reused is genuinely still in flight. Correct pacing for a twenty-line
change, and the result constrains the Phase 2 design.

### Phase 2 - command queue and render thread

The real rework, as scoped under "Corrected scope" above.

### Phase 3 - validate

Screenshots compared against the current build (the port already has a
`glReadPixels` capture path), a frame-time trace over ten minutes confirming
degradation is gone rather than merely delayed, and testing on actual Adreno
hardware. There is no substitute for the device.

## Corrections to the first version of this document

- The scope was estimated at a rewrite of `d3d8_gl.c`. It is not; see "Corrected scope".
- The shared-context option was dismissed in one line. It is the cheapest of
  the three and targets the upload path.
- The larger buffer ring was called "speculative" without being run, when it is
  the single cheapest measurement available and it would have informed the
  scoping.
- "Wait for the author's threading fix" was a reasonable default while a fix
  was believed to be in flight. It is no longer one.

## Recommendation

**Superseded.** This said to build the queue in the phase order above. That was
written before the measurements, and the measurements refuted the premise it
rests on. The phase order is preserved below as the record of what was planned;
the fix that was needed is one flag, described near the top.

## Key Files

- `port/linux/src/d3d8_gl.c` — OpenGL backend
- `port/linux/src/sdl_platform.c` — SDL platform init, `mesa_glthread` toggle
- `port/android/host/host_gl.c` — Android `host_gl_fence_frame` / `host_gl_wait_frame`
- `port/android/host/host_sdl.c` — SDL host layer on Android
- `port/android/host/host.h` — guest service declarations, including `host_resolve_import`
- `tools/android_gl_stubs.py` — generates the guest GL wrappers and the import list; extended for the host thunks
- `source/main/main.c` — main game loop

## References

- EGL context sharing and threading rules: Khronos / ARM documentation
- SDL3 `SDL_GL_SHARE_WITH_CURRENT_CONTEXT`: SDL3 wiki
- Filament command-buffer architecture: Google Filament docs
