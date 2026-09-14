# `rasterizer_xbox_screen_effect.obj` - opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_screen_effect.c`, target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_screen_effect.obj`.
- Compiler: XDK VC7 13.00.9254 `CL.Exe /nologo /c /O2 /Oy- /DDEBUG /Dxbox` (project default flags; no per-file override).
- Provenance used: January target COFF (instructions, relocations, assertion line literals), HCEA
  `rasterizer/dx9/rasterizer_screen_effect_set_texture_transforms.c` (later build; semantics only).
  No map-atlas or Ghidra file was prepared for this object. Stian's lift covers only the reciprocal helper
  and the screen flash; punpckhdq has the symbol list only.
- Worker: waveB `rasterizer_xbox_screen_effect` (claimed in `scratch/WORK_CLAIMS.json`).

## 2. Validated baseline (lane HEAD)

`== exact 10  residual 3  unwritten 0` (13 functions).

| Function | Target size / relocs / sha16 | Base size / relocs / sha16 | Status |
|---|---|---|---|
| `__rasterizer_screen_effect` | 3888 / 290 / d67a06f0c52e6846 | 3872 / 290 / 073549d7cf4ac30e | PARKED (locked) |
| `__rasterizer_screen_flash` | 1344 / 118 / f1d3e84eebfa355c | 1344 / 118 / 74b68c67df01d6b9 | PARKED (locked) |
| `_rasterizer_screen_effect_set_texture_transforms` | 2128 / 86 / c24f18204f9cfada | 2080 / 86 / 4b0ee325d2bf5c81 | residual (this lane) |

First meaningful divergence of the texture-transform residual at baseline: offset 0x3 - the frame
(`sub esp,0x108` vs `0x100`) and the entry sequence (January `push ebx; mov ebx,eax; test ebx,ebx`,
i.e. `parameters` arrives in EAX; ours loads it from `[ebp+8]` after evaluating the stub-bitmap initializer).

## 3. Accepted controls

None retained in production. The real file is byte-identical to lane HEAD (`git diff --stat` empty), because
the admission rule lands only strict-EXACT changes to an already-written residual.

## 4. Experiment matrix (whole-TU gate, all siblings stayed 10/10 exact)

Structural decode of January bytes, applied cumulatively to a copy (`scratch/workers/rasterizer_xbox_screen_effect/`):

| Shape | Evidence | Size / relocs | Result |
|---|---|---|---|
| v1: asserts 47/48 first, body in a nested block (stub `bitmap_data` initializer after the asserts) | January reads window bounds only after the second `system_exit`; VC7 emits block-entry initializers before statements; line budget leaves no room for ~15 declarations before line 47 | 2096 / 86 | private ABI now EAX (matches); frame 0x104 |
| v2: separate `viewport_size` vector for centring subtrahends | January subtrahend is `[ebp-8]/[ebp-4]`, distinct from `screen_size` `[ebp-0x50]` | 2112 / 86 | copy-propagated, reverted |
| v3: named `screen_width/height` scalars | same | 2112 / 86 | copy-propagated, reverted |
| v4: subtract `screen_bitmap.width/height`; `unit_scale` stores after the size conversions | CSE'd short-to-real temps give exactly January's `[ebp-8]/[ebp-4]`; January interleaves the `1.0f` stores with the filds | 2112 / 86 | frame 0x108 and 0x000-0x547 aligned |
| v5: window-bounds centre sums `x1 + x0`, `y1 + y0` | January loads `+0x42` before `+0x3e` and `+0x40` before `+0x3c` | 2112 / 86 / 670f5c43594c95aa | best candidate; 10 differing sites |

Tie probes on v5's remaining sites (all inert or worse, reverted): radius/scale operand swap; radius declared at
outer scope; radius assignment hoisted before the if-chain (ABI lost, 2080); direct `parameters->convolution_radius`
(ABI lost, 2096); `real_local_random()` inline; named random local; reordered random products; `constants[k][0]`
operand (frame layout broke); `(double)` casts (2144). Lab compiles with the same CL (scratchpad) showed VC7
reassociates float products independent of source order, always loads a named float local before an aggregate field,
and emits `fld X; fmul st(1)` plus a deferred `fstp st(0)` after a call only when the call result has a second use.

## 5. Do-not-repeat list

- Operand order spellings of `scale * radius` and of `scale * random * size` (reassociated; inert).
- Radius declaration scope (block vs outer) and named/unnamed random locals.
- Hoisting the radius load before the blur/warp chain, or replacing the named radius by the parameter field:
  both change the callee register pressure and lose the EAX private ABI.
- Casts to `double`, `constants[k][0]` substitutes (break the frame-slot ranking).

## 6. Residual classification

- `_rasterizer_screen_effect_set_texture_transforms` (best v5, not landed): **x87 operand-rank scheduling**.
  Measured: 8 blur sites where January emits `fld scale; fmul radius` and VC7 emits `fld radius; fmul scale`
  (size-neutral), and 2 video-jitter sites where January emits `call real_seed_random; fld scale; fmul st(1) ...
  fstp st(0)` and VC7 emits `fmul scale` (8 bytes, the whole 2112/2128 padded gap). Everything else, including
  the frame, slot layout, ABI, and all 86 relocations, is aligned. Inference (unproven): January's source gives the
  radius local and the random value a different rank or a second use that no natural spelling tested reproduces.
- Coupling (measured): v5 changes parked `__rasterizer_screen_effect` from `push ebx; call` to `mov eax,ebx; call`,
  the January form (`mov eax,esi; call`), so its normalized hash drifts (073549d7 -> 40f164b5, still 3872/290).
- `__rasterizer_screen_effect`, `__rasterizer_screen_flash`: parked and locked; not touched.

## 7. Reopen criteria

- Start from `scratch/workers/rasterizer_xbox_screen_effect/v5.c` (patch `v5.patch`), not the production body.
- Reopen when a same-compiler donor or authoritative source shows what (a) ranks the blur radius local after
  the scale-vector fields and (b) keeps `real_seed_random`'s result live across the jitter statement.
- When the function closes, the orchestrator must refresh `config/parked.json` `__rasterizer_screen_effect`
  `measurements.base` to the post-ABI hash (the caller moves toward January); that refresh is config-only.

## 8. Disposition

No new exact functions. Final real-file gate `== exact 10  residual 3  unwritten 0` (equal to baseline),
`_point_from_line3d` guard passed, park drift passed, owner census clean, fake-match scan 0 leads.
`_rasterizer_screen_effect_set_texture_transforms` stays an active residual (not parked), with a
structurally complete best candidate recorded above.
