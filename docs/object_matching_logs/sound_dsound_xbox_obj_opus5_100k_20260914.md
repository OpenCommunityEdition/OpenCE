# `sound_dsound_xbox.obj` Opus 5 100K consolidated lane (2026-09-14)

## Scope and provenance

- Translation unit `source/sound/sound_dsound_xbox.c`, target
  `build/split/source/sound/sound_dsound_xbox.obj` (January 2002 Xbox debug build).
- Compiler: XDK 3911 `CL.Exe` 13.00.9254, unit flags `/O2 /Oy- /DDEBUG /Dxbox`, repository
  include graph unchanged (XDK 3911 `DSound.h`, `_XTL_VER 3911`).
- Lane `opus/100k-consolidated-20260914`; worker scope is this one `.c` file. No header,
  configuration, park, symbol or tooling edit.
- Evidence read before experiments: all eight earlier `sound_dsound_xbox_obj_*` ledgers,
  `git log --all` for the file (e481d5142, 0c18c70e0, 608c66380, b654ee4f9, b8c442f9d,
  f54ec267f, ecd0fff2d, e830efbd7), `branch_sweep.py` (6 blobs, none better than the
  current 35 exact), the January map atlas and Ghidra output prepared in
  `opus5-30k-fresh-graphs-20260914/scratch/`, Stian `sound_dsound_xbox.c:790-890`, HCEA
  `src/blam/sound/` (no DirectSound 3D backend), punpckhdq (symbol listing only).

## Validated baseline

Gate at lane HEAD, `_point_from_line3d` guard passed: **35 exact / 4 residual / 0 unwritten**
of 39. No parked entries. The final real-file gate is identical and the source file is
unmodified (`git diff --stat` empty).

| Function | Target size / relocs / nsha16 | Base size / relocs / nsha16 | First divergence |
| --- | --- | --- | --- |
| `_dsound_set_listener_properties` | 816 / 63 / `2c07c059201ef12a` | 784 / 63 / `39384b94a33e124c` | +0x5b: 2nd/3rd float args staged on the x87 |
| `_channel_set_location` | 752 / 43 / `b509812c41541745` | 720 / 43 / `88e79a8ebcc18a51` | +0xee: same float-argument family |
| `_dsound_initialize` | 720 / 52 / `d73c7ae0918c2fc4` | 720 / 52 / `11e94e43be496945` | +0x29f: cross-jump survivor among four failure arms |
| `_channel_queue_packet` | 640 / 37 / `eedc03a5428064c8` | 640 / 37 / `b864c6f9b8f3ea40` | +0x2c: ebx/edi colour swap; +0xe9 zero pin |

## Accepted controls

None added. Nothing reached strict exact with an authentic spelling.

## New evidence

- **Cross-object precedent for the float-argument staging shape.** The January shape
  `fld [p+4]; mov r,[p]; ...; sub esp,8; fstp [esp+4]; fld [p+8]; fstp [esp]; push r`
  (first float argument integer-copied, the rest staged) also occurs in
  `_rasterizer_screen_geometry_submit_vertex` (EXACT, `rasterizer_xbox_dynavobgeom`) and in
  `__rasterizer_debug_immediate_line` (code-identical, `rasterizer_xbox_debug`). Both reach
  the XDK through the `D3DINLINE` (`static __forceinline`) forwarding wrappers
  `IDirect3DDevice8_SetVertexData2f/4f`, whose wrapper parameters become compiler temps.
  Found with `scratch/workers/sound_dsound_xbox/scan_pairstage.py` (whole `build/split`
  scan for `sub esp, 8 | fstp dword ptr [esp + 4] | fld dword ptr`).
- **The analogy does not transfer.** XDK 3911 `DSound.h` declares `IDirectSound_SetPosition`,
  `IDirectSoundStream_SetPosition` etc. as plain `STDAPI` prototypes with no C forwarding
  layer, and January's relocations name those functions directly. A TU-local forwarding
  wrapper that returns the HRESULT (the listener uses the result) was measured and does
  not reproduce the shape (below).
- **Atlas names.** The map atlas gives original names for 13 of this TU's statics (listed
  in the orchestrator proposal of `scratch/workers/sound_dsound_xbox.md`), for example
  `_channel_set_location` = `_dsound_channel_set_location`,
  `_create_inanity_channel` = `_dsound_fix_rear_speakers`,
  `_dsound_record_error` = `_interrupt_time_error`. No byte effect is expected; the rename
  is a coordinated `symbols.json` + source change for the orchestrator.

## Experiment matrix (scratch copies only; all reverted)

| Shape | Function | Result |
| --- | --- | --- |
| `x_cm6`: fresh-graphs `(real)(double)` on all three position args (re-measured) | listener | 800 / 63 / `078322718c10eb9e`; position block byte-identical to January; mechanism only, not source |
| `m_inline`: TU-local `static __inline HRESULT` wrapper around `IDirectSound_SetPosition` | listener | 800 / 63 / `2321edcf40828a69`; frame `0x30 -> 0x38`; wrapper params get frame homes (`[ebp+8]`, `[ebp-4]`, `[ebp-8]`) |
| `m_force`: same wrapper as `static __forceinline` (prohibited; D3DINLINE mechanism test only) | listener | identical to `m_inline` |

Sibling check: 35 exact rows stayed exact in every variant; point guard passed.

`_channel_set_location`, `_dsound_initialize` and `_channel_queue_packet` received no new
shapes: the first is blocked on the same spelling as the listener, the other two are
tie residuals whose three-shape budgets were spent by the fresh-graphs lane and no new
evidence appeared.

## Do-not-repeat list

- Everything in the fresh-graphs do-not-repeat list (cast-only / folding spellings,
  `real`/`double`/point locals, comma or identity arithmetic, failure-first nesting,
  shared dispose, per-arm `success` stores, declaration order and `sound` scope).
- TU-local forwarding wrappers (`__inline` or `__forceinline`) around the DirectSound 3D
  calls: measured, frame homes, no match. `__forceinline` is prohibited anyway.
- Hoping for a C inline layer in the tree's DSound.h: there is none (3911, plain STDAPI).

## Residual classification

- **Listener and channel location: source spelling unresolved.** Measured: any
  per-coordinate FP-typed argument expression that VC7 later folds reproduces January's
  staging exactly; a forwarding wrapper that returns the HRESULT does not. Inference: the
  original applied some per-component conversion (macro or conversion helper) to the
  DirectSound vector arguments; its text is not in any available source or header.
- **`_dsound_initialize`: scheduling/layout tie** (cross-jump survivor choice).
- **`_channel_queue_packet`: register-colour tie** (after the structural q1 order).

## Reopen criteria

1. Original DirectSound coordinate-conversion text (Bungie macro/helper) or a
   January-era XDK `DSound.h` with a C forwarding layer for the 3D setters. Then apply it
   uniformly to both functions; for `_channel_set_location` also re-check the fresh-graphs
   `p5` store order and the field-first obstruction compare.
2. `_dsound_initialize`: evidence that the four failure arms differed in source.
3. `_channel_queue_packet`: a source-evidenced use-weight lever moving the permutation web
   to ebx.

## Disposition

Still active, `NonMatching`: 35/39 exact, unchanged. No new exact credit, no park, no
fuzzy landing, no source change. Final gate object:
`scratch/workers/sound_dsound_xbox/final.obj` (owner census equal to `build/base`,
fake-match scan 0 leads).
