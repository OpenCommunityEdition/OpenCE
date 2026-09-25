# LEDGER rasterizer_xbox_hardware_geometry (round 2, r2w1) - ADMISSION via owner-queue packet hwgeom_B

Base f6d00a8c (worktree HEAD b7c1b7d9 = lane baseline commit on top). Claimed 2026-09-24 as r2w1-rasterizer_xbox_hardware_geometry.
Read: WORKER_BRIEF_R2 0-9; round-1 w/hardware_geometry/LEDGER.md (E1-E9, P1, P2, FINAL); owner_queue/pick_hwgeom_models
LEDGER/MEASURED/RULING/APPLY_ORDER; scratch/w/triage_rasterizer_a/LEDGER.md (P1, P7).

## R0 claim + drift (2026-09-24)
- claim.py claim -> CLAIMED (exit 0). git status clean at b7c1b7d9 (tree = f6d00a8c + lane baseline scratch files).

## R1 atlas re-verification (first-party map tier, data only)
- research/halo-symbol-atlas/index.csv: 6455066470... = 2001-09-25_2001_retail__cache_exe (9025 map records, tier map only);
  7eacac85... = 2001-09-25_2001_retail__cachebeta_xbe (8568 map records, tier map only).
- 6455066470 lines 4826-4835, lib rasterizer_xbox_hardware_geometry.obj: 0x504f10 _D3DResource_MoveResourceMemory@8,
  0x504f20 _D3DVertexBuffer_MoveResourceMemory@8, 0x504f30 _D3DVertexBuffer_Unlock@4, 0x504f70 _IDirect3DVertexBuffer8_Unlock@4,
  0x504f80 _D3DIndexBuffer_Lock@20, 0x504f90 _D3DIndexBuffer_Unlock@4, 0x504fb0 _IDirect3DIndexBuffer8_Lock@20,
  0x504fc0 _IDirect3DIndexBuffer8_Unlock@4, 0x504fd0 @rasterizer_vertex_buffer_new@20, 0x5050f0 @rasterizer_triangle_buffer_new@16.
- 7eacac85 lines 4295-4300, same lib: 0x167100 D3DResource_MRM@8, 0x167110 D3DVertexBuffer_MRM@8, 0x167120 D3DVertexBuffer_Unlock@4,
  0x167160 IDirect3DVertexBuffer8_Unlock@4, 0x167180 D3DIndexBuffer_Unlock@4, 0x1671c0 IDirect3DIndexBuffer8_Unlock@4.
  Relative offsets from the MRM pair base: 0,0x10,0x20,0x60,0x80,0xc0. January (symbols.json 0x158450 base): D3DRes 0, X 0x10,
  VB_Unlock 0x20 (0x158470), IDirect3DVertexBuffer8_Unlock 0x60 (0x1584b0), D3DIndexBuffer_Unlock 0x80 (0x1584d0),
  IDirect3DIndexBuffer8_Unlock 0xc0 (0x158510) -> IDENTICAL spacing (incl. the unnamed Release 0x10 / Lock 0x20 gaps).
  The map lists these under the object (static-symbols section) -> consistent with "static": true.
- No IDirect3DResource8_MoveResourceMemory record anywhere in either Sept map for this object -> option A refuted for the
  Sept-2001 ancestor; B attested by first-party linker-map name. Oct-2001 betaP (round-1 E2) has the same pair shape; January
  bytes of the pair are identical pure `ret 8` -> continuity Sept -> Oct -> Jan.
- Sept cachebeta xbe binary (7eacac85) is not on disk (hashes of local xbes: b49631 = January, a3402b = oct-betaP) -> names only.

## R2 packet check at f6d00a8c
- git apply --check of hwgeom_B 01/02/03: clean individually and together. Source blob c2bb9095 unchanged (last touch 70e1a15b
  predates cdc8ebd3). symbols.json changed cdc8ebd3..f6d00a8c only at rows 3249, 9058-9061, 22526 (other objects); rows 5813-5825
  unchanged. Packet 02 index line (a4e82fec) stale -> refreshed to e3580876..6b6e58d3; packet 03 had CRLF body -> normalised to LF
  with a git header (b928f182..0938f2d0). Post-images (patch utility on f6d00a8c blobs) == cand.c (blob dc158d97, = packet index),
  cfg_B/symbols.json, cfg_B/config.json. Refreshed packet: patches/01..03.
- Pre-existing BUG comment in rasterizer_vertex_buffer_new (XDK CreateVertexBuffer failure leaves the output unwritten) is NOT
  introduced by this packet: it landed via 6130661c (Astra wave6 vertex review: authenticated at cachebeta 0x5de713..0x5de719,
  "follows the user's explicit house-rule21 ruling"). Not in the brief section-8 held list. Disclosed in MANIFEST only.

## R3 split emulation (build/tools/csplit.exe -i cachebeta.exe -p <slug>/cfg_X -o <slug>/split_X; mk_cfg.py in-place line edits)
- split_ctl vs build/split: 833/833 byte-identical (build/split current for f6d00a8c).
- split_B vs split_ctl: exactly 1 object differs = source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.obj.
- New names are unique in symbols.json for the MRM pair (0 prior rows); the Unlock/Lock/Release names already exist as rows of
  other objects (decals, dynavobgeom with static:true; detail_objects/transparent_geometry external) - precedent, no collision in split.

## R4 measurements (with_split.py = open() redirect of build/split -> split_B, build/base/<unit>.obj -> cand; tools unmodified)
- gate prod vs build/split: 17/17 EXACT. gate cand vs build/split: 11 EXACT + 6 UNWRITTEN (names only; expected).
- gate cand vs split_B (--forbid-emitted-symbol _point_from_line3d): guard passed; 17/17 EXACT.
- gate prod vs split_B: 15 EXACT + 2 UNWRITTEN (the MRM pair) -> the .c and symbols.json patches MUST land together.
- .text emission order cand == split_B (CreateVB, CreateIB, D3DRes_MRM, D3DVB_MRM, D3DVB_Unlock, VB8_Release, VB8_Lock, VB8_Unlock,
  D3DIB_Lock, D3DIB_Unlock, IB8_Release, IB8_Lock, IB8_Unlock, 4 Halo fns); MRM bodies c2 08 00 + pad in both; all 13 static.
- object_audit cand vs split_B: PASS (25 sections ok incl. all 8 .rdata; 25 January symbols, 0 differ). Surplus = .drectve,
  3 pooled literals, 3 SDK tables (pre-existing). Production vs split_ctl: FAIL(7) + 4 surplus Unlock stubs.
- pdb_storage: cand vs split_B 0 disagreements; production vs build/split 13 (7 wrappers + 6 _code_ stubs).
- surplus_identity cand: 0 candidate-only code COMDATs. provider_link cand: 6 rows PASS both orders (count>0 -> ai, unsupported
  triangle -> draw_primitives, vertex_buffer -> environment, D3DPRIMITIVETOVERTEXCOUNT -> decals, D3DSIMPLERENDERSTATEENCODE ->
  progress_bar, D3DTEXTUREDIRECTENCODE -> headers); --baseline=production: no new surplus.
- fake_match_scan cand.c: 0 leads (production 0).
- objdiff 3.3.1 (sha1 3130e428) mini report: prod vs build/split 898/898 code 17/17 fn 584/584 data; cand vs split_B 898/898,
  17/17, 584/584 (credit-neutral); prod vs split_B 892/898 15/17 (confirms the joint-landing requirement).
- No references to the removed _code_001584xx stubs in any other source/config/obj (tools/test_import_symbol_names.py uses the
  string "_code_00158450@8" only as a placeholder-pattern literal; unaffected).
## VERDICT: OBJECT_COMPLETE_CANDIDATE (admission; credit-neutral). Only remaining disclosure: D3DMEM_VIDEO + call position are
   byte-inert and unattested (source comment). Spelling now first-party attested (Sept-2001 linker maps).

## R5 close: MANIFEST.md written; claim RELEASED (OBJECT_COMPLETE_CANDIDATE). gate.py left its fixed-path scratch/_gate_<pid>.c/.obj
   temporaries (tool behaviour; not removed per incident rules). git status clean.
