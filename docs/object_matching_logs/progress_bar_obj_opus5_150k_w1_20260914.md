# `progress_bar.obj` opus5 150K house-clean lane, wave w1 (2026-09-14)

Worker `w1:tiny_stubs`. Unit `source/interface/progress_bar.c`. Real file unchanged
(`git diff --stat` empty). Notes: `scratch/workers/tiny_stubs.md`.

## Baseline / final

exact 52 / residual 1 / unwritten 1 at both baseline and final; no parks in this unit; point guard pass;
fake scan 0 leads. branch_sweep: nothing better.

## `_code_000d16d0` (16, `ret`) — authenticated name, not landed (owner ruling)

Prior decisions: `progress_bar_obj_base_layer_reconciliation_20260907.md` forbids restoring it "as a guessed empty
noise helper or invent[ing] its caller"; the Jonas lane (76bdaf763) had landed the same body.

New evidence:
1. The name is a Bungie linker-map record, not an inference: halo-symbol-atlas tier `map` in two 2001-09-25 Xbox
   builds — `cache_exe` `0x49daa0 @progress_bar_create_noise_texture@0` (16 bytes; next
   `0x49dab0 @progress_bar_create_mask_texture@0`) and `cachebeta_xbe` `0xe0790 _progress_bar_create_noise_texture`,
   both directly after `generate_gravy_rect`.
2. January corroboration: the January atlas (exact tier) names `0xd2460 _progress_bar_create_mask_texture` (our
   `progress_bar_build_glow_texture`), so the noise/mask pair exists in January; the stub sits in January's link order
   after `generate_gravy_rect` and before `gen_cloud_coord`. Our own object shows VC7 emitting a function after its TU
   callees; the callee-free empty function stays in its source slot while the texture builders (which use late
   texture inline copies) are deferred, as in the Sept cachebeta map.
3. Alternative refuted: the bytes equal XDK `D3DDevice_BeginScene@0` / `D3DDevice_EndScene@0` copies
   (`section_infos_equal` True), but January's retained D3D8.h inline copies are strictly in header-line order in
   `progress_bar.obj` and `rasterizer_xbox.obj`; BeginScene/EndScene (lines 1327-1328) would precede
   `D3DDevice_SetRenderState` (1496), while the stub lies between `SetVertexData4f` (1809) and `Begin` (1814), whose
   intervening wrappers are non-empty.

Candidate `scratch/workers/tiny_stubs/pb_v1.c`: static empty `progress_bar_create_noise_texture(void)` called in
`progress_bar_setup` between the loading and mask textures (Sept map definition order). With
`--alias progress_bar_create_noise_texture=code_000d16d0`: EXACT, 53/1/0, all other rows unchanged, census only the
renamed owner. Not applied because the caller placement is still byte-invisible and the prior ruling explicitly
covers it. Proposal (if the owner accepts): land pb_v1.c and rename symbols.json line 3626 to
`{ "file_offset": 857808, "flags": 32, "name": "_progress_bar_create_noise_texture", "static": true },`.

## `_draw_gravy_layer` (352 vs 336) — not landed

The 16-byte delta: January reads each rectangle corner once into a variable with a register copy and a stack home
(x0 -> [ebp-4], reused at vertex 4; y0/x1/y1 with dead homes [ebp-8]/[ebp-0xc]/[ebp+0xc]); ours re-reads `rect`
memory. Shapes: named corner locals after `generate_gravy_rect` and pointer access through `&rect` are both
byte-identical to the base (VC7 copy-propagates). Reopen: a legal form that keeps corners as separate variables.

## Names-only proposal: `_IDirect3DResource8_Release@4` is `IDirect3DTexture8_Release`

Header-order law: slot 0xd1bb0 lies between D3D8.h lines 2102 and 2118; `IDirect3DTexture8_Release` is line 2107
(identical body), `IDirect3DResource8_Release` line 2020. `scratch/workers/tiny_stubs/pb_rel.c` (three
`IDirect3DTexture8_Release(progress_bar_mode.textureN)` calls without casts) emits a section equal to January's.
Must land together with symbols.json line 3639 ->
`{ "file_offset": 859056, "flags": 32, "name": "_IDirect3DTexture8_Release@4" },`. No credit.
