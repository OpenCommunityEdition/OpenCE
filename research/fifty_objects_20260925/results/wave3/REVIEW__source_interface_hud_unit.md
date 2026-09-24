# REVIEW source/interface/hud_unit

approve: False

## per_object
null

## checks
All evidence is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_hud_unit\ (LEDGER.md, REVIEW.md).

1. **Patches.** `git apply --check`: 01, 01alt, 02, 03 and 04 each CLEAN, and 01+02+03+04 CLEAN together. Applying each patch to a copy of the tracked file gives a result byte-identical to the worker's file (cand_final.c, cand.c, symbols.json, config.json, object_admission_rejections.json).
2. **Split emulation.**
   - csplit with an unchanged config copy equals build/split for 833/833 files.
   - With patch 02 applied, only source/interface/hud_unit.obj differs.
3. **Gate and object audit.**
   - `gate.py --source` on P01 against the production split: 20 exact, plus 2 unwritten that are name changes only.
   - `gate_split` against my emulated split: 22/22 EXACT; storage 3/3 for the 5 statics.
   - `object_audit` (SPLIT_ROOT): every January-owned section ok, 30 January symbols, 0 differ, PASS.
   - `cmp_objs`: P01, P01alt and production (modulo the 2 renames) are identical in all 41 sections.
4. **Surplus.**
   - `provider_link`: 11/11 PASS in both orders.
   - `surplus_identity`: `_fast_ftol` IDENTICAL to actor_combat's copy.
   - `nodup_census _fast_ftol`: 25 definers, 0 NODUP.
   - Retiring the `_fast_ftol` veto is justified (reopen branch 2 met, player_rumble 20260919 precedent).
5. **Names and storage.**
   - cachebeta publics: public only at C65B0 (init).
   - HCEX_Release DIA2Dump: both ticks are static, at 0x16D65D0 and 0x16D65D8.
   - Atlas 7eacac85 and 6455066: same names, order and sizes.
   - January atlas exact tier: 0x4c6600 is hud_unit's get_hud_state, 0x4c7f40 is hud_weapon's.
6. **/Od build.**
   - 0x6363b0/c0/d0 are 5-byte empties laid out alphabetically.
   - Render calls them at 0x63329d (shield tick) then 0x6332a2 (outline tick), after the assert and before the if.
   - Init is called at 0x6326c6.
   - update_local_player has per-arm stores and no goto, so patch (c) is attested and byte-inert.
7. **Admission scan.** No float, pragma, asm, volatile or register. No consumer-local function prototypes. No opaque spans (the hud_globals_definition prefix view is named from HCEX). fake_match_scan: 0 leads.
8. **New labs.**
   - **N1** (new_map written with the /Od helper calls `get_hud_state` + `initialize_hud_state`): new_map EXACT 240. All 41 sections are identical to P01. It is 22/22 against the emulated split and the audit passes.
   - **S1 and S2** (goto-free stack-sentinel spellings): both RESIDUAL; S2 also changes size (528 vs 544, 3488 vs 3504).
9. **Amended patches.** 01R and 01Ralt are each CLEAN alone and with 02+03+04. Each reproduces its cand/ file byte for byte, and each is 41/41 identical to P01, 22/22 and audit PASS.

## issues
**BLOCKER 1 (fixed in my slug):** `hud_initialize_unit_interface_for_new_map` writes out by hand, inside its loop, get_hud_state's 0x106/0x107 asserts and the whole body of initialize_hud_state.
- The /Od build at 0x632780 makes real calls there: call 0x632500 (get_hud_state), then call 0x635fc0 (initialize_hud_state).
- Lab N1 shows the hand-expansion is not needed for the match: the plain call spelling gives all 41 sections byte- and relocation-identical.
- Leaving it breaks house rule 13 and falls under the law-(g) class of hand-expanded helpers.
- Patches 01 and 01alt keep the hand-expansion, so replace them with:
  - scratch\w\review3_hud_unit\patches\01R_hud_unit_c_names_static_calls_od_arms_newmap_helpers.patch (= 01 + fix; result cand\cand_final_amended.c)
  - or 01Ralt_hud_unit_c_minimal_names_static_calls_newmap_helpers.patch (= 01alt + fix; result cand\cand_minimal_amended.c)
- Patches 02, 03 and 04 are unchanged. There is no header change and no new block scope.

**BLOCKER 2 (owner-gated, not fixed):** the stack-sentinel code in update_local_player (line 0x201) and render (line 0x3C9) is a hand-copied `check_stack_buffer` loop using two invented gotos (`corrupt_stack_found` / `stack_buffer_checked`).
- Both asserts in each sentinel carry the same line number, so January invoked a macro.
- January's public `_check_stack_buffer` has zero relocations anywhere and is inlined in 5 files.
- Labs S1 and S2 show January's bytes are specifically an inlined function's return merge, not a plain loop.
- Our own link map writes an `f i` flag for the `__inline` `_fast_ftol`. So the Sept-2001 map entry `_get_return_eip  i hud_draw.obj` means the stack-check helpers were header inlines.
- House rule 6 forbids hand-copying a helper to evade the provider check, and "invented goto" is already a held class. The worker removed `goto update_finished` on the same grounds.
- Function credit is unaffected. Whole-object Matching needs either an owner ruling admitting this spelling as disclosed, or a hud_draw.h packet: `__inline check_stack_buffer` plus a shared macro across hud_draw, hud_unit, hud_nav_points, hud_weapon and motion_sensor, a full hud_draw.h consumer sweep, and an ANY `_check_stack_buffer` identical to hud_draw's selected copy that links in both orders.
- If the owner admits it, apply 01R (or 01Ralt) + 02 + 03 + 04.

**Advisories (not blocking):**
- The /Od build also calls `unit_hud_shield_meter_mapper_init()` from `hud_initialize_unit_interface` (byte-inert; omitted). The public init has no prototype in hud_unit.h.
- /Od computes unit_index with `?:`; production uses a nested if (both exact).
- render has a bare `get_hud_state(local_player_index);` where /Od stores the result in a local that is never read.
- The consumer-local extern declarations of hud_globals and hud_scripted_globals have precedent in Matching hud.c and hud_messaging.c.

**INFRASTRUCTURE:** the worktree's root `.git` pointer file disappeared at about 2026-09-24 00:43:17 -0700, and git now reports "not a git repository".
- This was not caused by this reviewer: I wrote only under the slug and ran read-only tools.
- The admin directory halo-campaign.git/worktrees/claude-fifty-objects-20260925 is intact.
- The integrator must restore C:\halo-worktrees\claude-fifty-objects-20260925\.git containing `gitdir: C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/halo-campaign.git/worktrees/claude-fifty-objects-20260925`.
- Checks made after that time ran `git apply --check` as a plain file apply.
