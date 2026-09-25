# MANIFEST - review_critic3_render_cameras (adversarial review of critic_3 patch 21)

- Base/tip: worktree HEAD 5d3ca708; no tracked edits; no ninja/configure/git mutation. Claim: source/render/render_cameras
  (review_critic3_render_cameras), released at end.
- Packet reviewed: scratch/w/critic_3/patches/21_render_cameras.patch (config/symbols.json:6283 + render_cameras.c:
  header comment, prototype, 22 call sites, definition). `git apply --check` rc 0.

## Source evidence (name)
- Sept-2001 cachebeta.map Static symbols: `_render_camera_warn_once` 0001:00172bc0, size 0xa0 (next `_render_camera_new`).
- Aug-2001 map: `_render_camera_warn_once` 0001:00158150, size 0xa0.
- January atlas 4cc87b45: 0x574fc0 `_render_camera_warn_once` render_cameras.obj tier exact.
- Body: January split section vs Sept and Aug xbe bytes, masked at January's 13 relocation fields: 0 differing bytes (bodycmp.py).
- Callers: 22 E8 calls, all in render_camera_build_frustum, in Sept, Aug and January (callers.py).
- January strings: no contradicting literal. cachebeta publics: not public (file static) -> `"static": true` correct.
- HCEX.pdb 2011: helper absent under both names (neutral). Old name = PC reverse-engineering corpus (not first-party).

## Measurements (target vs candidate)
| function | target | candidate | gate |
|---|---|---|---|
| _render_camera_warn_once (was _check_warning_condition) | 160 / 13 relocs | 160 / 13 | EXACT (vs emulated split) |
| _render_camera_build_frustum (parked) | 3376 / 113 / 3ce4fcfd | 3408 / 112 / a0e3988a | residual (unchanged, = parked.json) |
| other 19 | - | - | EXACT (unchanged) |
- csplit: split_ctl == build/split (833/833); split_cand differs only in render_cameras.obj, IDENTICAL under the rename map.
- objcmp prod.obj vs cand.obj: IDENTICAL under the rename map (non-debug sections, relocations, symbol table).
- object_audit FAIL(1) both (build_frustum); pdb_storage 1 pre-existing disagreement both; surplus_identity 20/0 both;
  provider_link PASS both orders both.
- objdiff 3.3.1 (3130e428): 6721/10091 code, 20/21 fns, 1491/1491 data, 96.32176% for prod, cand and build/base.

## Credit
- Zero (names only). Integration: land symbols.json + .c together; regenerate the split (ninja does it).

## Notes
- parked.json build_frustum evidence prose still cites the PC-corpus name as a statement about that corpus (accurate; optional
  follow-up to add the first-party name). Docs/research ledgers keep the old name historically.
- Round-1 unlanded render_cameras FUZZY packets spell the old name; rename them first if ever landed after this packet.
