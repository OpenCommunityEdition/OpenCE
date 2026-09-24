# source/ai/action_charge

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_action_charge_perform"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\candidate.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\candidate_gate.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\candidate_audit.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\candidate_provider_link.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\melee_block.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\action_charge\\od_perform.txt"
]

## production_changes
1) Apply scratch/w/action_charge/production.patch to source/ai/action_charge.c. It is a unified diff, CRLF kept, and changes ONLY the body of action_charge_perform. `patch -p1 --binary` on a copy of production reproduces scratch/w/action_charge/candidate.c byte-for-byte. No header, symbols.json, parked.json, config flag or tool change. Helper copies are untouched, including the real_random rename package.
2) What the patch changes:
(a) Removes the `unit_index` and `target_range` locals, as in the /Od build.
(b) Four 2D aggregate copies `x = *(real_vector2d *)&v`: target_alignment_vector, the melee alignment_vector plus its facing_vector fallback, and state_data->leap_alignment_vector.
(c) Adds the /Od dead leap-arm code: RTC alignment_vector with the normalize2d fallback chain to *global_forward2d, and a nested block { boolean success = FALSE; real minimum_velocity; real_point3d target_point = prop->body_position; aim_vector/vertical/horizontal; target_point.z += 0.4f; success = projectile_aim_ballistic(...); if (success) {...} }.
(d) Adds named results closing_velocity (suicide arm) and alignment (dot_product2d).
(e) Uses the RTC names vector_to_target, estimated_target_position, target_relative_velocity, target_alignment_vector, alignment_vector and minimum_velocity, and /Od declaration order.
3) No park to retire: config/parked.json has no action_charge entry.
4) After the full ninja build, the stable sweep and owner acceptance of the review items (see blockers), flip config/config.json `source/ai/action_charge.c` (index 470) from NonMatching to Matching.
5) Add a matching-log doc disclosing the rule-24 casts per site, with the /Od addresses listed in LEDGER.md section 6.

## evidence
Baseline: `[size 3264!=3248, sha]`, 32 real regions. Candidate: gate `EXACT 3248 _action_charge_perform` and `== exact 22 residual 0 unwritten 0`. relocdiff 84/84 with 0 differing rows. object_audit PASS: 37 January symbols, 0 differ, surplus identical to production. provider_link SELECTED-PROVIDER LINK PASS. pdb_storage 0 disagreements. surplus_identity 2/2 IDENTICAL. Data 256/256 (100% in build/report.json). fake_match_scan 0 leads. /W3 clean. coffsecs against build/base: only the perform .text and .debug$S sections differ; every other section's bytes, flags and COMDAT selection are identical.

Mechanism:
- Lane B's owner_ruling_struct_copy body rebuilt on this tree (sc0) leaves one diff only: dot #2 term order at +0x8e1..+0x8f8 (January i,k,j; ours k,j,i).
- A dead-local instrument (diagnostic only) shows the key is a count of IL locals and that POSITION IS INERT: D=6/7 exact at function top, melee block, suicide arm or tail; D=0..5 not.
- The /Od build (fn 0x430170) has exactly 7 named slots that sc0 lacks: -0x3e, -0x88 (RTC alignment_vector), -0x8d, -0xac (RTC target_point), -0xf4, -0xf8, -0x110.
- The two RTC aggregates are dead code in the /Od build too. Measured: VC7 inlines and eliminates that dead block with ZERO byte trace (s4), so January's bytes cannot exclude it. Every /Od-attested exact set requires it.
- Constants were read from the /Od data: 0.4f and 0.3f. The fallback global 0xa31df4 = global_forward2d, derived from pointer-table offsets.
- Map of subsets that are exact (LEDGER section 3): at least 2 of {success, dead boolean, 3 named reals}, but not both 3D-dot reals. The full /Od set fails only on the fadd at +0x944.

Strip tests:
- Load-bearing casts: C1, C2 and C5 (local-to-local copies from vector_to_target). Every undecorated alternative is residual.
- Byte-inert casts: C3, C4 and C6 (member-wise copies are equally exact), each with the /Od single-base-load L,L,S,S struct-copy pattern.
- `= FALSE` on success is byte-inert and kept because /Od stores it at 0x430d83.
- Each named real alone is byte-inert; dropping both is residual.
- A cast-free control (production scalar copies with the same locals, D=0..7) stays at [size 3264] with 32 real regions and does not respond to the count, so the aggregate copies are required.

## blockers
Owner review is needed before admission:
(1) Rule-24 aggregate-copy casts.
- C3, C4 and C6 meet the 2026-09-21 per-site admission as it stands: byte-inert, /Od L,L,S,S at 0x431203 / 0x430f04 / 0x430d28, prefix layout.
- C1, C2 and C5 are load-bearing aggregate copies that cannot be strip-tested. This is the same class as the owner-admitted action_obey.c:1259 copy (commit 3ba2eb91).
(2) The /Od-attested dead leap-arm code (RTC alignment_vector/target_point) and the named result locals. They produce no bytes of their own and are proven only through the M8 IL-count key (the dot #2 term order). An earlier Astra review dismissed them on the premise 'absent from January bytes'; that premise is now measured false.
(3) Which exact subset of /Od locals January had cannot be decided from bytes. The alternatives are listed in LEDGER section 3.

Not a blocker for this object: nodup_census shows action_charge.obj is the only NODUP definer of `_real_random`. That is the provider worker's question, and it blocks objects.obj, not this object. Measured: deleting the real_random rename package keeps 22/22, with only a COMDAT selection change 1 to 2.

## reopen_criteria
If the owner rejects the dead-code or cast evidence, keep the current production body (fuzzy, zero credit) and hold candidate.c as the exact candidate. Reopen with an owner ruling on (i) the load-bearing aggregate-copy casts C1/C2/C5 and (ii) authentic /Od dead code / named locals attested only through the IL-count key. Do not re-run dead-local sweeps or declaration-order permutations. The count map is recorded in LEDGER sections 2 and 3.

## task notes
`_action_charge_perform` is now strict exact in scratch, and so is the whole object (22 of 22 functions). Nothing tracked was edited; ninja, configure and git were not run. The fix has three parts:
1. Lane B's 2D aggregate-copy body.
2. Authentic dead code and named result locals from the /Od build (halo_cache_symbols.exe).
3. The /Od (RTC) local names.

The key finding is that the compiler removes the /Od dead code without leaving a byte, but January's dot #2 term order proves those local declarations existed. Admission depends on the owner accepting that dead code and the aggregate-copy casts; see blockers.

Probe harness: scratch/w/action_charge/probe.py with v1.py to v5.py; outputs are in scratch/w/action_charge/p/.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\action_charge\LEDGER.md
