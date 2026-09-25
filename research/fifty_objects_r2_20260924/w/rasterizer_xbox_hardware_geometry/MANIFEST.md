# MANIFEST - source/rasterizer/xbox/rasterizer_xbox_hardware_geometry (round 2, r2w1)

- Base: f6d00a8c (worktree HEAD b7c1b7d9 = lane baseline commit on top of it). Tip: nothing committed; the packet is in `patches/`.
- Route: owner-queue item #7, option B (`research/fifty_objects_20260925/w/owner_queue/pick_hwgeom_models/patches/hwgeom_B`).
  The A-vs-B owner pick is settled by first-party linker-map evidence (below). The packet is re-verified here with refreshed index lines.

## Production changes (apply all three together, in order; `git apply --check` clean at f6d00a8c, singly and together)

| file | patch | what |
|---|---|---|
| source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.c | `patches/01_rasterizer_xbox_hardware_geometry.c.patch` (c2bb9095..dc158d97) | deletes the six hand stubs `code_00158450/460/470/4b0/4d0/510`; adds `D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO);` after the vertex-buffer Unlock block, with a 3-line disclosure comment; header "symbols in this file" block now uses the real names |
| config/symbols.json | `patches/02_symbols.json.patch` (e3580876..6b6e58d3); full copy `symbols.json` (working-tree CRLF, in-place line edits) | rows 5813-5825 only: 0x158450 -> `_D3DResource_MoveResourceMemory@8`, 0x158460 -> `_D3DVertexBuffer_MoveResourceMemory@8`, 0x158470/4b0/4d0/510 -> `_D3DVertexBuffer_Unlock@4` / `_IDirect3DVertexBuffer8_Unlock@4` / `_D3DIndexBuffer_Unlock@4` / `_IDirect3DIndexBuffer8_Unlock@4`; `"static": true` on all 13 XDK wrapper rows |
| config/config.json | `patches/03_config.json.patch` (b928f182..0938f2d0) | status NonMatching -> Matching (index 117) |

- Then regenerate the split. csplit emulation shows that only `rasterizer_xbox_hardware_geometry.obj` changes (832 of the 833 objects are identical).
- **Joint landing required.** Production source against the new split scores 15/17 (892/898): the MRM pair is not emitted without patch 01.
- No shared header is changed, so there are no consumers. No parks, rejections, semantic entries or ownership rows exist for this unit.

## Source evidence per changed construct

1. **The call exists (byte-attested).** January emits two static pure `ret 8` wrappers at 0x158450/0x158460 that no other object references.
   - Under the VC7 static-COMDAT law, a static header inline is emitted only if the TU defines it and uses it.
   - The whole-image census of pure `ret 8` functions finds only this pair plus named texture Unlock stubs (round-1 E3).
   - The October 2001 betaP build has the same pair (round-1 E2).
2. **Spelling (NEW, first-party).** The Halo symbol atlas has MSVC linker-map tier records, as ground truth dated 2001-09-25:
   - `6455066470...` is the retail cache exe (lines 4826-4835) and `7eacac85...` is the cachebeta xbe (lines 4295-4300). In lib `rasterizer_xbox_hardware_geometry.obj`, both list `_D3DResource_MoveResourceMemory@8`, then `_D3DVertexBuffer_MoveResourceMemory@8`, then `_D3DVertexBuffer_Unlock@4`, `_IDirect3DVertexBuffer8_Unlock@4`, `_D3DIndexBuffer_Unlock@4` and `_IDirect3DIndexBuffer8_Unlock@4`. The cache exe also lists `_D3DIndexBuffer_Lock@20` and `_IDirect3DIndexBuffer8_Lock@20`.
   - The cachebeta xbe offsets relative to the pair are 0, 0x10, 0x20, 0x60, 0x80 and 0xc0. January's are the same: 0x158450/460/470/4b0/4d0/510.
   - The string "MoveResourceMemory" occurs in no other object of either map, and `IDirect3DResource8_MoveResourceMemory` appears nowhere. So option A is refuted for the Sept-2001 ancestor, and B (no cast) is attested by name.
   - The same records attest the four Unlock names.
   - January's own atlas (4cc87b45) has no record here, because of the exact-tier size floor.
3. **Storage.** No wrapper name is in the cachebeta.pdb publics (`scratch/tools/cachebeta_publics.txt`), so all 13 are file-static. The map lists them per object, which agrees.
   - Precedents for the same rows as static: the rasterizer dynamic-geometry object (rows 5669-5700, e.g. `_D3DIndexBuffer_Unlock@4` static at 5696) and decals (rows 5637-5642).
4. **Emission order.** Our .text order equals split_B's order section for section (`text_order.txt`); the round-1 P1 lab also measured the d3d8.h definition-order law.
5. **Not attested (disclosed in the source comment).** The `D3DMEM_VIDEO` argument (D3D8Types.h: 1) and the call's position after the Unlock block are byte-inert. Round-1 P2 put the call at 4 different positions and all gave identical bytes. There is no /Od attestation, because the /Od build is the PC DX9 port and HCEX links no hardware_geometry code.

## Target vs candidate (candidate `cand_B.obj` from `cand.c` vs emulated `split_B`)

`gate.py --all --forbid-emitted-symbol _point_from_line3d` passed the guard. Result: **17/17 EXACT** (`gate_cand_vs_splitB.txt`).

| function | size | relocs | status |
|---|---:|---:|---|
| _IDirect3DDevice8_CreateVertexBuffer@24 | 32 | = | EXACT |
| _IDirect3DDevice8_CreateIndexBuffer@24 | 32 | = | EXACT |
| _D3DResource_MoveResourceMemory@8 | 16 | 0 | EXACT (c2 08 00) |
| _D3DVertexBuffer_MoveResourceMemory@8 | 16 | 0 | EXACT (c2 08 00) |
| _D3DVertexBuffer_Unlock@4 | 16 | 0 | EXACT |
| _IDirect3DVertexBuffer8_Release@4 | 16 | = | EXACT |
| _IDirect3DVertexBuffer8_Lock@20 | 32 | = | EXACT |
| _IDirect3DVertexBuffer8_Unlock@4 | 16 | 0 | EXACT |
| _D3DIndexBuffer_Lock@20 | 16 | = | EXACT |
| _D3DIndexBuffer_Unlock@4 | 16 | 0 | EXACT |
| _IDirect3DIndexBuffer8_Release@4 | 16 | = | EXACT |
| _IDirect3DIndexBuffer8_Lock@20 | 32 | = | EXACT |
| _IDirect3DIndexBuffer8_Unlock@4 | 16 | 0 | EXACT |
| _rasterizer_vertex_buffer_new | 368 | = | EXACT |
| _rasterizer_vertex_buffer_delete | 48 | = | EXACT |
| _rasterizer_triangle_buffer_new | 352 | = | EXACT |
| _rasterizer_triangle_buffer_delete | 48 | = | EXACT |

- **Data.** All 8 January `.rdata` literal sections are `ok` under coff_compare in object_audit. objdiff 3.3.1 shows 584/584.
- **Symbols.** `object_audit` gives PASS: 25 January symbols with 0 differences in storage, section or offset (`object_audit_cand_vs_splitB.txt`). Production against the current split is FAIL(7).
- **PDB storage.** The candidate has 0 disagreements; production has 13: the 7 wrappers plus the 6 `_code_` stubs (`pdb_storage.txt`).
- **Surplus.** There are 0 candidate-only code COMDATs (`surplus_identity.txt`). Pre-existing surplus is `.drectve`, 3 pooled literals and 3 SDK select-any tables. `provider_link` passes all 6 rows in both orders, and `--baseline=production` finds no new surplus (`provider_link.txt`).
- **Fake scan.** `fake_match_scan` on cand.c finds 0 leads.
- **objdiff 3.3.1 mini report** (sha1 3130e428, `objdiff_report.txt`):

  | pairing | code | functions | data |
  |---|---|---|---|
  | production vs build/split | 898/898 | 17/17 | 584/584 |
  | candidate vs split_B | 898/898 | 17/17 | 584/584 |
  | production vs split_B | 892/898 | 15/17 | 584/584 |

## House-rule review

- Real XDK names replace the house-rule-7 `code_` placeholder stubs, which are deleted. The new call is formatted one parameter per line and takes the typed `D3DVertexBuffer *` with no cast.
- There is no consumer-local prototype, no header change, and no new COMDAT from a header helper.
- **Strip test.** Removing the call removes two January functions (the pair is no longer emitted), so the call is load-bearing through existence. It is not a decoration.
- **Pre-existing, not introduced.** The BUG comment in `rasterizer_vertex_buffer_new` covers the XDK CreateVertexBuffer failure path that leaves the output unwritten.
  - It landed in 6130661c, authenticated in `docs/object_matching_logs/astra_90pct_wave6_vertex_independent_review_20260920.md` under the owner's house-rule-21 ruling.
  - This packet does not touch it, and it is not a brief section-8 held item.

## Tests run

`git apply --check` (singly and together), post-image byte equality, csplit ctl/B emulation, `gate.py`, `object_audit`, `pdb_storage`, `surplus_identity`, `provider_link` (both orders plus baseline), `fake_match_scan`, and the objdiff 3.3.1 mini report.

**Integrator:** run a full ninja, the stable sweep, parks/admission, pytest and the diff check.

## Credit

- Code, functions and data are credit-neutral (898 B code / 17 functions / 584 B data are already credited).
- Admission is **+1 Halo object**: rasterizer_xbox_hardware_geometry becomes COMPLETE.
- Zero-credit items: none held.
- Reopen: only if a first-party January-dated source names a different MoveResourceMemory argument or position. That would be a comment-only change.
