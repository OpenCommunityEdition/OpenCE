# `render_sprite.obj` — Opus5 150K house-clean lane, wave w3 (2026-09-14)

Worker scope: `source/render/render_sprite.c` only. No header, config or build file was edited. **Nothing landed;
`git diff -- source/render/render_sprite.c` is empty.** Notes: `scratch/workers/w3_render_sprite_render_particles_etc.md`;
probes: `scratch/workers/w3_render_sprite_render_particles_etc/`.

## Baseline and final real-file gate

Both `== exact 7  residual 3  unwritten 0 (of 10 listed)`, `_point_from_line3d` guard passing, rows identical. No parks.

| Function | Padded | Relocs | Row | Outcome |
| --- | ---: | ---: | --- | --- |
| `_build_sprite_transform_origin_and_direction` | 304 | 27 | residual [sha] | owner-blocked (original-bug UB), new HCEX corroboration |
| `_build_sprite_rotational` | 640 | 38 | residual [size 656!=640, sha] | scratch EXACT **without** the rejected `_pi` header change; blocked only by the transform ruling |
| `_build_sprite` | 1552 | 59 | residual [size 1536!=1552, sha] | owner-blocked; not re-ground |

## New evidence

1. **HCEX.pdb (SHIP, 2011) local records** (`DIA2Dump -sym <fn> HCEX.pdb`): `build_sprite_transform_origin_and_direction` has the six
   parameters and **no local**, while the same PDB lists every aggregate local of `build_sprite` (bounding_rectangle, origin, basis,
   direction, point, p0..p3) and `build_sprite_rotational` (origin, axis_of_rotation). The screen-space arm therefore had no
   view-vector local in the later source either. That independently corroborates the address-of-parameter original bug shown by
   January's `lea eax,[ebp+0xc]`. January `render_camera_screen_to_view` writes `[edi]`, `[edi+4]` and `[edi+8]` (re-verified), so the
   spelling is a 12-byte overrun (UB) and stays owner-blocked. No legal packing exists: the dead parameter homes in the screen arm are
   `[ebp+8..0x14)`, which would give `lea [ebp+8]`, not `+0xc`.
2. **Float-precision local-constant law (VC7 13.00.9254, measured).** January's `__real@3ecf817a` equals `1/(float(pi/2))^2`. The true
   value `4/pi^2` and every `_pi` expression fold to `...7b`, because VC7 folds `(real)M_PI` arithmetic in extended precision. A `real`
   (or `real const`) **local** initialised with `_pi/2` is propagated through its float storage. So `x - quarter_circle` loads
   `__real@3fc90fdb`, and `a*a/(quarter_circle*quarter_circle)` folds to `__real@3ecf817a`. This removes the rotational body's
   dependence on the rejected real_math.h `_pi` float-literal packet (150K HANDOFF P8).

## Measurements

Base: `scratch/workers/render_sprite/v1u.c` = real file + UB transform only, unchanged M_PI `_pi`.

| Shape | Result |
| --- | --- |
| t1: w1 `rot_q5` body (`real_euler_angles3d angles`) + `real quarter_circle = _pi/2;` | **640/38 EXACT** (TU: exact 9 residual 1) |
| t2: scalars (`angle`, `roll`) + the same local constant | 640 sha: frame 0x18, angle packed into `[ebp+0x18]` (the aggregate is required) |
| t3: t1 with `real const quarter_circle` | **640/38 EXACT** |

Candidate `t3.c` differs from the real file only in the transform arm and the rotational body. Owner census vs `build/base`: the only
change is the literal COMDAT `__real@3ecf817b` -> January's `__real@3ecf817a`, which the January object defines. Fake scan: 0 leads.

## `_build_sprite` status

HCEX's aggregate set equals the retained source, so the 0xb0 vs 0xac frame gap is not a missing aggregate. January homes the
v-mirror invariant `flags&4` in a new slot `[ebp-0x4c]` because `data` stays memory-homed and `flags` keeps `edi`. It also
pre-computes `sine*offset_x` while spilling `offset_y` to the color parameter home. No law-backed shape was available (A12 preheader
order already matches; R3 forbids operand-order steering), so no compile was spent.

## Orchestrator proposals

1. **Owner ruling (strengthened)** on the transform original-bug spelling
   `render_camera_screen_to_view(&render.camera, &render.frustum, (real_point2d const *)untransformed_origin, (real_vector3d *)&transformed_origin);`
   with a `/* BUG (preserved for exact matching): ... */` comment citing the January `lea` evidence and the HCEX no-local record; a
   corrected build passes a local vector whose result is discarded. Consumer: `source/render/render_sprite.c` only.
2. After (1): land `scratch/workers/w3_render_sprite_render_particles_etc/rot_t3.body` in the same commit: +304 transform, +640
   rotational. **No real_math.h change is needed any more.**

## Do-not-repeat

- rotational: scalar-angle forms (t2) and all `_pi` header spellings; the constant is solved by a local.
- transform: no legal spelling exists.
- build_sprite: w1 b0-b8/c1-c2, fresh-graphs bs1/bs2, `rotate_vector2d` (HCEX has no 2D aggregates, and the call stays out of line).

## Reopen criteria

- transform + rotational: owner ruling (1).
- build_sprite: ruling (1) plus a law that moves `data` out of a callee-saved register across the vertex loop.
