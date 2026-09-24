# review source/items/weapons
approve=False

## checks
All checks were independent. Notes are in scratch/w/review_weapons/REVIEW.md. Worktree e9e62b78 is clean and no tracked file was touched.
(1) Patch integrity: production.patch applied with `patch -p1 --binary` to a fresh mirror (review_weapons/applymirror) reproduces final/{weapons.c, weapons.h, object_types.c, action_charge.c, symbols.json} byte-for-byte.
(2) Consumer blast with my own harness (blast2.py + objcmp2.py). I grepped the includers: weapons.h has exactly 19 direct includers and no header includes it. The control reproduces all 19 build/base objects with 0 diffs (sections, bytes, relocations, full symbol table). With the final header and sources, the other 17 consumers and object_types are identical to the control once internal label numbering is normalised. Units stays 189/189, game_engine 180/180, objects 121/121, object_types 31/31. The research12 units regression does not reproduce.
(3) Own csplit with final/symbols.json: only weapons.obj differs from build/split.
- gate-equivalent against the re-split: 79/79 strict.
- object_audit (SPLIT_ROOT copy, checked against the original tool): PASS, 98 symbols, 0 differ.
- surplus_identity: 17 COMDATs, all identical, same set as production.
- objdiff 3.3.1 in a scratch project: .data and .rdata 100%, _weapon_place 100%. The objdiff under-credit on 3 other functions is pre-existing.
(4) cachebeta publics: all 11 functions and the data symbols are absent (file-static). A build/split census shows none of them is referenced outside weapons.obj; _weapon_place is referenced only by object_types.
(5) My own MSF7/DBI/TPI parser (pdbcheck.py) on HCEX.pdb (sha f55cfe95, not the same PDB research12 used):
- weapon_place GPROC32 is LF_PROCEDURE returning 0x12 (long), with args (long, scenario_weapon_datum *). Parser control: weapon_new and weapon_update return 0x20.
- finish_reload is an LPROC32 returning void (long, short).
- blurred_permutation_names is LDATA32 char*[2].
- All 9 other *_place callbacks return void.
- HCEX has no profile statics, and the symbol atlas has no name for this one.
(6) /W3 warnings are identical before and after: weapons 20, object_types 12, action_charge 12.
(7) STRIP TEST of the object_types cast: the plain `weapon_place` gives a byte- and symbol-identical object_types.obj and only adds C4028. The cast is inert.
(8) fake_match_scan: 0 leads.
(9) p4 (action_charge):
- Gate is 21/22 before and after. The residual is _action_charge_perform. The real_random caller, action_charge_setup, is exact.
- objcmp2 finds 0 diffs. The only change is _real_random's COMDAT selection, NODUP to ANY, which now matches every other header-inline helper in the object.
- object_audit is unchanged and provider_link passes.
- Pair links: weapons + base action_charge gives LNK2005 in both orders; weapons + p4 action_charge gives none in either order.
(10) Fallback (p1+p3+p4 without the header change): the object is identical to the primary.
(11) Subsets I measured:
- 'Void subset' (everything except the long return): 78/79 against the re-split.
- 'Static-only subset' (11 static flags plus static finish_reload): the csplit changes only weapons.obj, the gate stays 78/79 against both the current split and the re-split, and object_audit shows 97 symbols with 0 differ; its only failure is _weapon_place.

## issues
No technical defect found: the worker's measured claims reproduced independently. approve=false is a POLICY HOLD. The patch lands two items the brief and earlier owner rulings reserve for the owner.
(A) _weapon_place long return bound to the void datum_place slot with a `BUG:` comment is a reproduction of an authentic January interface bug. WORKER_BRIEF s2 says such a change "needs target+layout proof, a BUG comment and owner approval - report, don't land". This exact item is class F ("return ABI (_weapon_place)", 208 B) of the Lane D owner-ruling queue (docs/object_matching_logs/claude_lane_d_refresh_HANDOFF_20260922.md), and no ruling has been recorded since. Research12's House21 review was another lane's reviewer, not the owner.
New evidence the owner should see:
- The units regression is gone at e9e62b78 (the 19-consumer blast is clean).
- The raw long type appears in two different HCEA/HCEX PDBs.
- It is the only long return among the 10 placement callbacks.
- The strip test shows the cast is byte-inert.
(B) The p3 symbols.json split of _data_00307140 adds the static name `_weapon_update_section`. That name is descriptive, not authenticated: HCEX has no profile statics and the atlas has no name for it. The owner ruling of 2026-09-15 held symbols.json static splits with invented names and said not to re-ask without authenticated names. HCEX now authenticates blurred_permutation_names and shows the weapons_globals struct is invented, which justifies a re-ask, but it is still an owner question.
(C) The proposed FALLBACK (p1+p3+p4) is not house-rule clean and should not be offered. It leaves object_types.c's consumer-local `void weapon_place(long, struct scenario_object_datum *)` prototype contradicting a `long` definition, with no BUG disclosure (rule 9 and a lying cross-TU declaration). It also leaves a public function with no owner-header prototype. If the owner rejects p2, the long return should not land at all.
(D) Integration hazard: the p1 data respell and the p3 data lines must land atomically with the csplit regeneration. Production source against the new split scores 77/79, and the new source against the old split scores 78/79, losing _weapon_update on relocation identity.
Admissible now without a ruling (zero code credit; correctness only, measured):
- the 11 `"static": true` lines of p3;
- `static` on the weapon_magazine_finish_reload prototype and definition;
- p4 (remove action_charge's NODUP real_random hand copy).
After those, weapons is 78/79 with object_audit 97 symbols / 0 differ (only _weapon_place fails), and the _real_random LNK2005 is fixed.
Follow-up: objdiff under-credits the strict-exact _weapon_export_function_values and _weapon_update, and neither has a semantic entry. Both are pre-existing.
