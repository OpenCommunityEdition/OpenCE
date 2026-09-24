# source/items/weapons

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_weapon_place"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\p1_weapons_c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\p2_owner_header.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\p3_symbols_json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\p4_action_charge_provider.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\final\\weapons.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\final\\weapons.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\final\\object_types.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\final\\action_charge.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\final\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weapons\\LEDGER.md"
]

## production_changes
Apply scratch/w/weapons/production.patch from the worktree root with `patch -p1 --binary < scratch/w/weapons/production.patch`. This was checked on a scratch mirror and reproduces scratch/w/weapons/final/* byte-for-byte; the files are CRLF. As an alternative, copy the final/ files over the production files; their sha256 values are recorded in LEDGER.md. The patch has four parts:

(p1) source/items/weapons.c:
- `void weapon_place(` becomes `long weapon_place(`, and its terminal `return;` becomes `return weapon_index;`.
- `static` is added to the weapon_magazine_finish_reload prototype (l.449) and definition (l.1382).
- `struct weapons_globals` and its variable `data_00307140` are replaced by two statics: `static char *blurred_permutation_names[MAXIMUM_NUMBER_OF_TRIGGERS_PER_WEAPON] = {"~primary-blur", "~secondary-blur"};` and `static struct profile_section weapon_update_section = {"weapon_update", NONE, TRUE};`.
- The 4 uses are respelled, and the symbol comment block at the top is updated.

(p2) Owner header:
- source/items/weapons.h gains `struct scenario_weapon_datum;` and `long weapon_place(long weapon_index, struct scenario_weapon_datum *scenario_weapon);` (one parameter per line), next to weapon_new.
- source/objects/object_types.c drops its void weapon_place facade prototype. The weapon_data_definition slot becomes `(void (*)(long, struct scenario_object_datum *))weapon_place`, with a BUG comment above the table.

(p3) config/symbols.json, edited in place with no reordering:
- Add `"static": true` to 11 lines: _weapon_trigger_get, _weapon_magazine_get, _weapon_busy, _weapon_magazine_state_change_ok, _weapon_get_effect_object_index, _weapon_get_owner_object_index, _weapon_effect_new, _weapon_state_interruptable, _weapon_set_state, _weapon_magazine_finish_reload, _weapon_reset.
- Line 22579 `_data_00307140` becomes `{ "file_offset": 3174720, "flags": 0, "name": "_blurred_permutation_names", "static": true },`.
- Insert after it `{ "file_offset": 3174728, "flags": 0, "name": "_weapon_update_section", "static": true },`.
- Then regenerate csplit. In a scratch re-split only build/split/source/items/weapons.obj changed.

(p4) source/ai/action_charge.c, provider fix:
- Delete `#define real_random real_random_inline`, `#undef real_random`, and the hand-written `real real_random(void)` definition, so the genuine real_math.h __inline emits _real_random as SELECT_ANY. This is the reopen criterion already written for objects.obj in config/object_admission_rejections.json.

No park to retire; _weapon_place is unparked. No semantic_matches or semantic_data_matches change is needed.

After applying, the integrator must:
- Run a full ninja.
- Confirm weapons 79/79 and object_audit PASS.
- Re-gate the declaration-count sentinel `source/units/units --fn _unit_preprocess_node_orientations`, because other lanes' header packets may compose.
- Run parked_functions.
- Run `provider_link.py source/items/weapons` and expect a full PASS.

FALLBACK: if the owner rejects the object_types.c cast or the units sentinel flips, apply only p1+p3+p4. That leaves weapons.h and object_types.c untouched and gives a byte-identical weapons.obj (objcmp 0 diffs, 79/79 vs the re-split).

## evidence
Every compile was done in scratch and never touched tracked files. Details are in scratch/w/weapons/LEDGER.md.

1. Why the long return is justified. The HCEX.pdb weapons compiland, read by our own MSF7 parser, gives weapon_place type LF_PROCEDURE returning 0x12 (long). All other placement callbacks return void. In the /Od build the function reloads [ebp+8] into eax before its epilogue. January keeps weapon_index in EBX and ends with `mov eax,ebx`. With the long return, _weapon_place is strict exact: 208 B padded, 4 relocs, normalized sha 4e5e6cfe4629e367..., the same value research12 recorded.

2. The research12 units regression no longer happens. A shadow-header harness (blast.py + objcmp.py) swaps each consumer's weapons.h include for a scratch copy via gate --edits. The control run reproduces all 19 direct includers' build/base objects exactly.
- With the owner header plus the object_types edits, 18 consumers keep every function section, non-text section and non-label symbol identical. Units stays 189/189 and object_types 31/31.
- The only gain is _weapon_place. The only other differences are internal $L label renumbers.

3. Storage classes. Per cachebeta publics, 12 weapons symbols are file-static in January, and HCEX lists weapon_magazine_finish_reload as LPROC (static). No other split object references any of them.
- Making finish_reload static is byte-inert (objcmp 0 section diffs).
- The 11 functions need `"static": true` in symbols.json. A scratch csplit with the patched symbols.json changes only weapons.obj; the control csplit reproduces build/split exactly.

4. Data. HCEX has a file-static LDATA `blurred_permutation_names`, type LF_ARRAY of char* (8 B), and no globals struct. So the `weapons_globals` struct is invented; two loose statics replace it.
- The layout is 8 + 0x5F8 at +8 with 8-byte alignment, the same as before. The projectiles.obj precedent shows .data follows definition order.
- The profile section name is not in HCEX SHIP, so it gets the descriptive sibling name `weapon_update_section`.

5. Final candidate against the re-split target:
- gate-equivalent 79/79.
- object_audit PASS: 98 January symbols, 0 differ, every section ok.
- 0 PDB-storage disagreements.
- surplus_identity: 17 COMDATs, all identical.
- A scratch objdiff 3.3.1 report scores .data and .rdata 100% and _weapon_place 100%.
- /W3 warnings are the same as before in all three edited C files.
- fake_match_scan finds 0 leads.

6. Provider link. The inherited _real_random LNK2005 against action_charge's NODUP hand copy fails on production base too. With p4, action_charge stays 21/22 with zero function-section differences, its _real_random is byte-identical and now SELECT_ANY, and the VC7 pair link weapons+action_charge gives no LNK2005 in either order.

## blockers
None on the byte or object side once p1–p4 and the csplit regeneration are applied. Three owner judgements go with the package:
(a) The object_types.c function-pointer cast and BUG comment bind a long-returning function to the void datum_place slot. The return type is authentic, but the call is not defined ISO C. The fallback p1+p3+p4 avoids the cast with an identical object.
(b) The p4 action_charge provider fix follows the reopen text of the objects.obj rejection. Without p4, weapons fails selected-provider linkage only through that inherited _real_random conflict.
(c) `weapon_update_section` is a descriptive name that follows the sibling files' convention; it is not an authenticated January name.
Note also that header edits add up across packets: the units sentinel must be re-gated after all lanes' header packets land.

## reopen_criteria
If the integrated build shows units `_unit_preprocess_node_orientations` or any other consumer function moving after p2, drop p2 and land p1+p3+p4; that gives the same weapons.obj bytes. If the owner rejects p4, weapons.obj becomes FUNCTIONS_EXACT_OBJECT_BLOCKED on the _real_random NODUP provider only; all 79 functions and all data still land. Reopen the profile-section name only if a first-party source gives January's real identifier.

## task notes
Resumed after the cut-off. The earlier run's probe 1 and its HCEX re-attestation were reused, not redone.

New findings this session:
- Research12's rejection, a units declaration-count loss from the weapons.h owner prototype, does not reproduce at base e9e62b78. All 19 weapons.h consumers were checked section by section.
- The object had three whole-object gaps that nobody had recorded: 11 csplit storage-class errors (symbols.json lacks static), a static function we left external, and an invented globals struct under a placeholder name. All three are fixed and proven with a scratch csplit.
- The inherited _real_random NODUP provider conflict has a measured, zero-regression provider fix in action_charge.c.

Tools written under scratch/w/weapons/ that other workers can reuse:
- blast.py: shadow-header blast test using gate --edits include swapping.
- objcmp.py: whole-object section and symbol diff.
- gate_vs.py: gate against any target object.
- object_audit_splitroot.py: object_audit with a SPLIT_ROOT override.
- w3.py: /W3 warning census.
- mod_syms.py: HCEX per-module PROC/DATA global versus local lister.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\weapons\LEDGER.md
