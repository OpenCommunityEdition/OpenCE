# `xbox_sound_cache.obj` — opus5 150K lane, wave w2 (2026-09-14)

## Baseline and result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| real file, baseline (guard on) | 15 | 3 | 0 |
| real file, final (unchanged) | 15 | 3 | 0 |
| scratch park-reopen candidate `scratch/workers/xbox_texture_cache_cache_files_windows_etc/q3.c` | **16** | 2 | 0 |

The real file is unchanged because all three rows are parked. The candidate is the current real file with two
changes: the `__sound_cache_sound_request` body, and `#include <xtl.h>`.

Checks on the candidate:
- Only the request row changes.
- The `_point_from_line3d` guard passes.
- Park drift occurs only on the reopened function, which moves to the January hash.
- The fake-match scan reports 0 leads.

**Owner census.** The census shows the body hash plus the three XDK `IMAGE_COMDAT_SELECT_ANY` tables that `<xtl.h>`
brings in:
- `_D3DPRIMITIVETOVERTEXCOUNT` (88 bytes);
- `_D3DSIMPLERENDERSTATEENCODE` (328 bytes);
- `_D3DTEXTUREDIRECTENCODE` (16 bytes).

These tables have no incoming relocation and are discarded at link. They are the reviewed candidate-only
duplicate-COMDAT case already present in xbox_texture_cache, cache_files_windows and event_manager.

## Per-function outcomes

| function | padded | outcome |
| --- | ---: | --- |
| `__sound_cache_sound_request` | 352 | **PARK-REOPEN-PROPOSED** (strict EXACT in scratch) |
| `_render_inverse_transform_screen_point` | 256 | OWNER-BLOCKED (`_point_from_line3d` policy) |
| `_sound_cache_debug_render` | 368 | OWNER-BLOCKED (same policy; one x87 operand-order pair remains) |

### `_sound_cache_sound_request`

January keeps a `result` byte that is never written, homed at `[ebp-1]`, and reads it at both failure exits. That
means `result` is live around the wait loop.

This is the same idiom as the exact in-TU-family `_texture_cache_bitmap_get_hardware_format`:
`do {...} while (!hardware_format && block);`. The reconstruction therefore uses:
- a single `return result;` at the end;
- `result = TRUE` in the loaded branch;
- `SwitchToThread()` in the else branch;
- `while (!result && block)` as the loop condition.

**Second defect.** `SwitchToThread` had no declaration in the TU. The implicit declaration (C4013) produced a cdecl
`_SwitchToThread` reference, while January references `_SwitchToThread@0`. The fix is to include the genuine XDK
owner `<xtl.h>`, as the sibling cache TUs do.

Cleanup in the same body:
- the redundant `if (!load)` wrapper around the `load || !...` asserts was removed;
- the start-loading `if` now has braces;
- `cache_sound` is scoped to the loop body.

**Shapes tried:**

| shape | result |
| --- | --- |
| q1: loop | 352/25, `[reloc-identity]` |
| q2: q1 plus `<xtl.h>` | EXACT |
| q3: q2 plus cleanup | EXACT (chosen) |

**Park criterion.** The park asked for a natural same-compiler donor explaining the result lifetime and return
topology. The exact texture-cache request loop is that donor, and no `volatile` is used.

### `render_inverse_transform_screen_point`

This is identical to the texture-cache copy. With `point_from_line3d(&render.frustum.world_vertices[4],
global_zero_vector3d, 1.0f, world_position)` (sr7.c) it is EXACT, but only without the guard, because
`_point_from_line3d` is emitted.

### `sound_cache_debug_render`

On the sr7 helper, the renderer was restructured the same way as the exact texture renderer (sd1):
- locals scoped inside the debug predicate;
- the pointer-walking loop counting down from 2;
- separate `s0.x`, `s0.y`, `s1.x`, `s1.y` statements.

That leaves one differing pair: the first dot-product term. January emits `fld world_vector.k; fmul forward.k`;
ours emits the operands in the opposite order.

These variants were all identical to sd1:
- sd2: `dot_product3d` arguments swapped;
- sd3, sd4, sd5: `world_vector` declared at function top, in the loop block, or first among the locals.

## Orchestrator proposal

The `_point_from_line3d` COMDAT policy ruling gates this helper (256) and `sound_cache_debug_render` (368), together
with the texture-cache pair.

## Reopen criteria

- **Helper:** the ruling.
- **Debug render:** the ruling, plus an evidence-led explanation for the k-term load order.

Worker notes: `scratch/workers/xbox_texture_cache_cache_files_windows_etc.md`.
