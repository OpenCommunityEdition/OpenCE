# source/items/weapons (admit source/items/weapons)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weapons\\patches\\01_weapons.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weapons\\patches\\02_symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weapons\\final\\weapons.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weapons\\final\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weapons\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weapons\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weapons\\LEDGER.md"
]

## production_changes
Two patches that must land together, followed by a csplit regeneration (a full ninja regenerates build/split).
(1) C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weapons\patches\01_weapons.c.patch (index 72e3597..c0baa42): `static ` is prefixed to the weapon_magazine_finish_reload prototype (l.449) and definition (l.1382). Nothing else changes.
(2) ...\patches\02_symbols.json.patch (index f9abb17..17bd4c8): `, "static": true` is appended in place to config/symbols.json lines 4013-4018, 4023, 4030-4031, 4047 and 4054. These are _weapon_trigger_get, _magazine_get, _busy, _magazine_state_change_ok, _get_effect_object_index, _get_owner_object_index, _effect_new, _state_interruptable, _set_state, _magazine_finish_reload and _reset. Row order is untouched and the file is not re-serialised.
`git apply --check --whitespace=error` is clean at HEAD 434f0151, singly and together. CRLF working-tree post-images are in ...\r2w2_weapons\final\. No header, config.json status, park, rejection or semantic entry changes.
This is zero credit: no code, function, data or object credit.

## evidence
Precedent: 05255584 made 12 hs_runtime converters static per cachebeta publics, and the approved hs_runtime reviewer form was a bare `static` on the owning-.c prototype and definition. The brief s3 rule is 'public = external, absent = file-static'. The action_flee/guard symbols.json static flags are the same class.

All 7 conditions hold (MANIFEST.md, 'Precedent' section):
- P1: all 11 are absent from cachebeta publics, both by name and at their RVAs. The controls _weapon_place, _new, _overcharged and _set_total_rounds are present.
- P2: an xref scan of 1,454 build/split and build/base objects finds 0 references outside weapons.obj, and no header declares any of the 11.
- P3: byte-inert. head.obj vs cand.obj: 154/154 sections are equal; the only symbol diff is the finish_reload storage 2->3. Gate rows are identical. All 79 function fingerprints are unchanged in split_ctl vs split_s. objdiff 3.3.1 is identical: 13277/17595 code, 75/79 functions, 2052/2052 data, equal to build/report.json.
- P4: the form is exact.
- P5: split_ctl equals build/split for all 833 objects. In split_s only weapons.obj changes, by exactly 11 storage-class bytes.
- P6: surplus_identity finds 17 COMDATs, all identical. provider_link: all 31 rows PASS in both orders; --baseline shows no new surplus. The r1 p4 real_random fix is already canonical.
- P7: none of the changes falls in a held class.

Independent first-party sources:
- The Sept-2001 cachebeta.map lists all 11 under 'Static symbols' (l.20331-20358). The split-external names that the map lists as static are exactly these 11.
- HCEX DIA2Dump reports 'static function: true' for 7 of the 11; the other 4 have no HCEX record.

Measured results against the emulated split_s:
- gate --all: exact 78, residual 1 (_weapon_place [sha]).
- object_audit: FAIL(1), .text _weapon_place only. 97 January symbols, 0 differ. All 18 data sections (17 .rdata + .data 1536) are ok.
- pdb_storage: 12 disagreements drop to 1 (_data_00307140, the held naming item).
- The semantic_matches _weapon_set_state pin (sha 667efc08) still holds.
- /W3 warnings are identical (20 lines), and fake_match_scan finds 0 leads.

Joint landing is required, as measured with object_audit:
- source patch only: FAIL(12);
- symbols.json only: FAIL(2);
- both patches + regenerated split: FAIL(1).

## blockers
The object stays NonMatching at 78/79 because two owner-held items remain, and neither is touched here:
(A) _weapon_place (208 B) is exact only with the HCEX-attested `long` return bound to the void datum_place slot. That is class F, an authentic interface bug that needs owner approval.
(B) _data_00307140 (the invented weapons_globals struct) is external but not a PDB public. The fix, splitting it into blurred_permutation_names (HCEX) plus a descriptive weapon_update_section, is in the held naming class.

## reopen_criteria
Object completion needs owner rulings on both items.
(A) A yes to the class-F long-return item lands the r1 p2 packet: weapons.h long prototype plus the object_types.c slot cast with its BUG comment.
(B) A yes to the naming item lands the r1 p1 packet and the p3 data rows (loose statics blurred_permutation_names and weapon_update_section) together with the csplit regeneration.
After this packet, those two are the only audit failures.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weapons\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weapons\LEDGER.md (sections S0-S6). Evidence files are in the same slug:
- gate: gate_{head,cand}_vs_split{,_s}.txt
- object_audit: audit_{prod_vs_split,cand_vs_split,head_vs_split_s,cand_vs_split_s}.txt
- storage: pdb_storage_{prod,cand_split_s}.txt
- surplus and links: surplus_identity_{prod,cand}.txt, provider_link_{prod,cand,cand_baseline}.txt
- references and maps: xref_scan.txt, sept_map_weapons_static.txt, hcex_*.txt
- objdiff: od/report.json
- warnings: w3_{head,cand}.txt
- split emulation: mk_cfg.py, cfg_ctl/, cfg_s/, split_ctl/, split_s/
- helpers: with_split.py, objcmp.py, warn.py

The claim was taken as r2w2_weapons and has been released. Tracked files were never edited, and git status is unchanged (only the pre-existing untracked research/fifty_objects_r2_20260924/).
