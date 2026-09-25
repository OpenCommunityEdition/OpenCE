# review_critic3_render_cameras LEDGER

Review of scratch/w/critic_3/patches/21_render_cameras.patch at HEAD 5d3ca708.

## R0 claim
- claimed source/render/render_cameras (label review_critic3_render_cameras), rc=0.

## R1 name evidence (independent, read-only)
- Sept-2001 cachebeta.map (2001-09-25, Static symbols section, line 19673): `0001:00172bc0 _render_camera_warn_once f render_cameras.obj`;
  next fn `_render_camera_new` at 0x172c60 => size 0xa0 (January 0x174fc0 size 0xa0, next `_render_camera_new` 0x175060).
- Aug-2001 map (1749betaP line 18491): `0001:00158150 _render_camera_warn_once`, next `_render_camera_new` 0x1581f0 => 0xa0.
- Neither map contains `check_warning_condition` anywhere.
- January atlas 4cc87b45 (2002-01-14 cachebeta.exe): off 0x574fc0 `_render_camera_warn_once` render_cameras.obj tier exact.
- bodycmp.py: January split section (0xa0, 13 relocs) vs Sept xbe .text+0x172bc0 and Aug xbe .text+0x158150, bytes masked
  only at January's 13 relocation fields: 0 differing bytes in both (raw-byte identity, not just instruction shape).
- callers.py (E8 scan): Sept 22 calls, all inside `_render_camera_build_frustum`; Aug 22, same owner; January PE 22 calls to
  0x574fc0 in 0x577464..0x5777bb = inside render_camera_build_frustum (0x576aa0 + 0xd30). Source has 22 call sites.
- January strings (cachebeta.exe): no `warn_once` / `check_warning` literal; assert text `id>=0 && id<MAXIMUM_RENDER_CAMERA_WARNING_CONDITIONS`
  and "### ERROR cameras: frustum-integrity condition #%d violated by %f" only -> nothing contradicts the Sept name.
- cachebeta_publics.txt: neither name is public -> January file-static; symbols.json `"static": true` kept (storage 3 both sides).

## R2 reference consistency
- git grep (tracked, excluding scratch): old name only in config/symbols.json:6283 (patched), render_cameras.c (patched: prototype,
  22 calls, definition), header comment `_code_00174fc0` (patched to the new name), parked.json build_frustum evidence PROSE
  (a statement about the PC corpus; still true), docs ledgers + research/ round-1 patches (historical evidence).
- No header, semantic_matches, semantic_data_matches, admission-rejection, splits, symbol_ownership or tools reference.
- Patched file: 25 occurrences of the new name (1 comment + 1 prototype + 22 calls + 1 definition), 0 of the old/placeholder.
- parked.json key is `_render_camera_build_frustum` (unchanged); parked measurements are name-free (normalized sha zeroes
  relocation fields) and re-measure equal: split_cand 3376/113/3ce4fcfd..., cand 3408/112/a0e3988a... (= recorded target/base).

## R3 measurements
- git apply --check at 5d3ca708: rc 0. GNU patch applies to slug copies (applytest/).
- gate prod vs build/split: 20 EXACT / 1 residual (build_frustum, parked). cand vs stale build/split: helper UNWRITTEN
  (expected: csplit regen required at integration). cand vs emulated split_cand: 20/1, rows SAME modulo the rename.
- csplit (build/tools/csplit.exe, config copies in slug; symbols.json edited by one in-place byte replacement, CRLF kept):
  split_ctl == build/split 833/833; split_cand differs from split_ctl ONLY in render_cameras.obj.
- objcmp.py (all sections bytes+relocations with target names, full symbol table): split_ctl vs split_cand IDENTICAL under the
  rename map; prod.obj vs cand.obj IDENTICAL under the rename map (only .debug$S differs); build/base vs prod.obj IDENTICAL.
- Battery (with_split redirect, tools unmodified): object_audit FAIL(1)=build_frustum residual on both sides (helper row ok 160/160);
  pdb_storage 1 pre-existing disagreement (`_previous_projection_coefficients`) both sides; surplus_identity 20 COMDATs, 0 not
  identical both sides; provider_link PASS both orders both sides.
- objdiff 3.3.1 (sha1 3130e428) objdiff/: prod, cand and build/base all 6721/10091 code, 20/21 fns, 1491/1491 data, 96.32176%.

## R4 other checks
- HCEX.pdb (2011 SHIP, DIA2Dump -s streamed through grep): render_cameras.obj lists render_camera_build_frustum etc. but
  NO warning helper under either name (and no warning-values data) -> neutral, no contradiction (checks absent in 2011 ship).
- /Od build has no symbols (names not available there); no new scopes/declarations in this patch -> /Od order check N/A.
- Strip test: no unusual construct (identifiers + one comment only). House rules: authentic first-party name replaces a
  PC-corpus (non-first-party) name and the stale `_code_00174fc0` placeholder in the symbol-listing comment (rule 7).
- Held classes (brief 2/8/9): none touched. render_camera_build_frustum park untouched (key, measurements, class).
- Sequencing note: round-1 FUZZY packets research/fifty_objects_20260925/w/render_cameras/production{,_owner_gated}.patch
  (not landed) spell the old helper name in context and added lines; if ever landed after 21, rename in them first.
  No R2 packet in scratch/w touches render_cameras (grep of *.patch; claims.log).

## R5 verdict
- APPROVE unchanged (no amendment needed). Zero credit, byte-inert; csplit regen required with the symbols.json edit
  (cand vs stale build/split shows the helper UNWRITTEN until the split is regenerated).
