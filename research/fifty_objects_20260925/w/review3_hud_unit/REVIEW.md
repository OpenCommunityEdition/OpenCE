# Review 3: hud_unit OBJECT_COMPLETE_CANDIDATE (worker slug hud_unit_audit)

Verdict: **approve = false**. I fixed one blocker and built an amended packet for it. The other blocker needs an
owner ruling. Full probe log: `LEDGER.md` in this slug.

## What I reproduced independently (all confirmed)

- **Patches apply cleanly.** 01, 01alt, 02, 03 and 04 each pass `git apply --check` on 954eebd2, alone and together
  (01+02+03+04). Applying each one to a copy of the tracked file gives the worker's file byte for byte:
  cand_final.c, cand.c, and the config/symbols.json, config.json and rejections copies.
- **Emulated split.** csplit with an unchanged config copy equals build/split for 833/833 files. With patch 02 applied,
  only `source/interface/hud_unit.obj` differs.
- **Gate and audit.** Against the emulated split, P01 is 22/22 EXACT, and the 5 statics have storage 3/3.
  object_audit (SPLIT_ROOT): all January-owned sections ok, 30 January symbols, 0 differ, PASS.
  P01, P01alt and the production object (modulo the 2 renames) are identical in all 41 sections.
- **Surplus.** The surplus is `.drectve`, 5 pooled strings, 5 `__real` literals and `_fast_ftol`. provider_link:
  11/11 PASS in both orders. surplus_identity: `_fast_ftol` IDENTICAL vs actor_combat. nodup_census: 25 definers,
  0 NODUP. Retiring the `_fast_ftol` veto is justified: its reopen branch 2 is met, as in the player_rumble precedent
  of 20260919.
- **Names and storage.**
  - cachebeta publics: the only public is `_unit_hud_shield_meter_mapper_init` at C65B0. There is none at
    C6590/C65A0/C65C0/C6600/C6B80.
  - HCEX_Release (DIA2Dump): `unit_hud_outline_mapper_tick` @0x16D65D0 and `unit_hud_shield_meter_mapper_tick`
    @0x16D65D8 are both static.
  - Sept-2001 maps 7eacac85 and 6455066: same names, order and sizes.
  - January atlas exact tier: 0x4c6600 is hud_unit's `get_hud_state` and 0x4c7f40 is hud_weapon's.
  - Patch 02's four in-place line edits are correct.
- **/Od attestation.**
  - 0x6363b0/c0/d0 are 5-byte empties, and the file's functions are laid out alphabetically.
  - render 0x63329d calls d0 (shield tick), then 0x6332a2 calls b0 (outline tick). Both calls follow the 0x228 assert
    and come before the `if`.
  - hud_initialize_unit_interface 0x6326c6 calls c0 (init) through the ILT; the statics are called directly.
  - The tick calls are load-bearing (lab A) and first-party attested, so they pass.
  - In update_local_player, /Od has three per-arm `hit_time = game_time_get()` stores (0x635e30/0x635e71/0x635ec4)
    and no goto, so the patch (c) shape is attested and byte-inert.

## Blocker 1 (FIXED here): new_map hand-expands the TU's own helpers

`hud_initialize_unit_interface_for_new_map` inlines by hand, in its loop, `get_hud_state`'s 0x106/0x107 asserts and
`initialize_hud_state`'s whole body. The /Od 0x632780 loop body instead makes real calls:
`call 0x632500` (get_hud_state) into the hud_state local, then `call 0x635fc0` (initialize_hud_state), then
`sound_flags = 0` and the handle csmemset. Keeping the hand expansion breaks house rule 13 (no manual inlining;
keep ordinary helpers) and falls in the law (g) hand-expanded-helper class.

**Lab N1** replaces that body with `hud_state = get_hud_state(local_player_index); initialize_hud_state(hud_state);`.
Result: new_map is EXACT 240, and all 41 sections are byte- and relocation-identical to P01 (checked in both
directions). The emulated-split gate is 22/22 and the audit passes. VC7 inlines both statics at that site on its own.
The hand expansion was therefore never load-bearing.

**Amended packet** (use it instead of 01/01alt; 02, 03 and 04 are unchanged):

- `patches/01R_hud_unit_c_names_static_calls_od_arms_newmap_helpers.patch` = the worker's 01 + the N1 fix
  (result = `cand/cand_final_amended.c`, CRLF kept).
- `patches/01Ralt_hud_unit_c_minimal_names_static_calls_newmap_helpers.patch` = 01alt + the N1 fix
  (result = `cand/cand_minimal_amended.c`).

Both pass `git apply --check` alone and with 02+03+04, and both reproduce the cand/ files byte for byte. Both are
41/41 identical to P01 and 22/22 against the emulated split, the audit passes, and fake_match_scan finds 0 leads.
Neither adds a header change or a new block scope.

## Blocker 2 (OWNER-GATED, not fixed): the stack-sentinel goto loops emulate an inlined helper

update_local_player (line 0x201) and render (line 0x3C9) keep a hand-written backward scan with
`goto corrupt_stack_found` / `goto stack_buffer_checked`. What the evidence shows:

- Both asserts in each sentinel carry the same line number, so January invoked a macro.
- January's `_check_stack_buffer` is PUBLIC in hud_draw.obj (0xC08F0, 48 B), and nothing anywhere relocates to it. It
  is inlined in 5 TUs, so each of those TUs could see its body.
- **Labs S1/S2** tried two plain goto-free spellings (a break loop, and a break followed by
  `corrupt_index = buffer_index`). Both leave the functions RESIDUAL, and S2 also changes their sizes
  (528 vs 544, 3488 vs 3504). January's bytes contain the inlined-function return merge, an out-of-line
  `mov esi,eax; jmp` at the end of the function. The worker's lab F reproduced them only through a real
  `check_stack_buffer` call with the helper body in scope.
- Our own link map prints `_fast_ftol ... f i`, so `i` in the map means an inline COMDAT. The Sept-2001 map entry
  `_get_return_eip  i hud_draw.obj` therefore shows the stack-check helpers were header inlines in 2001.
- The 20260904 renderer review says it "does not claim to have recovered the literal original stack-check macro
  spelling".

So hud_unit.c hand-copies a helper that January had in scope. Rule 6 forbids exactly that ("do not hand-expand or
hand-copy the helper to evade this check"). The copy also relies on two invented gotos, the same class as the held
actor_aim_grenade item. The worker removed the other unattested goto (`update_finished`) on that same ground.

Function credit is not affected, because this code was already function-reviewed (6ad1964c, 20260904). Whole-object
Matching should wait for one of:

- (a) an owner ruling that admits the conventional-guard spelling as disclosed; then apply 01R (or 01Ralt) + 02 + 03 + 04.
- (b) a hud_draw.h packet: `__inline check_stack_buffer` + a shared check macro. It would touch hud_draw, hud_unit,
  hud_nav_points, hud_weapon and motion_sensor, and every hud_draw.h consumer needs a declaration-count sweep. The
  `_check_stack_buffer` ANY surplus must be identical to hud_draw's selected copy and must link in both orders.

## Advisories (not blocking)

- **Omitted init call.** /Od-attested `unit_hud_shield_meter_mapper_init()` is called in hud_initialize_unit_interface
  (lab D: byte-inert). The candidate omits it. The public init also has no prototype in hud_unit.h (pre-existing).
- **update_local_player at /Od.** /Od computes `unit_index` with a single `?:` temp ([ebp-0x14] → [ebp-4]). Production
  uses a nested `if`. This is pre-existing and exact.
- **Discarded call in render.** render's bare `get_hud_state(local_player_index);` corresponds to an /Od named local that
  is never read ([ebp-0xe8]). The call is attested and uses the plain spelling.
- **Consumer-local declarations.** hud_globals_definition is a TU-local prefix view. It is named from HCEX.pdb, including
  its `unused` pads, and has no opaque spans. The hud_globals/hud_scripted_globals variables are declared extern locally.
  Matching hud.c and hud_messaging.c already do the same, and these are not law (g) blockers.

## Infrastructure incident

Around 2026-09-24 00:43:17 -0700, the worktree's root `.git` pointer file disappeared, so git now reports
"not a git repository". The admin dir `halo-campaign.git/worktrees/claude-fifty-objects-20260925` is intact. This
reviewer wrote only under this slug and did not repair the file. The integrator must restore
`C:\halo-worktrees\claude-fifty-objects-20260925\.git` with
`gitdir: C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/halo-campaign.git/worktrees/claude-fifty-objects-20260925`
before integrating.
