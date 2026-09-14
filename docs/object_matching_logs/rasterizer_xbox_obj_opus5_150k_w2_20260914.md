# `rasterizer_xbox.obj` — Opus5 150K wave w2 (2026-09-14)

## Baseline

The unmodified real file gates at **93 exact / 2 residual / 0 unwritten**, with the guard passing. Both residuals are parked:
- `__rasterizer_window_begin`: 416 bytes / 39 relocations, target sha `fecabaa8a9edb25e`;
- `_rasterizer_set_model_skinning`: 320 bytes / 18 relocations, target sha `ba181c535da3f3a4`.

The real file is unchanged.

## `__rasterizer_window_begin` — park reopen proposed

### Prior history

- Donor `ff1144479` (branch `opus/30k-rasterizer-widgets-20260905`, not an ancestor) closed this function by making `suppress_window_begin_end` and `previous_window_index` static.
- `opus1_reconciliation_intake_20260906.md` held that change as an "unresolved linkage disagreement … Establish original ownership before accepting".
- `rasterizer_xbox_obj_three_natural_closures_20260906.md` kept the storage/BSS ownership question outside admission.

### New ownership evidence

- **Complete PDB publics.** The January PDB publics cover every non-XDK function in this object. In the object's .bss tail, `_global_d3d_device` (4581584) and `_global_rasterizer_data` are public, but nothing is public at 4581592 or 4581596.
- **First-party calibration.** The `static` globals in Bungie's `random_math.c` have no publics. Of the 60 data names with no public, 59 are referenced only by their own object.
- **No outside references.** No other January object references either name.
- **Codegen agreement.** January loads `parameters->window_index` before storing the byte global. VC7 may hoist that load only when the store target has internal linkage and its address is never taken. The same law closed four rasterizer_cinematics parks in this wave.

### Candidate

`scratch/workers/rasterizer_xbox_rasterizer_xbox_hardware_geometry_etc/xbox_v1.c` (sha256 `0a70a24dc69f11014001697bf2acb5ac86614989cce59777f078620dbdb09dbd`) is the current file plus `static` on lines 871–872. Checks:
- whole-TU gate: 94/1/0;
- the only row change is window_begin going from residual to EXACT;
- guard passed;
- parkcheck: only window_begin drifts (to the target), and skinning is OK;
- census: no candidate-only owner;
- fake scan: 0 leads.

### Proposal

Edit `config/symbols.json` lines 22637 and 22638 in place, adding `"static": true` to `_suppress_window_begin_end` and `_previous_window_index`. The only consumer is `rasterizer_xbox.obj`.

### Caveat for the verifier

- **Candidate layout.** VC7 places the two statics as `previous@0`, `suppress@2` in its own 3-byte .bss.
- **January layout.** January has `suppress@+8`, `previous@+12` behind `global_d3d_device`.
- **Why it does not block the reopen.** Function bytes and relocations are unaffected. The object's data layout is already non-matching (the D3D aggregate gap).

## `_rasterizer_set_model_skinning` — not landed

- **The residual.** The loop's index copy lands in EAX/ECX the wrong way round. The exchange propagates to the constant-upload push and to the statistics `+=` tail.
- **Donor closure is still exact but not admitted.** The donor fix is `real scale = skinning->node_matrices[node_index].scale;` placed before the `matrix` pointer. It re-verified as EXACT. It remains a declaration-order steer with no provenance, which is why the three_natural lane held it, so it is not proposed.

Shapes tried:

| Shape | Result |
| --- | --- |
| s1: no `scale` local | 320, sha differs |
| s2: all direct-index access | 368 |
| s3: constants, matrix, scale | 320, sha differs |
| s4: matrix, constants, scale | 320, sha differs |
| s5: static linkage on the D3D private aggregate plus the window statics | no effect |

**Reopen criterion:** authentic or same-compiler evidence for reading `scale` through the direct index, or an owner ruling on the donor order.
