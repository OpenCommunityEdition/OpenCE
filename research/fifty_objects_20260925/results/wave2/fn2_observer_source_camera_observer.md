# source/camera/observer

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\cand.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\pos_F1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\od_update_positions.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\od_apply_rot.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\fu.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\observer\\lab.py"
]

## production_changes
Optional fuzzy upgrade; no exact credit. Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\observer\production.patch to source/camera/observer.c. It is a unified diff, `git apply --check` is clean at 931ed8dc, CRLF is preserved, and it touches only the observer_update_positions body. The full candidate file is scratch/w/observer/cand.c. What the patch changes: `real displacement[11]` plus csmemset plus an unattested `real_vector3d rotational_displacement = {delta[0..2]}` copy become `struct observer_derivative new_camera_displacements` (name confirmed by both HCEX and the /Od RTC). Pointer locals follow the /Od declaration order (position, velocity, delta, target, a..f, timer). The main loop becomes `if (remaining_time > 0.f || !TEST_FLAG(flags, valid)) { short value_index; powers t2..t5; polynomial or velocity loop; `< orientation` add loop, else observer_apply_rotational_displacement((real_vector3d const *)delta, (real_vector3d *)position, (real_vector3d *)(position + 3)); } else { short value_index; snap loop }`, with advances taken from the count tables. The axes check becomes the /Od-attested tail loop: `position = &observer->forward.i; for (parameter_index = orientation; ...) { real_vector3d *forward = (real_vector3d *)position; real_vector3d *up = (real_vector3d *)(position + 3); if (!valid_real_vector3d_axes2(forward, up)) { real_vector3d left; cross_product3d(up, forward, &left); cross_product3d(forward, &left, up); normalize3d(forward); normalize3d(up); } position += observer_parameter_real_counts[parameter_index]; }`. Behaviour is equivalent: the one-trip tail loop, a dead memset, and orientation is the last group. If this is applied, the integrator must refresh the config/parked.json measurements.base for _observer_update_positions, because a moved parked body fails PROGRESS. Target values from the --source candidate: 1568/40, normalized sha256 985364af30c99c4d640d08e280c7903126651e0f83f85997de14475e569294a0. Remeasure after ninja and take objdiff % from the report. The park stays. The real*->real_vector3d* casts are type-only: a strip test with all of them removed gives a byte-identical function (sha1 914d5e1c3140acbe). The same cast idiom is already exact in this TU (observer_find_displacement). The brief asks for a strictly exact caller before a new view cast, so the owner or integrator should confirm the casts are admissible in a non-exact body. No header, symbols.json or config change otherwise.

## evidence
Baseline gate --all at 931ed8dc: exact 25, residual 1, the target at 1648/32 (frame 0x90). The candidate gives exact 25, residual 1, with the target at size 1568 = Jan, relocs 40 = Jan (the only relocdiff is the --source literal-naming artifact), frame 0x84 = Jan, and 471 = 471 instructions. The emitted-symbol guards pass. 8 instructions still differ, in 4 hunks of x87 commutative operand order in the tail. F is forward at edi+0xd0..0xd8 and U is up at edi+0xdc..0xe4:
- 0x43b, dot k-term: Jan F.k*U.k, ours U.k first.
- 0x48e, cross i-component first product: Jan loads F.k first, ours loads U.j first.
- 0x4ab, cross j-component first product: Jan loads F.i first.
- 0x4d4, cross k-component second product: Jan loads F.i first.
object_audit: only the target row differs, the 49 January symbols have 0 differences, and the surplus list is identical to production. pdb_storage finds 0 disagreements and fake_match_scan finds 0 leads.

Evidence chain (full list in LEDGER.md):
1. /Od fn 0x5302b0 (RTC: new_camera_displacements 56, left 12) shows the tail loop (position = observer+0xd0, parameter_index 4..5, forward = position, up = position+0xc), a separate short snap index declared after the powers block, and the order of the pointer locals.
2. /Od fn 0x52b600 confirms the inventory of the inlined rotational apply.
3. HCEX DIA2Dump confirms `struct observer_derivative new_camera_displacements`.

Probes (realdiff = differing instructions):
- Old best a3 (hand-expanded axes predicate, which the owner holds): 18.
- s1 (genuine header axes2): 20.
- L1 (/Od tail loop): 14. This fixes the inlined normalize3d magnitude hunk for the first time.
- L2 (/Od if/else with its own snap index): 8.
- L3/F1 (/Od declaration order, authentic names): 8.
- Ta, Tb, Tc, Te (tail reset or up spelling, left hoisted): byte-identical to L2.
- Cross_product3d body with 2 named locals as in the /Od copy (lab #define rename): byte-inert.

Lab-only M8 instrument (dead locals), about 120 variants at the function top, after observer, after timer, before forward, between forward and up, and after up or left:
- Dead locals after up or left are inert.
- Ordinals follow first-definition order, not declaration position. L7 moved the declaration and was byte-identical to L2.
- Every family's 8 count states were enumerated and none reproduces January. The L family (forward is position) never loads F.k first in the dot k-term or in the cross i-component first product. The M and s1 families (address or block forms) never produce the k,i,j sum order and never load U.k first in the second i-component product.
- Lf1 (up = forward + 1, which /Od refutes) scores 6.
- Mt2 (forward and up defined at the function top, contradicted by /Od) scores 6.
- X1 (forward as a local, up as an expression) at D=6 reproduces January's whole first cross product, but the dot and magnitude stay wrong.

Stop rule fired after about 20 natural shapes.

## blockers
_observer_update_positions is not strictly exact. The remaining 8 instructions are x87 commutative-leaf key order inside the tail's inlined valid_real_vector3d_axes2 dot and the first inlined cross_product3d. No /Od-attested, house-rule-compliant spelling reaches January's pattern: dot AAB k,i,j (F.k, F.i, U.j) together with cross (F.k, U.k | F.i, F.k | F.j, F.i). Each family reaches only part of it. The whole object is otherwise complete (every other function, the data/BSS sections and all symbols match), so this one function is the only object blocker.

## reopen_criteria
Reopen on any one of: (1) a decoded C2 per-leaf key rule for R1 pointer-local leaves (forward = position, up = position+3) that predicts January's AAB k,i,j dot together with the (F.k, U.k) pair in the cross i-component; (2) January-side or first-party evidence that forward and up have different provenance in the tail (lab: forward as a local with up as an inline expression, at a count shift of 6, reproduces the entire first cross product); (3) a same-compiler exact function that inlines valid_real_vector3d_axes2 plus a cross_product3d repair on position-derived pointer locals. Start from scratch/w/observer/pos_F1.c and use fu.py (F/U first-load table) and lab.py. Do not repeat anything in the ledger's inert list: Ta/Tb/Tc/Te, the declaration-order permutations, cross_product3d local count, dead locals after up or left, or any of the enumerated count-state sweeps.

## task notes
Production at 931ed8dc is 1648/32, frame 0x90; it is a structural mismatch. The candidate closes the whole structural gap (size, relocations, frame, layout), and the magnitude hunk that no earlier wave could move is fixed by the /Od-attested forward/up pointer locals. The Layer-2 conjunction header valid_real_vector3d_axes2 now inlines here, so the old hand-expanded predicate that the owner holds is no longer needed. The object is one function away from complete. If the residual is ever closed, run the brief's section-4 audit: provider_link for the surplus COMDATs, which are unchanged from production. object_audit and pdb_storage are already clean apart from the target row.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\observer\LEDGER.md
