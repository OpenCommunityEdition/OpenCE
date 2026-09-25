# LEDGER rasterizer_xbox_debug (round 2, wave 2) - ADMISSION: 8 XDK wrapper rows static (symbols.json) under 05255584 precedent

Worktree HEAD 434f0151 (= 05255584 + R2 ledger commit; no source/config change). Claimed r2w2_rasterizer_xbox_debug (exit 0).
Read: WORKER_BRIEF_R2 0-9; APPLY_ORDER.txt; independent patch; git show 05255584 (msg + symbols/config hunks);
w/rasterizer_xbox_hardware_geometry MANIFEST/LEDGER/mk_cfg/with_split/splitdiff; owner_queue names_xbox_hs_scen LEDGER/MEASURED/RULING;
docs rasterizer_xbox_debug_obj_{jonas_xdk_inline_provenance_20260830,complete_20260902}.md; r2 canonical ledger batch R2-1.

## R0 packet check at 434f0151
- git apply --check independent/rasterizer_xbox_debug_static_wrappers_symbols.json.patch: exit 0 (clean).
- Current rows 5602/5604/5606/5608-5612 = the 8 '-' lines of the patch (no drift).
- Neighbouring objects already carry the SAME names static: active_camouflage rows 5591-5598 (_D3DDevice_SetRenderState,
  _IDirect3DDevice8_SetRenderState@12, ..._SetVertexShaderConstant@16, _SetVertexData2s@16, _Begin@8, _End@4 all "static": true),
  decals rows 5620+ (_D3DDevice_SetRenderState static at 1353200, _IDirect3DDevice8_SetRenderState@12 static at 1354208).

## R1 storage evidence (cachebeta.pdb publics, first-party January)
- publics RVA == symbols.json file_offset (control _rasterizer_debug_drawing_end 0x149AE0 public at 0x149AE0).
- Object range 0x149930..0x14A6FF: 10 publics = exactly the 10 Halo functions; NO public at any of the 8 wrapper
  addresses (0x149930, 0x149AF0, 0x149D20, 0x149D40, 0x149D70, 0x149D80, 0x149D90, 0x149DA0) and none of the 8 names is
  public anywhere (publics_by_address.txt). => all 8 were file-static in January (pdb_storage law: absent = file-static).
- Our compiled object (production = candidate) already emits all 8 as storage 3 (static), from the stock XDK D3DINLINE
  definitions (no source construct). The mismatch is purely the split's attribution (symbols.json rows lack "static").
- Sept-2001 linker-map atlas tier (6455066470 / 7eacac85) lists only the Halo functions for rasterizer_xbox_debug.obj;
  no wrapper record (no D3DDevice_SetRenderState record anywhere in 7eacac85) -> the map gives no storage evidence
  either way for these rows; the governing evidence is cachebeta publics (same basis the precedent cites first).

## R2 split emulation (build/tools/csplit.exe -i cachebeta.exe -p <slug>/cfg_X -o <slug>/split_X; mk_cfg.py in-place edits)
- cfg_ctl/symbols.json cmp-identical to config/symbols.json; cfg_P differs at exactly the 8 rows (CRLF preserved).
- split_ctl vs build/split: 833/833 identical (build/split current). split_P vs split_ctl: 1 differs =
  source/rasterizer/xbox/rasterizer_xbox_debug.obj (splitdiff_ctl.txt, splitdiff_P.txt).

## R3 measurements (with_split.py open() redirect; tools unmodified)
- cand.c = verbatim copy of production source (no source change). cand.obj/cand_P.obj == prod.obj in all 44 sections
  (sha1) except .debug$S (embedded object path), 126 symbols identical (obj_identity.txt).
- gate cand vs build/split: 18/18 EXACT; gate cand vs split_P (--forbid-emitted-symbol _point_from_line3d): guard passed,
  18/18 EXACT (gate_cand_vs_splitP.txt).
- object_audit prod vs build/split: FAIL(8) (the 8 wrappers storage 2/3). cand vs split_P: PASS (24 sections ok incl.
  6 .rdata literals; 24 January symbols 0 differ). prod vs split_P: PASS.
- pdb_storage: production vs build/split 8 disagreements (the 8 wrappers, split 2 / ours 3, PDB-public False);
  candidate vs split_P 0 disagreements (pdb_storage_cand_vs_splitP.txt).
- surplus_identity cand vs split_P: 0 candidate-only code COMDATs. provider_link cand vs split_P: 7 rows PASS in both
  orders (global_d3d_device literal -> rasterizer_xbox, D3DPRIMITIVETOVERTEXCOUNT -> decals, D3DSIMPLERENDERSTATEENCODE ->
  progress_bar, D3DTEXTUREDIRECTENCODE -> headers, 3 __real literals -> action_charge/actor_moving/path_obstacles);
  --baseline=prod.obj: no new surplus. Identical to production vs build/split (7 PASS).
- Cross-object consumer check (name_census.txt): no split or base object has an UNDEFINED reference to any of the 8 names;
  no production base object defines any of them external (all static everywhere). Dropping the debug split's 8 external
  definitions therefore cannot create a 'NO JANUARY PROVIDER' row for any other unit's surplus check.
- split ctl vs P for this object (split_ctl_vs_P_detail.txt): 24 sections byte/reloc identical; 80 symbols, exactly the 8
  wrapper storage classes flip 2 -> 3; nothing else.
- Data: tinfo --data listing split_P == build/split (6 .rdata, 292 B); object_audit coff_compare ok for all 6 .rdata.
- objdiff 3.3.1 (sha1 3130e428) mini report: prod vs build/split, cand vs split_P, prod vs split_P all 3108/3108 code,
  18/18 fn, 292/292 data (credit-neutral).
- Patch refreshed with a git header (index f9abb17d..d7e57cfc; body unchanged): patches/01_symbols.json.patch.
  git apply --check at 434f0151: clean. patch -p1 --binary on a verify/ copy == cfg_P/symbols.json (cmp).
- git status: no tracked modification (only the pre-existing untracked research/fifty_objects_r2_20260924/).
  gate.py left its fixed-path scratch/_gate_<pid>.c/.obj temporaries (tool behaviour; not removed per incident rules).

## R4 out-of-scope observation (read-only; NOT claimed, NOT probed)
- Same storage class in five other Matching-labelled siblings (pdb_storage vs cachebeta publics, sibling_storage_census.txt):
  plasma_energy 5/5 disagreements are XDK wrappers, text 9/9, widgets 11/11, motion_sensor 10/11 (+_bss_00465e27),
  detail_objects 13/15 (+_bss_0045e904, _detail_object_build_vertices). Candidate follow-up packets of the identical class.
- Disclosure: this census redirected pdb_storage output to /tmp/ps_<unit>.txt (outside my slug, new files only; the
  copy of the summary is in sibling_storage_census.txt). No other writes outside the slug.

## VERDICT: OBJECT_COMPLETE_CANDIDATE (storage-only admission fix; the unit is already labelled Matching, index 126, so the
   Halo object count does not change; object_audit FAIL(8) -> PASS makes the label truthful). Precedent conditions all hold.
- R5 close: MANIFEST.md written; claim RELEASED (exit 0).
