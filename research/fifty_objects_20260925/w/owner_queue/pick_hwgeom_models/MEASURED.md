# MEASURED - owner_queue/pick_hwgeom_models (base cdc8ebd3, Halo objects 379)

All numbers below were measured in this wave on cdc8ebd3. Raw outputs are in `hwgeom/` and `models/`.

## Method

- **Split emulation.** `build/tools/csplit.exe -i cachebeta.exe -p cfg_<v> -o split_<v>` ran on config copies made by
  in-place line edits (`tools/mk_cfg.py`); symbols.json is never re-serialised.
  - `split_ctl`, the unmodified copy, equals `build/split` byte for byte (833/833).
  - Every variant split differs from `split_ctl` in exactly one object (`tools/splitdiff.py`).
- **Gate.** `tools/gate_split.py` is the cdc8ebd3 `tools/campaign/gate.py` plus `--split-root` and `--tmpdir`; the
  compiler and flags are identical. The `_point_from_line3d` emitted-symbol guard was on.
- **Object audit.** `tools/object_audit_split.py` is `scratch/tools/object_audit.py` plus `AUDIT_SPLIT_ROOT`.
- **Storage and surplus.** `tools/pdb_storage_cand.py` checks storage against the cachebeta publics.
  `tools/surplus_identity_cand.py` checks candidate-only code COMDATs against January's selected copies.
  `scratch/tools/provider_link.py` runs VC7 Link in both orders against the base provider objects.
  `tools/fake_match_scan.py` ran on each candidate source.
- **Report level.** objdiff 3.3.1 `report generate` ran on a one-shot mini project (`objdiff_proj/`): targets are the
  emulated splits, bases are the candidate objects.
- **Apply check.** `tools/verify_apply.py` does two things for each packet. It runs `git apply --check` in the repo,
  and it applies the patches in sequence to scratchpad copies and byte-compares the result with the measured files.
  Result: ALL OK (`verify_apply.txt`).

## Item 1 - rasterizer_xbox_hardware_geometry (17 January functions)

| candidate | vs split | gate --all | object_audit | pdb storage | code COMDAT surplus | provider_link | fake scan | objdiff 3.3.1 |
|---|---|---|---|---|---|---|---|---|
| production (cdc8ebd3) | build/split | 17/17 EXACT | FAIL(7): 7 wrapper rows static in source, external in symbols.json | 13 disagreements | - | - | 0 | 898/898 code, 584/584 data |
| **A** IDirect3DResource8_MoveResourceMemory | split_hgA | **17/17 EXACT** | **PASS** (25 symbols, 0 differ) | **0** | 0 | **PASS** (6 rows) | 0 | 898/898, 584/584 |
| **B** D3DVertexBuffer_MoveResourceMemory | split_hgB | **17/17 EXACT** | **PASS** (25, 0) | **0** | 0 | **PASS** (6 rows) | 0 | 898/898, 584/584 |
| fallback hg0 (no owner pick) | split_hg0 | 17/17 EXACT | PASS (25, 0) | 2 (`_code_00158450/460` external, January static) | 0 | PASS | 0 | - |

- **A vs B.** Every Halo section is byte- and relocation-identical: vertex_buffer_new 5442bf32, triangle_buffer_new
  e90e066b, and the two deletes 1d9f1c5d / 5075d6eb. Both 0x158460 bodies are sha 2a2f950f (a pure `ret 8`), and the
  .text emission order is the same. The only difference is the one symbol name.
- **Provider-link rows.** All six rows (A and B alike) are pre-existing production surplus: three pooled literals
  (count>0, the unsupported-triangle-buffer error, vertex_buffer) and three SDK select-any tables
  (D3DPRIMITIVETOVERTEXCOUNT, D3DSIMPLERENDERSTATEENCODE, D3DTEXTUREDIRECTENCODE), plus `.drectve`.
- **Data.** Every January .rdata section is `ok` in object_audit. No parks, semantic entries or admission rejections
  exist for the unit.
- **Admission queue.** At cdc8ebd3 `tools/audit_object_admission.py` lists the unit as "audit-coff-ownership-before-admission".

## Item 2 - models (12 January functions, 136-byte .bss)

January .bss offsets: fv 0, change_colors 16, effect 64, region 104.

| candidate | vs split | gate --all | object_audit | .bss offsets fv/change/effect/region | pdb storage | provider_link | fake scan | objdiff 3.3.1 |
|---|---|---|---|---|---|---|---|---|
| production | build/split | 12/12 | FAIL(4) | 72/88/32/0 | 2 (tangent helper, render_model_section) | - | 0 | 5038/5038, 2368/2368 |
| C1 only (fallback) | split_mC1 | 12/12 | FAIL(4) | 72/88/32/0 | 0 | PASS (11) | 0 | 5038/5038, 2368/2368 |
| **P1** file-scope `= {0}` | split_mC1 | **12/12** | **PASS** (30, 0) | **0/16/64/104** | **0** | **PASS** (11) | 0 | 5038/5038, 2368/2368 |
| **P2** fv static local + 3 `= {0}` | split_mP2 | **12/12** | **PASS** (30, 0) | **0/16/64/104** | **0** | **PASS** (11) | 0 | 5038/5038, 2368/2368 |
| **SL** in-block uninit static locals (first-party form) | split_mSL | 12/12 | **FAIL(4)** | 32/48/96/0 | 0 | PASS (11) | 0 | 5038/5038, 2368/2368 |

- **render_model.** It is byte-identical in all four candidates: 2144 B, sha bb666c87, 98 relocations. Only the
  relocation target names differ (decorated static-local names in SL and P2).
- **Code COMDAT surplus.** In every candidate it is `_negate_vector3d` and `_set_real_point3d`, both IDENTICAL to
  January's selected copies (action_obey and ai_debug). Both are pre-existing.
- **models.h (C1, shared header) consumer sweep** (`tools/hdr_sweep.py`, `models/sweep/SWEEP.txt`):
  - A shadow header placed in the gate tmpdir is used by every TU; the `#error` control failed each compile.
  - Current header vs the C1 header: first_person_weapons 33/34 (same single residual), model_animations 32/32,
    objects 121/121, render_objects 22/22, render_sky 1/1.
  - Gate rows are equal, and objeq2 is IDENTICAL for all five consumers.
  - models.c differs only in the two intended storage rows. Its .text order also changes: model_build_tangent_matrices
    is deferred to the end, which is January's order.
- **Admission queue.** At cdc8ebd3 `tools/audit_object_admission.py` lists the unit as "object-admission-rejected"
  (bss-symbol-layout-mismatch). P1 and P2 retire that entry; SL and C1 keep it.

## Objects that become COMPLETE if the owner says yes

| ruling | objects completed | Halo objects |
|---|---|---|
| Item 1 = A, or item 1 = B | source/rasterizer/xbox/rasterizer_xbox_hardware_geometry | +1 |
| Item 2 = P1, or item 2 = P2 | source/models/models | +1 |
| both yes | both | 379 -> 381 |

Objects that would still not be complete:

- **Item 1, neither A nor B.** hardware_geometry stays NonMatching. hg0 fallback leaves the `_code_00158450@8` and
  `_code_00158460@8` hand stubs, which are external while January has them static (2 pdb disagreements; house rule 7
  names).
- **Item 2, SL chosen (the first-party form).** models stays NonMatching and the rejection stays. The four .bss offsets
  are 32/48/96/0, not 0/16/64/104. January's order depends on the decorated-name hash of three names that are lost;
  steering the hash with chosen names is forbidden.
- **Item 2, C1 only.** Same as SL: FAIL(4).
- **Consumers.** No other object changes status. The models.h consumers keep their current gate rows exactly
  (first_person_weapons still has its 1 residual).

Credit: every option is byte- and credit-neutral (objdiff 3.3.1 numbers are unchanged). Only admission changes.
