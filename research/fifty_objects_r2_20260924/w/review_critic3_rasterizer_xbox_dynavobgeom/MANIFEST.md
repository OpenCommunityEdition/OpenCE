# MANIFEST - review of critic_3 patch 18 (rasterizer_xbox_dynavobgeom: submit_screen_vertex)

- Base/tip: worktree HEAD 5d3ca708; `git status` clean apart from the pre-existing untracked research/ dir, at start and end.
  No tracked file edited; no ninja/configure/git mutation. Claim label review_critic3_dynavobgeom (released at end).
- Packet under review: scratch/w/critic_3/patches/18_rasterizer_xbox_dynavobgeom.patch
  (`rasterizer_screen_geometry_submit_vertex` -> `submit_screen_vertex`, static; .c symbol comment + prototype + call +
  definition; config/symbols.json line 5720, file_offset 1372400 = 0x14F0F0, "static": true kept).

## Source evidence (the one changed construct: a name)
- Sept-2001 cachebeta.map (Static symbols): `0001:0014cda0 _submit_screen_vertex ... rasterizer_xbox_dynavobgeom.obj`, 0x40,
  between unlit_geometry_draw (0x270) and psuedo_dynamic_screen_quad_draw. That is the same slot, size and neighbours as January.
- Sept-2001 cache.map (Static symbols): `0001:000fcf30 @submit_screen_vertex@4`, 0x40, directly before quad_draw. The fastcall
  @4 shows exactly one 4-byte parameter, which matches our signature.
- Sept xbe body vs January split: all 0x40 bytes are identical outside the 3 rel32 fields. The Sept callees resolve to
  _D3DDevice_SetVertexDataColor@8 and _D3DDevice_SetVertexData2f@12 x2, which are January's relocation targets.
- Callers: none as E8 calls in either build, because the quad_draw loop is inlined and the static is still emitted.
- No contradiction: January exe strings, atlas (the Sept atlas tier holds publics only), HCEX.pdb, HaloCEA reference, Aug-2001
  map (object predates the helper). The PDB publics have nothing at [0014F0F0], so the function was a January file static.
- Current name: invented descriptive name at the 2026-09-03 admission (docs log line 12).

## Target vs candidate
| function | size | relocs | gate prod vs split_ctl | gate cand vs split_cand |
|---|---|---|---|---|
| submit (renamed) | 64 | 3 | EXACT | EXACT |
| other 16 | - | - | EXACT | EXACT |
- 17 exact / 0 / 0 on both sides. objcmp prod.obj vs cand.obj and split_ctl vs split_cand: IDENTICAL modulo the rename.
- Emulated split: split_ctl == build/split 833/833, and split_cand differs only in this object.

## Data / symbols / surplus
- Data identical. object_audit FAIL(2) on both sides (pre-existing .bss latch row). pdb_storage 1 on both sides (pre-existing).
  surplus_identity: 0 not identical. provider_link: identical to production; no new surplus.

## House rules
- Only identifiers change. The name is authentic first-party (rule 7) and the storage is static (PDB). Nothing else is added,
  so the strip test and the /Od checks are N/A.

## Credit
- Zero: objdiff 3.3.1 is unchanged (2159/5112 code, 16/17 fns, 525/525 data). The object stays held and NonMatching.

## Tests run
- git apply --check; GNU patch; csplit x2; gate x4; objcmp x3; object_audit / pdb_storage / surplus_identity /
  provider_link (+ --baseline) on prod and cand; tinfo --data x4; objdiff 3.3.1 report; DIA2Dump HCEX; map, atlas and
  string searches.
- Not run: full ninja and pytest (integrator).
