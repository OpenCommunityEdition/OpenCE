# LEDGER review_r2_rasterizer_xbox_hardware_geometry (adversarial review of hwgeom_B packet)

Tree: HEAD b7c1b7d9 (= f6d00a8c + lane baseline doc), git status clean at start.
Read: WORKER_BRIEF_R2 sections 0-9; worker MANIFEST.md + LEDGER.md (R0-R5); owner queue item #7.
Claimed unit as r2review-hwgeom (review lab builds only).

## V1 git apply + post-images (HEAD b7c1b7d9; blobs c2bb9095/e3580876/b928f182 = f6d00a8c)
- git apply --check: 01, 02, 03 each OK and together OK (worktree mode, core.autocrlf=true).
- Applied outside repo to HEAD blobs: post-image blobs dc158d97 / 6b6e58d3 / 0938f2d0 (= packet index lines);
  equal mod CR to worker cand.c, symbols.json, cfg_B/symbols.json, cfg_B/config.json.
- symbols.json diff vs HEAD: only the 13 rows 5813-5825 (26 diff lines); no reorder/re-serialisation.
## V2 atlas re-read (data only)
- index.csv row 230/231: 6455066470 = 2001-09-25 retail cache_exe (9025 map), 7eacac85 = 2001-09-25 retail cachebeta_xbe (8568 map).
- 6455066470.jsonl lines 4826-4835 and 7eacac85.jsonl lines 4295-4300 read verbatim: records exactly as the worker cites
  (tier "map", lib rasterizer_xbox_hardware_geometry.obj). Offsets 0x167100/110/120/160/180/1c0 -> relative 0/10/20/60/80/c0
  = January 0x158450/460/470/4b0/4d0/510 relative spacing.
## V3 XDK header readout (build/generated/d3d8/d3d8.h)
- 2015 D3DResource_MRM {} ; 2028 IDirect3DResource8_MRM (option A) ; 2235 D3DVertexBuffer_MRM (option B) ; 2243 D3DVertexBuffer_Unlock ;
  2246 IDirect3DVertexBuffer8_Release ; 2251 IDirect3DVertexBuffer8_MRM ; 2256/2257 VB8 Lock/Unlock.
  Emission order (definition order) cannot separate A (2028) from B (2235); it does exclude IDirect3DVertexBuffer8_MRM (2251 > 2246).
  The map name separates A from B. D3DMEM_VIDEO = 1 in xbox/include/D3D8Types.h:1795 (D3DMEM_AGP = 0).
## V4 independent split emulation
- cfg_ctl = copy of config/; cfg_R = config/ + MY post-image symbols.json/config.json (from applying the packet to HEAD blobs).
- build/tools/csplit.exe -i cachebeta.exe -p cfg_X -o split_X: split_ctl == build/split 833/833; split_R vs split_ctl: only
  source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.obj differs; split_R == worker split_B 833/833.
## V5 gate (with_split redirect to split_R)
- cand_post.c (= packet post-image) vs split_R, --forbid-emitted-symbol _point_from_line3d: guard passed, 17/17 EXACT.
- production vs split_R: 15 EXACT + 2 UNWRITTEN (MRM pair) -> joint landing of 01+02 is mandatory (confirmed).
- production vs build/split: 17/17 EXACT (current, under _code_ names).
## V6 whole-object audit (cand_R.obj from packet post-image; split_R)
- object_audit: PASS; 25 January sections ok (17 .text + 8 .rdata, flags/align eq); 25 symbols, 0 differ. Surplus = .drectve,
  3 pooled literals (count>0, unsupported-triangle, vertex_buffer), 3 SDK tables - identical set to production's pre-existing surplus
  minus production's 4 header Unlock stubs (which are now January-owned names). Production vs build/split: FAIL(7) (storage 2/3).
- pdb_storage: cand 0 disagreements; production 13 (7 wrappers + 6 _code_ stubs). No wrapper name among 19,094 cachebeta publics.
- surplus_identity: 0 candidate-only code COMDATs.
- provider_link: 6 rows PASS both orders; --baseline=production: no new surplus.
- objdiff 3.3.1 (build/tools/objdiff-cli.exe sha1 3130e428) mini report: prod vs build/split 898/898, 17/17, 584/584;
  cand_R vs split_R 898/898, 17/17, 584/584 -> credit-neutral; admission only.
## V7 strip test + inertness labs (mk_labs.py; each gated vs split_R; secorder.py = emission order + normalized sha)
- lab_strip (call removed): 15 EXACT + 2 UNWRITTEN (MRM pair) -> the call is load-bearing by EXISTENCE only.
- lab_agp (D3DMEM_AGP), lab_after_create (call right after the create block), lab_end_success (after hardware_format store):
  17/17 EXACT, every section name+sha identical to cand_R -> argument and position are byte-inert (confirms disclosure).
- lab_optA (IDirect3DResource8_MRM + cast): same slot (section after D3DRes_MRM), same bytes; only the name differs -> 16/17.
- lab_vb8 (IDirect3DVertexBuffer8_MRM): emitted after VB8_Release (d3d8.h 2251) -> excluded by order and name.
- So bytes/order cannot separate A from B; the 2001-09-25 map name is the only discriminator, and it says B.
## V8 January image refs (refscan.py, cachebeta.exe as data; file offset == RVA, validated on _csmemcpy 219 / _display_assert 5579 calls)
- 0 rel32 call/jmp and 0 absolute dword refs to 0x158450/460/470/480/4b0 and 0x158410 -> forceinline wrappers emitted but never called
  (static-COMDAT law: TU defines and USES them). D3DINLINE = static __forceinline (d3d8.h:21) -> any optimized build hides arg/position.
## V9 fake scan / warnings / references
- fake_match_scan cand_post.c: 0 leads (production 0). /W3 /WX and /W4 /WX: no warnings (cand and production), 17/17.
- code_001584xx names referenced only by symbols.json rows + this .c (+ a literal pattern test in tools/test_import_symbol_names.py
  that does not read symbols.json). No semantic/ownership/parked/rejection rows for the unit.
## V10 SDK-table surplus policy seam
- Canonical rejections for rasterizer_xbox_profile and rasterizer_xbox_dynavobgeom are class candidate-only-comdat-owner on
  _D3DSIMPLERENDERSTATEENCODE and say forced-unresolved pair links do not prove ownership; BOTH also carry consumer-local
  declaration debt. sdk_census.py: 62 of 489 canonical Matching objects (14 rasterizer_xbox_*) emit all 3 SDK tables as surplus.
  hardware_geometry has no consumer-local declarations (empty prototypes/globals sections). -> consistent with the 62-object
  canonical practice; disclosed for the integrator, not a blocker for this unit.
## V11 comment accuracy + amended packet
- Worker comment: "this no-op XDK wrapper and its D3DResource_MoveResourceMemory callee out of line (0x00158450, 0x00158460)"
  pairs wrapper->0x158450, callee->0x158460 by reading order; January is the reverse (D3DResource_MRM = 0x158450,
  D3DVertexBuffer_MRM = 0x158460; split_R sections 3/4, atlas map order). Non-blocking factual slip in a disclosure comment.
- amended/01 (mk_amended.py): comment only -> "wrapper (0x00158460) and its D3DResource_MoveResourceMemory callee (0x00158450)";
  blob c2bb9095..d14d32a2. git apply --check OK alone, with 02+03, and --cached. Post-image == cand_amended.c.
- cand_amended.obj vs split_R: 17/17 EXACT (guard passed), object_audit PASS, pdb 0, surplus 0, provider_link PASS, fake scan 0;
  every non-debug section byte-identical to cand_R.obj. amended/02 and amended/03 are byte copies of the worker's patches.
## VERDICT: APPROVE (use amended/01 + 02 + 03 jointly). REVIEW.md written.
