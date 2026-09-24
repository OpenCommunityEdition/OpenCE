# REVIEW source/objects/damage

approve: True

## per_object
[
 {
  "unit": "source/objects/damage:_object_damage_body (patch B or amended B+, plus the symbols.json static flags, plus retiring the park)",
  "approve": true,
  "issues": "Strict exact, and each load-bearing change has evidence behind it. `static` is supported by cachebeta (checked by address) and by HCEX. The single-lookup child walk matches /Od 0x770491..0x7704d9 and the units.c idiom. The named local is byte-inert and /Od-attested. B+ is preferred: it adds the /Od-attested region_damage re-read in place of the local, and it is exact and byte-inert."
 },
 {
  "unit": "source/objects/damage patch A (scratch/w/damage/production.patch) and the AOE park re-baseline",
  "approve": false,
  "issues": "Owner-gated. It newly emits the _collision_test_line and _real_random header COMDATs, and their only caller is the AOE, which is not strictly exact (1312 vs 1328). Rule 6 requires a strictly exact caller; this is the same class as the held actor_combat P1 item. Separately, the clamp block contradicts the /Od declaration order and the /Od ?: form, so item (5)'s claim to be /Od-faithful is inaccurate. The AOE re-baseline is unnecessary under B or B+."
 },
 {
  "unit": "config/symbols.json (4 in-place static flags)",
  "approve": true,
  "issues": "None. Only lines 5108, 5126, 5129 and 23108 change (+64 bytes, not re-serialised). The emulated csplit changes only damage.obj. Audit and pdb_storage both show 0 disagreements."
 }
]

## checks
All checks ran against tree 954eebd2. Notes are in C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review3_damage/REVIEW.md and LEDGER.md.

(1) git apply --check: clean for all three patches. Patch A is scratch/w/damage/production.patch, B is scratch/w/damage/production_minimal.patch, and B+ is my amended scratch/w/review3_damage/production_reviewed.patch. I applied each one to `git show HEAD:source/objects/damage.c` in a throwaway repo and converted to CRLF. The results are byte-identical: A = final.c, B = minimal.c, B+ = rev_Bplus.c.

(2) gate.py --all --forbid-emitted-symbol _point_from_line3d, on files I rebuilt myself (guard passed each time):
- production: 32 exact / 2 residual;
- rev_A: 33 / 1, AOE 1312/51;
- rev_B: 33 / 1, AOE 1312/52;
- rev_Bplus: 33 / 1, AOE 1312/52.
_object_damage_body passes section_infos_equal against January in A, B and B+, both against build/split and against the emulated split. No other row changed.

(3) Park snapshots, measured the way tools.parked_functions measures them: under B and B+ the AOE is 1312/52 with sha 46952a11..., which equals the current park base. Under A it is 1312/51 with sha 93606db1....

(4) Storage evidence:
- cachebeta publics have no symbol at 0x126090, 0x127020, 0x128150 or 0x456E48. I checked by address, not only by name, and confirmed the address mapping against the neighbouring public _object_cause_damage at 0x127570.
- HCEX.pdb reports `static function: true` for object_damage_body, area_of_effect_cause_damage_to_object and object_destroy_notify_children.
- Among 833 split and 621 base objects, only damage.obj names the four symbols, and no header declares them.

(5) Emulated split:
- A control csplit matches build/split on 833 of 833 objects.
- The worker's symbols.json differs only on lines 5108, 5126, 5129 and 23108 (+64 bytes, not re-serialised). With it, csplit changes only source/objects/damage.obj.
- object_audit_split against the emulated split, for A, B and B+: 47 January symbols with 0 differences. The only differing section is the AOE.
- pdb_storage against the emulated split: 0 disagreements.

(6) Surplus and linking:
- B and B+ emit the same 8 code COMDATs as production, all identical to January's selected copies.
- A emits 10: it adds _collision_test_line and _real_random, both identical to action_charge's copies.
- provider_link passes for A, B and B+.

(7) Data: 760/760 (100%) in build/report.json, and every .rdata/.bss section passes the audit.

(8) fake_match_scan: 0 leads for A, B, B+ and production.

(9) CL /W3 /Zs with the ninja cflags (w3.py): 0 warnings from damage.c itself for all of them. The 12 header diagnostics are the same as production's.

(10) Strip tests on the body:
- `static` alone: 1392/44 residual.
- B without `static`: residual [sha].
- all four statics without the child walk: 1392/44 residual.
- B+ with an inline cast in place of the named child_unit: EXACT.
Conclusion: the static and the single-lookup walk are both load-bearing, and both have independent evidence (cachebeta, HCEX, and /Od 0x770491..0x7704d9). The named local is byte-inert, but the /Od build attests it as slot [ebp-0x30], declared after child_object. The same idiom appears in admitted code: units.c at 1723 and 8093, and ai_script.c at 3505.

(11) /Od readouts:
- The AOE core block matches the /Od frame and declaration order.
- The AOE clamp block in A does not: /Od declares distance, scale, radius_delta and computes scale with ?: followed by PIN.
- The body's region_damage in /Od is re-read from the object; there is no local.

(12) Rules checked: brief sections 2, 7 and 8(g) and the rule-6 exception in docs/campaign_house_rules.md. No rejections are recorded for damage in config/object_admission_rejections.json. The parks validate as 89 active, 0 stale.

## issues
Approved only for patch B, or my amended B+ (preferred). Patch A, which the worker marked PREFERRED, is rejected.

Why patch A is rejected:
1. Rule 6 (the exception in docs/campaign_house_rules.md, and brief section 2) says a TU may emit a shared-header __inline COMDAT only if its caller is strictly exact. Patch A restores the genuine collisions.h collision_test_line and real_math.h real_random. The /Od build does attest these as calls, at 0x76c50b/0x76c558 and 0x76c6ca. But restoring them newly emits _collision_test_line and _real_random, and their only caller is the AOE, which is not strictly exact (1312 vs 1328). This is the same class as the held actor_combat P1 item, so it needs an owner ruling. Byte identity and provider_link both pass, but that is not enough.
2. Item (5) claims the AOE is rewritten in the /Od-faithful form, but the clamp block is not. /Od declares distance [-0x13c], scale [-0x140], radius_delta [-0x144] and computes scale with a ?: (temp at -0x168) followed by PIN. final.c keeps production's distance, radius_delta, scale order and the if/else clamp. The worker's own v1_declorder and od2 variants had the /Od forms, but they were left out of final.c. This is byte-inert, but it falls under law (d).
3. Patch A would re-baseline the AOE park for zero credit.
Reopen A's AOE half when the AOE reaches strict exact or the owner admits the P1 class, and include the od2 ?: clamp and the /Od declaration order at the same time.

Amended patch B+ (verified): C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review3_damage/production_reviewed.patch. It is production_minimal.patch plus the worker's item (4), which drops the `byte region_damage` local that /Od does not have and re-reads object->object.region_damage[region_index]; `*(1.f/255.f)` is kept.

Integrator actions:
- Apply exactly one patch: the B+ patch above (preferred), or scratch/w/damage/production_minimal.patch.
- In config/symbols.json, add `, "static": true` in place on lines 5108, 5126, 5129 and 23108 (full copy: scratch/w/damage/config/symbols.json), then run a csplit-only regen of source/objects/damage.
- In config/parked.json, delete only the source/objects/damage:_object_damage_body entry. The AOE park stays unchanged and still measures 1312/52/46952a11.
- No shared header is edited, so there are no header consumers to sweep.

Gain: +1 strict function, _object_damage_body (1360 padded / 1347 meaningful). The object is still incomplete. It remains blocked by the AOE x87 kill-form residual (fxch st(1); fstp st(0) vs fstp st(1) at +0x42e), the consumer-local prototype `boolean unit_unsuspecting(...)` at damage.c:336, and `extern boolean debug_damage;` at damage.c:366.

Not independently re-run: the worker's AOE lab, oracle and killscan probes (more than 60). They don't affect this verdict.
