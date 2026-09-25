# LEDGER - r2w2_weapons (admission: storage packet under cachebeta-publics precedent)

Tree: HEAD 434f0151 (= 05255584 + r2 ledger doc only). git status at start: only `?? research/fifty_objects_r2_20260924/` (untracked, pre-existing).
Claim: source/items/weapons label r2w2_weapons (CLAIMED, exit 0).

## S0 inputs read
- WORKER_BRIEF_R2 s0-9; TRIAGE__source_items_weapons.md; r1 REVIEW__source_items_weapons.md ('Admissible now without a ruling');
  r1 w/weapons/LEDGER.md (probes 1-5); precedent_audit LEDGER ('weapons: HELD_CLASS ... 11 static flags: COVERED'; 'FAIL (not a git patch): weapons/symbols_static.diff').
- Precedent sources: 05255584 commit msg (hs_runtime "12 typecast converters are static per cachebeta publics"; hwgeom "13 wrapper rows go static");
  7979cf8f (IsBusy wrappers static, "none is a cachebeta public"); review_r2_hs_runtime_storage_12_converters/REVIEW.md (APPROVE; the form = `static` keyword on prototype+definition, nothing else);
  claude_object_closure_20260924.md (action_flee/action_guard symbols.json "static": true per cachebeta publics; no cross-TU refs).
- Source state: the 10 non-finish_reload functions are ALREADY `static` in weapons.c (prototypes 367-444, definitions 1623-2033); only
  weapon_magazine_finish_reload is external (l.449 prototype, l.1382 definition); callers only in weapons.c (1989, 2951). No header declares any of the 11.
- symbols.json lines 4013-4018, 4023, 4030-4031, 4047, 4054 at HEAD equal the '<' side of symbols_static.diff (CRLF working tree, LF index).

## S1 candidate + gate vs CURRENT split (build/split)
- head.c = `git show HEAD:source/items/weapons.c` (== worktree file with CR stripped). cand.c = head.c with `static ` prefixed at l.449 (prototype) and l.1382 (definition); nothing else (diff: 2 lines).
- gate head.c: `== exact 78 residual 1 unwritten 0` (residual _weapon_place [sha]); gate cand.c: identical rows text (diff empty). gate_head_vs_split.txt / gate_cand_vs_split.txt.
- objcmp.py (slug): build/base weapons.obj vs head.obj = 154/154 sections, 380/380 symbols, 0 diffs (gate compile == production).
  head.obj vs cand.obj: 0 section diffs; exactly 1 symbol diff: _weapon_magazine_finish_reload storage 2 -> 3 (section 148, value 0 unchanged).
  => STRIP TEST: removing `static` restores production bytes exactly; the keyword buys no byte.

## S2 split emulation (build/tools/csplit.exe v0.0.2 -i cachebeta.exe -p <slug>/cfg_X -o <slug>/split_X; mk_cfg.py)
- cfg_ctl = verbatim copy of config/ (diff -r: identical). cfg_s = copy + in-place edits of symbols.json lines 4013-4018, 4023, 4030-4031, 4047, 4054
  (`, "static": true` appended inside the row; CRLF preserved; row order untouched). Line diff cfg_ctl->cfg_s (symbols_s.linediff) == r1 symbols_static.diff (CR-normalised).
- split_ctl vs build/split: 833/833 objects byte-identical (diff -rq empty) -> build/split is current for this tree.
- split_s vs split_ctl: exactly 1 object differs (source/items/weapons.obj). objcmp: 97/97 sections 0 diffs; 273/273 symbols, 11 diffs = the 11
  storage classes 2->3; `cmp -l` = exactly 11 differing bytes (same length 42231).
- with_split.py copied verbatim from scratch/w/rasterizer_xbox_hardware_geometry (read: open() redirect only, writes nothing).

## S3 gate / audit / storage vs emulated split_s
- gate cand vs split_s: `== exact 78 residual 1 unwritten 0` (only _weapon_place [sha]); head vs split_s: same rows (gate does not read storage).
- object_audit:
  prod (build/base) vs build/split: FAIL(11) = _weapon_place .text + 10 storage rows split 2/ours 3.
  cand vs build/split: FAIL(12) (adds finish_reload 2/3) -> source alone is WORSE; head vs split_s: FAIL(2) (finish_reload 3/2) -> symbols.json alone is incomplete.
  cand vs split_s: FAIL(1) = ONLY `DIFF .text sym:_weapon_place`; 97 January symbols, 0 differ; all 17 .rdata + .data _data_00307140 (1536) ok.
  => JOINT LANDING: weapons.c patch + symbols.json patch + csplit regen together.
- pdb_storage: prod 12 disagreements (the 11 + _data_00307140); cand vs split_s: 1 (_data_00307140 split 2 ours 2, PDB static = the held owner naming item B, out of scope).
- cachebeta publics: all 11 names absent (by name and by RVA = file_offset 0xEAB30..0xEC730); controls _weapon_place/_new/_overcharged/_set_total_rounds present.
- Sept-2001 cachebeta.map (first-party; .../astra-inputs/earlier-map-archives/2001-09-25/cachebeta.map, sha256 b63d3090...): 'Static symbols' header at l.19268;
  all 11 listed there (l.20331-20358, 'f weapons.obj'); controls _weapon_place/_overcharged/_set_total_rounds/_new in the public section (l.2299-2316).
  37 weapons.obj statics in Sept; census vs split: the split-external names that Sept lists static are EXACTLY the 11 (packet complete); no HEAD-static row contradicts Sept.
- HCEX.pdb DIA2Dump (own runs, hcex_*.txt): `static function: true` for magazine_state_change_ok, get_effect_object_index, get_owner_object_index, effect_new,
  set_state, magazine_finish_reload (`static void weapon_magazine_finish_reload(long, short)`), reset; trigger_get/magazine_get/busy/state_interruptable absent
  from HCEX (no record); controls weapon_place/weapon_new have SymTag 0xA publics.
- xref_scan.py: 1454 objects (build/split + build/base) -> 0 references/definitions of the 11 outside weapons.obj. Source grep (source libs tools config): only weapons.c.

## S4 surplus / provider / data / scorer / warnings / scans
- surplus_identity (cand vs split_s, with_split redirect): 17 candidate-only code COMDATs, 0 not identical; output byte-identical to production's run.
- provider_link cand vs split_s: 31 surplus rows (14 literals/reals + 17 COMDATs incl. _real_random, _point_from_line3d) all PASS in both link orders;
  rows identical to production's run; `--baseline=build/base/...weapons.obj`: no new surplus. (r1 p4 action_charge real_random fix is already canonical: action_charge.c has no #define/hand copy.)
- Data coff_compare: object_audit rows = all 18 January data sections ok in cand vs split_s (17 .rdata literals/reals + .data _data_00307140 1536 B);
  cand vs prod: 154/154 sections equal (objcmp). tinfo --data json dumps kept (tinfo_data_*.json).
- 79/79 function fingerprints (normalized sha + relocation json) identical split_ctl vs split_s -> config/semantic_matches.json `_weapon_set_state` pin (sha 667efc08...) still holds (cand sha 667efc08..., section_infos_equal True).
- objdiff 3.3.1 (sha1 3130e428) mini project od/: prod_vs_split, cand_vs_split_s, prod_vs_split_s all 13277/17595 code, 75/79 fn, 2052/2052 data; per-function rows identical;
  equals build/report.json weapons row (13277/17595, 75, 2052/2052). Credit-neutral. Pre-existing objdiff <100 rows: _weapon_place, _weapon_set_state (semantic entry), _weapon_export_function_values, _weapon_update.
- CL /Zs /W3 (warn.py; unit flags as gate.py): head 20 lines, cand 20 lines, identical text (no C4211/C4273 storage warnings introduced).
- tools/fake_match_scan.py (read first; stdout only): head 0 leads, cand 0 leads.
- parks / object_admission_rejections / semantic_data_matches / symbol_ownership: no weapons entries. config.json status stays NonMatching (index 235).

## S5 packet
- patches/01_weapons.c.patch (index 72e3597..c0baa42; 2 hunks, 2+/2-; sha256 67e27607...), patches/02_symbols.json.patch (index f9abb17..17bd4c8; 2 hunks, 11+/11-; sha256 8e7b6764...).
- `git apply --check -v --whitespace=error` at HEAD 434f0151: clean singly and together (read-only). GNU `patch -p1 --binary` on a mirror of the HEAD blobs reproduces cand.c and sym_s.json byte-for-byte; sym_s.json == cfg_s/symbols.json (CR-normalised).
- final/weapons.c + final/symbols.json = CRLF working-tree post-images.
- git status unchanged throughout (only the pre-existing untracked research/fifty_objects_r2_20260924/).

## S6 verdict
- ADMISSION_FIX_PROPOSED (zero credit): patches/01 + 02 land together with the csplit regen; object_audit FAIL(11)->FAIL(1), pdb_storage 12->1, gate 78/79 unchanged,
  provider link PASS both orders, surplus identical, data identical, objdiff credit-neutral. All 7 precedent conditions hold (MANIFEST.md 'Precedent').
- Object remains NonMatching: _weapon_place (class F long return) and _data_00307140 naming (item B) are owner-held and untouched.
- Claim released after this entry. git status unchanged.
