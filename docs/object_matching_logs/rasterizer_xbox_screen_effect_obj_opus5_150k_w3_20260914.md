# `rasterizer_xbox_screen_effect.obj` - opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_screen_effect.c`. The target is `build/split/source/rasterizer/xbox/rasterizer_xbox_screen_effect.obj`, built with VC7 13.00.9254 and project default flags.
- This is a second pass over both parks, with the w2 laws (`scratch/w2/laws_w2.md`, A1-A22 and R1-R12) run as detectors.
  - All work was in scratch only. The real file is unchanged: `git diff --stat` is empty at HEAD `210aa8f81`.
- Evidence used:
  - January target COFF;
  - w2 candidates and the w2 verifier record (`scratch/w2/w2-results.json`);
  - HCEA `rasterizer/dx9/_rasterizer_screen_flash.c` and `_rasterizer_screen_effect_ps.c`;
  - `HCEX.pdb` via `DIA2Dump -sym`;
  - the same-author exact sibling `rasterizer_xbox_lights.c::rasterizer_sun_glow_convolve`;
  - the CL `/E` declaration census and the w3 `CL /Zs /W3` warning census.
- Worker notes: `scratch/workers/w3_rasterizer_xbox_screen_effect.md`. Scratch files: `scratch/workers/w3_rasterizer_xbox_screen_effect/`.

## 2. Baseline

The gate reported `== exact 11  residual 2  unwritten 0`. The `_point_from_line3d` guard passed and there was no park drift.

| Function | Target size / relocs / sha16 | Real body | Status |
|---|---|---|---|
| `__rasterizer_screen_effect` | 3888 / 290 / d67a06f0c52e6846 | 3872 / 290 / 40f164b5 | PARKED |
| `__rasterizer_screen_flash` | 1344 / 118 / f1d3e84eebfa355c | 1344 / 118 / 74b68c67 | PARKED |

`_rasterizer_screen_effect_set_texture_transforms` was inherited as exact and was not touched.

## 3. `__rasterizer_screen_effect` - NOT LANDED (register-allocation tie)

Start point: `b0.c`, which equals w2 `effect_best_g67_8.c`. It measures 3888 / 290 / 478687ef9ed1ee73 and was reproduced.

### Residual, measured precisely

The comparison uses `normdiff.py`, a difflib pass with registers and frame slots normalised. It finds 13 blocks, and every one is allocation:

| Variable | January | Ours |
|---|---|---|
| `pass` | cached in ecx; reloaded after calls at 0x22b, 0x414, 0x53e | ebx at the loop top; memory in the stage loop |
| `source_target` | ebx, no home | esi, with a home at [ebp-0x14]; reloaded at 0x540 after the `movsx esi,di` stage temporary |
| `parameters` | esi; reloaded at 0x251 and 0x541 | ebx inside the loop |
| ebx save/restore | push/pop confined to the pass loop (0x1fc / 0xee8) | function-wide |

The zero register is simply whichever register `source_target` frees after the stage loop. No instruction, operand or relocation differs apart from these.

### Law detectors

| Law | Result |
|---|---|
| A1 | No TU-owned globals; only two statics, both exact callees. |
| A3 | Already single exit. |
| A5 | No one-use alias. The parameter reuse is proven by `mov [ebp+8],esi`. |
| A6 | Frame 0x40 is equal. |
| A7 | The C4013/C4133 census for this TU is clean. |
| A9 | No switch. |
| A10 | `mov byte [ebp-1],1` proves `boolean success = TRUE` at function scope. |
| A12, A13, A14, A20 | Hoist order, XDK inline decisions, success-chain cleanups and store orders are already equal. |
| DECLARATION-COUNT (diagnostic only) | +1 to +8 dummy declarations are inert, and removing the 11 TU-local offset-assert typedefs is inert. The CL `/E` census shows only CRT duplicate prototypes. |
| Declaration order and scope (diagnostic only, 21 variants) | Each loop-block local moved first or last, reversed order, the `pass_count` initializer placed last, and pass/source/destination/stage at function scope: all inert. |

### Shapes tried (4 of 5)

| Shape | Evidence | Result |
|---|---|---|
| S1: `source_target = (pass & 1) ? ... : ...;` and the destination as conditional expressions | Exact same-author sibling `rasterizer_sun_glow_convolve` | a54e5ef9, 1031 insns. VC7 goes branchless (and/neg/sbb/setne), so January's `test cl,1; jne` if/else statement form is proven. Rejected. |
| S2: `else if (pass & 1) {odd} else {even}` | HCEA topology | 70c6b2a4. The odd arm is laid out first (`je`), but January falls through to the even arm, so `!(pass & 1)` comes first. Rejected. |
| S3: `destination_target = source_target = NONE;` | In-function chained-bounds precedent (w2 fix 6) | 478687ef, inert. The mirror spelling s3b gives af8dad18 with the wrong store order. |
| S4: `const long video_overbright_table[3]` | HCEX.pdb local | 478687ef, inert. |

**Reopen criterion.** One of:
- a January-side fact that changes the IR definition order of `source_target` against the loop web of `parameters`, for example a recovered macro for the stage-texture binding ladder or for the success chain;
- a C2 allocator decode that explains why `pass` receives a caller-saved cache.

Start from `scratch/workers/w3_rasterizer_xbox_screen_effect/b0.c`.

**Do not repeat:**
- S1 to S4;
- declaration order, scope and count probes;
- typedef scaffolding removal;
- w2 h1-h4 and k1.

## 4. `__rasterizer_screen_flash` - SKIPPED (owner/verifier ruling pending; no new lever)

The w2 reopen (`scratch/workers/rasterizer_xbox_screen_effect/a8b.c`) was rejected under R5, for two reasons:
- the unassigned default arm is a preserved uninitialised read, which brief section 5 bans;
- the defined `constant_pixel = inverse_flash_pixel;` default arm (`scratch/verify/rasterizer_xbox_screen_effect/probe_default_inverse.c`) is equally exact, so the bytes cannot tell the two apart.

This wave looked for semantic evidence about the unsupported-type fallback:
- **HCEA retail DX9.** A `(type-1) <= 5` range check skips the switch entirely. The `default:` that HCEA attaches to tint is a jump-table artefact of a release build without asserts, not a fallback.
- **HCEX.pdb.** The flash locals are DX9-only: `psh_constants`, `vsh_constants__screenproj` (float[5][4]), `num_passes` and `vertex_data`.
- **Stian and punpckhdq.** Neither has a body.
- **Sapien and the Halo PC demo.** These exist only as binaries and were not executed.

January fact recorded from the bytes: the `rgb_input` and `alpha_input` initialisers (`xor edi,edi; xor ebx,ebx` at 0xf4) are exactly the values the default arm keeps. The join pixel is the only value the default arm redefines, through the reload of [ebp-4]. This does not decide between the unassigned form and the inverse form.

No shapes were spent.

**Reopen criterion.** An owner ruling on the preserved uninitialised default, or authoritative source for the unsupported-type fallback value.

## 5. Final disposition

- Real file: unchanged. The final gate reports `== exact 11  residual 2  unwritten 0`, and the point guard passed.
- `scratch/parkcheck.py`: 2 of 2 parks OK, 0 drift.
- Owner census: identical to `build/base`.
- `tools/fake_match_scan.py`: 0 leads.
- Newly exact: none.
- No park-reopen, orchestrator or header/config proposals.
