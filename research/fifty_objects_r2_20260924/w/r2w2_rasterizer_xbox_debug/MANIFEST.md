# MANIFEST - source/rasterizer/xbox/rasterizer_xbox_debug (round 2, wave 2)

- **Base / tip.** Worktree HEAD 434f0151 (05255584 + an R2 ledger-only commit). Nothing was committed; the packet is in `patches/`.
- **Route.** Storage-only admission fix under the canonical 05255584 precedent (the XDK wrapper rows go static per cachebeta publics).
- **Source packet.** `research/fifty_objects_20260925/w/owner_queue/names_xbox_hs_scen/patches/independent/rasterizer_xbox_debug_static_wrappers_symbols.json.patch`
  - Its body is unchanged here.
  - Only a git header was added: `index f9abb17d..d7e57cfc`.
- **Claim.** `r2w2_rasterizer_xbox_debug`, released at close.

## Production change (one file; `git apply --check` is clean at 434f0151)

| file | patch | what |
|---|---|---|
| config/symbols.json | `patches/01_symbols.json.patch` (f9abb17d..d7e57cfc); full post-image `cfg_P/symbols.json` (working-tree CRLF, in-place line edits) | Rows 5602, 5604, 5606 and 5608-5612 only. Each gets `, "static": true` appended. There is no rename, reorder or re-serialisation, and no other row changes. |

- **Rows changed.** `_D3DDevice_SetRenderState` (0x149930), `_IDirect3DDevice8_SetRenderState@12` (0x149AF0), `_IDirect3DDevice8_SetVertexShaderConstant@16` (0x149D20), `_IDirect3DDevice8_SetVertexData4f@24` (0x149D40), `_IDirect3DDevice8_SetVertexData2s@16` (0x149D70), `_IDirect3DDevice8_SetVertexDataColor@12` (0x149D80), `_IDirect3DDevice8_Begin@8` (0x149D90) and `_IDirect3DDevice8_End@4` (0x149DA0).
- **Unchanged files.** There is no source, header, config.json, parks, rejections, semantic or ownership change. The unit is already `"status": "Matching"` (config.json index 126).
- **After applying.** Regenerate the split (csplit). The emulation shows that only `rasterizer_xbox_debug.obj` changes, 1 of 833 objects.

## Precedent (governing canonical ruling) and its conditions

**Governing ruling:** canonical commit **05255584**, "R2 batch 1b: admit hardware_geometry and actors; render_debug_trigger_volumes exact". Its message says:

> "The six hand-written placeholder stubs become their XDK names, 13 wrapper rows go static, and the call's argument/position are disclosed as byte-inert and unattested."

The same commit also applies the class to hs_runtime:

> "12 typecast converters are static per cachebeta publics (provider link FAIL(12) -> PASS)."

The canonical ledger `docs/object_matching_logs/claude_fifty_objects_r2_20260924.md` (batch R2-1 table) records it as "...and 13 wrapper rows are static."

The storage basis, from the precedent's manifest (`scratch/w/rasterizer_xbox_hardware_geometry/MANIFEST.md`, "Storage"):

> "No wrapper name is in the cachebeta.pdb publics (`scratch/tools/cachebeta_publics.txt`), so all 13 are file-static."

The worker brief's section 4.3 gives the same rule: "cachebeta.pdb publics (January storage truth: public = external, absent = file-static)".

| # | Precedent condition | Holds here? | Evidence |
|---|---|---|---|
| C1 | The rows are stock XDK header-inline wrappers the compiland emits from the SDK header. They are not Halo code and have no project-authored body. | **yes** | `xbox/include/D3D8-Xbox.h:21` has `#define D3DINLINE static __forceinline`. It defines the eight at lines 1476/1497 (`D3DDevice_SetRenderState`, which is not WINAPI, hence the undecorated `_D3DDevice_SetRenderState`), 1708, 1739, 1789, 1790, 1793, 1794 and 1795. `docs/object_matching_logs/rasterizer_xbox_debug_obj_complete_20260902.md` records them as "compiler-retained XDK inline wrappers" with no wrapper body in source. Because the header declares them `static`, the C language itself fixes their storage as file-static. |
| C2 | No name is a cachebeta.pdb public, so each is file-static in January. | **yes** | See `publics_by_address.txt`. Publics RVA equals the symbols.json file_offset (control: `_rasterizer_debug_drawing_end` at 0x149AE0). In the object range 0x149930..0x14A6FF there are exactly 10 publics, all Halo functions. There is no public at any of the 8 wrapper addresses, and none of the 8 names is public anywhere. |
| C3 | The fix is `"static": true` added in place to the existing rows, in the same row form the precedent used. | **yes** | Exactly 8 lines differ, and CRLF is preserved. The row form `{ ..., "name": "...", "static": true },` matches 05255584 and the neighbouring rows for the same names: active_camouflage rows 5591-5598, and decals at 1353200/1354208, already static. |
| C4 | The csplit regen radius is this object only. | **yes** | `splitdiff_ctl.txt` shows split_ctl equals build/split (833/833). `splitdiff_P.txt` shows split_P vs split_ctl differs in 1 object, this one. Within it (`split_ctl_vs_P_detail.txt`), all 24 sections are byte- and relocation-identical, and only the 8 storage classes flip from 2 to 3. |
| C5 | Every January function is EXACT after the regen. | **yes** | 18/18 EXACT, with the `--forbid-emitted-symbol _point_from_line3d` guard passed (`gate_cand_vs_splitP.txt`). |
| C6 | object_audit PASS, pdb_storage 0, surplus identical, provider link PASS in both orders. | **yes** | See the tables below. |
| C7 | The byte-level result is credit-neutral; the gain is admission truthfulness only. | **yes** | objdiff 3.3.1 gives 3108/3108 code, 18/18 functions and 292/292 data, before and after. |

**Differences from the precedent.**
- None in the construct. This is a strict subset of the 05255584 hardware_geometry change: the storage rows only.
- There is no rename, because the eight rows already carry their decorated XDK names.
- There is no source call, so no disclosure comment is needed. The precedent's comment covered only its new MoveResourceMemory call.
- One consequence differs from hardware_geometry. There the admission gave +1 object. Here the unit is already labelled Matching, so the patch makes that label truthful (object_audit FAIL(8) -> PASS) without changing the object count.

**Not available (disclosed).**
- The Sept-2001 linker-map atlas tier (6455066470 / 7eacac85) lists only the Halo functions for `rasterizer_xbox_debug.obj`. It has no wrapper records, so it gives no storage evidence either way.
- The governing evidence is the one the precedent cites first: the cachebeta publics, together with the header's `static`.

## Target vs candidate per function

The candidate is `cand.c`, a verbatim copy of production source, compiled by gate.py into `cand_P.obj`. It is compared with the emulated `split_P`.

`cand.obj`/`cand_P.obj` equal the production `build/base` object in all 44 sections (sha1), except `.debug$S`, which differs only by the embedded object path. All 126 symbols are identical (`obj_identity.txt`).

| function | padded size | relocs | gate vs build/split | gate vs split_P |
|---|---:|---|---|---|
| _D3DDevice_SetRenderState | 432 | = | EXACT | EXACT |
| _IDirect3DDevice8_SetRenderState@12 | 544 | = | EXACT | EXACT |
| _IDirect3DDevice8_SetVertexShaderConstant@16 | 16 | = | EXACT | EXACT |
| _IDirect3DDevice8_SetVertexData4f@24 | 48 | = | EXACT | EXACT |
| _IDirect3DDevice8_SetVertexData2s@16 | 16 | = | EXACT | EXACT |
| _IDirect3DDevice8_SetVertexDataColor@12 | 16 | = | EXACT | EXACT |
| _IDirect3DDevice8_Begin@8 | 16 | = | EXACT | EXACT |
| _IDirect3DDevice8_End@4 | 16 | = | EXACT | EXACT |
| _rasterizer_debug_drawing_end | 16 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_end | 16 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_end_screenspace | 16 | = | EXACT | EXACT |
| _rasterizer_debug_drawing_begin | 416 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_begin | 240 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_line | 256 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_triangle | 336 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_begin_screenspace | 416 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_line_screenspace | 224 | = | EXACT | EXACT |
| __rasterizer_debug_immediate_linestrip_screenspace | 224 | = | EXACT | EXACT |

Result: 18/18 EXACT in both (`gate_cand_vs_buildsplit.txt`, `gate_cand_vs_splitP.txt`).

## Section-4 audit (split_P emulation, via `with_split.py` open() redirect; tools unmodified)

| check | production vs build/split | candidate vs split_P |
|---|---|---|
| object_audit | **FAIL(8)**: the 8 wrappers, storage 2/3 | **PASS**: 24 sections ok (18 .text and 6 .rdata literals, flags and alignment equal); 24 January symbols, 0 differ |
| pdb_storage vs cachebeta publics | 8 disagreements (split 2, ours 3, public False) | **0** disagreements |
| surplus_identity | - | 0 candidate-only code COMDATs |
| provider_link (both orders) | PASS 7/7 | **PASS 7/7**. `--baseline=prod.obj` finds no new surplus. |
| data (coff_compare) | 6 .rdata ok | 6 .rdata ok; the tinfo `--data` listing for split_P is identical to build/split's (292 B) |
| objdiff 3.3.1 (sha1 3130e428) | 3108/3108 code, 18/18 functions, 292/292 data | 3108/3108, 18/18, 292/292 (production vs split_P is the same) |

The provider_link rows cover the global_d3d_device literal (provider rasterizer_xbox), D3DPRIMITIVETOVERTEXCOUNT (decals), D3DSIMPLERENDERSTATEENCODE (progress_bar), D3DTEXTUREDIRECTENCODE (headers), and `__real@3f800000`/`bf800000`/`c0000000` (action_charge/actor_moving/path_obstacles).

- **Surplus.** The pre-existing surplus is unchanged: `.drectve`, the global_d3d_device literal, 3 SDK tables and 3 `__real` literals.
- **Cross-object consumers** (`name_census.txt`):
  - No split or base object has an undefined reference to any of the 8 names.
  - No production object defines any of them as external; all are static everywhere.
  - Removing these 8 external definitions from the debug split therefore cannot create a missing-provider row for any other unit.
- **Rejections and parks.** There are none for this unit in `object_admission_rejections.json`, `parked.json`, `semantic_matches.json` or `semantic_data_matches.json`.

## House-rule review

- **Source.** No source changed, so no source house rule is touched.
- **symbols.json rules.**
  - The edits are in place with CRLF preserved.
  - There is no re-serialisation or reordering (the symbols.json-order rule).
  - There is no new name, so there are no placeholder names.
- **Strip test.** Not applicable, because no source construct was added.
- **Fake-match classes.** None apply: no pragma, cast, dummy, filler or flag.

## Tests run

- `git apply --check`
- post-image `cmp` (`patch -p1 --binary` on a `verify/` copy equals `cfg_P/symbols.json`)
- csplit control/patched emulation with `splitdiff`
- `gate.py --all`, with the guard, against both splits
- `object_audit`
- `pdb_storage`
- `surplus_identity`
- `provider_link`: both orders, plus the baseline check
- `tinfo --data`
- the objdiff 3.3.1 mini report
- a cross-object name census
- `git status`: no tracked modification

**Integrator:**
1. Apply `patches/01_symbols.json.patch`.
2. Regenerate the split through ninja.
3. Run a full ninja.
4. Run the stable diff (expected: 0 function change).
5. Run progress, parks/admission, pytest and `git diff --check`.

## Credit

- **Code, functions, data.** Credit-neutral: 3,108 meaningful / 3,264 padded code bytes, 18 functions and 292 B data are already credited.
- **Admission.** The Halo object count is unchanged, because the unit is already Matching. The patch makes the Matching label truthful: object_audit FAIL(8) -> PASS and pdb_storage 8 -> 0.
- **Zero-credit / held items.** None.
- **Reopen.** Only if a January-dated first-party source showed any of the 8 as external. That would contradict both the cachebeta publics and the header's `static` storage.

## Out-of-scope observation (read-only; not claimed, not probed)

Five other Matching-labelled siblings carry the same storage class of defect. The counts are pdb_storage disagreements (`sibling_storage_census.txt`):

| object | disagreements | XDK wrappers | other rows |
|---|---:|---:|---|
| plasma_energy | 5 | 5 | none |
| text | 9 | 9 | none |
| widgets | 11 | 11 | none |
| motion_sensor | 11 | 10 | `_bss_00465e27` |
| detail_objects | 15 | 13 | `_bss_0045e904`, `_detail_object_build_vertices` |

These are candidate follow-up packets of the identical class.
