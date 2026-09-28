# Android Graphics Threading Investigation

## Problem

On Android, the game slows down over time and eventually freezes on some devices, particularly those with Snapdragon/Adreno GPUs. The author has stated that the renderer currently does all GL work on a single thread and that a fix requires moving the GL pipeline to a separate thread.

## Root Cause

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

## Methods Considered

### 1. Dedicated Render Thread

Move **all** GL calls to a single background thread that owns the EGL context and window surface. The main thread runs game logic and posts a "render this frame" message to the render thread.

- Pros: Matches the canonical Android model (`GLSurfaceView`); completely removes GL stalls from the main thread.
- Cons: Requires moving the entire `d3d8_gl.c` backend to another thread, or wrapping every D3D call.

### 2. Shared-Context Worker Thread

Create a second EGL context sharing textures and buffers with the main context, and use it for background uploads or shader compilation.

- Pros: Helps with upload stalls.
- Cons: Does **not** solve the core problem because the actual draw calls still happen on the main thread; a single GL context can only be current on one thread.

### 3. Command-Buffer Queue (Filament-Style)

Record every GL call into a lock-free command queue on the main thread, and have a dedicated driver thread replay the commands into real GL calls.

- Pros: Cleanest multi-threaded design; decouples command recording from GPU submission.
- Cons: Requires serializing every GL call and every piece of state. `d3d8_gl.c` is over 3600 lines and exports 100+ D3D functions. This is effectively a rewrite of the GL backend.

## Scope of a Command-Buffer Rewrite

A command-buffer approach would need to intercept and serialize:

- All draw calls (`D3DDevice_DrawIndexedPrimitive`, etc.)
- All state changes (`D3DDevice_SetRenderState`, `D3DDevice_SetTexture`, `D3DDevice_SetVertexShader`, etc.)
- Buffer/texture creation, updates, and reads
- Shader compilation and program linking
- Visibility queries and fence operations
- `D3DDevice_Present` and swap behavior

In addition:

- The main thread must maintain shadowed state for anything the game reads back.
- `D3DDevice_Present` must flush the command buffer and wait for the frame.
- `D3DDevice_GetVisibilityTestResult` must flush and read back GPU results.
- Buffer/texture lock/unlock paths need staging memory for the render thread to upload.

This is a multi-day project, not a small patch, and requires testing on affected hardware.

## Quick Experiment Considered

Increasing the buffer ring size:

```c
#define STREAM_BUFFER_RING 5   /* was 3 */
```

This might reduce stalls by giving the GPU more headroom, but it is speculative and may increase input latency.

## Recommendation

Wait for the author's threading fix. They have already identified the same root cause (single-threaded GL on Android) and stated they are uploading the fix. A full multi-threaded renderer is the correct solution, and attempting it without a Snapdragon test device risks introducing rendering bugs or crashes.

## Key Files

- `port/linux/src/d3d8_gl.c` — OpenGL backend
- `port/linux/src/sdl_platform.c` — SDL platform init, `mesa_glthread` toggle
- `port/android/host/host_gl.c` — Android `host_gl_fence_frame` / `host_gl_wait_frame`
- `port/android/host/host_sdl.c` — SDL host layer on Android
- `source/main/main.c` — main game loop

## References

- EGL context sharing and threading rules: Khronos / ARM documentation
- SDL3 `SDL_GL_SHARE_WITH_CURRENT_CONTEXT`: SDL3 wiki
- Filament command-buffer architecture: Google Filament docs
