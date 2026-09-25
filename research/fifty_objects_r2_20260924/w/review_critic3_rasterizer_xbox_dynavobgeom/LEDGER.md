# review_critic3_rasterizer_xbox_dynavobgeom LEDGER

Adversarial review of scratch/w/critic_3/patches/18_rasterizer_xbox_dynavobgeom.patch
(rasterizer_screen_geometry_submit_vertex -> submit_screen_vertex, static). Worktree HEAD 5d3ca708.
Claimed source/rasterizer/xbox/rasterizer_xbox_dynavobgeom as review_critic3_dynavobgeom.

## R1 body identity (body.py -> body.txt, independent of critic_3's xcmp)
- Sept-2001 cachebeta.map: `0001:0014cda0 _submit_screen_vertex ... rasterizer_xbox_dynavobgeom.obj` in the Static symbols
  section; next symbol __rasterizer_psuedo_dynamic_screen_quad_draw @0x14cde0 => size 0x40; preceded by
  __rasterizer_dynamic_unlit_geometry_draw @0x14cb30 (0x270). January split: unlit draw 0x270 @0x14EE80, submit 0x40 @0x14F0F0,
  quad draw @0x14F130: same neighbour order and sizes.
- Sept xbe bytes at VA 0x15dda0 (.text va 0x11000) vs January split `_rasterizer_screen_geometry_submit_vertex`: all 0x40
  bytes identical outside the 3 rel32 call fields; Sept call targets 0x2087e0 / 0x208660 / 0x208660 resolve (map section 0002
  = xbe D3D va 0x201dc0) to `_D3DDevice_SetVertexDataColor@8` (0002:6a20) and `_D3DDevice_SetVertexData2f@12` (0002:68a0) x2 =
  January's three relocation targets exactly. => body identical INCLUDING callees.
- Callers: no E8 call to the Sept static; no relocation to it in the January split (the loop in quad_draw is inlined and
  the static is still emitted, brief sec 9 law). Callers evidence is vacuous but consistent (empty in both builds).
## R2 name provenance (independent)
- Second first-party map: Sept-2001 retail `cache.map` (2001-09-25 12:29:10) line 14157 `0001:000fcf30 @submit_screen_vertex@4
  rasterizer_xbox_dynavobgeom.obj`, immediately followed by `@_rasterizer_psuedo_dynamic_screen_quad_draw@8` @0xfcf70 (size 0x40).
  So TWO Sept-2001 Bungie link maps (cachebeta.map cdecl, cache.map fastcall) name this slot submit_screen_vertex.
- Aug-2001 map: object has neither quad_draw nor the helper (added between Aug and Sept) - no contradiction.
- Atlas (halo_ce + hcea jsonl, all builds): neither name anywhere; January atlas names only 0x54ee80 (unlit draw) in this range.
- HCEX.pdb (DIA2Dump -sym, tool validated on create_weapon): neither name (quad_draw present, PPC lineage) - no contradiction.
- HaloCEA reference src: no helper name. January cachebeta.exe strings: none of submit_screen_vertex / submit_vertex /
  screen_geometry_submit / screen_vertex. The only dynavobgeom literal is the __FILE__ path.
- Current name origin: docs/object_matching_logs/rasterizer_xbox_dynavobgeom_obj_opus_reconciliation_20260903.md line 12 -
  an invented descriptive name chosen at admission. First-party name should replace it (house rule 7 prefers authentic).
## R3 patch application + emulated split + gate (independent)
- `git apply --check` of patch 18 at 5d3ca708: rc 0. GNU `patch -p1` onto HEAD copies in patchroot/: 4 .c lines
  (symbol-listing comment, prototype, call site, definition) + symbols.json line 5720 (file_offset 1372400 = 0x14F0F0,
  "static": true kept). `git grep` (outside scratch/research): old name remains only in two historical docs logs (prose).
  New name has zero prior occurrences in the tree -> no collision.
- csplit.exe -p cfg_ctl (HEAD config copy) -> split_ctl == build/split 833/833 (emulation validated); cfg_cand (patched
  symbols.json) -> split_cand differs from split_ctl in rasterizer_xbox_dynavobgeom.obj ONLY; objcmp (non-debug sections
  bytes+relocs, ordered symbols with rename map) IDENTICAL, 133/133 symbols; raw size -20 B = string-table name shrink.
- gate (with_split.py redirect, tools unmodified): cand vs split_cand 17 exact / 0 / 0 (`_submit_screen_vertex` EXACT 64);
  prod vs split_ctl 17/0/0; cand vs old split 16 + UNWRITTEN (expected: symbols.json must land with the .c).
- objcmp build/base vs prod.obj IDENTICAL (control); prod.obj vs cand.obj IDENTICAL modulo rename (51 non-debug sections,
  195 symbols same order/storage/section).
## R4 section-4 battery (battery.txt; with_split.py redirects build/split -> split_cand and build/base/<unit>.obj -> cand.obj)
- object_audit: production FAIL(2) == candidate FAIL(2); the only row that changes is the renamed .text owner
  (`_submit_screen_vertex` ok 64/64). Both FAIL rows are PRE-EXISTING and unrelated: January .bss is 1 byte (`?warned@...`
  at 0) while ours is 2 bytes (external `_reported_too_many_transparent_geometry_groups` at 0, `warned` at 1).
  NOTE for integrator: config/object_admission_rejections.json dynavobgeom entry says "all 31 January-owned sections/symbols
  match" - the current audit disagrees on that .bss row (not caused by this patch).
- pdb_storage: 1 disagreement both sides (`_reported_too_many_transparent_geometry_groups` candidate-only external), pre-existing.
  cachebeta publics list [0014EE80] unlit draw and [0014F130] quad draw but nothing at [0014F0F0] -> January static;
  split storage 3, source `static`, symbols.json "static": true: agree.
- surplus_identity: 4 code COMDATs, 0 not identical (both). provider_link (both orders, internal): same 19 PASS + the
  pre-existing `_reported_too_many_transparent_geometry_groups` NO PROVIDER FAIL on both sides; `--baseline=prod.obj`:
  no new surplus (PASS, nothing to link).
- Data: tinfo --data prod.obj vs cand.obj differ only in .debug$S object-path text (gate PID); split_ctl vs split_cand data IDENTICAL.
- objdiff 3.3.1 (sha1 3130e428..., objdiff/report.json): prod/split_ctl, cand/split_cand, build/base/build/split all
  2159/5112 code, 16/17 fns, 525/525 data, 98.69271 (quad_draw 97.74 = known scorer defect, unchanged). Zero credit.
## R5 consistency / policy
- References: prototype, call site, definition, symbol-listing comment, symbols.json 5720 all updated; no parked.json /
  semantic_matches / semantic_data_matches / object_admission_rejections / symbol_ownership / tools / tests reference either
  name (git grep). Two historical docs logs mention the old name in prose (historical record; no update required).
- No other patch in critic_3/patches touches symbols.json within +-60 lines of 5720 (19 @5783, 20 @5807, 09 @5874): independent.
- House rules: identifiers only; no new constructs -> strip test and /Od declaration-order checks are N/A (no casts, parens,
  locals, scopes or statements added). Rule 7: authentic first-party name replaces an invented descriptive one.
  The fastcall `@submit_screen_vertex@4` in cache.map also attests exactly one 4-byte parameter = our signature.
- Held class (brief 8): dynavobgeom stays held for SDK-table coalescing + consumer-local global_window_parameters; this packet
  claims no admission and does not touch either held reason; the rename is backed by NEW first-party evidence (two Sept maps).
## R6 verdict
- APPROVE as-is (no amendment). Zero credit, byte-inert, object remains NonMatching/held.
