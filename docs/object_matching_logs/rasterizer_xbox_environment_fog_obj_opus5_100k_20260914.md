# `rasterizer_xbox_environment_fog.obj`: opus5 100K consolidated worker (2026-09-14)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`,
  target `build/split/source/rasterizer/xbox/rasterizer_xbox_environment_fog.obj`.
- Compiler VC7 13.00.9254 (XDK 3911 toolchain), repository flags
  `/O2 /Oy- /DDEBUG /Dxbox` (unchanged).
- Lane `opus/100k-consolidated-20260914` at `b8cd50113`.
- Evidence used: January COFF (code, relocations, `.rdata` literals), January
  debug map atlas `scratch/atlas/rasterizer_xbox_environment_fog.obj.txt`,
  Ghidra decompilation `scratch/ghidra/out/rasterizer_xbox_environment_fog.obj.decomp.c`,
  HCEA dx9 fog sources (screen_begin/model_submit are empty stubs there, so no
  semantic help). No Stian or punpckhdq body exists for these functions.

## Validated baseline (unchanged at finish)

`gate.py --all --forbid-emitted-symbol _point_from_line3d`: exact 24,
residual 3, unwritten 0; 5,648 of 10,352 padded code bytes exact. No data
credit is claimed.

| Function | Target size / relocs / sha16 | Candidate | Tag |
|---|---|---|---|
| `__rasterizer_environment_fog_screen_begin` | 3904 / 235 / `0746e626b346e49c` | 4016 / 235 / `a0d3fe81bdb69c8d` | size, sha |
| `_rasterizer_environment_fog_screen_active` (PARKED) | 512 / 42 / `ea96544f961a6289` | 512 / 42 / `19b4d27dcd140be1` | sha |
| `_rasterizer_environment_fog_screen_model_submit` | 288 / 20 / `f0a72d54e684a223` | 288 / 20 / `9b914f753289dc5b` | sha |

## Accepted controls

No source change is retained by this worker. `git diff` of the unit is empty.

## Experiment matrix

### model_submit (register stagger tie)

First divergence at `+0x34`: January hoists `dynamic_triangle_buffer_index`
into `edx` before `push esi`; ours hoists `shader_permutation_index`. The store
sequence itself already equals January's (Ghidra lines 520-544).

| Shape | Result | Kept |
|---|---|---|
| Stores in struct-declaration order, no `model` local, componentwise scale | 288 / 22 relocs (model global reloaded after the float copy) | no |
| Componentwise scale through the local | byte-identical to baseline | no |
| `model` local hoisted to block top | model load moves to the count increment; still sha | no |

### screen_begin (diagnostic copies only)

| Shape | Size / relocs / frame | Notes | Kept |
|---|---|---|---|
| a1: `%f[%x]` bits read in place instead of csmemcpy temporaries | 3872 / 230 / 0x19c | frame -0x28 | no |
| a2: a1 + alpha expression passed directly | unchanged, still out-of-line call | no |
| d1: a2 + `match_dassert` literals + TU-local `__forceinline` copy of `real_alpha_to_pixel32` (diagnostic, prohibited in production) | 3952 / 235 / 0x19c | relocation multiset identical to January | no |
| d2: d1 + `short offset` + `base_z < 0.0f || base_z >= 1.0f` + `set_real_point2d` reseeds | 3968 / 236 / 0x19c | base-z/offset block aligned | no |
| d3: d2 + `wind_matrix`/`vector` scoped into the wind block | 3936 / 236 / 0x194 | | no |

Lab (minimal TU): the checked-in `__inline real_alpha_to_pixel32` is never
expanded by VC7 13.00.9254 under `/O2` with any of `/Ob1 /Ob2 /Og /Ox /O1 /Oa
/Ow /G6 /GL`, nor as `static __inline`; `__forceinline` expands it.

## Measured facts for screen_begin

1. January inlines `real_alpha_to_pixel32` here (header assert literal at
   line 291, 255.0f home, 32-bit `fistp` + in-memory `shl`); all other January
   consumers call it out-of-line.
2. Assert literals use `bitmap_group->bitmaps.count`; the shared field is
   `bitmap_data`.
3. The debug `%f[%x]` block reads the float bits directly from the objects.
4. `offset` is 16-bit.
5. Base-z failure is the OR form.
6. Reseed sites are two-real inline setter packets, but January stores the
   second random into offset 0 while VC7 evaluates identical
   `real_local_random()` arguments left-to-right (lab) - unresolved.
7. Viewport width/height are `short` differences reused as x87 values.
8. January frame: texture_transforms -0x180, matrix -0x100, screen_constants
   -0xcc, wind_matrix -0x7c sharing bytes with animation_times (-0x58) and
   vsh_constants__texscale (-0x78); phase -0x34 shares with vector/point.
9. Rotation update emits `fchs; fsubr` (ours `fadd`).

## Do-not-repeat

- model_submit store permutations and `model` local placement (three shapes above).
- Any ordinary `__inline`/`static __inline`/flag spelling to obtain the
  alpha inline expansion.
- `set_real_point2d`/`set_real_vector2d`/custom setter body order and cast
  variants for the reseed evaluation order (labs l1-l4).

## Residual classification

- screen_begin: ownership/inline-boundary block (fact 1), plus reachable
  source-topology items 2-9. Cannot be strict exact under current house rules.
- model_submit: scheduling (register stagger) tie.
- screen_active: parked scheduling residual, untouched and drift-free
  (512 / 42 / `19b4d27d...` equals `measurements.base`).

## Orchestrator proposals

- A: owner decision on how January expanded `real_alpha_to_pixel32` inline in
  this object only (lab: only `__forceinline` reproduces it).
- B: rename `struct bitmap_group.bitmap_data` to `bitmaps` (January assert text),
  or accept `match_dassert` for the two literals.
- C: authentic atlas name for the parked helper is
  `_rasterizer_environment_fog_screen_is_active` (atlas line 6, exact tier).
  A source rename plus `--alias` gate is byte-neutral (24/3/0). Rename
  `config/symbols.json` offset 1398880, the parked entry, and the six
  source occurrences together.

## Reopen criteria

- screen_begin: an owner ruling on proposal A; then resume from diagnostic d3
  (facts 6-9 and the frame census).
- model_submit: new source evidence for the statement/local shape.

## Disposition

Still active. No new exact credit. Checks at finish: emitted-symbol guard
passed, park drift none, owner census identical to `build/base`, fake-match
scan 0 leads.
