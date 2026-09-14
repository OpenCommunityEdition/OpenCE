# `rasterizer_xbox_screen_effect.obj` - opus5 150K house-clean lane, wave w2 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_screen_effect.c`, target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_screen_effect.obj`. VC7 13.00.9254, project default flags.
- Both non-exact rows are parks (`config/parked.json`, class `unclassified`). This wave re-investigated them in scratch only.
  The real file is unchanged (`git diff --stat` is empty).
- Evidence used:
  - January target COFF: instructions, relocations and `.rdata` texts.
  - Ghidra/atlas dumps (`scratch/ghidra/out`, `scratch/atlas`).
  - HCEA `rasterizer/dx9/_rasterizer_screen_flash.c` and `_rasterizer_screen_effect_ps.c`. These come from a later DX9 build and were used for topology only.
  - HCEX.pdb `DIA2Dump -sym`. The flash locals are DX9-only, and `_rasterizer_screen_effect` is a 4-byte stub whose parameter is `const struct rasterizer_screen_effect_parameters *`.
  - In-tree exact precedents: `rasterizer_xbox_widgets.c`, `rasterizer_xbox_lights.c`, `rasterizer_xbox_debug.c`, and `memory/data_encoding.c` (`data_decode_memory`).
- Worker notes: `scratch/workers/rasterizer_xbox_screen_effect.md`. Scratch candidates are in `scratch/workers/rasterizer_xbox_screen_effect/`.

## 2. Baseline (lane HEAD `ea9175577`)

The gate reported `== exact 11  residual 2  unwritten 0`. The `_point_from_line3d` guard passed and there was no park drift.

| Function | Target size / relocs / sha16 | Real body | Status |
|---|---|---|---|
| `__rasterizer_screen_effect` | 3888 / 290 / d67a06f0c52e6846 | 3872 / 290 / 40f164b599db7d92 | PARKED |
| `__rasterizer_screen_flash` | 1344 / 118 / f1d3e84eebfa355c | 1344 / 118 / 74b68c67df01d6b9 | PARKED |

`_rasterizer_screen_effect_set_texture_transforms` was inherited as exact from the 100K lane and was not touched.

## 3. `__rasterizer_screen_flash` - strict EXACT in scratch, PARK-REOPEN proposed

Candidate: `scratch/workers/rasterizer_xbox_screen_effect/a8b.c`. It is the current real file plus only the flash body. The flash function measures 1344 / 118 / f1d3e84eebfa355c, which equals the target.

At baseline two defects remained.

### A. Viewport reciprocals

January converts both short sizes before the `1/width` `fdivr` and defers `1/height`. The exact form drops the `one_over_width` / `one_over_height` locals and uses CSE'd expressions:

```c
vertex_constants[0][0] = 1.0f / viewport_width * 2.0f;
vertex_constants[0][3] = -1.0f - 1.0f / viewport_width;
vertex_constants[1][1] = 1.0f / viewport_height * -2.0f;
vertex_constants[1][3] = 1.0f / viewport_height + 1.0f;
```

This is the same spelling as the exact screen-projection blocks in `rasterizer_xbox_widgets.c` (lines 431-434) and `rasterizer_xbox_lights.c` (lines 246-253).

### B. Join variable and impossible default arm

After `system_exit`, January's default arm reloads the tint arm's value: `mov esi,[ebp-4]` at 0x356. The esi web at the join is used in three places:
- the `constant_0[0]` store;
- the `D3DRS_BLENDCOLOR` argument in max, min and invert;
- the tint load.

The exact source uses a per-case `pixel32 constant_pixel`. Each case assigns it right before the `D3DRS_BLENDCOLOR` use, or at the same point for lighten and darken. The default arm (`match_vassert(FALSE, "### ERROR unsupported screen flash type")`) leaves it unassigned. The body carries this comment:
`/* BUG (original): ... A corrected build should assign constant_pixel = flash_pixel ... */`.

This is the law recorded in `exact_match_acceleration_playbook.md` under "Original bugs". The landed `data_decode_memory` precedent has the same mechanism: a reload after a fatal default arm means the original default arm left the local unassigned.

### Shapes tried

| Shape | Result |
|---|---|
| v0 (real file) | 5 diff blocks |
| e1 / la (tint call before assignment; height subtraction first) | inert |
| e4 / e5 (drop the `rgb_input` / `alpha_input` initialisers) | 1328 bytes, worse; January's `xor edi` / `xor ebx` prove the initialisers |
| lb / lc (`one_over_height` computed next to `one_over_width`) | 11 blocks, worse |
| m3 (inline viewport expressions in `SetVertexData2s`) | 1376 bytes / 122 relocs, worse |
| b1 (reciprocal expressions) | site A closed |
| a1 / a3 / a4 (tint assignment placement) | site B remains |
| a2, a8, a8b (per-case `constant_pixel`, default unassigned) | EXACT |
| a5 (`constant_pixel = flash_pixel` before the switch) | frame 0x78, 44 blocks |
| a6 (`constant_pixel = inverse_flash_pixel` before the switch) | 1360 bytes |
| a9, the safe corrected form (a8b plus `constant_pixel = flash_pixel;` in default) | not exact; only the missing reload differs (2 blocks) |

### Checks on a8b

- Every other row is unchanged: the gate goes from 11 exact to 12 exact.
- The point guard passed.
- `__rasterizer_screen_effect` does not drift.
- Owner census is identical to `build/base`.
- The fake-match scan reports 0 leads.

### Policy note for the verifier

Section 5 of the brief forbids preserving an undefined-behaviour path. The read here happens only if `system_exit(-1)` returns. The playbook's original-bug gate explicitly admits this mechanism when three conditions hold, and all three are met here:
- January's control flow proves the read;
- defined alternatives were measured as non-matching (a5, a6, a9);
- the safe form is recorded.

The in-tree precedent `data_encoding.c:420` is already landed. An owner ruling is needed.

### Park criterion

The park asked for "authoritative January source/local records or a natural compatible-compiler donor explaining the remaining allocation and schedule". Its premise is refuted: the residual was two source-shape defects, not allocation. Both are explained by an established law and by in-tree exact precedents.

## 4. `__rasterizer_screen_effect` - structurally complete, allocation tie (NOT landed, stays parked)

Best candidate: `scratch/workers/rasterizer_xbox_screen_effect/effect_best_g67_8.c`, which includes the flash reopen. It measures 3888 / 290 / 478687ef9ed1ee73.

- The padded size and the relocation count now equal the target's.
- After register-name normalisation, the instruction stream is identical to January's: 1028 instructions each, and 0 blocks differ in anything other than registers.

These new January-proven defects were fixed cumulatively:

1. **Noise alpha.** The alpha is `PIN(parameters->video_noise_intensity, 0.0f, 1.0f)`. January stores the zero first behind `test ah,5; jp`, which is the cseries `PIN` ladder.
2. **Light-enhancement `final_combiner_inputs_efg`.** The expression is `(parameters->convolution_mask ? 8 : 0) << 8`. This matches January's `and 8; shl 8`, the shape of the `PS_COMBINERINPUTS` g slot.
3. **Alpha-input selections.** The two `filter_*_uses_convolution_mask ? 0x68 : 0x20` selections are assigned to locals before the `constant_0` / `constant_1` `real_alpha_to_pixel32` calls. January loads them and applies `sbb` at 0x9d9, before the calls, and stores `alpha_inputs[]` after the calls (store/call law). This change alone closed the 16-byte size gap.
4. **Error texts.** The `rasterizer_error` texts use `VSDE_VERTEX`, as in January's `.rdata`.
5. **Duplicated mask arms.** The `pass==1` non-convolution section has two complete mask arms. The convolution section sets `texture_modes = 0x8421` in both mask arms; January hoists these common stores between `cmp` and `je`.
6. **Constant vertex bounds.** The bounds are `x0 = y1 = -1.0f; x1 = y0 = 1.0f;`. January's store order is y1, x0, y0, x1; separate statements in that order give identical bytes.
7. **Wave C literal fixes.** The p_f14 changes were reused: `pass==0` and similar assertion texts, and the line-584 `match_vassert`. Literal COMDAT owners now carry January's names.

### Progression

| Candidate | Size | Diff blocks |
|---|---|---|
| f14 | 3872 | 96 |
| g4 | 3888 | 80 |
| g5 | 3888 | 76 |
| g67 | 3888 | 13 structural |
| g67_8 | 3888 | 73 total, 11 register-only |

Allocation probes on g67_8 were all inert:
- h1-h4: source/destination targets, `stage`, and viewport sizes plus `vertex_bounds` scoped to their natural blocks;
- k1: `const` on `parameters`.

### Residual (register allocation only)

| Variable | January | Ours |
|---|---|---|
| `pass` | cached in ecx between calls, home `[ebp-8]` | ebx at the loop head |
| `source_target` | ebx, pushed at 0x1fc | esi |
| `parameters` | esi throughout | ebx inside the loop |
| zero register | ebx | esi |

Ours also needs one extra spill slot.

### Checks on g67_8

- Siblings and flash stay exact.
- The point guard passed.
- Owner census shows no new owner; only the literal names change, and they now match January's.
- The fake-match scan reports 0 leads.

**Reopen criterion:** an explanation, from legal C or new evidence, of why VC7 gives `pass` a caller-saved cache and gives `source_target` ebx. Start from `effect_best_g67_8.c`.

**Do not repeat:** local scoping of the pass-loop variables, `const parameters`, or re-deriving fixes 1-7.

## 5. Final disposition

- Real file: unchanged. The gate reports `== exact 11  residual 2  unwritten 0`, the point guard passes, parks show no drift, owner census is clean and the fake-match scan has 0 leads.
- Newly exact in the real file: none.
- PARK-REOPEN proposed:
  - Function: `__rasterizer_screen_flash`.
  - Candidate: `scratch/workers/rasterizer_xbox_screen_effect/a8b.c`.
  - Requires: an owner ruling on the original-bug default arm, the unpark, and a full build.
  - If admitted, `__rasterizer_screen_effect` does not drift, so its park base stays 3872 / 290 / 40f164b5.
- Recorded for a future reopen: the structurally complete `__rasterizer_screen_effect` candidate above.
