# rasterizer_geometry.obj: Opus5 150K house-clean lane, wave w3 (2026-09-14)

Unit `source/rasterizer/rasterizer_geometry`. The real file is unchanged (`git diff --stat` empty).

## Baseline and final

| gate (guard) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 16 | 3 | 0 |
| final | 16 | 3 | 0 |

All three residuals are parked:
- `_uncompress_int32_to_real_vector3d`: 144/9
- `_compress_real_vector3d_to_int32`: 544/38
- `_compress_real_vector3d_to_int32_clamp`: 512/38

Final checks:
- Park drift: 0.
- Owner census: matches base.
- Fake scan: 0 leads.

## `_uncompress_int32_to_real_vector3d` — SKIPPED-EXHAUSTED

Only one instruction differs, as `alndiff` on the real object shows. January emits the i-row `fadd __real@3f800000` between the k field's `shr eax,0xb` and `shl eax,0x16`. Ours emits it after the shift pair.

Every earlier i/j integer interleave is identical.

- **Shapes already measured in w2 (6):** member-wise stores, `1.0f + x*s`, product orders, `>>=` folding into j and k, definition position.
- **Laws checked:** A11, A12, A20 and the x87 laws. None predicts the placement of the integer scheduler relative to the x87 chain.
- **Result:** no new shape compiled.

## `_compress_real_vector3d_to_int32` / `_clamp` — SKIPPED-EXHAUSTED

The round-trip copy differs in load order:

| | load/store order |
| --- | --- |
| January | loads all three returned components (`ecx`, `edx`, `eax` from `[eax]`) first, then stores i, starts the first assert's x87 (`fld v2.i; fsub v->i`), stores k, performs the deferred `add esp,8`, stores j |
| ours | interleaves each load with its store around the same x87 block |

Frame, relocations and all other instructions are equal.

- **w2 controls already covered:** named or scoped pointer, v2 first, by-value diagnostic with a +12 frame.
- **Laws:** no w2 law maps to this.
- **Result:** no new shape compiled.

## Proposals

None. Reopen criteria unchanged (parked.json / w2 ledger).
