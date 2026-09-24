# Adversarial review: source/items/weapons OBJECT_COMPLETE_CANDIDATE (reviewer slug review_weapons)

Base e9e62b78, worktree clean (no tracked modifications). Reviewing scratch/w/weapons/{production.patch,final/*}.

## R1 patch integrity
- production.patch applied (`patch -p1 --binary`) to a fresh scratch mirror (review_weapons/applymirror) of the 5 production
  files reproduces final/{weapons.c,weapons.h,object_types.c,action_charge.c,symbols.json} byte-for-byte (cmp). Files CRLF.

## R2 independent 19-consumer blast (reviewer harness blast2.py + objcmp2.py, own code)
- weapons.h has exactly 19 direct includers (grep of source/**/*.c and *.h; no header includes weapons.h).
- CONTROL (absolute include of an unmodified weapons.h copy): all 19 objects == build/base (objcmp2: every section's
  size/flags/bytes/relocs + full symbol table, 0 diffs each, label names literal). Gate counts identical to worker's baseline.
- FINAL (final/weapons.h; final/weapons.c + final/object_types.c): 17 other consumers AND object_types: 0 diffs vs control once
  internal $L/$label$NNN numbering is normalised (relocation targets to labels keyed by section+offset). units 189/189,
  game_engine 180/180, objects 121/121, object_types 31/31 (object_types.obj fully identical incl. symbol table -> the
  facade removal + slot cast is byte-inert). weapons: only _weapon_place, _weapon_update (data-symbol displacement/reloc
  target), .data symbol names/storage and finish_reload storage change. research12's units regression does NOT reproduce
  at e9e62b78 (independently confirmed).

## R3 re-split + whole-object audit (own csplit run)
- csplit with final/symbols.json (review_weapons/splitcfg -> review_weapons/split): `diff -rq build/split` = ONLY
  source/items/weapons.obj differs (no other object, no added/removed files).
- gate-equivalent (gate_vs.py, verified identical logic to gate.py) blast_fin/weapons.obj vs re-split: **79/79 strict**.
  Production base vs re-split: 77/79 (_weapon_place + _weapon_update) -> p1 data respell and p3 symbols.json MUST land
  atomically with the csplit regen, else _weapon_update drops. (Final source vs OLD split: 78/79, _weapon_update reloc-identity.)
- object_audit (SPLIT_ROOT variant; only diff vs scratch/tools/object_audit.py is the split root) vs re-split: PASS,
  98 January symbols, 0 differ, every section ok. Surplus = 17 code COMDATs + literals, identical set to production base
  (objcmp2 shows surplus sections unchanged), incl. the already-admitted _point_from_line3d (weapons_trigger_point_inline_20260923).
- surplus_identity (production weapons, same surplus set): 17 candidate-only code COMDATs, 0 not identical.
- objdiff 3.3.1 scratch project (review_weapons/odproj): cand .data 100% / .rdata 100%, _weapon_place 100%, 76/79 fns;
  control 75/79. Remaining objdiff<100 (_weapon_set_state [has semantic entry], _weapon_export_function_values,
  _weapon_update) are identical in control -> pre-existing objdiff under-credit, not introduced (follow-up opportunity).
- cachebeta publics: none of the 11 functions nor _data_00307140/_blurred_permutation_names/_weapon_update_section is public;
  no public at RVA 0x306A91..0x30774F; controls _weapon_place/_weapon_new/_weapon_update are public.
- build/split census: the 11 functions and _data_00307140 are defined only in weapons.obj and referenced by no other
  split object; _weapon_place is referenced only by object_types.obj. The 10 functions are already `static` in production
  weapons.c; no other production .c/.h names any of the 11 -> static flags are link-safe and pure PDB-truth corrections.

## R4 HCEX evidence (own MSF7/DBI/TPI parser review_weapons/pdbcheck.py, written independently of the worker's)
- HCEX.pdb sha256 f55cfe95... (research12 used a DIFFERENT PDB, sha 0fd11b08...; both give the same answer).
- weapons.obj module: GPROC32 weapon_place type 0x635bb LF_PROCEDURE ret 0x12 (long), args (long, scenario_weapon_datum *).
  Parser control: weapon_new / weapon_update ret 0x20 (unsigned char = boolean).
- LPROC32 weapon_magazine_finish_reload ret void (long, short); DIA2Dump also prints "static function: true".
- LDATA32 blurred_permutation_names = char *[2] (LF_ARRAY 8 bytes); DIA2Dump "File Static". No globals struct.
- Every other *_place (control, light_fixture, machine, placeholder, scenery, biped, unit, vehicle, equipment) returns void
  and takes its own scenario_X_datum * -> weapon_place's long return is authored and unique.
- No profile statics in HCEX (vehicle/unit/item/projectile/weapon/ai/game *_update_section all absent); symbol atlas has no
  name either -> `weapon_update_section` is a DESCRIPTIVE (not authenticated) name, sibling-convention.
- January object_type_place: `call eax; add esp,8; inc esi` -> EAX discarded (target confirmed).

## R5 house rules / strip tests
- /W3 (own run): weapons.c 20=20, object_types.c 12=12, action_charge.c 12=12 (identical warning lists prod vs final).
- STRIP TEST of the object_types slot cast: plain `weapon_place` -> object_types.obj byte/symbol identical (objcmp2 0 diffs),
  31/31, and only adds C4028 (formal parameter 2 different). The cast is NOT byte steering; it is a type annotation.
- fake_match_scan on final weapons.c/object_types.c/action_charge.c: 0 leads.
- long-return strip: the plain `void` spelling does not match (that is the pre-existing residual); the long type is carried by
  HCEX raw type evidence, not a decoration -> passes the fake-matching test on evidence grounds.
- p2 weapons.h prototype: one parameter per line, genuine owner header, forward decl needed (tag scope) - OK. Removing the
  consumer-local facade from object_types.c reduces a rule-9 violation.
- p1 loose statics: authenticated name/type for blurred_permutation_names; MAXIMUM_NUMBER_OF_TRIGGERS_PER_WEAPON = 2 (enum in weapons.c).

## R6 p4 action_charge provider fix
- History: hand copy + #define/#undef was added 20260826 (action_charge_obj_jonas_real_random_recovery) when the caller
  (action_charge_setup) was unwritten; the caller is now written and strict exact, so the genuine header inline emits it.
- Own gate: 21/22 before and after (residual _action_charge_perform, pre-existing, not the real_random caller).
  objcmp2 base vs p4: 0 diffs (sections, bytes, relocs, symbols). Only change: _real_random COMDAT selection 1 (NODUP) -> 2
  (ANY) - now identical to every other header-inline helper in production action_charge.obj (magnitude3d, point_from_line3d, ...);
  January split marks everything sel=1 (csplit artefact). object_audit action_charge unchanged (FAIL(1) = perform only).
- provider_link candidate weapons vs base action_charge: _real_random LNK2005 both orders (inherited). Own VC7 pair link
  weapons cand + p4 action_charge: no LNK2005 either order. provider_link action_charge(p4): PASS. nodup_census: action_charge
  was the only NODUP definer of 6. Satisfies the objects.obj rejection reopen pattern for _real_random.

## R7 fallback check
- final weapons.c compiled against PRODUCTION weapons.h = identical to the primary object (objcmp2 0 diffs).
- BUT the fallback is not house-rule clean: it leaves object_types.c's consumer-local `void weapon_place(long, struct
  scenario_object_datum *)` facade contradicting a `long` definition with no BUG disclosure, and a public function with no
  owner-header prototype. It must not be preferred to p2; if the owner rejects p2 the right fallback is NOT to land the long return.
- "void subset" (final weapons.c with void weapon_place / return;) + p3 + p4: 78/79 vs re-split (only the pre-existing
  _weapon_place residual), object_audit 98 symbols 0 differ, FAIL(1) = _weapon_place only.

## Verdict: approve = false (POLICY HOLD, no technical defect found)
Every technical claim of the worker reproduced independently. Blocking items are owner rulings the brief reserves:
1. `_weapon_place` long return + void-slot binding carries a `BUG:` comment = reproducing an authentic January interface bug;
   WORKER_BRIEF s2: "needs target+layout proof, a BUG comment and owner approval - report, don't land". It is item F
   ("return ABI (_weapon_place)") of the Lane D owner ruling queue (claude_lane_d_refresh_HANDOFF_20260922.md); no ruling is
   recorded since. New evidence to put to the owner: units regression gone at e9e62b78 (19-consumer blast clean), raw long
   type in two different HCEA/HCEX PDBs, unique among 10 placement callbacks.
2. symbols.json split of _data_00307140 introduces a non-authenticated descriptive static name `_weapon_update_section`;
   owner 20260915 held symbols.json static splits with invented names ("don't re-ask without authenticated names").
   New evidence (HCEX proves the struct is invented and authenticates the other half) makes it a legitimate re-ask.
Landable now without a ruling (zero code credit, correctness only): p3's 11 "static": true lines + `static` on
weapon_magazine_finish_reload (p1 part) + p4. Everything else waits for the owner.

## R8 no-ruling subset measured (review_weapons/splitcfg_s, subset_static_weapons.c)
- symbols.json with ONLY the 11 `"static": true` edits (in place, no reorder) -> csplit changes only weapons.obj.
- production weapons.c + `static` on weapon_magazine_finish_reload (prototype + definition): gate 78/79 vs current split AND
  78/79 vs the static re-split (only the pre-existing _weapon_place residual); object_audit 97 January symbols, 0 differ,
  FAIL(1) = _weapon_place only. Zero code loss, zero code gain; clears 11 storage disagreements. Can land with p4.
