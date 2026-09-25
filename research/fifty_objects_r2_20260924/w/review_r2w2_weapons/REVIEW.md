# REVIEW - source/items/weapons storage packet (r2w2_weapons, ADMISSION_FIX_PROPOSED)

Reviewer slug: scratch/w/review_r2w2_weapons/. Tree: HEAD 434f0151. git status is clean apart from the pre-existing untracked research/fifty_objects_r2_20260924/, at the start and at the end.
I made no claim, because I edited no candidate source. The packet is reproduced here from HEAD blobs only.

## Verdict: APPROVE (no amendment)

The packet is zero-credit. It lands two patches jointly:
- 01: `static` on the weapon_magazine_finish_reload prototype (l.449) and definition (l.1382);
- 02: 11 in-place `, "static": true` flags in symbols.json.

The csplit regeneration is then done by the full ninja: build.ninja lists config/symbols.json as an implicit input of the csplit edge.

## Independent checks (all run by me, own tools where noted)
| check | result |
|---|---|
| git apply --check -v --whitespace=error at 434f0151 | clean for 01, for 02, and for both together |
| reproduction | GNU patch on HEAD blobs (mirror/) gives cand == worker cand.c and final/{weapons.c,symbols.json} with CR normalised. The blob hashes match the patch index lines (72e3597->c0baa42, f9abb17->17bd4c8). The diff is exactly 2 + 11 lines. |
| source state | The other 10 functions are already `static` on prototype and definition in weapons.c. Only finish_reload was external. |
| strip test / byte-inertness | own objdiff2.py, which compares bytes + reloc records + flags. build/base vs head.obj: 0 non-debug section diffs and 0 symbol diffs. head.obj vs cand.obj: 0 section diffs; 1 symbol diff, `_weapon_magazine_finish_reload` storage 2->3. |
| split emulation (own csplit runs, own cfg copies) | split_ctl == build/split for 833/833 objects. split_s vs split_ctl: only items/weapons.obj differs, by 11 bytes, which are exactly the 11 storage classes 2->3. 97/97 sections are equal. All 79 function fingerprints (section_infos_equal) are unchanged ctl vs s. |
| gate --all | cand vs build/split, cand vs split_s and head vs split_s all give the same rows: exact 78, residual 1 (_weapon_place [sha]), unwritten 0 |
| object_audit | prod vs split FAIL(11). cand vs split FAIL(12). head vs split_s FAIL(2) (finish_reload 3/2). cand vs split_s FAIL(1): only .text _weapon_place; 97 January symbols, 0 differ; 17 .rdata + .data all ok. JOINT landing is required. |
| pdb_storage | prod: 12 disagreements. cand vs split_s: 1 (_data_00307140, the held naming item B). |
| cachebeta publics | all 11 are absent by name and at their RVAs (file_offset == RVA). A post-patch census of every symbols.json row in 0xEA400..0xECF00 finds every external row is a public and every static row is a non-public, so the function packet is complete. |
| cross-TU refs | Own raw scan: NUL-bounded exact names across all 1,454 build/split + build/base objects give 0 hits outside weapons.obj. Source/header grep: only weapons.c. The config files relocs/splits/contribs/config/symbol_ownership/semantic_data have 0 hits. |
| Sept-2001 cachebeta.map (sha256 b63d3090...) | 'Static symbols' header at l.19268. All 11 are listed at l.20331-20358 (f weapons.obj). The controls _weapon_place/_new/_overcharged/_set_total_rounds are in the public section (l.2299-2316). |
| HCEX DIA2Dump (own runs) | `static function: true` for finish_reload (`static void (long, short)`), set_state and reset. weapon_busy has no record. The control weapon_place is a SymTag 0xA public. |
| surplus_identity | 17 COMDATs, 0 not identical. The output is identical to production's. |
| provider_link | 31 rows PASS in both orders against split_s, identical to production. `--baseline` finds no new surplus. |
| semantic pin | _weapon_set_state sha 667efc08 in split_ctl, split_s and cand, section_infos_equal True. The verifier (semantic_progress.apply_semantic_matches / coff_compare.symbol) does not depend on storage. |
| objdiff 3.3.1 (sha1 3130e428, own mini project) | prod/ctl, cand/s, prod/s and cand/ctl all give 13277/17595 code, 75/79 fn, 2052/2052 data, with identical per-function rows. This equals the build/report.json weapons row, so it is credit-neutral. |
| /W3 (own w3.py) | head and cand have 20 lines each, identical modulo the file name; no storage warning |
| fake_match_scan | 0 leads for head and for cand |

## Precedent audit (read the original commits)
- f04103c2 (owner commit, 2026-09-23) is the target-side precedent. It set symbols.json `"static": true` on action_flee/guard privates, with the quote "January cachebeta.pdb publics, no cross-TU refs". It also dropped `static` on aim_assist's compute_composite_attenuation because that function is public in January, so the storage rule runs both ways and the source follows the publics.
- 05255584 is the source-side precedent: 12 hs_runtime converters became static "per cachebeta publics", on an object that stays NonMatching. That makes it a zero-credit precedent on an incomplete object. The same commit gave hwgeom 13 symbols.json static rows, and 7979cf8f gave the IsBusy wrappers static rows ("none is a cachebeta public").
- 9f74e993 had already flagged weapons' own start_reload/state_next static. This packet completes the same object's set.
- Every condition these rulings applied holds here: absent from publics; no cross-TU reference; byte-inert; authentic names that are unchanged; no header, COMDAT, park or status change; not a held class. The precedent is not being stretched. The finish_reload joint edit is the union of the two directions that f04103c2 itself used.

## House rules
- The only construct is a storage keyword. The static prototype stays in the owning .c among the other static prototypes.
- There is no new scope or declaration, so /Od declaration order does not apply and the declaration count is unchanged.
- Strip test: removing `static` gives production bytes, so the keyword buys no byte. It is required only for storage truth, and three first-party sources attest it.

## Integrator notes
- Apply 01 and 02 together, then run the full ninja (which regenerates the split), the stable sweep and pytest.
- Expected: the weapons object_audit goes from FAIL(11) to FAIL(1) and pdb_storage from 12 to 1. Code, function, data and object credit are all 0.
- The object stays NonMatching on _weapon_place (class F) and the _data_00307140 naming item B. Both are owner-held and untouched.
