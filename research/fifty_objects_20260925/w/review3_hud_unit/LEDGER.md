# review3_hud_unit - adversarial review of hud_unit OBJECT_COMPLETE_CANDIDATE (wave 3)

Reviewer slug: scratch/w/review3_hud_unit/. Tree 954eebd2. Worker slug scratch/w/hud_unit_audit/.

## R0 reading
- Read WORKER_BRIEF 0-8, campaign_house_rules, fifty_objects lane log (held items), hud_unit ledgers
  (tick boundary, get_state_tick_shield, new_map first shot, state migration, update pair, renderer review),
  rejections (hud_unit _fast_ftol candidate-only-comdat-owner), parked.json (no hud_unit park), config.json L644 NonMatching.
- Initial concerns to test:
  (C1) new_map hand-expands get_hud_state asserts (0x106/0x107) + initialize_hud_state body inline in its loop
       (house rule 13 manual inlining / law g hand-expanded helper?). Test the natural call spelling.
  (C2) stack-sentinel goto loop = hand-expanded check_stack_buffer (worker F7); renderer review 20260904
       says the literal macro spelling was NOT recovered; both asserts carry the same line (0x201 / 0x3C9) => January used a macro.
  (C3) render tick calls are load-bearing for emission -> verify /Od attestation independently.

## R1 independent re-verification of the worker's claims
- git apply --check: 01, 01alt, 02, 03, 04 each CLEAN on 954eebd2; 01+02+03+04 together CLEAN.
- patch 01 applied to a copy == hud_unit_audit/cand_final.c; 01alt == cand.c; patch 02 applied to a config copy == worker's config/symbols.json.
- csplit control (unchanged config copy) == build/split 833/833; emulated split (patch 02) differs ONLY in source/interface/hud_unit.obj.
- gate.py --source applied/hud_unit.c (P01) vs production split: 20 exact + 2 names-only unwritten (expected).
- gate_split vs emulated split: exact 22 / residual 0 / unwritten 0; storages 3/3 for the 5 statics.
- object_audit (SPLIT_ROOT=review split): every January-owned section ok, 30 January symbols 0 differ, PASS.
  Surplus: .drectve, 5 pooled strings, 5 __real literals, _fast_ftol.
- provider_link P01 obj: 11/11 PASS both orders; surplus_identity _fast_ftol IDENTICAL vs actor_combat; nodup_census _fast_ftol 25 definers, 0 NODUP.
- cachebeta publics: public at C65B0 (init) only; none at C6590/C65A0/C65C0/C6600/C6B80 -> statics confirmed.
- HCEX_Release DIA2Dump: unit_hud_outline_mapper_tick @0x16D65D0, unit_hud_shield_meter_mapper_tick @0x16D65D8 (static, len 4).
- Atlas 7eacac85 (Sept cachebeta map) + 6455066 (cache.exe, @..@0): same names/order/sizes; January atlas 4cc87b45 exact tier
  0x4c6600 _get_hud_state hud_unit.obj, 0x4c7f40 _get_hud_state hud_weapon.obj. Names verified.
- /Od: 0x6363b0/c0/d0 are 5-byte empties; callers: d0 <- 0x63329d, b0 <- 0x6332a2 (render, right after the 0x228 assert,
  before the if); c0 <- 0x6326c6 (hud_initialize_unit_interface via ILT thunk; statics are called direct). Alphabetical layout verified.
  => tick calls /Od-attested. The init call is also /Od-attested but omitted by the worker (byte-inert) - advisory only.

## R2 NEW FINDING C1: hud_initialize_unit_interface_for_new_map hand-expands get_hud_state + initialize_hud_state
- Production loop body = inline copies of get_hud_state's 0x106/0x107 asserts + &unit_hud_globals->hud_states[i]
  and of initialize_hud_state's body (csmemset aux, -1.0f x3, NONE x3), then sound_flags=0 + csmemset handles.
- /Od 0x632780 (new_map): loop body is `call 0x632500` (get_hud_state) -> [ebp-8] hud_state; `call 0x635fc0`
  (initialize_hud_state)(hud_state); sound_flags=0; csmemset(hud_state+0x28, -1, ..) = REAL CALLS.
- House rule 13 (no manual inlining; retain ordinary helpers) + law (g) (hand-expanded helpers block admission).
- Hypothesis H1 (lab N1): January's inline copies are VC7's own inlining of the two statics at this site; the /Od call
  spelling reproduces _hud_initialize_unit_interface_for_new_map EXACT 240.
- LAB N1 (labN1_newmap_calls.c = P01 + new_map loop body `hud_state = get_hud_state(local_player_index); initialize_hud_state(hud_state);`
  replacing the hand-expanded asserts + body): H1 CONFIRMED. new_map EXACT 240; cmp_objs vs P01 obj: 41/41 sections
  byte+reloc identical (both directions); gate_split 22/22; object_audit PASS (same surplus). The hand-expansion is NOT
  load-bearing -> it is a fixable house-rule defect (rule 13 + /Od contradiction). Amendment required.

## R3 C2 stack-sentinel spelling (update_local_player 0x201 + render 0x3C9)
- Facts: both asserts of each sentinel carry ONE line number (0x201 / 0x3C9) => January invoked a MACRO. January
  _check_stack_buffer is PUBLIC in hud_draw.obj (0xC08F0, 48 B) with 0 relocations to it anywhere => every use is inlined;
  inlined in 5 TUs (hud_draw, hud_unit x2, hud_nav_points, hud_weapon, motion_sensor) => its body was visible to them.
  Sept-2001 map: _get_return_eip 'i hud_draw.obj' (inline COMDAT). Renderer review 20260904: "does not claim to have
  recovered the literal original stack-check macro spelling". Production spelling = hand-written backward scan with
  two gotos emulating the inlined helper's return merge (not /Od-attested: /Od has no sentinel at all).
- Hypothesis H2 (lab S1): if January's macro expanded a plain loop (no helper), a goto-free break form
  (corrupt_index=NONE; for(..) if(hit){corrupt_index=buffer_index; break;}) reproduces both functions; if it does NOT,
  the bytes are specifically the inlined-FUNCTION return merge => January inlined check_stack_buffer => hud_unit's loop
  is a hand-copy of a helper that January had header-visible (rule 6 / law g class).
- LAB S1 (goto-free break form, corrupt_index=NONE before the loop, both sentinels): update_local_player + render
  RESIDUAL (sha). alndiff: January's hit path is an out-of-line `mov esi,eax; jmp back` block at the function end =
  the inlined-function return merge.
- LAB S2 (break, then corrupt_index = buffer_index): RESIDUAL, size 528!=544 / 3488!=3504.
- => H2 REFUTED: January's bytes are specifically an INLINED check_stack_buffer (worker lab F: the genuine helper call
  with the body visible reproduces all 41 sections + emits a 48-B ANY _check_stack_buffer COMDAT). The production goto
  loop is a hand-emulation of a helper January had visible in hud_unit.c (header inline / shared macro), i.e. the
  "hand-expand or hand-copy the helper to evade this check" class of rule 6 / law (g), plus two invented gotos
  (held-item class 'invented goto'); the worker itself removed goto update_finished on that ground. Pre-existing and
  function-reviewed (6ad1964c, 20260904), so function credit is unaffected, but whole-object admission needs an owner
  ruling or a hud_draw.h check_stack_buffer header-inline packet (provider hud_draw ANY + full hud_draw.h consumer sweep).
- Corroboration of the 'i' flag meaning: our own VC7 link map for the _fast_ftol probe prints
  `_fast_ftol ... f i cand_final.obj` (cseries.h __inline, SELECT_ANY) => the Sept-2001 map's `_get_return_eip  i hud_draw.obj`
  means get_return_eip was an INLINE (header) function in 2001: the stack-check helpers were header inlines.

## R4 amended packet (C1 fixed; C2 left for owner)
- cand/cand_final_amended.c = patch 01 + new_map /Od helper calls (CRLF preserved); cand/cand_minimal_amended.c = 01alt + same.
- gate --source: 20 exact + 2 names-only (production split) for both; gate_split vs review emulated split: 22/22 both;
  object_audit_split: 30 January symbols 0 differ, PASS both; cmp_objs vs P01 obj: 41/41 identical both.
- patches/01R_*.patch and patches/01Ralt_*.patch: git apply --check CLEAN alone and with 02+03+04; applying to a copy of the
  tracked file reproduces the cand/ files byte-for-byte.
- fake_match_scan on the amended candidate: 0 leads.

## R5 infrastructure incident (not caused by this reviewer)
- The worktree's `.git` gitdir file DISAPPEARED at ~2026-09-24 00:43:17 -0700 (root dir mtime); `git status/ls-files/log` now fail
  with "not a git repository". The admin dir halo-campaign.git/worktrees/claude-fifty-objects-20260925 is intact (gitdir file
  inside says C:/halo-worktrees/claude-fifty-objects-20260925/.git). This reviewer only wrote under scratch/w/review3_hud_unit/
  and ran read-only tools (gate/audit/provider_link/csplit into the slug); no command touched the root. NOT repaired here
  (hard rule: no git/config mutation). Integrator must restore the pointer file:
  `gitdir: C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/halo-campaign.git/worktrees/claude-fifty-objects-20260925`
  (format copied from sibling worktrees), then re-run `git apply --check`. The --check results above ran as plain file applies.

## VERDICT: approve=false (fixable part amended; one owner-gated blocker remains)
