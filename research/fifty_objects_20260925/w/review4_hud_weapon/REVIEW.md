# REVIEW - hud_weapon P5 (shared stack frame), wave 4

- Reviewer slug: scratch/w/review4_hud_weapon/.
- Tree: 16542e46. `git status` was clean before and after the review.
- Worker slug: scratch/w/hud_stack_check/.
- Patch under review: P5_hud_weapon_c_shared_stack_frame.patch, sha256 55cb5670...
  - It is the same file the review4_hud_draw reviewer pinned.
  - A verbatim copy is in patches/, with the hashes in SHA256SUMS.txt.

## Verdict

**APPROVE P5 unchanged, but only inside the atomic hud_draw.h packet.**

- **Landing set:** P1 + P2 + (P3 or 01RS) + P5 + P7.
  - P4 is optional.
  - P6 follows the review4_hud_draw ruling.
- **P5 is zero-credit hygiene, not an admission.** hud_weapon stays at 13 EXACT and 3 residual.

The worker states only "depends on P1". That dependency statement is incomplete. The correct one is:

| Dependency | Why | Evidence |
|---|---|---|
| P5 needs P1 | to compile: the macro and `STACK_BUFFER_*` live in the header | - |
| P5 needs P2 | to link: against production's NODUP hud_draw `_check_stack_buffer`, the candidate gives LNK2005 in both orders | reproduced (plink.py control) |
| P1 needs P3 or 01RS | hud_unit.c otherwise fails with a redefinition of `STACK_BUFFER_*` | review4_hud_draw |
| P1 needs P7 | otherwise the declaration count flips models `__rasterizer_model_transparent_geometry_submit` | review4_hud_draw |
| P1 needs P5 | otherwise hud_weapon's `_render_weapon_hud` bytes drift | reproduced here |

If the owner picks the P-A focused header instead of P1, P5 has to be regenerated: add the new include and drop nothing else.

## Independent checks (own tools in the slug)

| Check | Result |
|---|---|
| `git apply --check` on 16542e46 | Clean for each of these sets: P5 alone; P1+P5; {P1,P2,P3,P5,P7}; {P1,P2,01RS,P4,P5,P6,P7} |
| Candidate reproduction | `git show HEAD:` blobs + `patch -p1` (P5, P1, P2) give the worker's cand files (EOL-normalised) |
| Harness control | Verbatim header redirect: rawcmp 0 differences, for hud_weapon and for all 23 consumers against build/base |
| gate `--all` for prod / ctrl / cand | 13 EXACT / 3 residual, identical rows: `_crosshairs_draw` relocs 77!=76; `_hud_update_weapon_local_player` size 1456!=1472; `_render_weapon_hud` size 2624!=2656 |
| Raw compare prod vs cand (bytes, relocs, flags, COMDAT selection, full symbol table) | Only the new `_check_stack_buffer` section (ANY) and its external symbol differ |
| Subset P1 without P5 | `_render_weapon_hud` bytes DRIFT, so the worker's A5 is reproduced |
| Cause of the drift (lab L0) | Lab L0 keeps all five goto copies but drops the TU `HUD_WEAPON_STACK_BUFFER_*` enum; it is identical to cand minus the COMDAT. So the drift comes from the two TU enum names, which are declaration-count duplicates of the header constants. P5 removes them for a genuine reason, not as count compensation. |
| Surplus identity | `_check_stack_buffer` is IDENTICAL to January hud_draw's selected copy (48 B, 0 relocs). The pre-existing `_square_root` and `_power` are IDENTICAL too |
| provider_link.py (repo, production providers) | `_check_stack_buffer` FAIL, LNK2005 in both orders. This is the expected control and proves the P2 dependency. Every other surplus passes |
| Pair link against HEAD+P1+P2 hud_draw.obj | PASS in both orders. The 3-object packet (hud_weapon + hud_draw + hud_unit) also passes in both orders |
| object_audit prod vs cand | Text differs only by `+.text sym:_check_stack_buffer size 48`. 23/23 January symbols ok, all data/rdata/bss sections ok. FAIL(3) = the 3 pre-existing residuals |
| pdb_storage | 0 disagreements. `_check_stack_buffer` is PUBLIC in cachebeta (000C08F0), so the non-static `__inline` is right |
| /W3 /Zs | Warning multisets identical (1x C4146, 13x C4244) |
| Consumer list for P1 | 23 TUs, spaces-safe grep; the list equals the worker's. No header includes hud_draw.h, and every consumer spells the include `interface/hud_draw.h` |
| Own shadow sweep: 23 consumers, minimal set P1+P2+P3+P5+P7 | 20 objects show 0 differences (models with P7). hud_draw differs only by the selection change 1->2. hud_unit and hud_weapon differ only by the new ANY COMDAT. Gate summaries are identical everywhere, so no park drift |

## Laws and house rules

- **Law (i): a new header COMDAT from a caller.** The helper has five inlined sites in hud_weapon:
  - Strictly exact: `_render_grenade_hud` (0x3A2) and `_hud_update_weapon` (0xD4).
  - Residual and unparked: the other three.
  - Lab L1 has only the two exact callers use the inline. The COMDAT is still emitted, byte-identical, and the object is unchanged. So the emission is owned by strictly exact callers, which is the same standing as hud_nav_points P4 and not the damage-A/P6 class.
  - Every residual caller's inlined tail has January's exact shape: the loop, the `or esi,-1` merge, the return-eip compare, and two assert pairs at the site line. The differences are only loop-alignment filler and the pre-existing frame offsets. No site calls the helper out of line.
- **Strip test.** ST1/ST2 drop the macro's `{}` and declare `short corrupt_index;` at the function top. The result is identical, so the block scope is byte-inert and buys nothing.
  - The macro form is chosen on the F2 evidence: both asserts of every January site carry one line literal.
- **/Od declaration order.** The later build has no sentinel. There are no "corrupt stack at" or "corrupt return address" strings and no `stack_buffer` RTC descriptors in the hud_weapon functions. So no /Od frame contradicts the block, and law (d) does not apply.
- **Law (g) scan of the diff.**
  - P5 removes a hand copy of a header helper (the goto macro with 2 invented labels, a held class).
  - It adds no prototypes, views, `float`, aggregates or pragmas.
  - `match_assert_stack_frame` is a descriptive macro name. It replaces the equally descriptive `hud_weapon_stack_buffer_check`; Bungie's name is lost. Disclose it.
- **symbols.json.** No change needed.

## Pre-existing hud_weapon admission blockers (NOT touched by P5; this object is not admission-ready)

- A caller-local view with an opaque span, `struct hud_weapon_globals_definition { byte reserved000[0x2C0]; ... }`, plus a consumer-local `extern struct hud_weapon_globals_definition *hud_globals;`.
- The 3 residual functions.
- `goto finished` / `draw_crosshair` / `use_flat_color` control flow. I did not assess it in this review.
- `_code_000c8xxx` placeholders in the file's header comment. January atlas names should replace them.

## For the integrator

- Apply P5 only in the same commit as P1 and P2, plus P3/01RS and P7.
- After applying, re-sweep all 23 hud_draw.h consumers and the parks on the live tree, together with the other wave-4 header packets. Isolated measurements do not compose, and hud_weapon's residuals are declaration-count sensitive.

Evidence: LEDGER.md (V1-V9), out/, labs/, sw_ctrl/, sw_min/, sweep_*.txt, audit_*.txt, warn_*.txt.
