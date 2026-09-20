# The frame-mismatch census: 101 KB of structural work, 41 KB of it parked as ties

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, baseline
`6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.

**Scope note.** This document is a *measurement*. It reads `build/split`
(January) and `build/base` (ours) and writes nothing. No source file outside
Lane A's eleven allowed translation units was touched, and none was compiled.
Everything below is reproducible with `python scratch/orch/boardframes.py`.

## The claim

The campaign already holds this law ([[frame-layout-lever]], proven three times
on `units.obj`):

> A differing `sub esp,N` is **structural**. The *set* of homed locals differs -
> a local's scope or type - and it is source-reachable. It is never an
> allocation tie, and it can masquerade as hundreds of "register/slot
> permutation" diff rows.

Nobody had ever applied that law as a **board-wide filter**. Doing so takes one
pass over the object files, no recompilation, and it finds:

| | functions | padded bytes |
|---|---:|---:|
| frame mismatch, board-wide | **56** | **101,056** |
| ...of those, currently **parked** | **35** | **41,104** |
| the `+12` class alone | 8 | 34,432 |

**Thirty-five parked functions carry a frame-size mismatch.** Twenty-eight of
them are parked `unclassified`, two `instruction-scheduling`, two
`register-allocation`, three otherwise. By the campaign's own law every one of
those has an **unexamined source-reachable lead** sitting in its prologue, and a
park that says "compiler tie" over a differing `sub esp,N` is mis-stating the
evidence.

This is not a claim that 41 KB will close. It is a claim that 41 KB was
classified as *unreachable* without anyone reading the one number that decides
whether it is reachable.

## Why the histogram is the convincing part

    delta (ours minus January)    count
        +4                         15
        -4                         10
       +12                          8
        +8                          4
        -8                          4
       +48                          2
       +20                          2
       -16                          2
       +16, +24, +40, +68           1 each
       -12, -20, -24, -64, -104     1 each

Forty-one of the fifty-six are `±4`, `±8` or `+12` - **one or two locals'
worth**. If these were wholesale reconstruction failures the deltas would be
large and unpatterned. They are not. They are single missing or extra locals,
which is exactly what the law predicts and exactly what a source edit can fix.

The one obvious outlier is worth calling out for the opposite reason:
`_game_set_game_variant_from_name` (`source/game/game`) is a **96-byte** function
whose frame is **104 bytes smaller** than January's. A function cannot be 96
bytes and lose 104 bytes of frame to a local; that body is not the same function
January compiled, and it should be triaged as a reconstruction gap, not a tie.

## The `+12` class - 8 functions, 34,432 bytes

Twelve bytes is exactly `sizeof(real_point3d)` / `sizeof(real_vector3d)`.

| function | unit | B | park |
|---|---|---:|---|
| `_ai_debug_render_actor` | `source/ai/ai_debug` | 24,976 | - |
| `_update_alien_scout_physics` | `source/units/vehicles` | 2,464 | - |
| `_rasterizer_sun_glow_draw` | `source/rasterizer/xbox/rasterizer_xbox_lights` | 2,352 | parked |
| `_observer_update_positions` | `source/camera/observer` | 1,568 | parked |
| `_pre_evaluator_attack` | `source/ai/actor_firing_position` | 1,120 | parked |
| `_ai_test_line_of_sight` | `source/ai/ai` | 1,008 | parked |
| `_projectile_collision_test_line` | `source/items/projectiles` | 512 | - |
| `_editor_camera_set_scripted` | `source/camera/editor_flying_camera` | 432 | parked |

Physics, rasterizer, camera, projectile collision, line-of-sight, AI debug
rendering. Every one is geometry code, and in every one **we home twelve bytes
more than January**. The working hypothesis is that we bind a vector or point
result to a named local where January consumes it without binding it - the
inverse of the accessor-binding law, applied to an aggregate.

Two independent corroborations from the slot census
(`python scratch/orch/plus12.py`):

- **`_ai_debug_render_actor`.** The entire deep half of the frame is a *uniform*
  `+0xc` shift - `0x2b0 -> 0x2bc`, `0x310 -> 0x31c`, `0x410 -> 0x41c`,
  `0x510 -> 0x51c`, `0x710 -> 0x71c`, `0x810 -> 0x81c` - with an **identical
  callee multiset** and an already-identical section size (24,976) and
  relocation count (1,905). One inserted 12-byte cell explains the whole
  picture, and `alndiff`'s 1,473 "differing regions" are overwhelmingly the same
  instruction at a different `[ebp-X]`.
- **`_pre_evaluator_attack`.** January homes one 4-byte cell at `[ebp-0x34]`
  (3 references). We home cells at `[ebp-0x38]` and `[ebp-0x3c]` (2 references
  each) *and* four bytes at `[ebp-0x40]` that are **never referenced at all** -
  the elided-aggregate-member signature. Callee multiset identical.

## How to use this

Run it first, on any object, before classifying anything as a tie:

```bash
python scratch/orch/boardframes.py
```

It prints the histogram, the parked cross-reference, and every row with
January's and our `sub esp,N`. It is cheap because it never recompiles: it
disassembles at most a dozen prologue instructions per function.

Then, for a flagged function:

1. `python scratch/orch/plus12.py`-style slot census: the **slot-set delta with
   reference counts**, and the **callee multiset**. An identical callee multiset
   says the gap is locals, not logic.
2. A frame cell that is homed and **never referenced** is an elided aggregate
   member - look for an aggregate local whose members are not all read.
3. A *uniform* shift of the deep slots localises the inserted cell to a depth
   shallower than the first shifted slot.
4. Remember: only promotion to **function** scope changes the frame. VC7
   flattens inner scopes for slot assignment and declaration **order** is inert.
   Local **type shape** is a first-class allocator input - merging or splitting
   an aggregate re-packs everything.

## What this does not say

- It does not say these functions will close. A frame fix can leave real
  divergence behind, and a frame **match** proves nothing on its own - once the
  sizes agree you must read the slot census, not the prologue.
- It does not authorise editing anything outside Lane A's eleven translation
  units. Twenty-one of the fifty-six rows, and most of the parked ones, are
  outside this lane's scope. They are reported for whoever owns them.
- The park entries it flags are not thereby wrong about the *residual*; they are
  wrong, or at least unproven, about the *class*. Reopening one means doing the
  slot work, not deleting the park.

## Provenance

`scratch/orch/boardframes.py` (census + park cross-reference),
`scratch/orch/framecensus.py` (Lane A portfolio census with slot-set deltas),
`scratch/orch/plus12.py` (slot and callee census for the `+12` functions). Full
output kept at `scratch/orch/boardframes.txt`. 482 object pairs scanned.
