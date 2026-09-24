# weapons.obj closure worker ledger (slug scratch/w/weapons)

Base: e9e62b78 (lane claude/fifty-objects-20260925). Target: `_weapon_place` (208 padded, unparked).

## Baseline
- gate.py source/items/weapons --all: exact 78, residual 1 (`_weapon_place` [sha]), unwritten 0.
- Prior ledgers read: weapons_obj_evidence_closeout_20260919 (long-return exact but rejected as HCEA-void premise),
  astra research12 tail/owner/probe (HCEA raw LF_PROCEDURE return 0x12 long; /Od PE reloads [ebp+8] into EAX
  before epilogue; 3-file owner patch exact for weapons but weapons.h edit regressed units
  `_unit_preprocess_node_orientations` 189->188 (load-order tie; decl-count context sensitivity)).
- Recorded negatives NOT to repeat: 3-file weapons.h+object_types.c owner patch as-is; placement+projection composition.

## Probe 1 - long return in weapons.c only (hypothesis: target `mov ebx,[ebp+8]` ... `mov eax,ebx` at +0xac = returned weapon_index)
- Evidence: alndiff base vs January: only diffs are EBX-homed weapon_index (+ push/pop ebx), flag RMW scheduling
  consequences, and the terminal `mov eax, ebx` (target +0xac) absent in ours.
- Change: `long weapon_place(...)`, terminal `return weapon_index;` (scratch/w/weapons/cand_long.c). No header change.
- Result: gate `== exact 79 residual 0 unwritten 0` (79/79).

## HCEX.pdb raw type re-attestation (own MSF7 parser, scratch/w/weapons/tpi_read.py, struct_read.py, place_scan.py)
- HCEX.pdb module weapons.obj (stream 2733), S_GPROC32 weapon_place type 0x635bb = LF_PROCEDURE ret 0x12 (long), cdecl, 2 params
  (arglist 0x635ba = [0x12 long, 0x635b9 -> ptr to 0x635b8]). Control: weapon_magazine_finish_reload 0x614fa ret 0x3 (void).
  (research12 cited different stream/offset/type indices 2892/0xfb0/0x708b0 - probably a different HCEA PDB; same conclusion.)
- object_type_definition (0x628f0, 196 bytes): datum_place @+0x2c proc 0x628d9 ret void (long, void*).
- ALL other placement callbacks in HCEX return void: control/light_fixture/machine/placeholder/scenery/biped/unit/vehicle/equipment_place
  (ret 0x3). weapon_place is the UNIQUE long-returning placement function -> an authored type, not a pattern artefact.

## Resume session 2 (after usage-limit cut-off)
- Read LEDGER + od_weapon_place.txt; probe 1 (weapons.c-only long return = 79/79) stands. Next: measure the owner-header
  landing at CURRENT base e9e62b78 (research12 regression was measured at dc56bd8e; units source has changed since).

## Harness: shadow-header blast test (scratch/w/weapons/blast.py + objcmp.py)
- Each of the 19 direct weapons.h includers is compiled via gate.py --edits that swaps its weapons.h include for an absolute
  include of scratch/w/weapons/<variant>/weapons.h (no tracked file touched). objcmp.py compares every function section
  (section_infos_equal), every non-text section (size, reloc count, bytes) and the symbol set.
- CONTROL (shadow_ctl = verbatim copy): all 19 objects == build/base production objects, 0 diffs each.
  Baseline gates: actors 74/76, actor_combat 32/34, aim_assist 16/16, game_engine 180/180, ctf 39/39, oddball 38/38,
  players 67/70, player_control 49/49, fpw 33/34, hud 22/22, hud_draw 22/23(1 unw), hud_weapon 13/16, interface 17/18,
  weapons 78/79, objects 121/121, object_types 31/31, bipeds 42/51, units 189/189, unit_scripting_commands 11/11.

## Probe 2 - genuine-owner 3-file landing at CURRENT base (variant shadow_new)
- Hypothesis: research12's units loss (_unit_preprocess_node_orientations 189->188) was a name-count tie measured at
  dc56bd8e; units.c has since been rewritten to 189/189 at e9e62b78, so the tie point may have moved. Measure, don't assume.
- Change: weapons.h += `struct scenario_weapon_datum;` + `long weapon_place(long weapon_index, struct scenario_weapon_datum *scenario_weapon);`
  (beside weapon_new); weapons.c = cand_long.c; object_types.c: delete the void weapon_place facade prototype and bind the
  table slot with `(void (*)(long, struct scenario_object_datum *))weapon_place`.
- Result (19 consumers): weapons 79/79 (+_weapon_place), object_types 31/31, units 189/189, every other consumer unchanged.
  objcmp vs control: ZERO function-section / non-text-section / non-label-symbol differences in all 18 other consumers;
  only internal $L / $state_not_allowed$ label renumbering (name-count shift, same section+offset). => research12's
  regression does NOT reproduce at current base. Evidence: scratch/w/weapons/blast/{shadow_ctl,shadow_new}/, new_vs_ctl.txt.

## Section-4 audit of the probe-2 object (cand_owner.obj = blast/shadow_new/weapons.obj)
- object_audit: every January-owned SECTION ok (incl. .data 1536, .rdata literals, all 79 .text). Surplus = the known
  admitted shared-header COMDAT set (identical to production base; nothing new). FAIL(10) = symbol storage only:
  split=2(external) ours=3(static) for _weapon_busy/_effect_new/_get_effect_object_index/_get_owner_object_index/
  _magazine_get/_magazine_state_change_ok/_reset/_set_state/_state_interruptable/_trigger_get.
- pdb_storage: all 10 are ABSENT from cachebeta publics (=January file-static) -> ours is right, split is csplit
  attribution (symbols.json lacks "static": true). Also _weapon_magazine_finish_reload (ours 2, PDB static, HCEX LPROC)
  and _data_00307140 (ours 2, PDB static) disagree with PDB truth.
- Census: none of these 12 names is referenced by ANY other build/split object (only weapons.obj defines them;
  _weapon_place is the only one referenced cross-object, by object_types) -> static in split is link-safe.
- /W3 census (scratch/w/weapons/w3.py): weapons.c 20 and object_types.c 12 warnings, identical ctl vs new (no new warning).

## Probe 3 - static weapon_magazine_finish_reload (HCEX LPROC + PDB-absent)
- Change: `static` on its prototype (l.449) and definition (l.1382) in cand_static_reload.c (on top of cand_long.c).
- Result: 79/79; objcmp vs probe-2 object: ZERO section differences, only the storage class of the symbol (2->3).
  Confirms the 150k_w3 r1 note (storage inert for the body). Audit now FAIL(11) = 11 split-vs-ours storage rows, all
  "split external / ours static / PDB static" -> symbols.json "static": true is the fix.

## Probe 4 - symbols.json storage fix, proven with a scratch csplit (no tracked file touched)
- csplit control: build/tools/csplit.exe -i cachebeta.exe -p scratch/w/weapons/splitcfg_ctl -o scratch/w/weapons/split_ctl
  reproduces build/split byte-for-byte (diff -rq: 0 files).
- Patched scratch symbols.json (splitcfg_s; in-place line edits, order untouched; diff in symbols_static.diff): add
  "static": true to the 11 lines _weapon_trigger_get/_magazine_get/_busy/_magazine_state_change_ok/_get_effect_object_index/
  _get_owner_object_index/_effect_new/_state_interruptable/_set_state/_magazine_finish_reload/_reset.
  Re-split: ONLY build/split/source/items/weapons.obj changes (diff -rq: 1 file).
- Audit of shadow_s2 weapons.obj (long place + static finish_reload + owner header) vs the re-split target:
  OBJECT AUDIT: PASS (97 January symbols, 0 differ, all sections ok); gate-equivalent 79/79 exact.
- Remaining PDB-truth gap: _data_00307140 (placeholder name, ours+split external, PDB static). HCEX weapons.obj has a
  file-static LDATA `blurred_permutation_names` and NO globals struct -> our `struct weapons_globals` looks invented.

## Probe 5 - data: loose file statics replace the invented `struct weapons_globals data_00307140`
- Evidence: HCEX weapons.obj LDATA `blurred_permutation_names` type 0x2101d = LF_ARRAY of 0x0470 (char *), 8 bytes, i.e.
  `static char *blurred_permutation_names[2]`; no globals struct/LDATA in HCEX; HCEX SHIP has no profile statics at all
  (game/effects/items/projectiles/ai/particles/player_control/units modules checked) so the profile name is not
  recoverable -> descriptive sibling-convention name `weapon_update_section` (cf. projectile_update_section /
  item_update_section / game_update_section {"x_update", NONE, TRUE}). January .data: 0x600 = 8 + 0x5F8 (profile_section,
  8-aligned) at +8; projectiles.obj precedent proves .data = definition order for such loose statics.
- Change (cand_data.c): two `static` definitions (names first, then profile), accesses respelled; header comment block
  updated. Scratch symbols.json (splitcfg_d): `_data_00307140` line renamed `_blurred_permutation_names` + "static": true,
  new line at file_offset 3174728 `_weapon_update_section` "static": true. Re-split changes only weapons.obj.
- Result: vs re-split target 79/79 exact; OBJECT AUDIT PASS (98 January symbols, 0 differ); objcmp vs probe 3: only
  _weapon_update relocation identity (bytes equal) + the data symbols; no name-count tie flipped.
- objdiff 3.3.1 in a scratch project (odproj/): cand .data 100% / .rdata 100% (2052/2052), code 76/79 (ctl 75/79);
  _weapon_place now 100%. The other 3 objdiff-uncredited (_weapon_set_state has a semantic entry,
  _weapon_export_function_values, _weapon_update) are identical in the control - pre-existing, not introduced.

## Section-4 surplus/provider audit of the final candidate (blast/shadow_d/weapons.obj)
- surplus_identity: 17 candidate-only code COMDATs, 0 not identical (same set as production base).
- provider_link: all surplus PASS except `_real_random` (LNK2005 both orders vs action_charge.obj). INHERITED: production
  build/base weapons.obj fails identically; `--baseline` run = no new surplus. Cause = action_charge.c NODUP hand copy
  (`#define real_random real_random_inline` ... `#undef` + hand-written `real real_random(void)`); nodup_census:
  6 definers, only action_charge NODUP. Same blocker class as the objects.obj admission rejection
  (config/object_admission_rejections.json, class candidate-only-comdat-owner; reopen = replace hand-written provider).
- LAB provider fix (scratch/w/weapons/lab_ac/action_charge.c): drop the #define/#undef pair and the hand copy so the genuine
  real_math.h __inline emits `_real_random`. action_charge gate 21/22 before and after; objcmp: ZERO function-section
  differences (only section renumbering); `_real_random` section byte-identical (base==lab==weapons copy), now SELECT_ANY.
  VC7 Link.Exe pair link weapons cand + lab action_charge: NO LNK2005 in either order (base provider: LNK2005 both orders).

## FINAL candidate (scratch/w/weapons/final/*, patches production.patch = p1+p2+p3+p4)
- Files: final/weapons.c (long weapon_place + `return weapon_index;`; static weapon_magazine_finish_reload; loose statics
  blurred_permutation_names / weapon_update_section), final/weapons.h (fwd `struct scenario_weapon_datum;` + long
  weapon_place prototype), final/object_types.c (void facade removed; slot cast + BUG comment), final/action_charge.c
  (real_random hand copy + #define/#undef rename dropped), final/symbols.json (11 static flags; data rename + 1 new line).
- 19-consumer blast (shadow_final): every consumer identical to control except weapons (+_weapon_place) - objcmp 0 diffs
  (units only renumbers an internal `$state_not_allowed$` label). /W3 warning sets identical for weapons.c,
  object_types.c, action_charge.c. fake_match_scan: 0 leads.
- vs re-split target (split_d): 79/79 strict; object_audit PASS (98 symbols, 0 differ); PDB-storage 0 disagreements;
  surplus 17 identical; provider links all PASS with the action_charge provider fix (only _real_random needed it).
- objdiff 3.3.1 scratch report: .data/.rdata 100%; _weapon_place 100%.
- FALLBACK (if the owner refuses the object_types.c cast): p1+p3+p4 only (weapons.h/object_types.c untouched) compiles
  to an object identical to the primary (objcmp 0 diffs; 79/79 vs split_d).
- patch verified: `patch -p1 --binary < production.patch` on a scratch mirror reproduces final/* byte-for-byte.
