# `render_sprite.obj` Opus5 150K house-clean lane, wave w1 (20260914)

## Scope and provenance

- Translation unit `source/render/render_sprite.c`; January target `build/split/source/render/render_sprite.obj`.
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Worker scope: this one `.c` file. No header, config
  or other source edited.
- Compiler: XDK 3911 Microsoft C/C++ 13.00.9254, whole-TU `tools/campaign/gate.py --source` compiles.
- Evidence: January COFF bytes/relocations (authoritative); frame censuses computed on January's own bytes; the
  exact `_yaw_vectors` (real_math.obj) as the aggregate-slot witness; prior ledgers `render_sprite_obj.md`,
  `render_sprite_obj_jonas_public_salvage_20260821.md`, `render_sprite_obj_opus5_fresh_graphs_20260914.md`,
  `render_cameras_obj_opus5_100k_waveD_20260914.md` (the `_pi` header proposal); HCEA
  `halocea-review/src/blam/networking/build_sprite*.c` (semantics only).

## Baseline and final real-file gate

Both are `== exact 7  residual 3  unwritten 0  (of 10 listed)` with the `_point_from_line3d` guard passing. No
`config/parked.json` entry exists for this unit. **Nothing was landed; `git diff -- source/render/render_sprite.c`
is empty.**

| Function | Padded | Relocs | Row | Outcome |
| --- | ---: | ---: | --- | --- |
| `_build_sprite_transform_origin_and_direction` | 304 | 27 | residual [sha] | owner-blocked (original-bug UB) |
| `_build_sprite_rotational` | 640 | 38 | residual [size 656!=640, sha] | source SOLVED in scratch; owner + header blocked |
| `_build_sprite` | 1552 | 59 | residual [size 1536!=1552, sha] | owner-blocked; allocation chain decoded, not solved |

## `_build_sprite_transform_origin_and_direction`

Re-verified: the callee has no `sub esp`; the screen-space arm passes `lea eax,[ebp+0xc]` (address of the
`transformed_origin` parameter home) as `render_camera_screen_to_view`'s `view_vector`, which writes three reals
(`render_cameras.c:261-296`). `build_sprite` pushes exactly three stack arguments, so the third write lands in the
caller's frame. The exact spelling `(real_vector3d *)&transformed_origin` is undefined behaviour and is not landed.
It is the private-ABI prerequisite (ebx=data, esi=untransformed_origin, edi=untransformed_direction; stack flags,
transformed_origin, transformed_direction) of both callers. Owner ruling required.

## `_build_sprite_rotational`: aggregate-local frame law (new)

January frame 0x24: `transformed_origin` [-0x24,-0x18), `transformed_axis_of_rotation` [-0x18,-0xc) and a third
12-byte object [-0xc,0) whose only referenced member is at +4 ([ebp-8]: the equator angle, fst/fmul/fld/fld).
Holes at -0xc and -4 are elided aggregate members, exactly as in the exact `_yaw_vectors` (`real_vector3d cross`
with only `.k` homed). The three 12-byte objects tie at reference density 4/12, so declaration order after the axis
puts it nearest EBP. A scalar `angle` (density 1.0) is packed into a parameter home instead, which produced the whole
26-block residual (frame 0x18, angle in [ebp+0x18], edge flags in [ebp+0x1c]).

Scratch measurements on `v1up.c` (real file + UB transform + TU-local `_pi` float-literal simulation):

| Shape | Result |
| --- | --- |
| r1: edge flags declared first in the polar block | 640, 24 blocks (store placement fixed) |
| p1: separate raw-angle local | no frame change |
| q2: `real_euler_angles2d angles` (8 bytes) | frame 0x20, sha |
| q1: `real_euler_angles3d angles; angles.pitch = angle_between - _pi/2` | **EXACT** |
| q3: q1 + `angles.yaw = arctangent(axis.j, axis.i)` passed to build_sprite | **EXACT** |
| q5: q1 + `angles.roll = arctangent(axis.j, axis.i)` | **EXACT** (best body) |
| q4: sprite rotation moved into `angles.roll` | sha (3 blocks) |
| q3 on the M_PI base | reloc-identity only: `__real@3ecf817b` vs January `__real@3ecf817a` |
| q3 with the legal transform | 656 (ABI) |

Authenticity: `angle_between_vectors3d(origin, axis) - pi/2` is the viewer's latitude relative to the rotation
axis's equator (pitch) and `arctangent(axis.j, axis.i)` is the axis's on-screen rotation about the view direction
(roll); yaw (rotation about the axis) is represented by the frame index, so it stays unassigned. Evidence beyond "it
matched": the frame census on January's bytes independently proves a 12-byte aggregate with only its +4 member homed.
Candidate: `scratch/workers/render_sprite/q5.c` (body `rot_q5.body`). Owner census vs `build/base`: only the literal
COMDAT `__real@3ecf817b` becomes January's `__real@3ecf817a`. Fake scan: 0 leads.

## `_build_sprite`: decoded allocation chain (unsolved)

On the UB + q5 + `_pi` scratch base: 517/517 instructions, 1536 vs 1552, 95 blocks. January frame objects (census
on January bytes): point[-0xc], vertex_index[-0x10], group/bitmap_group[-0x14], sprite[-0x18], origin[-0x24],
cosine[-0x28], a spill slot [-0x2c] shared by `bitmap` and the x87 temp `sine*offset_x`, sine[-0x30],
bounds[-0x48,-0x30), the hoisted `flags&4` value [-0x4c] (the 4 extra bytes), three debug points, basis, direction.
January spills offset_y to color's home and pre-computes `sine*offset_x` before x; u therefore takes mode's home;
the v-mirror value loses that home and takes a frame slot; `flags` keeps edi across the fade/pixel calls while
`data` stays in [ebp+8]; the debug tail pre-computes three vertex pointers into esi/edi/ebx. The operand load order
of `bounds.x0 + registration_point.x` follows value-numbering age, not source operand order (c2).

Rejected shapes: y before x (b1/b2, 514 insns), y operand swap (b3, identical), `rotate_vector2d` (b4, out-of-line
call, 1568/60), loop scalars at block scope (b5, identical), debug vertex pointer locals (b7, copy-propagated),
direct `group->vertices` in the tail (b8, re-fetches), `reg.x + x0` (c1, identical), `u - x0 - reg.x` (c2, fsub tree).

## Orchestrator proposals

1. Owner ruling on the transform original-bug spelling (UB). Consumer: `source/render/render_sprite.c` only.
2. `source/math/real_math.h`: `#define _pi 3.14159265359f` (text already proposed by render_cameras waveD; this
   object is its second independent witness, `__real@3ecf817a`). Needs a full-board rebuild and per-function diff.
   Consumers: every file including real_math.h (`grep -rl 'real_math.h' source`).
3. After both: apply `rot_q5.body` and the transform spelling together, then re-gate (rotational +640, transform +304).

## Do-not-repeat

- rotational: scalar/declaration-order/scope shapes, 2D euler angles, raw-angle split, sprite rotation as a member.
- build_sprite: the eight b/c shapes above; HCEA order; pointer locals; hoisted shader local.
- transform: no legal spelling produces `lea [ebp+0xc]` with no frame.

## Reopen criteria

- transform: owner admits the original-bug site.
- rotational: transform ruling + `_pi` float literal header change -> land `rot_q5.body`.
- build_sprite: transform ruling + a source recipe that reproduces the offset_y spill / pre-computed `sine*offset_x`
  temp and keeps `data` in memory through the vertex loop.
