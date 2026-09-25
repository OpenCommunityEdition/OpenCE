# REVIEW source/physics/breakable_surfaces

approve: True

amended_patch: 

## per_object
[
 {
  "unit": "source/physics/breakable_surfaces",
  "approve": true,
  "issues": "Approve PACKET_breakable_surfaces.patch (01+02+03) as submitted, applied atomically, then rerun configure and a csplit-only regeneration. Verified: 12/12 EXACT; object_audit PASS against the emulated split (26/0); pdb_storage 0; surplus_identity drops from 7 to 1 (B1 only); the new _bsp3d_get_plane_from_designator is IDENTICAL to the decals copy and links in both orders; data and objdiff identical; /W3 17 in both; fake_scan 0. Non-blocking: the /Ow removal rests on the uniform-profile precedent, not on per-TU image proof, and the do-not-repeat note in breakable_surfaces_obj.md should get a superseding note. Zero credit; B1 and B2 still block completion."
 }
]

## checks
Tree: HEAD 434f0151 (05255584 plus a ledger-only commit), clean. I claimed the unit as review-r2w2-breakable_surfaces and released it at the end.

Patch checks: `git apply --check` gives rc 0 for PACKET, 01, 02 and 03 at HEAD. PACKET is byte-equal to `cat 01 02 03`, and the sha256 values match patches_sha256.txt. A round trip on LF copies (apply, then reverse) restores all three files exactly.

Lab compiles use labgate.py, a copy of gate.py that adds a flag-drop option and a split-root option. The lab production object equals build/base except for .debug$S. The variants were:
- A: production source, production flags
- B: production source, default flags
- C: candidate source, production flags
- D: candidate source, default flags (the packet)
- E: candidate, /Ow dropped only
- F: production, /QIfist dropped only

All six gate 12/12 EXACT against build/split. D also gates 12/12 against my own emulated split.

Object comparisons by owner key:
- A equals F and D equals E, so /QIfist has no effect on this object.
- A vs B: only 7 helper COMDATs differ (the static hand copy plus cross_product3d, plane3d_from_point_and_normal, plane3d_negate, project_point2d, project_point3d and vector_from_points3d). The 12 functions and all data are unchanged.
- B vs D: the only change is the static hand copy (96 B, static) being replaced by _bsp3d_get_plane_from_designator (112 B, external, SELECT_ANY).

Emulated split: I ran csplit on a copy of config. The unmodified control matches build/split on 833/833 objects. With 03 applied, only breakable_surfaces.obj differs, and only in storage class (2 to 3) for _breakable_surface_effect and _globals.

Audit, storage, surplus and link (candidate D):
- object_audit against the emulated split: PASS, 26 sections ok (12 .rdata, 12 .text, .data, .bss), 26 January symbols, 0 differ. Production is FAIL(2) on the two storage rows.
- pdb_storage: 2 disagreements in production, 0 for the candidate.
- surplus_identity: 7 not identical in production, 1 for the candidate (_real_local_random vs effects, which is B1).
- The new _bsp3d_get_plane_from_designator is IDENTICAL to decals, the only January definer. items, collision_bsp, collision_features and bipeds reference it only as undefined externals.
- nodup_census: 0 NODUP definers.
- provider_link: FAIL(1) on _real_local_random, the same pre-existing row as production. With --baseline=A, the new COMDAT PASSes against decals in both link orders.

Other checks:
- objdiff 3.3.1 mini project: production and candidate are identical (5331/5331 code, 12/12 functions, 685/685 data).
- CL /Zs /W3: 17 warnings in both, the same set.
- fake_match_scan: 0 leads in both.
- No parks or admission rejections exist for the unit.

/Od evidence: the /Od breakable_surface_effect (0x7a0ca0) calls the single shared helper 0x5666f0 at 0x7a0e64. That helper has 11 call sites across TUs and the header's body (mask &0x7fffffff, test &0x80000000, then plane3d_negate or a copy).

Precedents, read at the source:
- Owner ruling 2 of 2026-09-21 (Lane B handoff section 3) and the Rule 6 exception in docs/campaign_house_rules.md: every condition was verified, including ruling 6 (every newly emitted symbol is stated).
- Uniform compile profile: c1620772 (lens flares /QIfist removal, with its ruling text), 0eda57d6 (periodic_functions override removed), and the "flag-uniform" statement in the weather doc.
- Override origin: 369b71e7, a WIP commit with no rationale.
- Hand-copy history: added in 888065f3 when bsp3d.h still tested `<0`; the header was fixed to LONG_MIN in 7a3bbb9c.
- House rule 22 wording was checked.

Strip test: nothing to strip (no casts, parentheses or declarations were added). /Od declaration order: no new scopes. House rules: OK. git status after all work: clean apart from the pre-existing untracked research/fifty_objects_r2_20260924/.

## issues
No blocking issues; approve as submitted, no amended patch needed. Non-blocking notes:

(1) The three parts must land atomically. Patch 01 without 02 makes the new _bsp3d_get_plane_from_designator COMDAT DIFFERENT from decals (8 not identical), because it is compiled with /Ow.

(2) Disclosure on dropping /Ow: January's own breakable bytes cannot tell whether this TU used /Ow. All 12 functions and the data are the same either way. The justification is the uniform-profile precedent (c1620772, 0eda57d6) plus an override (369b71e7) that has no stated reason. It is not image proof. The helpers becoming identical to January's selected copies (compiled in other TUs) is admission evidence, not proof of January's flags. The precedent is not stretched.

(3) docs/object_matching_logs/breakable_surfaces_obj.md still records the flags as '/Ow /QIfist' and lists '/Ow removal ... compiler-flag changes' under Do-not-repeat. That list was about levers for closing the residual (A03 was byte-neutral). The integrator should add a note superseding it when landing.

(4) Credit is zero. Status stays NonMatching, and the object is still blocked by B1 (_real_local_random vs effects, real_math RESERVED) and B2 (static _breakable_surface_plane_distance, owner ruling).

(5) Self-report: I ran `python -B -m tools.parked_functions --help` before reading its source. git status afterwards is clean and no tracked file changed.

Evidence is in C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2w2_breakable_surfaces/REVIEW.md and LEDGER.md (plus the gate_*.txt, audit_D_emu.txt, surplus_D.txt, pdb_D_emu.txt, provider_link_D_*.txt, objcmp_variants.txt, w3.txt, od_effect.txt and odproj/report.json files).
