# Depth precision and decals

OpenCE's reversed-depth implementation (#495) improves the separation of
nearby surfaces at a distance. GulchCE keeps `display.reversed_depth = true`
on desktop. Set it to `false` and restart to use the Xbox-style 24-bit depth
path. Android and web keep their existing conventional depth path.

A depth buffer stores the depth of the visible surface at each pixel.
Conventional depth grows toward the far plane; reversed depth grows toward
the camera. Floating-point storage has finer steps near zero, so reversing
depth improves distant precision. This does not remove all floating-point
error or guarantee separation at every distance.

## Reading route

1. [port_config.c](../../port/linux/src/port_config.c) declares the default,
   desktop scope and `HALO_REVERSED_DEPTH` override. In
   [d3d8_gl.c](../../port/linux/src/d3d8_gl.c), context creation reads it once;
   the startup log records the selected depth format.
2. `render_target_get` chooses `GL_DEPTH32F_STENCIL8` for reversed depth and
   `GL_DEPTH24_STENCIL8` otherwise. The renderer reverses depth comparison,
   clears and viewport ranges together. Read `apply_raster_state` and
   `D3DDevice_Clear` alongside this allocation.
3. [nv2a_vsh.c](../../port/linux/src/nv2a_vsh.c),
   `reversed_depth_instruction` and `nv2a_vertex_shader_to_glsl`, recognizes
   compatible projection rows. It subtracts the rows before transforming
   the position, avoiding loss of distant precision from subtracting two
   already rounded clip coordinates. Unrecognized programs fall back to
   subtracting clip z from w. `nv2a_fixed_function_glsl` handles positions
   already transformed by the CPU, including the loading screen.
4. In `d3d8_gl.c`, `prepare_draw` carries
   the enabled state and polygon offset into the cached `depth_bias`
   uniform. Both translated and fixed-function vertex shaders apply it.
   The disabled state supplies zero, including after a biased draw.

## Why decals need a separate bias

A decal sits on another surface. A small bias moves its depth toward the
camera so it remains visible. OpenCE #540 retains the Xbox's constant bias
units when using floating-point depth: a hardware floating-point polygon
offset would otherwise become much smaller at a distance. The vertex
shader adds the constant term, and hardware polygon offset retains the
slope term (the contribution from the surface's angle).

For example, a bias of eight 24-bit units moves window depth by
`8 / 16777215` toward the camera. The uniform divides by the viewport's
depth scale; the later viewport transformation cancels that scale.
A zero-width viewport range produces zero bias instead of division by zero.
The slope term scales with the render target relative to the game's
480-line view, on both depth paths, including supersampling.

The vertex correction precedes clipping. Geometry within the bias distance
of a clip plane can therefore behave differently from a hardware-only
offset. Fragment programs that write their own depth, such as `DOT_ZW`
effects in [nv2a_psh.c](../../port/linux/src/nv2a_psh.c), override interpolated
vertex depth and retain their existing conversion. These are limits to
test when investigating a particular effect, not promises that every
shader has identical precision.

## Ownership and checks

These changes belong to the existing shared OpenGL backend: target format,
shader emission and render-state translation cannot be implemented in an
independent game feature module. They do not change map loading or route
Xbox and CE data through MCC adapters. No simulation or network packet
changes are involved. The existing MCC environment emitter, optional model
lighting and shield-honeycomb uniforms retain their ownership and guards.
In particular, the draw cache includes both honeycomb and depth-bias inputs.

The setting is fixed for a graphics context's lifetime; changing the config
requires a restart. Render targets and shader programs use that context's
mode. Ordinary map transitions continue through the existing resource
cleanup, without a new per-map depth setting or feature state.

[test_reversed_depth.py](../../tools/test_reversed_depth.py) exercises the
production bias calculations, viewport ranges, render scales and fixed-function
shader generation. With `HALO_TEST_GPU=1`, GLFW and PyOpenGL installed, it
uses a hidden OpenGL 4.5 context to compile the 67 stock vertex programs
with depth and lighting variations, and checks depth readback, bias removal
and 4x MSAA resolves in both formats. Transform feedback (reading the vertex
shader's output directly) checks closely spaced surfaces through 1,000 world
units using the production projection program.
[test_plasma_model_shader.py](../../tools/harness/tests/test_plasma_model_shader.py)
checks the production translations for desktop, Android and web with
dynamic lighting enabled and disabled. These tests supplement game captures;
they do not establish visual compatibility across every map, GPU or effect.

The [integration validation record](../reversed_depth_validation.md) separates
the November 2026 build, GPU and game results from checks that remain unavailable.
