# `rasterizer_xbox.obj` — Opus5 250K lane, wave w3c (2026-09-15)

Lane `opus/250k-house-clean-20260915`, worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
Scope: `source/rasterizer/xbox/rasterizer_xbox.c` only.

## Result

**No source change landed.** `git diff --stat` for the file is empty.

| gate (real file, `--all --forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 94 | 1 | 0 |
| final | 94 | 1 | 0 |

Guard: pass. `scratch/parkcheck.py`: 1 park, drift 0. `tools/fake_match_scan.py`: 0 review leads.

`__rasterizer_window_begin` — the other w2 residual — is exact at this HEAD (the w2 `"static": true` symbols.json packet for
`_suppress_window_begin_end` / `_previous_window_index` landed), so the object now has a single non-exact row.

## `_rasterizer_set_model_skinning` (320 B, parked) — skipped; the 2026-09-06 hold binds (R13)

Target 320 padded / 18 relocs / `ba181c535da3f3a4…`; park base 320 / 18 / `3b0471f353677f94…`; objdiff 95.92233.

`alndiff` at this HEAD: the first 42 instructions (the three asserts and the loop guard) are identical, and from `+0x8a` the
entire loop is a systematic EAX/ECX exchange:

```
January: movsx eax,dx ; mov ecx,eax ; imul ecx,ecx,0x34 ; fld [ecx+edi] ; …
         lea eax,[eax+eax*2] ; shl eax,4 ; add eax,_rasterizer_xbox_d3d_globals
ours:    movsx ecx,dx ; mov eax,ecx ; imul eax,eax,0x34 ; fld [eax+edi] ; …
         lea ecx,[ecx+ecx*2] ; shl ecx,4 ; add ecx,_rasterizer_xbox_d3d_globals
```

The exchange costs exactly one byte, because `add eax,imm32` has a short encoding and `add ecx,imm32` does not (the
`_rasterizer_xbox_d3d_globals` relocation sits at `+0xa4` in January and `+0xa5` in ours). Everything else — the twelve
`fld st(0)` / `fmul` / `fstp` transposed scale-folded constant stores, both integer position copies, the statistics tail and
all 18 relocations — is identical. This is a pure register tie, which WORKER_BRIEF section 6 forbids grinding.

### Law detector run

- **A5 counter-law** ("a pointer local used four or more times is genuine") protects the existing `matrix` local, which has
  thirteen uses, so the alias-removal family is out.
- **A20** (store order = struct declaration order), **A31(c)** (field copies need a parameter-based base) and **A24**
  (parameter-home offer) do not fire: the store order already matches, there is no field-copy base question, and the
  function has one parameter whose home is not reused by either build.
- The only known closure remains the historical donor's `real scale = skinning->node_matrices[node_index].scale;` placed
  **before** the `matrix` pointer. It re-verified strict EXACT in wave w2, but it is a declaration-order steer with no
  provenance and was explicitly HELD by `rasterizer_xbox_obj_three_natural_opus_closures_20260906.md`; laws_w3 **R13**
  ("holds and rejections recorded in older ledgers bind later waves … spending arrangements a hold forbids is itself a
  defect") makes that hold binding here. It is therefore not proposed and no shape was spent.

**Reopen criterion:** an owner ruling on the held donor declaration order, or authentic evidence that January read `scale`
through the full `skinning->node_matrices[node_index]` accessor before binding the `matrix` pointer.

## Evidence used

January split COFF bytes and relocations; `tools.campaign.alndiff`, `tools/campaign/gate.py`, `scratch/parkcheck.py`,
`tools/fake_match_scan.py`; `config/parked.json`; `docs/object_matching_logs/rasterizer_xbox_obj_opus5_150k_w2_20260914.md`
(shapes s1-s5), `…_obj_opus_safe_reconciliation_20260903.md`, `…_obj_three_natural_opus_closures_20260906.md` (the hold) and
`…_obj_interrupted_opus_dirty_reaudit_20260904.md`. No supplied binary was executed.
Worker notes: `scratch/workers/w3c_rasterizer_lights_shader_transparent_chicago_preprocessor_etc.md`.
