# REVIEW source/interface/hud_weapon

approve: True

## per_object
[
 {
  "unit": "source/interface/hud_weapon",
  "approve": true,
  "issues": "Approve P5 unchanged (sha256 55cb5670...) only inside the atomic set P1+P2+(P3|01RS)+P5+P7. P5 needs P1 to compile and P2 to link (LNK2005 vs the production NODUP hud_draw copy, reproduced). Zero credit: 13/3 unchanged, residual bodies byte-identical, the only object change is the new ANY _check_stack_buffer, identical to January's. The COMDAT has two strictly exact callers, which alone emit it identically (lab L1), so law (i) is met. Strip test: the macro block is byte-inert. There is no /Od sentinel. Pre-existing admission blockers remain: an opaque hud_weapon_globals_definition view, a consumer-local extern hud_globals, and 3 residuals."
 }
]

## checks
All checks were run independently with tools in scratch/w/review4_hud_weapon/. Tree 16542e46; git status was clean before and after.

1. **Patch application.** `git apply --check` is clean for each of these sets: P5 alone; P1+P5; {P1,P2,P3,P5,P7}; {P1,P2,01RS,P4,P5,P6,P7}. `git show HEAD:` blobs plus `patch -p1` (P5, P1, P2) reproduce the worker's cand hud_weapon.c, hud_draw.h and hud_draw.c (EOL-normalised). Patch sha256 values match review4_hud_draw's pins (P5 55cb5670...).
2. **Gate.** `gate.py --all` on prod, a verbatim-header control and the candidate (P5 + P1 header) gives 13 EXACT / 3 residual everywhere, with identical rows:
   - `_crosshairs_draw` relocs 77!=76
   - `_hud_update_weapon_local_player` size 1456!=1472
   - `_render_weapon_hud` size 2624!=2656
3. **Raw compare (own rawcmp.py).** It checks bytes, relocations with $L labels normalised by value, flags, COMDAT selection and the full symbol table.
   - prod vs control: 0 differences.
   - prod vs cand: only the new ANY `_check_stack_buffer` section and its external symbol.
   - P1 without P5: `_render_weapon_hud` bytes drift, which reproduces the worker's A5.
   - Lab L0 (P1 header, all five goto copies kept, the TU `HUD_WEAPON_STACK_BUFFER_*` enum dropped) equals cand minus the COMDAT. So the drift comes from the TU's duplicate enum names, and removing them is genuine de-duplication, not count compensation.
4. **Inlined sentinel tails (sentinel_tail.py / tail_dis.py).**
   - The exact callers `_render_grenade_hud` and `_hud_update_weapon` have tails byte- and reloc-identical to January.
   - The three residual callers have January's inlined shape. The differences are only alignment filler and pre-existing frame offsets, and no site calls the helper out of line.
5. **Law (i) labs.** L1 (only the exact callers use the inline) still emits the identical COMDAT, with 0 object differences, so the emission is owned by strictly exact callers. L2 (only the residual callers use it) also gives 0 differences.
6. **Strip test.** ST1 and ST2 (no macro braces; `short corrupt_index` declared after stack_buffer, or first) are identical to cand, so the block scope is byte-inert.
7. **/Od build.** odbuild finds no "corrupt stack at" or "corrupt return address" strings, and the hud_weapon functions' RTC descriptors have no `stack_buffer`. No /Od frame exists, so law (d) does not apply.
8. **Surplus identity (slug copy that reads the candidate object).** `_check_stack_buffer` is IDENTICAL to January hud_draw's copy. The pre-existing `_square_root` and `_power` are IDENTICAL too.
9. **Provider links.**
   - Repo provider_link.py with production providers: `_check_stack_buffer` gives LNK2005 in both orders. This is the expected control and proves P5 also needs P2.
   - Slug pair link against my HEAD+P1+P2 hud_draw.obj (22/0/1; differs from prod only by selection 1->2): PASS in both orders.
   - The 3-object hud_weapon + hud_draw + hud_unit link: PASS in both orders.
   - hud_draw is the only production definer of `_check_stack_buffer`.
10. **object_audit prod vs cand.** The text differs only by `+.text _check_stack_buffer 48`. All 23 January symbols are ok, and every data/rdata/bss section is ok. FAIL(3) is the three pre-existing residuals.
11. **pdb_storage.** 0 disagreements. `_check_stack_buffer` is PUBLIC in cachebeta at 000C08F0.
12. **Warnings.** The /W3 /Zs multisets are identical.
13. **P1 consumer list.** A spaces-safe grep over .c/.h/.cpp/.inl finds 23 TUs, the same as the worker's list. Every consumer spells the include `interface/hud_draw.h`, and no header includes hud_draw.h.
14. **Own shadow sweep of all 23 consumers.**
    - Control: 23/23 show 0 differences against build/base.
    - Minimal set P1+P2+P3+P5+P7: 20 objects show 0 differences (models with P7); hud_draw differs only by the selection change; hud_unit and hud_weapon differ only by the new ANY COMDAT. Gate summaries are identical everywhere.
15. **Records checked.** parked.json and object_admission_rejections.json have no hud_weapon entries. The earlier hud_weapon ledgers record no sentinel negatives.

Notes are in REVIEW.md and LEDGER.md in the slug.

## issues
P5's content is approved unchanged (sha256 55cb5670...; verbatim copy in scratch/w/review4_hud_weapon/patches/). It is approved ONLY inside the atomic hud_draw.h packet, and it is zero credit: hud_weapon stays 13/3 and is not an admission.

1. **The worker's dependency statement ("depends on P1") is incomplete.** The correct dependencies are:
   - P5 needs P1 to compile (the macro and `STACK_BUFFER_*` live in the header).
   - P5 needs P2 to link: against production's NODUP hud_draw `_check_stack_buffer` it gives LNK2005 in both orders, which I reproduced.
   - P1 in turn needs P3 or 01RS (hud_unit `STACK_BUFFER_*` redefinition) and P7 (the models declaration-count flip).
   - P1 needs P5 (`_render_weapon_hud` drift, which I reproduced).
   - Landing set: P1 + P2 + (P3|01RS) + P5 + P7 in one commit. P4 is optional; P6 follows the review4_hud_draw exclusion.
   - If the owner picks the P-A focused header instead of P1, P5 must be regenerated with the new include.
2. **Disclosures.**
   - `match_assert_stack_frame` is a descriptive macro name that replaces the equally descriptive `hud_weapon_stack_buffer_check`; Bungie's name is lost.
   - The macro's block scope is byte-inert (strip test identical). The macro form rests on the F2 evidence that both asserts of every January site carry one line literal.
   - The new `_check_stack_buffer` COMDAT has two strictly exact callers (`_render_grenade_hud`, `_hud_update_weapon`), and lab L1 shows they alone emit it identically. So this is not the damage-A/P6 class. Three callers stay residual (unparked), and their inlined tails match January's shape.
3. **Pre-existing hud_weapon admission blockers that P5 does not touch.**
   - A caller-local opaque view `struct hud_weapon_globals_definition { byte reserved000[0x2C0]; ... }`, with a consumer-local `extern ... *hud_globals`.
   - The three residuals.
   - Unassessed `goto finished` / `draw_crosshair` / `use_flat_color` control flow.
   - `_code_000c8xxx` placeholders in the header comment.
4. **Integrator.** Re-sweep all 23 hud_draw.h consumers and the parks on the live tree, together with the other wave-4 header packets. hud_weapon's residuals are declaration-count sensitive, and isolated measurements do not compose.
